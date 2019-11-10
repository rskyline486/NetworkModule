#include "stdafx.h"
#include "ProxyWrapper.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CProxyKernel::CProxyKernel()
{
	ZeroMemory(&m_szDefaultAddress, sizeof(m_szDefaultAddress));
	m_wDefaultPort = 0;
	m_wDefaultProtocol = 0;
}

//析构函数
CProxyKernel::~CProxyKernel()
{
}

//启动内核
bool CProxyKernel::StartKernel(WORD wListenPort, LPCTSTR pszDefaultAddress, WORD wDefaultPort)
{
	//初始化监听服务
	if (m_Server.Init(this, 4, 10000L, 1, 512) == false) return false;
	//监听端口
	if (m_Server.Listen(wListenPort, 0L, EnableIPv4, true) == false) return false;

	//初始化连接服务
	if (m_Client.Init(this, 4, 10000L, 512) == false) return false;

	//设置参数
	lstrcpyn(m_szDefaultAddress, pszDefaultAddress, CountArray(m_szDefaultAddress));
	m_wDefaultPort = wDefaultPort;
	m_wDefaultProtocol = EnableIPv4;

	return true;
}

//停止内核
bool CProxyKernel::StopKernel()
{
	//停止服务
	m_Server.Release();
	//停止连接
	m_Client.Release();

	//重置参数
	ZeroMemory(&m_szDefaultAddress, sizeof(m_szDefaultAddress));
	m_wDefaultPort = 0;
	m_wDefaultProtocol = 0;

	return true;
}

//连接事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CProxyKernel::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect连接事件=>连接标识:%u, 用户数据:%u"), dwSocketID, dwUserData);
	
	//发送一次缓冲
	m_Client.SendData(dwSocketID, NULL, 0);
	
	return true;
}

//关闭事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CProxyKernel::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect关闭事件=>连接标识:%u, 用户数据:%u"), dwSocketID, dwUserData);

	//关闭连接
	if (CHECK_SOCKETID(dwUserData))
	{
		//异步关闭
		m_Server.CloseSocket(dwUserData);
	}
	
	return true;
}

//读取事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
int CProxyKernel::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect读取事件=>连接标识:%u, 用户数据:%u, 数据大小:%u"), dwSocketID, dwUserData, dwDataSize);
	
	//转发数据
	if (CHECK_SOCKETID(dwUserData))
	{
		//范围校验
		if (dwDataSize > SOCKET_BUFFER)
		{
			Logger_Error(TEXT("转发数据到客户端失败,数据大小异常,连接标识:%u, 用户数据:%u, 数据大小:%u"), dwSocketID, dwUserData, dwDataSize);
			return -1;
		}

		//转发数据
		if (m_Server.SendData(dwUserData, pData, (WORD)dwDataSize) == true)
		{
			return dwDataSize;
		}
	}

	return -1;
}

//绑定事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CProxyKernel::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket绑定事件=>连接标识:%u, 对端地址:%s:%d, 用户数据:%u"), dwSocketID, pszClientIP, wClientPort, dwUserData);
	
	return true;
}

//关闭事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CProxyKernel::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket关闭事件=>连接标识:%u, 对端地址:%s:%d, 激活时间:%u, 连接时间:%u秒, 用户数据:%u"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime), dwUserData);
	
	//关闭连接
	if (CHECK_SOCKETID(dwUserData))
	{
		//异步关闭
		m_Client.CloseSocket(dwUserData);
	}

	return true;
}

