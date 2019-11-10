#include "stdafx.h"
#include "AttemperWrapper.h"

//////////////////////////////////////////////////////////////////////////

//事件定义
#define EVENT_TIMER					0x0001								//时间事件
#define EVENT_CUSTOMIZE				0x0002								//自定事件
#define EVENT_CUSTOMIZE_EX			0x0003								//自定事件

#define EVENT_SOCKET_LINK			0x0004								//连接事件
#define EVENT_SOCKET_SHUT			0x0005								//关闭事件
#define EVENT_SOCKET_READ			0x0006								//读取事件

#define EVENT_NETWORK_ACCEPT		0x0007								//应答事件
#define EVENT_NETWORK_SHUT			0x0008								//关闭事件
#define EVENT_NETWORK_READ			0x0009								//读取事件

//////////////////////////////////////////////////////////////////////////

//定时器事件
struct NTY_TimerEvent
{
	DWORD							dwTimerID;							//时间标识
	WPARAM							dwBindParameter;					//绑定参数
};

//自定事件
struct NTY_CustomEvent
{
	DWORD							dwCustomID;							//自定标识
};

//自定事件
struct NTY_CustomEventEx
{
	DWORD							dwCustomID;							//自定标识
	DWORD							dwContextID;						//对象标识
};

//连接事件
struct NTY_SocketLinkEvent
{
	DWORD							dwSocketID;							//连接标识
	DWORD							dwUserData;							//用户数据
};

//关闭事件
struct NTY_SocketShutEvent
{
	DWORD							dwSocketID;							//连接标识
	DWORD							dwUserData;							//用户数据
};

//读取事件
struct NTY_SocketReadEvent
{
	DWORD							dwSocketID;							//连接标识
	DWORD							dwUserData;							//用户数据
	WORD							wDataSize;							//数据大小
	NT_Command						Command;							//命令信息
};

//应答事件
struct NTY_NetworkAcceptEvent
{
	DWORD							dwSocketID;							//连接标识
	TCHAR							szClientAddress[MAX_ADDRSTRLEN];	//用户地址
	WORD							wClientPort;						//用户端口
};

//关闭事件
struct NTY_NetworkShutEvent
{
	DWORD							dwSocketID;							//连接标识
	TCHAR							szClientAddress[MAX_ADDRSTRLEN];	//用户地址
	WORD							wClientPort;						//用户端口
	DWORD							dwActiveTime;						//连接时间
};

//读取事件
struct NTY_NetworkReadEvent
{
	DWORD							dwSocketID;							//连接标识
	WORD							wDataSize;							//数据大小
	NT_Command						Command;							//命令信息
};

//////////////////////////////////////////////////////////////////////////

//构造函数
CAttemperKernel::CAttemperKernel()
{
}

//析构函数
CAttemperKernel::~CAttemperKernel()
{
}

//启动内核
bool CAttemperKernel::StartKernel()
{
	//初始化异步对象
	if (m_AsynchronismEngine.Init(this, 1) == false) return false;

	//初始化计时器对象
	if (m_TimerEngine.Init(this, 30) == false) return false;

	//初始化连接服务
	if (m_Client.Init(this, 4, 10000L, 512) == false) return false;

	//初始化监听服务
	if (m_Server.Init(this, 4, 10000L, 1, 512) == false) return false;
	//监听端口
	if (m_Server.Listen(9999, 0L, EnableIPv4, true) == false) return false;

	return true;
}

//停止内核
bool CAttemperKernel::StopKernel()
{
	//停止服务
	m_Server.Release();

	//停止服务
	m_Client.Release();

	//停止计时器对象
	m_TimerEngine.Release();

	//停止异步对象
	m_AsynchronismEngine.Release();

	return true;
}

//异步开始
bool CAttemperKernel::OnEventAsynchronismStrat()
{
	return true;
}

//异步结束
bool CAttemperKernel::OnEventAsynchronismStop()
{
	return true;
}

//异步事件
bool CAttemperKernel::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	Logger_Info(TEXT("异步事件=>标识信息:%u, 数据大小:%u"), wIdentifier, wDataSize);
	return true;
}

