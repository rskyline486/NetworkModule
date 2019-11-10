#include "stdafx.h"
#include "ClientWrapper.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CClientKernel::CClientKernel()
{
}

//析构函数
CClientKernel::~CClientKernel()
{
}

//启动内核
bool CClientKernel::StartKernel()
{
	//初始化连接服务
	if (m_Client.Init(this, 4, 10000L, 512) == false) return false;

	return true;
}

//停止内核
bool CClientKernel::StopKernel()
{
	//停止服务
	m_Client.Release();

	return true;
}

//连接事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CClientKernel::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect连接事件=>连接标识:%u, 用户数据:%u"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("测试数据=>test data"));
	m_Client.SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));

	return true;
}

//关闭事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
bool CClientKernel::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect关闭事件=>连接标识:%u, 用户数据:%u"), dwSocketID, dwUserData);

	return true;
}

//读取事件(此处回调为工作线程回调,不宜使用同步的方式来操作其他连接对象,否则容易造成死锁)
int CClientKernel::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect读取事件=>连接标识:%u, 用户数据:%u, 数据大小:%u"), dwSocketID, dwUserData, dwDataSize);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);

	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CClientInstance::CClientInstance()
{
	m_pClientEvent = NULL;
}

//析构函数
CClientInstance::~CClientInstance()
{
}

//启动服务
bool CClientInstance::StartServer(tagClientOption ClientOption, IClientEvent * pClientEvent)
{
	//参数校验
	//if (pServerEvent == NULL) return false;

	//初始化连接服务
	if (m_Client.Init(this, ClientOption.wThreadCount, ClientOption.dwDetectTime, ClientOption.wMaxConnectCount) == false) return false;

	//设置接口
	m_pClientEvent = pClientEvent;

	return true;
}

//停止服务
bool CClientInstance::StopServer()
{
	//停止服务
	__super::StopKernel();

	//重置接口
	m_pClientEvent = NULL;

	return true;
}

//连接服务
bool CClientInstance::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return (m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch) != INVALID_SOCKETID);
}

//发送数据
bool CClientInstance::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_Client.SendData(dwSocketID, pData, wDataSize);
}

//发送数据
bool CClientInstance::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_Client.SendDataBatch(pData, wDataSize);
}

//关闭连接
bool CClientInstance::CloseSocket(DWORD dwSocketID)
{
	return m_Client.CloseSocket(dwSocketID);
}

//连接事件
bool CClientInstance::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//回调处理
	if (m_pClientEvent)
	{
		return m_pClientEvent->OnClientSocketLink(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketLink(dwSocketID, dwUserData);
}

//关闭事件
bool CClientInstance::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	//回调处理
	if (m_pClientEvent)
	{
		return m_pClientEvent->OnClientSocketShut(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketShut(dwSocketID, dwUserData);
}

//读取事件
int CClientInstance::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//回调处理
	if (m_pClientEvent)
	{
		return m_pClientEvent->OnClientSocketRead(dwSocketID, pData, dwDataSize, dwUserData);
	}

	return __super::OnEventSocketRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//////////////////////////////////////////////////////////////////////////
