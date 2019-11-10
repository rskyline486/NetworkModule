#include "stdafx.h"
#include "ServiceKernel.h"

//////////////////////////////////////////////////////////////////////////

//动作定义
#define ASYNCHRONISM_SEND_DATA		1									//发送标识
#define ASYNCHRONISM_SEND_BATCH		2									//群体发送
#define ASYNCHRONISM_CLOSE_SOCKET	3									//关闭连接
#define ASYNCHRONISM_SET_DETECT		4									//设置监测
#define ASYNCHRONISM_SET_USER_DATA	5									//绑定数据
#define ASYNCHRONISM_SHUT_DOWN		6									//安全关闭

//////////////////////////////////////////////////////////////////////////

//发送请求
struct tagSendDataRequest
{
	DWORD							dwSocketID;							//连接标识
	WORD							wDataSize;							//数据大小
	BYTE							cbSendBuffer[SOCKET_BUFFER];		//发送缓冲
};

//群发请求
struct tagBatchSendRequest
{
	WORD							wDataSize;							//数据大小
	BYTE                            cbBatchMask;                        //数据掩码
	BYTE							cbSendBuffer[SOCKET_BUFFER];		//发送缓冲
};

//关闭连接
struct tagCloseSocket
{
	DWORD							dwSocketID;							//连接标识
};

//设置监测
struct tagSetDetect
{
	DWORD							dwSocketID;							//连接标识
	bool							bDetect;							//监测标识
};

//设置数据
struct tagSetUserData
{
	DWORD							dwSocketID;							//连接标识
	DWORD							dwUserData;							//用户数据
};

//安全关闭
struct tagShutDownSocket
{
	DWORD							dwSocketID;							//连接标识
};

//////////////////////////////////////////////////////////////////////////

//构造函数
CServiceKernel::CServiceKernel()
{
	m_hCompletionPort = NULL;
}

//析构函数
CServiceKernel::~CServiceKernel()
{
}

//初始化服务
bool CServiceKernel::InitService(WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount, WORD wMaxConnectCount)
{
	if (m_hCompletionPort != NULL)
	{
		Logger_Error(TEXT("服务已初始化"));
		return false;
	}

	//系统信息
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	//调整线程数目
	wThreadCount = (WORD)(wThreadCount < SystemInfo.dwNumberOfProcessors ? wThreadCount : SystemInfo.dwNumberOfProcessors);
	wThreadCount = wThreadCount > 0 ? wThreadCount : 1;

	//完成端口
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, wThreadCount);
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("创建完成端口失败"));
		return false;
	}

	//创建并启动工作线程
	m_WorkerThreadPtrList.clear();
	for (WORD i = 0; i < wThreadCount; i++)
	{
		//申请线程对象
		CWorkerThread * pWorkerThread = new(std::nothrow) CWorkerThread(i);
		if (pWorkerThread == NULL)
		{
			Logger_Error(TEXT("申请工作线程失败"));
			break;
		}

		//初始化线程对象
		if (pWorkerThread->InitThread(m_hCompletionPort) == false)
		{
			Logger_Error(TEXT("初始化工作线程失败"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//启动线程
		if (pWorkerThread->StartThread() == false)
		{
			Logger_Error(TEXT("启动工作线程失败"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//加入线程池列表
		m_WorkerThreadPtrList.push_back(pWorkerThread);
	}

	//工作线程校验
	if (m_WorkerThreadPtrList.empty())
	{
		Logger_Error(TEXT("服务启动失败,工作线程数:0"));
		ReleaseService();
		return false;
	}

	//初始化检测线程
	if (m_DetectThread.InitThread(this, dwDetectTime) == false)
	{
		Logger_Error(TEXT("初始化检测线程失败"));
		ReleaseService();
		return false;
	}
	//启动线程
	if (m_DetectThread.StartThread() == false)
	{
		Logger_Error(TEXT("启动检测线程失败"));
		ReleaseService();
		return false;
	}

	//初始化监听对象
	m_AcceptItemManager.Init(m_hCompletionPort, this, wMaxAcceptCount);
	//初始化网络对象
	m_SocketItemManager.Init(m_hCompletionPort, this, wMaxSocketCount);
	//初始化连接对象
	m_ConnectItemManager.Init(m_hCompletionPort, this, wMaxConnectCount);
	//初始化异步对象
	m_AsynchronismEngine.Init(this, 1);

	Logger_Info(TEXT("服务启动成功,工作线程数:%d"), wThreadCount);

	return true;
}

//释放服务
bool CServiceKernel::ReleaseService()
{
	//数据校验
	if (m_hCompletionPort == NULL) return true;

	//停止检测线程
	m_DetectThread.StopThread(INFINITE);

	//关闭监听对象
	m_AcceptItemManager.Release();
	//关闭网络对象
	m_SocketItemManager.Release();
	//关闭连接对象
	m_ConnectItemManager.Release();

	//通知工作线程结束
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	}

	//停止工作线程
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;

		m_WorkerThreadPtrList[i]->StopThread(INFINITE);
		delete m_WorkerThreadPtrList[i];
		m_WorkerThreadPtrList[i] = NULL;
	}
	m_WorkerThreadPtrList.clear();

	//停止异步对象
	m_AsynchronismEngine.Release();

	//关闭对象
	if (m_hCompletionPort != NULL)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}

	Logger_Info(TEXT("服务停止"));

	return true;
}

//监听端口
DWORD CServiceKernel::ListenPort(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//校验数据
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("监听端口[%d]失败=>服务器未初始化"), wListenPort);
		return INVALID_SOCKETID;
	}

	//启动服务
	return m_AcceptItemManager.ActiveAcceptItem(wListenPort, dwUserData, wProtocol, bWatch);
}

