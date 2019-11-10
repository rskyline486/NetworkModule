#include "stdafx.h"
#include "AcceptItem.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CAcceptItem::CAcceptItem(WORD wIndex, IAcceptItemSink * pAcceptItemSink)
	: CNativeInfo(wIndex), m_pAcceptItemSink(pAcceptItemSink)
{
	m_wListenPort = 9999;
	m_Protocol = EnableIPv4;
	m_fnAcceptEx = NULL;
}

//析构函数
CAcceptItem::~CAcceptItem(void)
{
}

//监听对象
DWORD CAcceptItem::Listen(HANDLE hCompletionPort, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch)
{
	//校验参数
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("完成端口对象为空"));
		return INVALID_SOCKETID;
	}

	//记录参数
	m_wListenPort = wPort;
	m_Protocol = Protocol;

	//获取地址
	const_cast<Address&>(m_LocalAddress) = CNetwork::GetAddress(NULL, wPort, Protocol, true);
	if (CNetwork::IsValidAddress(m_LocalAddress) == false)
	{
		Logger_Error(TEXT("获取地址失败,错误码:%d"), WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//创建SOCKET
	m_hSocket = CNetwork::CreateSocket(m_LocalAddress, Protocol);
	if (m_hSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("创建SOCKET失败,错误码:%d"), WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//获取AcceptEx函数地址
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;
	if (WSAIoctl(m_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &m_fnAcceptEx, sizeof(m_fnAcceptEx), &dwBytes, NULL, NULL) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("获取AcceptEx函数地址失败,错误码:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//绑定地址
	const_cast<Address&>(m_LocalAddress) = CNetwork::Bind(m_hSocket, m_LocalAddress);
	if (CNetwork::IsValidAddress(m_LocalAddress) == false)
	{
		Logger_Error(TEXT("绑定端口失败,端口号:%u,错误码:%d"), wPort, WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		m_fnAcceptEx = NULL;
		return INVALID_SOCKETID;
	}

	//监听端口
	if (CNetwork::Listen(m_hSocket, 200) == false)
	{
		Logger_Error(TEXT("监听端口失败,端口号:%u,错误码:%d"), wPort, WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		m_fnAcceptEx = NULL;
		return INVALID_SOCKETID;
	}

	//绑定到完成端口
	if (CreateIoCompletionPort((HANDLE)m_hSocket, hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		Logger_Error(TEXT("绑定到完成端口失败,错误码:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		m_fnAcceptEx = NULL;
		return INVALID_SOCKETID;
	}

	//设置数据
	__super::InitData();
	//用户数据
	m_dwUserData = dwUserData;
	//设置守护
	if (bWatch) __super::SetDetect();

	//事件通知
	m_pAcceptItemSink->OnEventListenStart(this);

	//同步控制
	//防止PostAccept方法投递接收后,还未来得及设置接收标识,就触发了接收完成回调
	//完成回调重置接收标识后,PostAccept方法后续又设置接收标识
	//从而导致后续投递接收请求时程序误以为上一次接收还未返回
	//因此程序会等待返回,进而导致该标识永远都不会被重置
	CLocker Locker(m_Mutex);

	//投递请求
	PostAccept();

	return __super::GetSocketID();
}

//恢复监听
DWORD CAcceptItem::ResumeListen(HANDLE hCompletionPort)
{
	//监听判断
	if (__super::IsUsed()) return __super::GetSocketID();

	//参数校验
	if (hCompletionPort == NULL) return INVALID_SOCKETID;

	return Listen(hCompletionPort, m_wListenPort, m_dwUserData, m_Protocol, true);
}

//处理请求
void CAcceptItem::DealAsync(COverLapped * pOverLapped, DWORD dwThancferred)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//设置标识
	pOverLapped->SetHandleIng(false);

	//获取句柄
	SOCKET hAcceptSocket = m_OverLappedAccept.GetOutSocket();

	//判断关闭
	if (m_hSocket == INVALID_SOCKET || hAcceptSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("SOCKET句柄无效:m_hSocket:[0x%p],hAcceptSocket:[0x%p],关闭监听"), m_hSocket, hAcceptSocket);
		CloseSocket();
		return;
	}

	//数据校验
	if (static_cast<int>(dwThancferred) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("数据异常,关闭监听"));
		CloseSocket();
		return;
	}

	//设置属性
	if (setsockopt(hAcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (CHAR*)&hAcceptSocket, sizeof(hAcceptSocket)) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("设置属性[SO_UPDATE_ACCEPT_CONTEXT]失败,错误码:%d,关闭监听"), WSAGetLastError());
		CloseSocket();
		return;
	}

	//事件通知
	m_pAcceptItemSink->OnEventListenAccept(this, hAcceptSocket);

	//投递请求
	PostAccept();
}

//发送函数
bool CAcceptItem::SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID)
{
	//数据校验
	if (lpData == NULL || dwDataSize == 0) return false;

	//同步控制
	CLocker Locker(m_Mutex);

	//发送判断
	if (SendVerdict(wRountID) == false) return false;

	return false;
}

//关闭连接
bool CAcceptItem::CloseSocket(WORD wRountID)
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
void CAcceptItem::CloseSocket()
{
	//关闭监听
	if (m_hSocket != INVALID_SOCKET)
	{
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	//关闭连接
	SOCKET hAcceptSocket = m_OverLappedAccept.GetOutSocket();
	if (hAcceptSocket != INVALID_SOCKET)
	{
		CNetwork::CloseSocket(hAcceptSocket);
		hAcceptSocket = INVALID_SOCKET;
	}

	//判断关闭
	if (m_OverLappedAccept.GetHandleIng() == false)
	{
		if (__super::IsUsed())
		{
			//事件通知
			m_pAcceptItemSink->OnEventListenStop(this);
			//恢复数据
			ResumeData();
		}
	}
}

//恢复数据
void CAcceptItem::ResumeData()
{
	//重置接口
	m_fnAcceptEx = NULL;
	//重置数据
	m_OverLappedAccept.ResetData();
	//重置数据
	__super::ResetData();
}

//投递请求
void CAcceptItem::PostAccept()
{
	//投递校验
	if (m_OverLappedAccept.GetHandleIng() == true)
	{
		//Logger_Error(TEXT("已存在投递请求"));
		return;
	}

	//恢复缓冲
	if (!m_OverLappedAccept.ResumeBuffer())
	{
		Logger_Error(TEXT("整理重叠对象失败,关闭监听"));
		CloseSocket();
		return;
	}

	//申请连接
	SOCKET hAcceptSocket = CNetwork::CreateSocket(m_LocalAddress);
	if (hAcceptSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("申请连接对象失败,hAcceptSocket:%u,错误码:%d,关闭监听"), hAcceptSocket, WSAGetLastError());
		CloseSocket();
		return;
	}

	//存储连接
	if (!m_OverLappedAccept.StoreSocket(hAcceptSocket))
	{
		Logger_Error(TEXT("存储连接失败???"));
		//取出并释放连接(理论上不会走到这里)
		SOCKET hTempSocket = m_OverLappedAccept.GetOutSocket();
		if (hTempSocket)
		{
			CNetwork::CloseSocket(hTempSocket);
			hTempSocket = INVALID_SOCKET;
		}
		m_OverLappedAccept.StoreSocket(hAcceptSocket);
	}

	//接收数据
	DWORD dwRecv = 0;
	DWORD dwLocalAddrSize = m_OverLappedAccept.GetBufferSize() / 2;
	DWORD dwRemoteAddrSize = m_OverLappedAccept.GetBufferSize() / 2;
	if (!m_fnAcceptEx(m_hSocket, hAcceptSocket, m_OverLappedAccept.GetBufferAddr(), 0, dwLocalAddrSize, dwRemoteAddrSize, &dwRecv, m_OverLappedAccept))
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Error(TEXT("AcceptEx调用失败,错误码:%d,关闭监听"), WSAGetLastError());
			CloseSocket();
			return;
		}
	}

	//设置标识
	m_OverLappedAccept.SetHandleIng(true);
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CAcceptItemManager::CAcceptItemManager()
{
	m_pAcceptItemSink = NULL;
}

//析构函数
CAcceptItemManager::~CAcceptItemManager()
{
	__super::ReleaseNativeInfo(true);
}

//初始化管理对象
bool CAcceptItemManager::Init(HANDLE hCompletionPort, IAcceptItemSink * pAcceptItemSink, WORD wMaxItemCount)
{
	m_pAcceptItemSink = pAcceptItemSink;
	__super::SetParameter(hCompletionPort, INDEX_LISTEN, wMaxItemCount);
	return true;
}

//释放管理对象
bool CAcceptItemManager::Release()
{
	return __super::ReleaseNativeInfo(false);
}

//激活对象
DWORD CAcceptItemManager::ActiveAcceptItem(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//获取对象
	CAcceptItem * pAcceptItem = dynamic_cast<CAcceptItem *>(__super::GetFreeNativeInfo());
	if (pAcceptItem == NULL)
	{
		if (m_NativeInfoPtrList.size() < m_wMaxNativeItemCount)
		{
			pAcceptItem = new(std::nothrow) CAcceptItem((WORD)(m_NativeInfoPtrList.size() + m_wStartIndex), m_pAcceptItemSink);
			if (pAcceptItem)
			{
				//添加到最后
				m_NativeInfoPtrList.push_back(pAcceptItem);
			}
		}
		else
		{
			Logger_Error(TEXT("监听对象已达上限:%u"), m_wMaxNativeItemCount);
		}
	}

	//对象校验
	if (pAcceptItem == NULL)
	{
		Logger_Error(TEXT("监听端口[%d]失败=>申请监听对象失败"), wListenPort);
		return INVALID_SOCKETID;
	}

	//协议类型
	ProtocolSupport Protocol = EnableIPv4;
	if (wProtocol == EnableIPv6) Protocol = EnableIPv6;
	if (wProtocol == EnableBoth) Protocol = EnableBoth;

	//启动服务
	return pAcceptItem->Listen(m_hCompletionPort, wListenPort, dwUserData, Protocol, bWatch);
}

//检测对象
bool CAcceptItemManager::DetectItem()
{
	CAcceptItem * pAcceptItem = NULL;
	//遍历对象
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//如果对象空闲,且需要被监测
		if (m_NativeInfoPtrList[i]->IsUsed() == false && m_NativeInfoPtrList[i]->GetDetect() == true)
		{
			pAcceptItem = dynamic_cast<CAcceptItem *>(m_NativeInfoPtrList[i]);
			pAcceptItem->ResumeListen(m_hCompletionPort);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
