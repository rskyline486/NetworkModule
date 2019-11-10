#include "stdafx.h"
#include "NetworkModule.h"
#include "ProxyWrapper.h"
#include "ServerWrapper.h"
#include "ClientWrapper.h"
#include "AsynchronismWrapper.h"
#include "TimerWrapper.h"
#include "AttemperWrapper.h"
#include <iostream>
#include <Psapi.h>

//链接文件
#pragma comment(lib,"Psapi.lib")

//////////////////////////////////////////////////////////////////////////

//启动进程
DWORD CNetworkModule::StartProcess(LPCTSTR pszProcessPath, LPCTSTR pszWindownTitle, LPCTSTR pszCommandLine, bool bCreateNewWindow, bool bHideWindow)
{
	//获取信息
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);
	::GetStartupInfo(&si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = bHideWindow ? SW_HIDE : SW_SHOWNORMAL;
	si.lpTitle = (LPTSTR)pszWindownTitle;

	//配置参数
	DWORD dwCreationFlags = bCreateNewWindow ? CREATE_NEW_CONSOLE : 0;
	TCHAR szCmdInfo[512] = { 0 };
	_stprintf_s(szCmdInfo, TEXT("%s %s"), pszProcessPath, pszCommandLine);

	//启动进程
	if (::CreateProcess(NULL, szCmdInfo, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &si, &pi))
	{
		::CloseHandle(pi.hThread);
		::CloseHandle(pi.hProcess);
		return pi.dwProcessId;
	}

	return 0L;
}

//查找进程
bool CNetworkModule::FindProcess(DWORD dwProcesID)
{
	HANDLE hProcess;
	hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcesID);
	if (hProcess != NULL)
	{
		HMODULE hMod;
		DWORD cbNeeded;
		if (::EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
		{
			::CloseHandle(hProcess);
			return true;
		}
		::CloseHandle(hProcess);
	}

	return false;
}

//结束进程
bool CNetworkModule::KillProcess(DWORD dwProcesID)
{
	HANDLE hProcess;
	hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, dwProcesID);
	if (hProcess != NULL)
	{
		::TerminateProcess(hProcess, 0);
		::CloseHandle(hProcess);

		return true;
	}

	return false;
}

//启动模块
bool CNetworkModule::StartupNetwork()
{
	WSAData wsaData;
	int nErrMsg = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nErrMsg != NO_ERROR)
	{
		Logger_Error(TEXT("WSAStartup失败=>返回值:%d, 错误码:%d"), nErrMsg, GetLastError());
		return false;
	}

	return true;
}

//清理模块
bool CNetworkModule::CleanupNetwork()
{
	WSACleanup();

	return true;
}

//运行网络
bool CNetworkModule::RunNetwork()
{
	//运行服务
	char ch = 0;
	while (ch != 'x' && ch != 'X')
	{
		ch = std::cin.get();
	}

	return true;
}

//创建代理
bool CNetworkModule::CreateService(IProxyService ** ppProxyService)
{
	//参数校验
	if (ppProxyService == NULL) return false;
	if (*ppProxyService != NULL) return false;

	//创建对象
	IProxyService * pProxyService = new(std::nothrow) CProxyInstance();
	if (pProxyService)
	{
		//参数赋值
		*ppProxyService = pProxyService;

		return true;
	}

	return false;
}

//销毁代理
bool CNetworkModule::DestroyService(IProxyService ** ppProxyService)
{
	//参数校验
	if (ppProxyService == NULL) return false;
	if (*ppProxyService == NULL) return false;

	//对象转换
	CProxyInstance * pProxyService = dynamic_cast<CProxyInstance *>(*ppProxyService);
	if (pProxyService)
	{
		//释放对象
		delete pProxyService;
		pProxyService = NULL;
		*ppProxyService = NULL;

		return true;
	}

	return false;
}

//创建服务
bool CNetworkModule::CreateService(IServerService ** ppServerService)
{
	//参数校验
	if (ppServerService == NULL) return false;
	if (*ppServerService != NULL) return false;

	//创建对象
	IServerService * pServerService = new(std::nothrow) CServerInstance();
	if (pServerService)
	{
		//参数赋值
		*ppServerService = pServerService;

		return true;
	}

	return false;
}

