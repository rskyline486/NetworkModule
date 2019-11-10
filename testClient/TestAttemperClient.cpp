#include "stdafx.h"
#include "TestAttemperClient.h"
#include <time.h>

CTestAttemperClient::CTestAttemperClient()
{
	m_pAttemperService = NULL;
}

CTestAttemperClient::~CTestAttemperClient()
{
}

//启动服务
bool CTestAttemperClient::StartClient()
{
	//启动模块
	CNetworkModule::StartupNetwork();

	//创建服务
	if (CNetworkModule::CreateService(&m_pAttemperService) == false) return false;

	//心跳检测
	TCHAR szHeartbeat[128] = TEXT("");
	_sntprintf_s(szHeartbeat, CountArray(szHeartbeat), TEXT("服务端心跳检测"));

	//配置参数
	tagServerOption ServerOption;
	ServerOption.wListenPort = 9999;
	ServerOption.wThreadCount = 4;
	ServerOption.dwDetectTime = 10000;
	ServerOption.wProtocol = 0;
	ServerOption.wMaxAcceptCount = 1;
	ServerOption.wMaxSocketCount = 512;
	ServerOption.pHeartbeatData = szHeartbeat;
	ServerOption.wHeartbeatDataSize = CountStringBuffer(szHeartbeat);

	//配置参数
	tagClientOption ClientOption;
	ClientOption.wThreadCount = 4;
	ClientOption.dwDetectTime = 10000;
	ClientOption.wMaxConnectCount = 512;

	//调度参数
	tagAttemperOption AttemperOption;
	ZeroMemory(&AttemperOption, sizeof(AttemperOption));
	//AttemperOption.pServerOption = &ServerOption;
	AttemperOption.pClientOption = &ClientOption;

	//启动服务
	return m_pAttemperService->StartServer(AttemperOption, this);
}

//停止服务
bool CTestAttemperClient::StopClient()
{
	//停止服务
	if (m_pAttemperService)
	{
		m_pAttemperService->StopServer();
		CNetworkModule::DestroyService(&m_pAttemperService);
	}

	//清理模块
	CNetworkModule::CleanupNetwork();

	return true;
}

//连接服务
bool CTestAttemperClient::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//启动计时器
	m_pAttemperService->SetTimer(5555, 8 * 1000, 10, 8888);
	return m_pAttemperService->ConnectServer(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//发送数据
bool CTestAttemperClient::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_pAttemperService->SendData(dwSocketID, 100, 200, pData, wDataSize);
}

//群发数据
bool CTestAttemperClient::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_pAttemperService->PostCustomEvent(22, pData, wDataSize);
	//return m_pAttemperService->SendDataBatch(200, 800, pData, wDataSize);
}

//时间事件
bool CTestAttemperClient::OnAttemperTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	_tprintf(TEXT("时间事件=>时间标识:%u, 绑定数据:%u\n"), dwTimerID, dwBindParameter);
	//测试数据
	TCHAR szTestData[1024] = TEXT("");
	_sntprintf_s(szTestData, CountArray(szTestData), TEXT("计时器消息"));
	m_pAttemperService->SendDataBatch(55, 800, szTestData, CountStringBuffer(szTestData));
	return true;
}

//自定事件
bool CTestAttemperClient::OnAttemperCustom(DWORD dwCustomID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("自定事件=>自定标识:%u, 数据大小:%u\n"), dwCustomID, wDataSize);
	m_pAttemperService->SendDataBatch(22, 800, pData, wDataSize);
	return true;
}

//自定事件
bool CTestAttemperClient::OnAttemperCustomEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("自定事件=>自定标识:%u, 内容标识:%u, 数据大小:%u\n"), dwCustomID, dwContextID, wDataSize);
	return true;
}

//连接绑定
bool CTestAttemperClient::OnAttemperSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("连接绑定=>连接标识:%u, 用户数据:%u\n"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("测试数据=>test data"));
	SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));
	return true;
}

//连接关闭
bool CTestAttemperClient::OnAttemperSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("连接关闭=>连接标识:%u, 用户数据:%u\n"), dwSocketID, dwUserData);
	return true;
}

//连接读取
bool CTestAttemperClient::OnAttemperSocketRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwUserData)
{
	_tprintf(TEXT("连接读取=>连接标识:%u, 用户数据:%u, 主消息号:%u, 子消息号:%u, 数据大小:%u\n"), dwSocketID, dwUserData, wMainCmdID, wSubCmdID, wDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return true;
}

//网络绑定
bool CTestAttemperClient::OnAttemperNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort)
{
	_tprintf(TEXT("网络绑定=>连接标识:%u, 对端地址:%s:%d\n"), dwSocketID, pszClientIP, wClientPort);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("测试数据=>The server has received your connection, 您的IP地址为:%s, 端口为:%d"), pszClientIP, wClientPort);
	SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	return true;
}

//网络关闭
bool CTestAttemperClient::OnAttemperNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime)
{
	_tprintf(TEXT("网络关闭=>连接标识:%u, 对端地址:%s:%d, 激活时间:%u, 连接时间:%u秒\n"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime));
	return true;
}

//网络读取
bool CTestAttemperClient::OnAttemperNetworkRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("网络读取=>连接标识:%u, 主消息号:%u, 子消息号:%u, 数据大小:%u\n"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return true;
}

//运行方法
#include <iostream>
int CTestAttemperClient::Run(int argc, TCHAR* argv[])
{
	//启动服务
	CTestAttemperClient TestAttemperClient;
	if (TestAttemperClient.StartClient() == true)
	{
		//连接服务
		TestAttemperClient.ConnectServer(TEXT("127.0.0.1"), 9999, 1234, 0, true);

		//测试数据
		TCHAR szTestData[1024] = TEXT("");
		_sntprintf_s(szTestData, CountArray(szTestData), TEXT("客户端数据=>测试数据cesgagd154545阿斯顿发送到发asdf1545asdf45a4sd5f"));

		//运行服务
		char ch = 0;
		while (ch != 'x' && ch != 'X')
		{
			ch = std::cin.get();
			//群发数据
			TestAttemperClient.SendDataBatch(szTestData, CountStringBuffer(szTestData));
		}
	}

	//停止服务
	TestAttemperClient.StopClient();

	//结束程序
	system("pause");

	return 0;
}