//连接服务
DWORD CServiceKernel::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//校验数据
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("连接服务[%s:%d]失败=>服务器未初始化"), pszConnectAddress, wConnectPort);
		return INVALID_SOCKETID;
	}

	//连接服务
	return m_ConnectItemManager.ActiveConnectItem(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//发送函数(之所以调整为异步,是因为在回调线程中调用这些方法容易产生死锁-[比如,两个socket同时给对方发送数据])
bool CServiceKernel::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	//校验数据
	if (wDataSize > SOCKET_BUFFER) return false;

	//同步控制
	CLocker Locker(m_Mutex);
	tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)m_cbBuffer;

	//构造数据
	pSendDataRequest->dwSocketID = dwSocketID;
	pSendDataRequest->wDataSize = wDataSize;
	if (wDataSize > 0)
	{
		CopyMemory(pSendDataRequest->cbSendBuffer, pData, wDataSize);
	}

	//发送请求
	WORD wSendSize = sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + wDataSize;
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SEND_DATA, m_cbBuffer, wSendSize);
}

//群发数据(之所以调整为异步,是因为在回调线程中调用这些方法容易产生死锁-[比如,两个socket同时给对方发送数据])
bool CServiceKernel::SendDataBatch(VOID * pData, WORD wDataSize)
{
	//校验数据
	if (wDataSize > SOCKET_BUFFER) return false;

	//同步控制
	CLocker Locker(m_Mutex);
	tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)m_cbBuffer;

	//构造数据
	pBatchSendRequest->wDataSize = wDataSize;
	pBatchSendRequest->cbBatchMask = 0;
	if (wDataSize > 0)
	{
		CopyMemory(pBatchSendRequest->cbSendBuffer, pData, wDataSize);
	}

	//发送请求
	WORD wSendSize = sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + wDataSize;
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SEND_BATCH, m_cbBuffer, wSendSize);
}

//关闭连接(之所以调整为异步,是因为在回调线程中调用这些方法容易产生死锁-[比如,两个socket同时关闭对方])
bool CServiceKernel::CloseSocket(DWORD dwSocketID)
{
	//同步控制
	CLocker Locker(m_Mutex);
	tagCloseSocket * pCloseSocket = (tagCloseSocket *)m_cbBuffer;

	//构造数据
	pCloseSocket->dwSocketID = dwSocketID;

	//发送请求
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_CLOSE_SOCKET, m_cbBuffer, sizeof(tagCloseSocket));
}