//销毁服务
bool CNetworkModule::DestroyService(IServerService ** ppServerService)
{
	//参数校验
	if (ppServerService == NULL) return false;
	if (*ppServerService == NULL) return false;

	//对象转换
	CServerInstance * pServerService = dynamic_cast<CServerInstance *>(*ppServerService);
	if (pServerService)
	{
		//释放对象
		delete pServerService;
		pServerService = NULL;
		*ppServerService = NULL;

		return true;
	}

	return false;
}

//创建连接
bool CNetworkModule::CreateService(IClientService ** ppClientService)
{
	//参数校验
	if (ppClientService == NULL) return false;
	if (*ppClientService != NULL) return false;

	//创建对象
	IClientService * pClientService = new(std::nothrow) CClientInstance();
	if (pClientService)
	{
		//参数赋值
		*ppClientService = pClientService;

		return true;
	}

	return false;
}

//销毁连接
bool CNetworkModule::DestroyService(IClientService ** ppClientService)
{
	//参数校验
	if (ppClientService == NULL) return false;
	if (*ppClientService == NULL) return false;

	//对象转换
	CClientInstance * pClientService = dynamic_cast<CClientInstance *>(*ppClientService);
	if (pClientService)
	{
		//释放对象
		delete pClientService;
		pClientService = NULL;
		*ppClientService = NULL;

		return true;
	}

	return false;
}

//创建异步
bool CNetworkModule::CreateService(IAsynchronismService ** ppAsynchronismService)
{
	//参数校验
	if (ppAsynchronismService == NULL) return false;
	if (*ppAsynchronismService != NULL) return false;

	//创建对象
	IAsynchronismService * pAsynchronismService = new(std::nothrow) CAsynchronismInstance();
	if (pAsynchronismService)
	{
		//参数赋值
		*ppAsynchronismService = pAsynchronismService;

		return true;
	}

	return false;
}

//销毁异步
bool CNetworkModule::DestroyService(IAsynchronismService ** ppAsynchronismService)
{
	//参数校验
	if (ppAsynchronismService == NULL) return false;
	if (*ppAsynchronismService == NULL) return false;

	//对象转换
	CAsynchronismInstance * pAsynchronismService = dynamic_cast<CAsynchronismInstance *>(*ppAsynchronismService);
	if (pAsynchronismService)
	{
		//释放对象
		delete pAsynchronismService;
		pAsynchronismService = NULL;
		*ppAsynchronismService = NULL;

		return true;
	}

	return false;
}

//创建时间
bool CNetworkModule::CreateService(ITimerService ** ppTimerService)
{
	//参数校验
	if (ppTimerService == NULL) return false;
	if (*ppTimerService != NULL) return false;

	//创建对象
	CTimerInstance * pTimerService = new(std::nothrow) CTimerInstance();
	if (pTimerService)
	{
		//参数赋值
		*ppTimerService = pTimerService;

		return true;
	}

	return false;
}

//销毁时间
bool CNetworkModule::DestroyService(ITimerService ** ppTimerService)
{
	//参数校验
	if (ppTimerService == NULL) return false;
	if (*ppTimerService == NULL) return false;

	//对象转换
	CTimerInstance * pTimerService = dynamic_cast<CTimerInstance *>(*ppTimerService);
	if (pTimerService)
	{
		//释放对象
		delete pTimerService;
		pTimerService = NULL;
		*ppTimerService = NULL;

		return true;
	}

	return false;
}

//创建调度
bool CNetworkModule::CreateService(IAttemperService ** ppAttemperService)
{
	//参数校验
	if (ppAttemperService == NULL) return false;
	if (*ppAttemperService != NULL) return false;

	//创建对象
	CAttemperInstance * pAttemperService = new(std::nothrow) CAttemperInstance();
	if (pAttemperService)
	{
		//参数赋值
		*ppAttemperService = pAttemperService;

		return true;
	}

	return false;
}

//销毁调度
bool CNetworkModule::DestroyService(IAttemperService ** ppAttemperService)
{
	//参数校验
	if (ppAttemperService == NULL) return false;
	if (*ppAttemperService == NULL) return false;

	//对象转换
	CAttemperInstance * pAttemperService = dynamic_cast<CAttemperInstance *>(*ppAttemperService);
	if (pAttemperService)
	{
		//释放对象
		delete pAttemperService;
		pAttemperService = NULL;
		*ppAttemperService = NULL;

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
