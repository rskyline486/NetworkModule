#include "stdafx.h"
#include "SocketItem.h"

//////////////////////////////////////////////////////////////////////////

//系数定义
#define DEAD_QUOTIETY				0									//死亡系数
#define DANGER_QUOTIETY				1									//危险系数
#define SAFETY_QUOTIETY				2									//安全系数

//////////////////////////////////////////////////////////////////////////

//构造函数
CSocketItem::CSocketItem(WORD wIndex, ISocketItemSink * pSocketItemSink)
	: CNativeInfo(wIndex), m_pSocketItemSink(pSocketItemSink)
{
	m_dwRecvDataCount = 0L;
	m_dwRecvDataCount = 0L;
	m_wSurvivalTime = 0;
}

//析构函数
CSocketItem::~CSocketItem(void)
{
}

//绑定对象
DWORD CSocketItem::Attach(HANDLE hCompletionPort, SOCKET hSocket)
{
	//数据校验
	if (hSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("连接句柄无效"));
		return INVALID_SOCKETID;
	}
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("完成端口对象为空"));
		CNetwork::CloseSocket(hSocket);
		return INVALID_SOCKETID;
	}

	//绑定到完成端口
	if (CreateIoCompletionPort((HANDLE)hSocket, hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		Logger_Error(TEXT("绑定到完成端口失败,错误码:%d"), WSAGetLastError());
		CNetwork::CloseSocket(hSocket);
		return INVALID_SOCKETID;
	}

	//设置变量
	m_hSocket = hSocket;
	m_wSurvivalTime = SAFETY_QUOTIETY;

	//设置数据
	__super::InitData();
	//设置监测
	__super::SetDetect();

	//连接通知
	m_pSocketItemSink->OnEventSocketBind(this);

	//同步控制
	//防止PostRecv方法投递接收后,还未来得及设置接收标识,就触发了接收完成回调
	//完成回调重置接收标识后,PostRecv方法后续又设置接收标识
	//从而导致后续投递接收请求时程序误以为上一次接收还未返回
	//因此程序会等待返回,进而导致该标识永远都不会被重置
	CLocker Locker(m_Mutex);

	//投递请求
	PostRecv();

	return __super::GetSocketID();
}

//获取接收
DWORD CSocketItem::GetRecvDataCount()
{
	return InterlockedExchangeAdd(&m_dwRecvDataCount, 0);
}

//获取发送
DWORD CSocketItem::GetSendDataCount()
{
	return InterlockedExchangeAdd(&m_dwSendDataCount, 0);
}

//心跳检测
bool CSocketItem::Heartbeat(DWORD dwNowTime)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//接收判断
	if (m_dwRecvDataCount > 0)
	{
		if (m_wSurvivalTime == 0)
		{
			Logger_Debug(TEXT("长时间不发包,关闭连接[dwSocketID:%u, RecvIng:%u, SendIng:%u][wIndex:%u, wRountID:%u, bUsed:%u, bDetect:%u, bShutDown:%u]")
				, GetSocketID(), m_OverLappedRecv.GetHandleIng(), m_OverLappedSend.GetHandleIng(), m_wIndex, m_wRountID, m_bUsed, m_bDetect, m_bShutDown);
			CloseSocket();
			return false;
		}
	}
	else
	{
		//关闭不发包连接
		if ((m_dwActiveTime + 4) <= dwNowTime)
		{
			Logger_Debug(TEXT("连接不发包,关闭连接[dwSocketID:%u, RecvIng:%u, SendIng:%u][wIndex:%u, wRountID:%u, bUsed:%u, bDetect:%u, bShutDown:%u]")
				, GetSocketID(), m_OverLappedRecv.GetHandleIng(), m_OverLappedSend.GetHandleIng(), m_wIndex, m_wRountID, m_bUsed, m_bDetect, m_bShutDown);
			CloseSocket();
			return false;
		}
	}

	//存活递减
	m_wSurvivalTime--;
	return (m_wSurvivalTime == 0);
}