//设置监测(之所以调整为异步,是因为在回调线程中调用这些方法容易产生死锁-[比如,两个socket同时调用对方方法])
bool CServiceKernel::SetDetect(DWORD dwSocketID, bool bDetect)
{
	//同步控制
	CLocker Locker(m_Mutex);
	tagSetDetect * pSetDetect = (tagSetDetect *)m_cbBuffer;

	//构造数据
	pSetDetect->dwSocketID = dwSocketID;
	pSetDetect->bDetect = bDetect;

	//发送请求
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SET_DETECT, m_cbBuffer, sizeof(tagSetDetect));
}

//设置数据(之所以调整为异步,是因为在回调线程中调用这些方法容易产生死锁-[比如,两个socket同时调用对方方法])
bool CServiceKernel::SetUserData(DWORD dwSocketID, DWORD dwUserData)
{
	//同步控制
	CLocker Locker(m_Mutex);
	tagSetUserData * pSetUserData = (tagSetUserData *)m_cbBuffer;

	//构造数据
	pSetUserData->dwSocketID = dwSocketID;
	pSetUserData->dwUserData = dwUserData;

	//发送请求
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SET_USER_DATA, m_cbBuffer, sizeof(tagSetUserData));
}

//设置关闭
bool CServiceKernel::ShutDownSocket(DWORD dwSocketID)
{
	//同步控制
	CLocker Locker(m_Mutex);
	tagShutDownSocket * pShutDownSocket = (tagShutDownSocket *)m_cbBuffer;

	//构造数据
	pShutDownSocket->dwSocketID = dwSocketID;

	//发送请求
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SHUT_DOWN, m_cbBuffer, sizeof(tagShutDownSocket));
}

//检测事件
VOID CServiceKernel::OnEventDetectBeat()
{
	//显示队列负荷
	//if (1)
	//{
	//	tagBurthenInfo BurthenInfo;
	//	m_AsynchronismEngine.GetAsynchronismBurthenInfo(BurthenInfo);
	//	Logger_Info(TEXT("队列负荷信息,缓冲长度:%u,数据大小:%u,数据包数:%u"), BurthenInfo.dwBufferSize, BurthenInfo.dwDataSize, BurthenInfo.dwPacketCount);
	//}

	m_AcceptItemManager.DetectItem();
	m_ConnectItemManager.DetectItem();
	m_SocketItemManager.DetectItem();
}

//异步开始
bool CServiceKernel::OnEventAsynchronismStrat()
{
	return true;
}

//异步结束
bool CServiceKernel::OnEventAsynchronismStop()
{
	return true;
}

//异步事件
bool CServiceKernel::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case ASYNCHRONISM_SEND_DATA:		//发送请求
		{
			//效验数据
			tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)pData;
			if (wDataSize < (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + pSendDataRequest->wDataSize)) return false;
			
			//获取索引
			WORD wIndex = SOCKET_INDEX(pSendDataRequest->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
			}

			return m_SocketItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
		}
	case ASYNCHRONISM_SEND_BATCH:		//群发请求
		{
			//效验数据
			tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)pData;
			if (wDataSize < (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + pBatchSendRequest->wDataSize)) return false;

			return false;
		}
	case ASYNCHRONISM_CLOSE_SOCKET:		//关闭连接
		{
			//效验数据
			tagCloseSocket * pCloseSocket = (tagCloseSocket *)pData;
			if (wDataSize != sizeof(tagCloseSocket)) return false;

			//获取索引
			WORD wIndex = SOCKET_INDEX(pCloseSocket->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.CloseSocket(pCloseSocket->dwSocketID);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.CloseSocket(pCloseSocket->dwSocketID);
			}

			return m_SocketItemManager.CloseSocket(pCloseSocket->dwSocketID);
		}
	case ASYNCHRONISM_SET_DETECT:		//设置监测
		{
			//效验数据
			tagSetDetect * pSetDetect = (tagSetDetect *)pData;
			if (wDataSize != sizeof(tagSetDetect)) return false;

			//获取索引
			WORD wIndex = SOCKET_INDEX(pSetDetect->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
			}

			return m_SocketItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
		}
	case ASYNCHRONISM_SET_USER_DATA:	//绑定数据
		{
			//效验数据
			tagSetUserData * pSetUserData = (tagSetUserData *)pData;
			if (wDataSize != sizeof(tagSetUserData)) return false;

			//获取索引
			WORD wIndex = SOCKET_INDEX(pSetUserData->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
			}

			return m_SocketItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
		}
	case ASYNCHRONISM_SHUT_DOWN:		//安全关闭
		{
			//效验数据
			tagShutDownSocket * pShutDownSocket = (tagShutDownSocket *)pData;
			if (wDataSize != sizeof(tagShutDownSocket)) return false;

			//获取索引
			WORD wIndex = SOCKET_INDEX(pShutDownSocket->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
			}

			return m_SocketItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
		}
	}

	return false;
}