//读取事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
int CProxyKernel::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket读取事件=>连接标识:%u, 数据大小:%u, 用户数据:%u"), dwSocketID, dwDataSize, dwUserData);
	
	//连接判断
	if (dwUserData == INVALID_SOCKETID)
	{
		//连接服务
		DWORD dwConnectSocketID = m_Client.Connect(m_szDefaultAddress, m_wDefaultPort, dwSocketID, m_wDefaultProtocol, false);
		if (dwConnectSocketID == INVALID_SOCKETID)
		{
			Logger_Error(TEXT("连接服务失败=>szServerAddr:%s, wServerPort:%u"), m_szDefaultAddress, m_wDefaultPort);
			return -1;
		}

		//绑定句柄(同步更新自己)
		m_Server.SetUserData(dwSocketID, dwConnectSocketID, true);
		//更新数据
		dwUserData = dwConnectSocketID;
	}

	//范围校验
	if (dwDataSize > SOCKET_BUFFER)
	{
		Logger_Error(TEXT("转发数据到服务器失败,数据大小异常,连接标识:%u, 用户数据:%u, 数据大小:%u"), dwSocketID, dwUserData, dwDataSize);
		return -1;
	}

	//转发数据
	if (m_Client.SendData(dwUserData, pData, (WORD)dwDataSize) == true)
	{
		return dwDataSize;
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CProxyInstance::CProxyInstance()
{
	m_pProxyEvent = NULL;
}

//析构函数
CProxyInstance::~CProxyInstance()
{
}

//启动服务
bool CProxyInstance::StartServer(tagProxyOption ProxyOption, IProxyEvent * pProxyEvent)
{
	//参数校验
	//if (pProxyEvent == NULL) return false;

	//标识信息
	DWORD dwUserData = ProxyOption.wListenPort;
	//守护标识
	bool bWatch = true;

	//初始化监听服务
	if (m_Server.Init(this, ProxyOption.wThreadCount, ProxyOption.dwDetectTime, ProxyOption.wMaxAcceptCount, ProxyOption.wMaxSocketCount) == false) return false;
	//监听端口
	if (m_Server.Listen(ProxyOption.wListenPort, dwUserData, ProxyOption.wProtocol, bWatch) == false) return false;

	//初始化连接服务
	if (m_Client.Init(this, ProxyOption.wThreadCount, ProxyOption.dwDetectTime, ProxyOption.wMaxConnectCount) == false) return false;

	//设置接口
	m_pProxyEvent = pProxyEvent;

	//设置参数
	lstrcpyn(m_szDefaultAddress, ProxyOption.szDefaultAddress, CountArray(m_szDefaultAddress));
	m_wDefaultPort = ProxyOption.wDefaultPort;
	m_wDefaultProtocol = ProxyOption.wProtocol;

	return true;
}

//停止服务
bool CProxyInstance::StopServer()
{
	//停止服务
	__super::StopKernel();

	//重置接口
	m_pProxyEvent = NULL;

	return true;
}

//连接服务
bool CProxyInstance::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return (m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch) != INVALID_SOCKETID);
}

//代理连接
DWORD CProxyInstance::ProxyConnect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol)
{
	return m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, false);
}

//设置数据
bool CProxyInstance::SetUserData(DWORD dwSocketID, DWORD dwUserData)
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
		return m_Client.SetUserData(dwSocketID, dwUserData, true);
	}

	return m_Server.SetUserData(dwSocketID, dwUserData, true);
}

//发送数据
bool CProxyInstance::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
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
		return m_Client.SendData(dwSocketID, pData, wDataSize);
	}

	return m_Server.SendData(dwSocketID, pData, wDataSize);
}

//关闭连接
bool CProxyInstance::CloseSocket(DWORD dwSocketID)
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

//连接事件
bool CProxyInstance::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//回调处理
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyServerLink(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketLink(dwSocketID, dwUserData);
}

//关闭事件
bool CProxyInstance::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	//回调处理
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyServerShut(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketShut(dwSocketID, dwUserData);
}

//读取事件
int CProxyInstance::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//回调处理
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyServerRead(dwSocketID, pData, dwDataSize, dwUserData);
	}

	return __super::OnEventSocketRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//绑定事件
bool CProxyInstance::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	//回调处理
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyClientBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
	}

	return __super::OnEventNetworkBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
}

//关闭事件
bool CProxyInstance::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	//回调处理
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyClientShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
	}

	return __super::OnEventNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
}

//读取事件
int CProxyInstance::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//回调处理
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyClientRead(dwSocketID, pData, dwDataSize, dwUserData);
	}

	return __super::OnEventNetworkRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//////////////////////////////////////////////////////////////////////////