//处理请求
void CSocketItem::DealAsync(COverLapped * pOverLapped, DWORD dwThancferred)
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
	if (static_cast<int>(dwThancferred) == SOCKET_ERROR || dwThancferred == 0)
	{
		Logger_Debug(TEXT("数据异常,关闭连接"));
		CloseSocket();
		return;
	}

	switch (pOverLapped->GetOperationType())
	{
	case enOperationType_Recv:
		{
			//接收长度
			if (m_OverLappedRecv.RecvCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("接收数据处理失败,dwThancferred:%u,关闭连接"), dwThancferred);
				CloseSocket();
				return;
			}

			//中断判断
			if (m_bShutDown == true)
			{
				//处理数据
				m_OverLappedRecv.DealCompleted(pOverLapped->GetBufferSize());

				//投递请求
				//PostRecv();
				return;
			}

			//解析真实IP
			if (m_dwRecvDataCount == 0L && m_dwRealIPV4 == 0L)
			{
				//尝试从nginx数据包中解析出用户真实IP信息
				DWORD dwNginxPacketSize = PraseClientIP((CHAR *)pOverLapped->GetBufferAddr(), pOverLapped->GetBufferSize());
				if (dwNginxPacketSize > 0L)
				{
					//处理数据
					m_OverLappedRecv.DealCompleted(dwNginxPacketSize);

					//校验数据
					if (pOverLapped->GetBufferSize() == 0)
					{
						m_wSurvivalTime = SAFETY_QUOTIETY;

						//投递请求
						PostRecv();
						return;
					}
				}
			}

			//读取数据
			DWORD dwDealSize = m_pSocketItemSink->OnEventSocketRead(this, pOverLapped->GetBufferAddr(), pOverLapped->GetBufferSize());
			if (static_cast<int>(dwDealSize) == SOCKET_ERROR || m_OverLappedRecv.DealCompleted(dwDealSize) == false)
			{
				Logger_Debug(TEXT("解析数据处理失败,dwDealSize:%u,关闭连接"), dwDealSize);
				CloseSocket();
				return;
			}

			//接收流量(原子操作)
			InterlockedExchangeAdd(&m_dwRecvDataCount, dwDealSize);
			m_wSurvivalTime = SAFETY_QUOTIETY;

			//投递请求
			PostRecv();
		}
		break;
	case enOperationType_Send:
		{
			//发送长度
			if (m_OverLappedSend.SendCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("发送数据处理失败,dwThancferred:%u,关闭连接"), dwThancferred);
				CloseSocket();
				return;
			}

			//发送流量(原子操作)
			InterlockedExchangeAdd(&m_dwSendDataCount, dwThancferred);
			m_wSurvivalTime = SAFETY_QUOTIETY;

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
bool CSocketItem::SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID)
{
	//数据校验
	if (lpData == NULL || dwDataSize == 0) return false;

	//同步控制
	CLocker Locker(m_Mutex);

	//发送判断
	if (SendVerdict(wRountID) == false) return false;

	//缓冲数据
	if (m_OverLappedSend.SendData(lpData, dwDataSize) == false)
	{
		Logger_Error(TEXT("发送失败,缓冲数据失败,dwDataSize:%u"), dwDataSize);
		return false;
	}

	//投递发送
	PostSend();

	return true;
}

//关闭连接
bool CSocketItem::CloseSocket(WORD wRountID)
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
void CSocketItem::CloseSocket()
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
			m_pSocketItemSink->OnEventSocketShut(this);
			//恢复数据
			ResumeData();
		}
	}
}

//恢复数据
void CSocketItem::ResumeData()
{
	//重置数据
	m_OverLappedRecv.ResetData();
	//重置数据
	m_OverLappedSend.ResetData();
	//重置接收
	m_dwRecvDataCount = 0L;
	//重置发送
	m_dwSendDataCount = 0L;
	//重置存活
	m_wSurvivalTime = 0;
	//取消监测
	__super::CancelDetect();
	//重置数据
	__super::ResetData();
}

//投递请求
void CSocketItem::PostRecv()
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
void CSocketItem::PostSend()
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