//开始监听
VOID CServiceKernel::OnEventListenStart(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	Logger_Info(TEXT("开始监听=>%s:%d"), szLocalAddress, wLocalPort);
}

//接收事件
VOID CServiceKernel::OnEventListenAccept(CNativeInfo* pNativeInfo, SOCKET hSocket)
{
	//激活连接
	m_SocketItemManager.ActiveSocketItem(hSocket);
}

//结束监听
VOID CServiceKernel::OnEventListenStop(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	Logger_Info(TEXT("结束监听=>%s:%d"), szLocalAddress, wLocalPort);
}

//绑定事件
VOID CServiceKernel::OnEventSocketBind(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Socket绑定事件=>本地地址:%s:%d, 对端地址:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//读取事件
DWORD CServiceKernel::OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	Logger_Info(TEXT("Socket读取事件=>dwSocketID:%u, 数据大小:%u"), pNativeInfo->GetSocketID(), dwDataSize);
	return dwDataSize;
}

//关闭事件
VOID CServiceKernel::OnEventSocketShut(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Socket关闭事件=>本地地址:%s:%d, 对端地址:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//连接事件
VOID CServiceKernel::OnEventConnectLink(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Connect连接事件=>本地地址:%s:%d, 对端地址:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//读取事件
DWORD CServiceKernel::OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	Logger_Info(TEXT("Connect读取事件=>dwSocketID:%u, 数据大小:%u"), pNativeInfo->GetSocketID(), dwDataSize);
	return dwDataSize;
}

//关闭事件
VOID CServiceKernel::OnEventConnectShut(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Connect关闭事件=>本地地址:%s:%d, 对端地址:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CServer::CServer()
{
	m_pNetworkEvent = this;
}

//析构函数
CServer::~CServer()
{
	//
}

//初始化服务
bool CServer::Init(INetworkEvent * pNetworkEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount)
{
	if (__super::InitService(wThreadCount, dwDetectTime, wMaxAcceptCount, wMaxSocketCount, 0))
	{
		m_pNetworkEvent = (pNetworkEvent != NULL) ? pNetworkEvent : this;
		return true;
	}
	
	return false;
}

//释放服务
bool CServer::Release()
{
	__super::ReleaseService();
	m_pNetworkEvent = this;
	return true;
}

//设置心跳包
bool CServer::SetHeartbeatPacket(VOID * pData, WORD wDataSize)
{
	return m_SocketItemManager.SetHeartbeatData(pData, wDataSize);
}

//监听端口
DWORD CServer::Listen(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return __super::ListenPort(wListenPort, dwUserData, wProtocol, bWatch);
}

//获取地址
DWORD CServer::GetClientIP(DWORD dwSocketID)
{
	return m_SocketItemManager.GetClientIP(dwSocketID);
}

//发送数据
bool CServer::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SendData(dwSocketID, pData, wDataSize);
	}

	return __super::SendData(dwSocketID, pData, wDataSize);
}

//群发数据
bool CServer::SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SendDataBatch(pData, wDataSize);
	}

	return __super::SendDataBatch(pData, wDataSize);
}

//关闭连接
bool CServer::CloseSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.CloseSocket(dwSocketID);
	}

	return __super::CloseSocket(dwSocketID);
}

//设置监测
bool CServer::SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SetDetect(dwSocketID, bDetect);
	}

	return __super::SetDetect(dwSocketID, bDetect);
}

//设置数据
bool CServer::SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SetUserData(dwSocketID, dwUserData);
	}

	return __super::SetUserData(dwSocketID, dwUserData);
}