//时间事件
bool CAttemperKernel::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	//同步控制
	CLocker Locker(m_Mutex);
	NTY_TimerEvent * pTimerEvent = (NTY_TimerEvent *)m_cbBuffer;

	//构造数据
	pTimerEvent->dwTimerID = dwTimerID;
	pTimerEvent->dwBindParameter = dwBindParameter;

	//投递数据
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_TIMER, m_cbBuffer, sizeof(NTY_TimerEvent));
}

//连接事件
bool CAttemperKernel::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//同步控制
	CLocker Locker(m_Mutex);
	NTY_SocketLinkEvent * pConnectEvent = (NTY_SocketLinkEvent *)m_cbBuffer;

	//构造数据
	pConnectEvent->dwSocketID = dwSocketID;
	pConnectEvent->dwUserData = dwUserData;

	//投递数据
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_SOCKET_LINK, m_cbBuffer, sizeof(NTY_SocketLinkEvent));
}

//关闭事件
bool CAttemperKernel::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	//同步控制
	CLocker Locker(m_Mutex);
	NTY_SocketShutEvent * pCloseEvent = (NTY_SocketShutEvent *)m_cbBuffer;

	//构造数据
	pCloseEvent->dwSocketID = dwSocketID;
	pCloseEvent->dwUserData = dwUserData;

	//投递数据
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_SOCKET_SHUT, m_cbBuffer, sizeof(NTY_SocketShutEvent));
}

//读取事件
int CAttemperKernel::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//校验数据
	if (dwDataSize == 0 || pData == NULL) return -1;
	if (dwDataSize > SOCKET_BUFFER) return -1;

	//命令提取
	if (dwDataSize < sizeof(NT_Command)) return -1;
	NT_Command * pCommand = (NT_Command *)pData;

	//更新数据
	pData = pCommand + 1;
	dwDataSize = dwDataSize - sizeof(NT_Command);

	//同步控制
	CLocker Locker(m_Mutex);
	NTY_SocketReadEvent * pReadEvent = (NTY_SocketReadEvent *)m_cbBuffer;

	//构造数据
	pReadEvent->dwSocketID = dwSocketID;
	pReadEvent->dwUserData = dwUserData;
	pReadEvent->wDataSize = (WORD)dwDataSize;
	pReadEvent->Command = *pCommand;

	//附加数据
	if (dwDataSize > 0)
	{
		CopyMemory(m_cbBuffer + sizeof(NTY_SocketReadEvent), pData, dwDataSize);
	}

	//投递数据
	if (m_AsynchronismEngine.PostAsynchronismData(EVENT_SOCKET_READ, m_cbBuffer, sizeof(NTY_SocketReadEvent) + (WORD)dwDataSize) == true)
	{
		return dwUserData;
	}

	return -1;
}

//绑定事件
bool CAttemperKernel::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	//同步控制
	CLocker Locker(m_Mutex);
	NTY_NetworkAcceptEvent * pAcceptEvent = (NTY_NetworkAcceptEvent *)m_cbBuffer;

	//构造数据
	pAcceptEvent->dwSocketID = dwSocketID;
	lstrcpyn(pAcceptEvent->szClientAddress, pszClientIP, CountArray(pAcceptEvent->szClientAddress));
	pAcceptEvent->wClientPort = wClientPort;

	//投递数据
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_NETWORK_ACCEPT, m_cbBuffer, sizeof(NTY_NetworkAcceptEvent));
}

//关闭事件
bool CAttemperKernel::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	//同步控制
	CLocker Locker(m_Mutex);
	NTY_NetworkShutEvent * pCloseEvent = (NTY_NetworkShutEvent *)m_cbBuffer;

	//构造数据
	pCloseEvent->dwSocketID = dwSocketID;
	lstrcpyn(pCloseEvent->szClientAddress, pszClientIP, CountArray(pCloseEvent->szClientAddress));
	pCloseEvent->wClientPort = wClientPort;
	pCloseEvent->dwActiveTime = dwActiveTime;

	//投递数据
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_NETWORK_SHUT, m_cbBuffer, sizeof(NTY_NetworkShutEvent));
}