//解析地址(不处理需要粘包的情况,如果数据包长度不足,则忽略nginx代理协议信息)
DWORD CSocketItem::PraseClientIP(CHAR * pszBuffer, DWORD dwBufferSize)
{
	//数据校验
	if (m_dwRecvDataCount != 0L || m_dwRealIPV4 != 0L) return 0L;

	DWORD dwReadSize = 0;
	//Logger_Debug(TEXT("尝试从数据包中解析出nginx代理IP信息[dwSocketID:%u,wIndex:%u,wRountID:%u],数据包长度:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
	//代理协议
	if ((dwBufferSize - dwReadSize) >= 6 && memcmp("PROXY ", &pszBuffer[dwReadSize], 6) == 0)
	{
		dwReadSize += 6;
		//IPV4
		if ((dwBufferSize - dwReadSize) >= 5 && memcmp("TCP4 ", &pszBuffer[dwReadSize], 5) == 0)
		{
			dwReadSize += 5;
			//数据校验
			if ((dwBufferSize - dwReadSize) >= 15)
			{
				CHAR szIPInfo[16] = { 0 };
				int nDotCount = 0;
				for (int i = 0; i < 15; i++)
				{
					if (pszBuffer[dwReadSize + i] == ' ') break;
					if (pszBuffer[dwReadSize + i] != '.' && (pszBuffer[dwReadSize + i] < '0' || pszBuffer[dwReadSize + i] > '9')) break;
					szIPInfo[i] = pszBuffer[dwReadSize + i];
					if (pszBuffer[dwReadSize + i] == '.') nDotCount++;
				}

				if (nDotCount == 3)
				{
					if (inet_pton(AF_INET, szIPInfo, &m_dwRealIPV4) <= 0)
					{
						//Logger_Debug(TEXT("nginx代理协议,解析真实IP失败,szIPInfo:%hs"), szIPInfo);
						m_dwRealIPV4 = 0L;
					}
					else
					{
						//Logger_Debug(TEXT("nginx代理协议,解析真实IP成功,szIPInfo:%hs"), szIPInfo);
					}
				}
			}
		}
		else
		{
			//Logger_Debug(TEXT("貌似nginx数据包中IP信息并非IPV4[dwSocketID:%u,wIndex:%u,wRountID:%u],数据包长度:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
		}

		//跳字节
		CHAR * pszEnd = strstr(pszBuffer, "\r\n");
		if (pszEnd)
		{
			//计算nginxIP数据包长度
			dwReadSize = (pszEnd - pszBuffer) + 2;

			//数据校验
			if (dwBufferSize < dwReadSize)
			{
				//Logger_Debug(TEXT("貌似数据包中包含nginx数据包信息,但数据包长度异常[dwSocketID:%u,wIndex:%u,wRountID:%u],数据包长度:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
				return 0L;
			}

			return dwReadSize;
		}
		else
		{
			//Logger_Debug(TEXT("貌似nginx数据包不完整或协议异常[dwSocketID:%u,wIndex:%u,wRountID:%u],数据包长度:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
		}
	}
	else
	{
		//Logger_Debug(TEXT("貌似数据包中不含nginx代理IP信息[dwSocketID:%u,wIndex:%u,wRountID:%u],数据包长度:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
	}

	return 0L;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CSocketItemManager::CSocketItemManager()
{
	m_pSocketItemSink = NULL;
}

//析构函数
CSocketItemManager::~CSocketItemManager()
{
	__super::ReleaseNativeInfo(true);
}

//初始化管理对象
bool CSocketItemManager::Init(HANDLE hCompletionPort, ISocketItemSink * pSocketItemSink, WORD wMaxItemCount)
{
	m_pSocketItemSink = pSocketItemSink;
	__super::SetParameter(hCompletionPort, INDEX_SOCKET, wMaxItemCount);
	return true;
}

//释放管理对象
bool CSocketItemManager::Release()
{
	return __super::ReleaseNativeInfo(false);
}

//激活对象
DWORD CSocketItemManager::ActiveSocketItem(SOCKET hSocket)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//获取对象
	CSocketItem * pSocketItem = dynamic_cast<CSocketItem *>(__super::GetFreeNativeInfo());
	if (pSocketItem == NULL)
	{
		if (m_NativeInfoPtrList.size() < m_wMaxNativeItemCount)
		{
			pSocketItem = new(std::nothrow) CSocketItem((WORD)(m_NativeInfoPtrList.size() + m_wStartIndex), m_pSocketItemSink);
			if (pSocketItem)
			{
				//添加到最后
				m_NativeInfoPtrList.push_back(pSocketItem);
			}
		}
		else
		{
			Logger_Error(TEXT("网络对象已达上限:%u"), m_wMaxNativeItemCount);
		}
	}

	//对象校验
	if (pSocketItem == NULL)
	{
		//获取本地地址
		Address LocalAddr = CNetwork::GetLocalAddress(hSocket);
		TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 };
		WORD wLocalPort = CNetwork::GetPort(LocalAddr);
		CNetwork::GetIP(LocalAddr, szLocalAddress, CountArray(szLocalAddress));
		//获取对端地址
		Address RemoteAddr = CNetwork::GetRemoteAddress(hSocket);
		TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 };
		WORD wRemotePort = CNetwork::GetPort(RemoteAddr);
		CNetwork::GetIP(RemoteAddr, szRemoteAddress, CountArray(szRemoteAddress));
		Logger_Error(TEXT("申请网络对象失败, 本地地址:%s:%d, 对端地址:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
		//申请连接对象失败
		CNetwork::CloseSocket(hSocket);
		return INVALID_SOCKETID;
	}

	//绑定对象
	return pSocketItem->Attach(m_hCompletionPort, hSocket);
}

//检测对象
bool CSocketItemManager::DetectItem()
{
	DWORD dwNowTime = (DWORD)time(NULL);
	CSocketItem * pSocketItem = NULL;
	//遍历对象
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//如果对象正常,且需要被监测
		if (m_NativeInfoPtrList[i]->IsUsed() == true && m_NativeInfoPtrList[i]->GetDetect() == true)
		{
			pSocketItem = dynamic_cast<CSocketItem *>(m_NativeInfoPtrList[i]);
			if (pSocketItem->Heartbeat(dwNowTime))
			{
				//发送心跳包
				if (m_HeartbeatPacket.wDataSize > 0)
				{
					pSocketItem->SendData(m_HeartbeatPacket.cbSendBuffer, m_HeartbeatPacket.wDataSize, pSocketItem->GetRountID());
				}
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