//设置关闭
bool CServer::ShutDownSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.ShutDownSocket(dwSocketID);
	}

	return __super::ShutDownSocket(dwSocketID);
}

//异步事件
bool CServer::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case ASYNCHRONISM_SEND_DATA:		//发送请求
		{
			//效验数据
			tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)pData;
			if (wDataSize < (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + pSendDataRequest->wDataSize)) return false;

			return m_SocketItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
		}
	case ASYNCHRONISM_SEND_BATCH:		//群发请求
		{
			//效验数据
			tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)pData;
			if (wDataSize < (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + pBatchSendRequest->wDataSize)) return false;

			return m_SocketItemManager.SendDataBatch(pBatchSendRequest->cbSendBuffer, pBatchSendRequest->wDataSize);
		}
	case ASYNCHRONISM_CLOSE_SOCKET:		//关闭连接
		{
			//效验数据
			tagCloseSocket * pCloseSocket = (tagCloseSocket *)pData;
			if (wDataSize != sizeof(tagCloseSocket)) return false;

			return m_SocketItemManager.CloseSocket(pCloseSocket->dwSocketID);
		}
	case ASYNCHRONISM_SET_DETECT:		//设置监测
		{
			//效验数据
			tagSetDetect * pSetDetect = (tagSetDetect *)pData;
			if (wDataSize != sizeof(tagSetDetect)) return false;

			return m_SocketItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
		}
	case ASYNCHRONISM_SET_USER_DATA:	//绑定数据
		{
			//效验数据
			tagSetUserData * pSetUserData = (tagSetUserData *)pData;
			if (wDataSize != sizeof(tagSetUserData)) return false;

			return m_SocketItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
		}
	case ASYNCHRONISM_SHUT_DOWN:		//安全关闭
		{
			//效验数据
			tagShutDownSocket * pShutDownSocket = (tagShutDownSocket *)pData;
			if (wDataSize != sizeof(tagShutDownSocket)) return false;

			return m_SocketItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
		}
	}

	return false;
}

//绑定事件
VOID CServer::OnEventSocketBind(CNativeInfo* pNativeInfo)
{
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	m_pNetworkEvent->OnEventNetworkBind(pNativeInfo->GetSocketID(), szRemoteAddress, wRemotePort, pNativeInfo->GetUserData());
}

//读取事件
DWORD CServer::OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	int nReadSize = m_pNetworkEvent->OnEventNetworkRead(pNativeInfo->GetSocketID(), pData, dwDataSize, pNativeInfo->GetUserData());
	if (nReadSize < 0) return SOCKET_ERROR;
	return nReadSize;
}

//关闭事件
VOID CServer::OnEventSocketShut(CNativeInfo* pNativeInfo)
{
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	m_pNetworkEvent->OnEventNetworkShut(pNativeInfo->GetSocketID(), szRemoteAddress, wRemotePort, pNativeInfo->GetActiveTime(), pNativeInfo->GetUserData());
}

//绑定事件
bool CServer::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket绑定事件=>连接标识:%u, 对端地址:%s:%d, 用户数据:%u"), dwSocketID, pszClientIP, wClientPort, dwUserData);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("测试数据=>The server has received your connection, 您的IP地址为:%s, 端口为:%d"), pszClientIP, wClientPort);
	SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	return true;
}

//关闭事件
bool CServer::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket关闭事件=>连接标识:%u, 对端地址:%s:%d, 激活时间:%u, 连接时间:%u秒, 用户数据:%u"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime), dwUserData);
	return true;
}

//读取事件
int CServer::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket读取事件=>连接标识:%u, 数据大小:%u, 用户数据:%u"), dwSocketID, dwDataSize, dwUserData);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);
	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CClient::CClient()
{
	m_pISocketEvent = this;
}

//析构函数
CClient::~CClient()
{
	//
}

//初始化服务
bool CClient::Init(ISocketEvent * pISocketEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxConnectCount)
{
	if (__super::InitService(wThreadCount, dwDetectTime, 0, 0, wMaxConnectCount))
	{
		m_pISocketEvent = (pISocketEvent != NULL) ? pISocketEvent : this;
		return true;
	}

	return false;
}