//读取事件
int CAttemperKernel::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//校验数据
	if (dwDataSize == 0 || pData == NULL) return -1;
	if (dwDataSize > SOCKET_BUFFER) return -1;

	//命令提取
	if (dwDataSize < sizeof(NT_Command)) return -1;
	NT_Command * pCommand = (NT_Command *)pData;

	//更新数据
	pData = pCommand + 1;
	dwDataSize = dwDataSize - sizeof(NT_Command);

	//同步控制
	CLocker Locker(m_Mutex);
	NTY_NetworkReadEvent * pReadEvent = (NTY_NetworkReadEvent *)m_cbBuffer;

	//构造数据
	pReadEvent->dwSocketID = dwSocketID;
	pReadEvent->wDataSize = (WORD)dwDataSize;
	pReadEvent->Command = *pCommand;

	if (dwDataSize > 0)
	{
		//附加数据
		CopyMemory(m_cbBuffer + sizeof(NTY_NetworkReadEvent), pData, dwDataSize);
	}

	//投递数据
	if (m_AsynchronismEngine.PostAsynchronismData(EVENT_NETWORK_READ, m_cbBuffer, sizeof(NTY_NetworkReadEvent) + (WORD)dwDataSize) == true)
	{
		return dwUserData;
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CAttemperInstance::CAttemperInstance()
{
	m_pAttemperEvent = NULL;
}

//析构函数
CAttemperInstance::~CAttemperInstance()
{
}

//启动服务
bool CAttemperInstance::StartServer(tagAttemperOption AttemperOption, IAttemperEvent * pAttemperEvent)
{
	//参数校验
	if (pAttemperEvent == NULL) return false;
	if (AttemperOption.pClientOption == NULL && AttemperOption.pServerOption == NULL) return false;

	//初始化管理对象
	if (m_CryptoManager.Init() == false) return false;

	//初始化异步对象
	if (m_AsynchronismEngine.Init(this, 1) == false) return false;

	//初始化计时器对象
	if (m_TimerEngine.Init(this, 30) == false) return false;

	//如果有客户端
	if (AttemperOption.pClientOption)
	{
		//初始化连接服务
		if (m_Client.Init(this, AttemperOption.pClientOption->wThreadCount, AttemperOption.pClientOption->dwDetectTime, AttemperOption.pClientOption->wMaxConnectCount) == false) return false;
	}

	//如果有服务端
	if (AttemperOption.pServerOption)
	{
		//标识信息
		DWORD dwUserData = AttemperOption.pServerOption->wListenPort;
		//守护标识
		bool bWatch = true;
		//初始化监听服务
		if (m_Server.Init(this, AttemperOption.pServerOption->wThreadCount, AttemperOption.pServerOption->dwDetectTime, AttemperOption.pServerOption->wMaxAcceptCount, AttemperOption.pServerOption->wMaxSocketCount) == false) return false;

		//加密对象
		CCryptoHelper CryptoHelper(m_CryptoManager);
		//加密数据
		tagEncryptData EncryptData;
		if (CryptoHelper.Encrypt(MDM_NT_COMMAND, SUB_NT_DETECT_SOCKET, NULL, 0, EncryptData) == false) return false;
		//设置心跳数据
		if (m_Server.SetHeartbeatPacket(EncryptData.pDataBuffer, EncryptData.wDataSize) == false) return false;

		//监听端口
		if (m_Server.Listen(AttemperOption.pServerOption->wListenPort, dwUserData, AttemperOption.pServerOption->wProtocol, bWatch) == false) return false;
	}

	//设置接口
	m_pAttemperEvent = pAttemperEvent;

	return true;
}

//停止服务
bool CAttemperInstance::StopServer()
{
	//停止服务
	__super::StopKernel();

	//释放管理对象
	m_CryptoManager.Release();

	return true;
}

//连接服务
bool CAttemperInstance::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return (m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch) != INVALID_SOCKETID);
}

