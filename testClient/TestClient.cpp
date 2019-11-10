#include "stdafx.h"
#include "TestClient.h"

CTestClient::CTestClient()
{
	m_pClientService = NULL;
}

CTestClient::~CTestClient()
{
}

//启动服务
bool CTestClient::StartClient()
{
	//启动模块
	CNetworkModule::StartupNetwork();

	//创建服务
	if (CNetworkModule::CreateService(&m_pClientService) == false) return false;

	//配置参数
	tagClientOption ClientOption;
	ClientOption.wThreadCount = 4;
	ClientOption.dwDetectTime = 8000;
	ClientOption.wMaxConnectCount = 512;

	//启动服务
	return m_pClientService->StartServer(ClientOption, this);
}

//停止服务
bool CTestClient::StopClient()
{
	//停止服务
	if (m_pClientService)
	{
		m_pClientService->StopServer();
		CNetworkModule::DestroyService(&m_pClientService);
	}

	//清理模块
	CNetworkModule::CleanupNetwork();

	return true;
}

//连接服务
bool CTestClient::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return m_pClientService->ConnectServer(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//发送数据
bool CTestClient::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_pClientService->SendData(dwSocketID, pData, wDataSize);
}

//群发数据
bool CTestClient::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_pClientService->SendDataBatch(pData, wDataSize);
}

//连接事件
bool CTestClient::OnClientSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("连接事件=>连接标识:%u, 用户数据:%u\n"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("测试数据=>test data"));
	m_pClientService->SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));
	return true;
}

//关闭事件
bool CTestClient::OnClientSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("关闭事件=>连接标识:%u, 用户数据:%u\n"), dwSocketID, dwUserData);
	return true;
}

//读取事件
int CTestClient::OnClientSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	_tprintf(TEXT("读取事件=>连接标识:%u, 用户数据:%u, 数据大小:%u\n"), dwSocketID, dwUserData, dwDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return dwDataSize;
}

//运行方法
#include <iostream>
int CTestClient::Run(int argc, TCHAR* argv[])
{
	//启动服务
	CTestClient TestClient;
	if (TestClient.StartClient() == true)
	{
		//连接服务
		TestClient.ConnectServer(TEXT("192.168.29.224"), 9999, 1234, 0, true);

		//测试数据
		TCHAR szTestData[1024] = TEXT("");
		_sntprintf_s(szTestData, CountArray(szTestData), TEXT("客户端数据=>测试数据cesgagd154545阿斯顿发送到发asdf1545asdf45a4sd5f"));

		//运行服务
		char ch = 0;
		while (ch != 'x' && ch != 'X')
		{
			ch = std::cin.get();
			//群发数据
			TestClient.SendDataBatch(szTestData, CountStringBuffer(szTestData));
		}
	}

	//停止服务
	TestClient.StopClient();

	//结束程序
	system("pause");

	return 0;
}