//释放服务
bool CClient::Release()
{
	__super::ReleaseService();
	m_pISocketEvent = this;
	return true;
}

//设置心跳包
bool CClient::SetHeartbeatPacket(VOID * pData, WORD wDataSize)
{
	return m_ConnectItemManager.SetHeartbeatData(pData, wDataSize);
}

//监听端口
DWORD CClient::Connect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return __super::ConnectServer(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//发送数据
bool CClient::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SendData(dwSocketID, pData, wDataSize);
	}

	return __super::SendData(dwSocketID, pData, wDataSize);
}

//群发数据
bool CClient::SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SendDataBatch(pData, wDataSize);
	}

	return __super::SendDataBatch(pData, wDataSize);
}

//关闭连接
bool CClient::CloseSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.CloseSocket(dwSocketID);
	}

	return __super::CloseSocket(dwSocketID);
}

//设置监测
bool CClient::SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SetDetect(dwSocketID, bDetect);
	}

	return __super::SetDetect(dwSocketID, bDetect);
}

//设置数据
bool CClient::SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SetUserData(dwSocketID, dwUserData);
	}

	return __super::SetUserData(dwSocketID, dwUserData);
}

//设置关闭
bool CClient::ShutDownSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.ShutDownSocket(dwSocketID);
	}

	return __super::ShutDownSocket(dwSocketID);
}

//异步事件
bool CClient::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case ASYNCHRONISM_SEND_DATA:		//发送请求
		{
			//效验数据
			tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)pData;
			if (wDataSize < (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + pSendDataRequest->wDataSize)) return false;

			return m_ConnectItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
		}
	case ASYNCHRONISM_SEND_BATCH:		//群发请求
		{
			//效验数据
			tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)pData;
			if (wDataSize < (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + pBatchSendRequest->wDataSize)) return false;

			return m_ConnectItemManager.SendDataBatch(pBatchSendRequest->cbSendBuffer, pBatchSendRequest->wDataSize);
		}
	case ASYNCHRONISM_CLOSE_SOCKET:		//关闭连接
		{
			//效验数据
			tagCloseSocket * pCloseSocket = (tagCloseSocket *)pData;
			if (wDataSize != sizeof(tagCloseSocket)) return false;

			return m_ConnectItemManager.CloseSocket(pCloseSocket->dwSocketID);
		}
	case ASYNCHRONISM_SET_DETECT:		//设置监测
		{
			//效验数据
			tagSetDetect * pSetDetect = (tagSetDetect *)pData;
			if (wDataSize != sizeof(tagSetDetect)) return false;

			return m_ConnectItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
		}
	case ASYNCHRONISM_SET_USER_DATA:	//绑定数据
		{
			//效验数据
			tagSetUserData * pSetUserData = (tagSetUserData *)pData;
			if (wDataSize != sizeof(tagSetUserData)) return false;

			return m_ConnectItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
		}
	}

	return false;
}

//连接事件
VOID CClient::OnEventConnectLink(CNativeInfo* pNativeInfo)
{
	m_pISocketEvent->OnEventSocketLink(pNativeInfo->GetSocketID(), pNativeInfo->GetUserData());
}

//读取事件
DWORD CClient::OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	int nReadSize = m_pISocketEvent->OnEventSocketRead(pNativeInfo->GetSocketID(), pData, dwDataSize, pNativeInfo->GetUserData());
	if (nReadSize < 0) return SOCKET_ERROR;
	return nReadSize;
}

//关闭事件
VOID CClient::OnEventConnectShut(CNativeInfo* pNativeInfo)
{
	m_pISocketEvent->OnEventSocketShut(pNativeInfo->GetSocketID(), pNativeInfo->GetUserData());
}

//连接事件
bool CClient::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect连接事件=>连接标识:%u, 用户数据:%u"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("测试数据=>test data"));
	SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));
	return true;
}

//关闭事件
bool CClient::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect关闭事件=>连接标识:%u, 用户数据:%u"), dwSocketID, dwUserData);
	return true;
}

//读取事件
int CClient::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect读取事件=>连接标识:%u, 用户数据:%u, 数据大小:%u"), dwSocketID, dwUserData, dwDataSize);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);
	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////