//发送数据
bool CAttemperInstance::SendData(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	//标识检测
	if (CHECK_SOCKETID(dwSocketID) == false) return false;

	//获取索引
	WORD wIndex = SOCKET_INDEX(dwSocketID);
	if (wIndex >= INDEX_LISTEN)
	{
		Logger_Info(TEXT("发送数据=>异常索引信息,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize);
		return false;
	}

	//加密对象
	CCryptoHelper CryptoHelper(m_CryptoManager);

	//加密数据
	tagEncryptData EncryptData;
	if (CryptoHelper.Encrypt(wMainCmdID, wSubCmdID, pData, wDataSize, EncryptData) == false)
	{
		Logger_Info(TEXT("发送数据=>加密数据失败,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize);
		return false;
	}

	//连接发送
	if (wIndex >= INDEX_CONNECT)
	{
		if (m_Client.SendData(dwSocketID, EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
		{
			Logger_Info(TEXT("连接发送=>发送数据失败,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
			return false;
		}
	}
	
	//服务发送
	if (m_Server.SendData(dwSocketID, EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
	{
		Logger_Info(TEXT("服务发送=>发送数据失败,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
		return false;
	}

	return true;
}

//群发数据
bool CAttemperInstance::SendDataBatch(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	//加密对象
	CCryptoHelper CryptoHelper(m_CryptoManager);

	//加密数据
	tagEncryptData EncryptData;
	if (CryptoHelper.Encrypt(wMainCmdID, wSubCmdID, pData, wDataSize, EncryptData) == false)
	{
		Logger_Info(TEXT("群发数据=>加密数据失败,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), wMainCmdID, wSubCmdID, wDataSize);
		return false;
	}

	//发送数据
	if (m_Client.SendDataBatch(EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
	{
		//Logger_Info(TEXT("连接群发=>发送数据失败,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
	}

	//发送数据
	if (m_Server.SendDataBatch(EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
	{
		//Logger_Info(TEXT("网络群发=>发送数据失败,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
	}

	return true;
}

//关闭连接
bool CAttemperInstance::CloseSocket(DWORD dwSocketID)
{
	//标识检测
	if (CHECK_SOCKETID(dwSocketID) == false) return false;

	//获取索引
	WORD wIndex = SOCKET_INDEX(dwSocketID);
	if (wIndex >= INDEX_LISTEN)
	{
		return false;
	}
	else if (wIndex >= INDEX_CONNECT)
	{
		return m_Client.CloseSocket(dwSocketID);
	}

	return m_Server.CloseSocket(dwSocketID);
}

//安全关闭
bool CAttemperInstance::ShutDownSocket(DWORD dwSocketID)
{
	//标识检测
	if (CHECK_SOCKETID(dwSocketID) == false) return false;

	//获取索引
	WORD wIndex = SOCKET_INDEX(dwSocketID);
	if (wIndex >= INDEX_LISTEN)
	{
		return false;
	}
	else if (wIndex >= INDEX_CONNECT)
	{
		return m_Client.ShutDownSocket(dwSocketID);
	}

	return m_Server.ShutDownSocket(dwSocketID);
}

//设置定时器
bool CAttemperInstance::SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter)
{
	return m_TimerEngine.SetTimer(dwTimerID, dwElapse, dwRepeat, dwBindParameter);
}

//删除定时器
bool CAttemperInstance::KillTimer(DWORD dwTimerID)
{
	return m_TimerEngine.KillTimer(dwTimerID);
}

//删除定时器
bool CAttemperInstance::KillAllTimer()
{
	return m_TimerEngine.KillAllTimer();
}

//投递控制
bool CAttemperInstance::PostCustomEvent(DWORD dwCustomID, VOID * pData, WORD wDataSize)
{
	//校验数据
	if (wDataSize > SOCKET_BUFFER) return false;

	//同步控制
	CLocker Locker(m_Mutex);
	NTY_CustomEvent * pCustomEvent = (NTY_CustomEvent *)m_cbBuffer;

	//构造数据
	pCustomEvent->dwCustomID = dwCustomID;

	//附加数据
	if (wDataSize>0)
	{
		CopyMemory(m_cbBuffer + sizeof(NTY_CustomEvent), pData, wDataSize);
	}

	//投递数据
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_CUSTOMIZE, m_cbBuffer, sizeof(NTY_CustomEvent) + wDataSize);
}

//投递事件
bool CAttemperInstance::PostCustomEventEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//校验数据
	if (wDataSize > SOCKET_BUFFER) return false;

	//同步控制
	CLocker Locker(m_Mutex);
	NTY_CustomEventEx * pCustomEventEx = (NTY_CustomEventEx *)m_cbBuffer;

	//构造数据
	pCustomEventEx->dwCustomID = dwCustomID;
	pCustomEventEx->dwContextID = dwContextID;

	//附加数据
	if (wDataSize > 0)
	{
		CopyMemory(m_cbBuffer + sizeof(NTY_CustomEventEx), pData, wDataSize);
	}

	//投递数据
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_CUSTOMIZE_EX, m_cbBuffer, sizeof(NTY_CustomEventEx) + wDataSize);
}

//异步事件
bool CAttemperInstance::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	//内核事件
	switch (wIdentifier)
	{
	case EVENT_TIMER:				//时间事件
		{
			//数据校验
			if (wDataSize!=sizeof(NTY_TimerEvent)) return false;

			//处理消息
			NTY_TimerEvent * pTimerEvent=(NTY_TimerEvent *)pData;
			m_pAttemperEvent->OnAttemperTimer(pTimerEvent->dwTimerID,pTimerEvent->dwBindParameter);

			return true;
		}
	case EVENT_CUSTOMIZE:			//自定事件
		{
			//数据校验
			if (wDataSize<sizeof(NTY_CustomEvent)) return false;

			//处理消息
			NTY_CustomEvent * pCustomEvent=(NTY_CustomEvent *)pData;
			m_pAttemperEvent->OnAttemperCustom(pCustomEvent->dwCustomID, pCustomEvent +1,wDataSize-sizeof(NTY_CustomEvent));

			return true;
		}
	case EVENT_CUSTOMIZE_EX:		//自定事件
		{
			//数据校验
			if (wDataSize<sizeof(NTY_CustomEventEx)) return false;

			//处理消息
			NTY_CustomEventEx * pCustomEventEx=(NTY_CustomEventEx *)pData;
			m_pAttemperEvent->OnAttemperCustomEx(pCustomEventEx->dwCustomID, pCustomEventEx->dwContextID, pCustomEventEx +1,wDataSize-sizeof(NTY_CustomEventEx));

			return true;
		}
	case EVENT_SOCKET_LINK:			//连接事件
		{
			//数据校验
			if (wDataSize!=sizeof(NTY_SocketLinkEvent)) return false;

			//处理消息
			NTY_SocketLinkEvent * pConnectEvent=(NTY_SocketLinkEvent *)pData;
			m_pAttemperEvent->OnAttemperSocketLink(pConnectEvent->dwSocketID,pConnectEvent->dwUserData);

			return true;
		}
	case EVENT_SOCKET_SHUT:			//关闭事件
		{
			//数据校验
			if (wDataSize!=sizeof(NTY_SocketShutEvent)) return false;

			//处理消息
			NTY_SocketShutEvent * pCloseEvent=(NTY_SocketShutEvent *)pData;
			m_pAttemperEvent->OnAttemperSocketShut(pCloseEvent->dwSocketID,pCloseEvent->dwUserData);

			return true;
		}
	case EVENT_SOCKET_READ:			//读取事件
		{
			//变量定义
			NTY_SocketReadEvent * pReadEvent=(NTY_SocketReadEvent *)pData;

			//数据校验
			if (wDataSize<sizeof(NTY_SocketReadEvent)) return false;
			if (wDataSize!=(sizeof(NTY_SocketReadEvent)+pReadEvent->wDataSize)) return false;

			//处理消息
			m_pAttemperEvent->OnAttemperSocketRead(pReadEvent->dwSocketID,pReadEvent->Command.wMainCmdID,pReadEvent->Command.wSubCmdID,pReadEvent+1,pReadEvent->wDataSize,pReadEvent->dwUserData);

			return true;
		}
	case EVENT_NETWORK_ACCEPT:		//应答事件
		{
			//数据校验
			if (wDataSize!=sizeof(NTY_NetworkAcceptEvent)) return false;

			//变量定义
			bool bSuccess=false;
			NTY_NetworkAcceptEvent * pAcceptEvent=(NTY_NetworkAcceptEvent *)pData;

			//处理消息
			try
			{ 
				bSuccess= m_pAttemperEvent->OnAttemperNetworkBind(pAcceptEvent->dwSocketID, pAcceptEvent->szClientAddress, pAcceptEvent->wClientPort);
			}
			catch (...)	{ }

			//失败处理
			if (bSuccess == false)
			{
				Logger_Warn(TEXT("网络绑定=>处理异常,关闭连接,dwSocketID:%u,szClientAddress:%s,wClientPort:%u"), pAcceptEvent->dwSocketID, pAcceptEvent->szClientAddress, pAcceptEvent->wClientPort);
				m_Server.CloseSocket(pAcceptEvent->dwSocketID);
			}

			return true;
		}
	case EVENT_NETWORK_SHUT:		//关闭事件
		{
			//数据校验
			if (wDataSize!=sizeof(NTY_NetworkShutEvent)) return false;

			//处理消息
			NTY_NetworkShutEvent * pCloseEvent=(NTY_NetworkShutEvent *)pData;
			m_pAttemperEvent->OnAttemperNetworkShut(pCloseEvent->dwSocketID,pCloseEvent->szClientAddress,pCloseEvent->wClientPort,pCloseEvent->dwActiveTime);

			return true;
		}
	case EVENT_NETWORK_READ:		//读取事件
		{
			//效验大小
			NTY_NetworkReadEvent * pReadEvent=(NTY_NetworkReadEvent *)pData;

			//数据校验
			if (wDataSize<sizeof(NTY_NetworkReadEvent)) return false;
			if (wDataSize!=(sizeof(NTY_NetworkReadEvent)+pReadEvent->wDataSize)) return false;

			//处理消息
			bool bSuccess=false;
			try
			{ 
				bSuccess= m_pAttemperEvent->OnAttemperNetworkRead(pReadEvent->dwSocketID,pReadEvent->Command.wMainCmdID,pReadEvent->Command.wSubCmdID,pReadEvent+1,pReadEvent->wDataSize);
			}
			catch (...)	{ }

			//失败处理
			if (bSuccess == false)
			{
				Logger_Warn(TEXT("网络读取=>处理异常,关闭连接,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), pReadEvent->dwSocketID, pReadEvent->Command.wMainCmdID, pReadEvent->Command.wSubCmdID, pReadEvent->wDataSize);
				m_Server.CloseSocket(pReadEvent->dwSocketID);
			}

			return true;
		}
	}

	return __super::OnEventAsynchronismData(wIdentifier, pData, wDataSize);
}

//时间事件
bool CAttemperInstance::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	return __super::OnEventTimer(dwTimerID, dwBindParameter);
}

//连接事件
bool CAttemperInstance::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//发送认证
	NT_Validate Validate;
	ZeroMemory(&Validate, sizeof(Validate));
	lstrcpyn(Validate.szValidateKey, szValidateSecretKey, CountArray(Validate.szValidateKey));
	SendData(dwSocketID, MDM_NT_COMMAND, SUB_NT_VALIDATE_SOCKET, &Validate, sizeof(Validate));

	return __super::OnEventSocketLink(dwSocketID, dwUserData);
}

//关闭事件
bool CAttemperInstance::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	return __super::OnEventSocketShut(dwSocketID, dwUserData);
}

//读取事件
int CAttemperInstance::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//解析数据
	int nReadSize = 0;
	while ((dwDataSize - nReadSize) >= sizeof(NT_Head))
	{
		//变量定义
		NT_Info * pInfo = (NT_Info *)((BYTE*)pData + nReadSize);

		//长度校验
		if ((pInfo->wPacketSize < sizeof(NT_Info)) || (pInfo->wPacketSize > SOCKET_BUFFER))
		{
			Logger_Error(TEXT("连接读取=>数据包长度异常,dwSocketID:%u,dwDataSize:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwDataSize, dwUserData, pInfo->wPacketSize);
			return -1;
		}

		//完整判断
		if ((dwDataSize - nReadSize) < pInfo->wPacketSize)
		{
			return nReadSize;
		}

		//变量定义
		WORD wBufferDataSize = pInfo->wPacketSize;

		//解密对象
		CCryptoHelper CryptoHelper(m_CryptoManager);

		//解密数据
		tagDecryptData DecryptData;
		if (CryptoHelper.Decrypt((LPBYTE)pData + nReadSize, wBufferDataSize, DecryptData) == false)
		{
			Logger_Error(TEXT("连接读取=>解析数据包异常,dwSocketID:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwUserData, wBufferDataSize);
			return -1;
		}

		//更新处理
		nReadSize += wBufferDataSize;

		//网络检测
		if ((DecryptData.Command.wMainCmdID == MDM_NT_COMMAND) && (DecryptData.Command.wSubCmdID == SUB_NT_DETECT_SOCKET))
		{
			//Logger_Info(TEXT("连接读取=>心跳数据,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
			//SendData(dwSocketID, MDM_NT_COMMAND, SUB_NT_DETECT_SOCKET, NULL, 0);
			continue;
		}

		//常规判断
		if (DecryptData.Command.wMainCmdID != MDM_NT_COMMAND)
		{
			//处理数据
			if (__super::OnEventSocketRead(dwSocketID, DecryptData.pDataBuffer, DecryptData.wDataSize, dwUserData) < 0)
			{
				Logger_Error(TEXT("连接读取=>处理数据包异常,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
				return -1;
			}
		}
	}

	return nReadSize;
}

//绑定事件
bool CAttemperInstance::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	return __super::OnEventNetworkBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
}

//关闭事件
bool CAttemperInstance::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	return __super::OnEventNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
}

//读取事件
int CAttemperInstance::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//解析数据
	int nReadSize = 0;
	while ((dwDataSize - nReadSize) >= sizeof(NT_Head))
	{
		//变量定义
		NT_Info * pInfo = (NT_Info *)((BYTE*)pData + nReadSize);

		//长度校验
		if ((pInfo->wPacketSize < sizeof(NT_Info)) || (pInfo->wPacketSize > SOCKET_BUFFER))
		{
			Logger_Error(TEXT("网络读取=>数据包长度异常,dwSocketID:%u,dwDataSize:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwDataSize, dwUserData, pInfo->wPacketSize);
			return -1;
		}

		//完整判断
		if ((dwDataSize - nReadSize) < pInfo->wPacketSize)
		{
			return nReadSize;
		}

		//变量定义
		WORD wBufferDataSize = pInfo->wPacketSize;

		//解密对象
		CCryptoHelper CryptoHelper(m_CryptoManager);

		//解密数据
		tagDecryptData DecryptData;
		if (CryptoHelper.Decrypt((LPBYTE)pData + nReadSize, wBufferDataSize, DecryptData) == false)
		{
			Logger_Error(TEXT("网络读取=>解析数据包异常,dwSocketID:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwUserData, wBufferDataSize);
			return -1;
		}

		//更新处理
		nReadSize += wBufferDataSize;

		//认证判断
		if (dwUserData == 0)
		{
			if (DecryptData.Command.wMainCmdID != MDM_NT_COMMAND || DecryptData.Command.wSubCmdID != SUB_NT_VALIDATE_SOCKET || (DecryptData.wDataSize - sizeof(NT_Command)) != sizeof(NT_Validate))
			{
				Logger_Error(TEXT("网络读取=>认证消息异常,wMainCmdID:[%d],wSubCmdID:[%d],wDataSize:[%d]"), DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
				return -1;
			}

			//获取授权结构
			NT_Validate * pValidate = (NT_Validate *)((LPBYTE)DecryptData.pDataBuffer + sizeof(NT_Command));
			pValidate->szValidateKey[CountArray(pValidate->szValidateKey) - 1] = 0;
			if (memcmp(pValidate->szValidateKey, szValidateSecretKey, sizeof(szValidateSecretKey)) != 0)
			{
				Logger_Error(TEXT("网络读取=>异常授权码=>%s"), pValidate->szValidateKey);
				return -1;
			}

			//更新用户数据(同步跟新,注意:这里只能同步更新自己,若更新其他socket对象则容易产生死锁)
			dwUserData = 1;
			m_Server.SetUserData(dwSocketID, dwUserData, true);
			continue;
		}

		//网络检测
		if ((DecryptData.Command.wMainCmdID == MDM_NT_COMMAND) && (DecryptData.Command.wSubCmdID == SUB_NT_DETECT_SOCKET))
		{
			//Logger_Info(TEXT("网络读取=>心跳数据,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
			//SendData(dwSocketID, MDM_NT_COMMAND, SUB_NT_DETECT_SOCKET, NULL, 0);
			continue;
		}

		//常规判断
		if (DecryptData.Command.wMainCmdID != MDM_NT_COMMAND)
		{
			//处理数据
			if (__super::OnEventNetworkRead(dwSocketID, DecryptData.pDataBuffer, DecryptData.wDataSize, dwUserData) < 0)
			{
				Logger_Error(TEXT("网络读取=>处理数据包异常,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
				return -1;
			}
		}
	}

	return nReadSize;
}

//////////////////////////////////////////////////////////////////////////
