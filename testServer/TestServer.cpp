#include "stdafx.h"
#include "TestServer.h"
#include <time.h>

CTestServer::CTestServer()
{
	m_pServerService = NULL;
}

CTestServer::~CTestServer()
{
}

//启动服务
bool CTestServer::StartServer()
{
	//启动模块
	CNetworkModule::StartupNetwork();

	//创建服务
	if (CNetworkModule::CreateService(&m_pServerService) == false) return false;

	//心跳检测
	TCHAR szHeartbeat[128] = TEXT("");
	_sntprintf_s(szHeartbeat, CountArray(szHeartbeat), TEXT("服务端心跳检测"));

	//配置参数
	tagServerOption ServerOption;
	ServerOption.wListenPort = 9999;
	ServerOption.wThreadCount = 4;
	ServerOption.dwDetectTime = 8000;
	ServerOption.wProtocol = 0;
	ServerOption.wMaxAcceptCount = 1;
	ServerOption.wMaxSocketCount = 512;
	ServerOption.pHeartbeatData = szHeartbeat;
	ServerOption.wHeartbeatDataSize = CountStringBuffer(szHeartbeat);

	//启动服务
	return m_pServerService->StartServer(ServerOption, this);
}

//停止服务
bool CTestServer::StopServer()
{
	//停止服务
	if (m_pServerService)
	{
		m_pServerService->StopServer();
		CNetworkModule::DestroyService(&m_pServerService);
	}

	//清理模块
	CNetworkModule::CleanupNetwork();

	return true;
}

//发送数据
bool CTestServer::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_pServerService->SendData(dwSocketID, pData, wDataSize);
}

//群发数据
bool CTestServer::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_pServerService->SendDataBatch( pData, wDataSize);
}

//绑定事件
bool CTestServer::OnServerNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort)
{
	_tprintf(TEXT("绑定事件=>连接标识:%u, 对端地址:%s:%d\n"), dwSocketID, pszClientIP, wClientPort);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("测试数据=>The server has received your connection, 您的IP地址为:%s, 端口为:%d"), pszClientIP, wClientPort);
	m_pServerService->SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	return true;
}

//关闭事件
bool CTestServer::OnServerNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime)
{
	_tprintf(TEXT("关闭事件=>连接标识:%u, 对端地址:%s:%d, 激活时间:%u, 连接时间:%u秒\n"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime));
	return true;
}

//读取事件
int CTestServer::OnServerNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize)
{
	_tprintf(TEXT("读取事件=>连接标识:%u, 数据大小:%u\n"), dwSocketID, dwDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return dwDataSize;
}

//运行方法
#include <iostream>
int CTestServer::Run(int argc, TCHAR* argv[])
{
	//启动服务
	CTestServer TestServer;
	if (TestServer.StartServer() == true)
	{
		//测试数据
		TCHAR szTestData[1024] = TEXT("");
		_sntprintf_s(szTestData, CountArray(szTestData), TEXT("服务端数据=>测试数据cesgagd154545阿斯顿发送到发asdf1545asdf45a4sd5f"));

		//运行服务
		char ch = 0;
		while (ch != 'x' && ch != 'X')
		{
			ch = std::cin.get();
			//群发数据
			TestServer.SendDataBatch(szTestData, CountStringBuffer(szTestData));
		}
	}

	//停止服务
	TestServer.StopServer();

	//结束程序
	system("pause");

	return 0;
}