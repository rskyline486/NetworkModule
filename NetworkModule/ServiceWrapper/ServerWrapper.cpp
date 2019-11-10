#include "stdafx.h"
#include "ServerWrapper.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CServerKernel::CServerKernel()
{
}

//析构函数
CServerKernel::~CServerKernel()
{
}

//启动内核
bool CServerKernel::StartKernel()
{
	//初始化监听服务
	if (m_Server.Init(this, 4, 10000L, 1, 512) == false) return false;
	//监听端口
	if (m_Server.Listen(9999, 0L, EnableIPv4, true) == false) return false;

	return true;
}

//停止内核
bool CServerKernel::StopKernel()
{
	//停止服务
	m_Server.Release();

	return true;
}

//绑定事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CServerKernel::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket绑定事件=>连接标识:%u, 对端地址:%s:%d, 用户数据:%u"), dwSocketID, pszClientIP, wClientPort, dwUserData);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("测试数据=>The server has received your connection, 您的IP地址为:%s, 端口为:%d"), pszClientIP, wClientPort);
	m_Server.SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	
	return true;
}

//关闭事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CServerKernel::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket关闭事件=>连接标识:%u, 对端地址:%s:%d, 激活时间:%u, 连接时间:%u秒, 用户数据:%u"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime), dwUserData);
	
	return true;
}

//读取事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
int CServerKernel::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket读取事件=>连接标识:%u, 数据大小:%u, 用户数据:%u"), dwSocketID, dwDataSize, dwUserData);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);

	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CServerInstance::CServerInstance()
{
	m_pServerEvent = NULL;
}

//析构函数
CServerInstance::~CServerInstance()
{
}

//启动服务
bool CServerInstance::StartServer(tagServerOption ServerOption, IServerEvent * pServerEvent)
{
	//参数校验
	//if (pServerEvent == NULL) return false;

	//标识信息
	DWORD dwUserData = ServerOption.wListenPort;
	//守护标识
	bool bWatch = true;

	//初始化监听服务
	if (m_Server.Init(this, ServerOption.wThreadCount, ServerOption.dwDetectTime, ServerOption.wMaxAcceptCount, ServerOption.wMaxSocketCount) == false) return false;
	//设置心跳数据
	if (ServerOption.pHeartbeatData != NULL && ServerOption.wHeartbeatDataSize > 0)
	{
		if (m_Server.SetHeartbeatPacket(ServerOption.pHeartbeatData, ServerOption.wHeartbeatDataSize) == false) return false;
	}
	//监听端口
	if (m_Server.Listen(ServerOption.wListenPort, dwUserData, ServerOption.wProtocol, bWatch) == false) return false;

	//设置接口
	m_pServerEvent = pServerEvent;

	return true;
}

//停止服务
bool CServerInstance::StopServer()
{
	//停止服务
	__super::StopKernel();

	//重置接口
	m_pServerEvent = NULL;

	return true;
}

//发送数据
bool CServerInstance::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_Server.SendData(dwSocketID, pData, wDataSize);
}

//发送数据
bool CServerInstance::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_Server.SendDataBatch(pData, wDataSize);
}

//关闭连接
bool CServerInstance::CloseSocket(DWORD dwSocketID)
{
	return m_Server.CloseSocket(dwSocketID);
}

//绑定事件
bool CServerInstance::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	//回调处理
	if (m_pServerEvent)
	{
		return m_pServerEvent->OnServerNetworkBind(dwSocketID, pszClientIP, wClientPort);
	}

	return __super::OnEventNetworkBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
}

//关闭事件
bool CServerInstance::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	//回调处理
	if (m_pServerEvent)
	{
		return m_pServerEvent->OnServerNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime);
	}

	return __super::OnEventNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
}

//读取事件
int CServerInstance::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//回调处理
	if (m_pServerEvent)
	{
		return m_pServerEvent->OnServerNetworkRead(dwSocketID, pData, dwDataSize);
	}

	return __super::OnEventNetworkRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//////////////////////////////////////////////////////////////////////////
