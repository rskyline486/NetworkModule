#include "stdafx.h"
#include "ConnectItem.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CConnectItem::CConnectItem(WORD wIndex, IConnectItemSink * pConnectItemSink)
	: CNativeInfo(wIndex), m_pConnectItemSink(pConnectItemSink)
{
	ZeroMemory(m_szConnectAddress, sizeof(m_szConnectAddress));
	m_wConnectPort = 9999;
	m_Protocol = EnableIPv4;
}

//析构函数
CConnectItem::~CConnectItem(void)
{
}

//连接对象
DWORD CConnectItem::Connect(HANDLE hCompletionPort, LPCTSTR pszConnectAddress, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch)
{
	//数据校验
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("完成端口对象为空"));
		return INVALID_SOCKETID;
	}

	//记录参数
	lstrcpyn(m_szConnectAddress, pszConnectAddress, CountArray(m_szConnectAddress));
	m_wConnectPort = wPort;
	m_Protocol = Protocol;

	//获取地址
	Address serverAddr = CNetwork::GetAddress(m_szConnectAddress, wPort, Protocol, true);
	if (CNetwork::IsValidAddress(serverAddr) == false)
	{
		Logger_Error(TEXT("获取地址失败,连接地址:%s,连接端口:%u,协议版本:%d,错误码:%d"), m_szConnectAddress, wPort, Protocol, WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//创建SOCKET
	m_hSocket = CNetwork::CreateSocket(serverAddr, Protocol);
	if (m_hSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("创建SOCKET失败,错误码:%d"), WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//获取ConnectEx函数地址
	LPFN_CONNECTEX fnConnectEx = NULL;
	GUID GuidConnectEx = WSAID_CONNECTEX;
	DWORD dwBytes;
	if (WSAIoctl(m_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &fnConnectEx, sizeof(fnConnectEx), &dwBytes, NULL, NULL) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("获取ConnectEx函数地址失败,错误码:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//绑定地址
	Address bindAddr = CNetwork::GetAddress(serverAddr, 0);
	bindAddr = CNetwork::Bind(m_hSocket, bindAddr);
	if (CNetwork::IsValidAddress(bindAddr) == false)
	{
		Logger_Error(TEXT("绑定随机端口失败,错误码:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//绑定到完成端口
	if (CreateIoCompletionPort((HANDLE)m_hSocket, hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		Logger_Error(TEXT("绑定到完成端口失败,错误码:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//设置数据
	__super::InitData();
	//用户数据
	m_dwUserData = dwUserData;
	//设置守护
	if (bWatch) __super::SetDetect();

	//同步控制
	//防止ConnectEx方法投递接收后,还未来得及设置接收标识,就触发了接收完成回调
	//完成回调重置接收标识后,此处代码继续执行,后续又设置接收标识
	//从而导致后续投递接收请求时程序误以为上一次接收还未返回
	//因此程序会等待返回,进而导致该标识永远都不会被重置
	//另:此处后续若不设置投递标识,那么程序在完成回调还未返回时,就可以直接主动关闭连接
	//从而导致后续该对象会被分配给其他连接,进而导致不可预料的问题出现(因为上一次的回调还未返回)
	CLocker Locker(m_Mutex);

	//投递连接
	m_OverLappedRecv.SetOperationType(enOperationType_Connect);
	if (!fnConnectEx(m_hSocket, &serverAddr.sa, CNetwork::GetAddressSize(serverAddr), 0, 0, 0, m_OverLappedRecv))
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Error(TEXT("ConnectEx调用失败,错误码:%d"), WSAGetLastError());
			CNetwork::CloseSocket(m_hSocket);
			m_hSocket = INVALID_SOCKET;

			//重置数据
			__super::ResetData();

			return INVALID_SOCKETID;
		}
	}

	//设置标识
	m_OverLappedRecv.SetHandleIng(true);

	return __super::GetSocketID();
}

//恢复连接
DWORD CConnectItem::ResumeConnect(HANDLE hCompletionPort)
{
	//连接判断
	if (__super::IsUsed()) return __super::GetSocketID();

	//参数校验
	if (hCompletionPort == NULL) return INVALID_SOCKETID;

	return Connect(hCompletionPort, m_szConnectAddress, m_wConnectPort, m_dwUserData, m_Protocol, true);
}

//处理请求
void CConnectItem::DealAsync(COverLapped * pOverLapped, DWORD dwThancferred)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//设置标识
	pOverLapped->SetHandleIng(false);

	//判断关闭
	if (m_hSocket == INVALID_SOCKET)
	{
		Logger_Debug(TEXT("SOCKET句柄无效,关闭连接"));
		CloseSocket();
		return;
	}

	//数据校验
	if (static_cast<int>(dwThancferred) == SOCKET_ERROR)
	{
		Logger_Debug(TEXT("数据异常,关闭连接"));
		CloseSocket();
		return;
	}

	switch (pOverLapped->GetOperationType())
	{
	case enOperationType_Connect:
		{
			//设置属性
			if (setsockopt(m_hSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) == SOCKET_ERROR)
			{
				Logger_Error(TEXT("设置属性[SO_UPDATE_CONNECT_CONTEXT]失败,错误码:%d,关闭连接"), WSAGetLastError());
				CloseSocket();
				return;
			}

			//设置为接收类型
			pOverLapped->SetOperationType(enOperationType_Recv);

			//连接通知
			m_pConnectItemSink->OnEventConnectLink(this);

			//投递请求
			PostRecv();
		}
		break;
	case enOperationType_Recv:
		{
			//数据校验
			if (dwThancferred == 0)
			{
				CloseSocket();
				return;
			}
			//接收长度
			if (m_OverLappedRecv.RecvCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("接收数据处理失败,dwThancferred:%u,关闭连接"), dwThancferred);
				CloseSocket();
				return;
			}
			//读取数据
			DWORD dwDealSize = m_pConnectItemSink->OnEventConnectRead(this, pOverLapped->GetBufferAddr(), pOverLapped->GetBufferSize());
			if (static_cast<int>(dwDealSize) == SOCKET_ERROR || m_OverLappedRecv.DealCompleted(dwDealSize) == false)
			{
				Logger_Debug(TEXT("解析数据处理失败,dwDealSize:%u,关闭连接"), dwDealSize);
				CloseSocket();
				return;
			}
			//投递请求
			PostRecv();
		}
		break;
	case enOperationType_Send:
		{
			//数据校验
			if (dwThancferred == 0)
			{
				CloseSocket();
				return;
			}
			//发送长度
			if (m_OverLappedSend.SendCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("发送数据处理失败,dwThancferred:%u,关闭连接"), dwThancferred);
				CloseSocket();
				return;
			}
			//投递请求
			PostSend();
		}
		break;
	default:
		{
			Logger_Error(TEXT("异常通知类型???,OperationType:%d,dwThancferred:%u,关闭连接"), pOverLapped->GetOperationType(), dwThancferred);
			CloseSocket();
		}
		break;
	}
}

//发送函数
bool CConnectItem::SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//发送判断
	if (SendVerdict(wRountID) == false) return false;

	//缓冲数据
	if (lpData != NULL && dwDataSize != 0)
	{
		if (m_OverLappedSend.SendData(lpData, dwDataSize) == false)
		{
			Logger_Error(TEXT("发送失败,缓冲数据失败,dwDataSize:%u"), dwDataSize);
			return false;
		}
	}

	//状态判断
	if (m_OverLappedRecv.GetOperationType() == enOperationType_Connect)
	{
		return true;
	}

	//投递发送
	PostSend();

	return true;
}

//关闭连接
bool CConnectItem::CloseSocket(WORD wRountID)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//状态判断
	if (m_wRountID != wRountID) return false;

	//重复校验(防止在关闭回调接口中再次调用该接口而导致死循环)
	if (m_hSocket == INVALID_SOCKET) return true;

	CloseSocket();
	return true;
}

//关闭连接
void CConnectItem::CloseSocket()
{
	//关闭socket
	if (m_hSocket != INVALID_SOCKET)
	{
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	//判断关闭
	if ((m_OverLappedRecv.GetHandleIng() == false) && (m_OverLappedSend.GetHandleIng() == false))
	{
		if (__super::IsUsed())
		{
			//关闭通知
			m_pConnectItemSink->OnEventConnectShut(this);
			//恢复数据
			ResumeData();
		}
	}
}

//恢复数据
void CConnectItem::ResumeData()
{
	//重置数据
	m_OverLappedRecv.ResetData();
	//重置数据
	m_OverLappedSend.ResetData();
	//重置数据
	__super::ResetData();
}

//投递请求
void CConnectItem::PostRecv()
{
	//投递校验
	if (m_OverLappedRecv.GetHandleIng() == true)
	{
		//Logger_Debug(TEXT("已存在接收请求"));
		return;
	}

	//恢复缓冲
	if (!m_OverLappedRecv.ResumeBuffer())
	{
		Logger_Debug(TEXT("整理重叠对象失败,关闭连接"));
		CloseSocket();
		return;
	}

	//接收数据
	DWORD dwThancferred = 0, dwFlags = 0;
	if (WSARecv(m_hSocket, m_OverLappedRecv, 1, &dwThancferred, &dwFlags, m_OverLappedRecv, NULL) == SOCKET_ERROR)
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Debug(TEXT("WSARecv调用失败,错误码:%d,关闭连接"), WSAGetLastError());
			CloseSocket();
			return;
		}
	}

	//设置标识
	m_OverLappedRecv.SetHandleIng(true);
}

//投递请求
void CConnectItem::PostSend()
{
	//投递校验
	if (m_OverLappedSend.GetHandleIng() == true)
	{
		//Logger_Debug(TEXT("已存在发送请求"));
		return;
	}

	//恢复缓冲
	if (!m_OverLappedSend.ResumeBuffer())
	{
		//Logger_Debug(TEXT("发送完毕"));
		return;
	}

	//发送数据
	DWORD dwThancferred = 0;
	if (WSASend(m_hSocket, m_OverLappedSend, 1, &dwThancferred, 0, m_OverLappedSend, NULL) == SOCKET_ERROR)
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Debug(TEXT("WSASend调用失败,错误码:%d,关闭连接"), WSAGetLastError());
			CloseSocket();
			return;
		}
	}

	//设置标识
	m_OverLappedSend.SetHandleIng(true);
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CConnectItemManager::CConnectItemManager()
{
	m_pConnectItemSink = NULL;
}

//析构函数
CConnectItemManager::~CConnectItemManager()
{
	__super::ReleaseNativeInfo(true);
}

//初始化管理对象
bool CConnectItemManager::Init(HANDLE hCompletionPort, IConnectItemSink * pConnectItemSink, WORD wMaxItemCount)
{
	m_pConnectItemSink = pConnectItemSink;
	__super::SetParameter(hCompletionPort, INDEX_CONNECT, wMaxItemCount);
	return true;
}

//释放管理对象
bool CConnectItemManager::Release()
{
	return __super::ReleaseNativeInfo(false);
}

//激活对象
DWORD CConnectItemManager::ActiveConnectItem(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//获取对象
	CConnectItem * pConnectItem = dynamic_cast<CConnectItem *>(__super::GetFreeNativeInfo());
	if (pConnectItem == NULL)
	{
		if (m_NativeInfoPtrList.size() < m_wMaxNativeItemCount)
		{
			pConnectItem = new(std::nothrow) CConnectItem((WORD)(m_NativeInfoPtrList.size() + m_wStartIndex), m_pConnectItemSink);
			if (pConnectItem)
			{
				//添加到最后
				m_NativeInfoPtrList.push_back(pConnectItem);
			}
		}
		else
		{
			Logger_Error(TEXT("连接对象已达上限:%u"), m_wMaxNativeItemCount);
		}
	}

	//对象校验
	if (pConnectItem == NULL)
	{
		Logger_Error(TEXT("连接服务[%s:%d]失败=>申请连接对象失败"), pszConnectAddress, wConnectPort);
		return INVALID_SOCKETID;
	}

	//协议类型
	ProtocolSupport Protocol = EnableIPv4;
	if (wProtocol == EnableIPv6) Protocol = EnableIPv6;
	if (wProtocol == EnableBoth) Protocol = EnableBoth;

	//连接服务
	return pConnectItem->Connect(m_hCompletionPort, pszConnectAddress, wConnectPort, dwUserData, Protocol, bWatch);
}

//检测对象
bool CConnectItemManager::DetectItem()
{
	CConnectItem * pConnectItem = NULL;
	//遍历对象
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//如果对象空闲,且需要被监测
		if (m_NativeInfoPtrList[i]->IsUsed() == false && m_NativeInfoPtrList[i]->GetDetect() == true)
		{
			pConnectItem = dynamic_cast<CConnectItem *>(m_NativeInfoPtrList[i]);
			pConnectItem->ResumeConnect(m_hCompletionPort);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
