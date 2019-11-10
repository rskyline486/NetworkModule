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

//�����ļ�
#pragma comment(lib,"Psapi.lib")

//////////////////////////////////////////////////////////////////////////

//��������
DWORD CNetworkModule::StartProcess(LPCTSTR pszProcessPath, LPCTSTR pszWindownTitle, LPCTSTR pszCommandLine, bool bCreateNewWindow, bool bHideWindow)
{
	//��ȡ��Ϣ
	STARTUPINFO si = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	si.cb = sizeof(si);
	::GetStartupInfo(&si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = bHideWindow ? SW_HIDE : SW_SHOWNORMAL;
	si.lpTitle = (LPTSTR)pszWindownTitle;

	//���ò���
	DWORD dwCreationFlags = bCreateNewWindow ? CREATE_NEW_CONSOLE : 0;
	TCHAR szCmdInfo[512] = { 0 };
	_stprintf_s(szCmdInfo, TEXT("%s %s"), pszProcessPath, pszCommandLine);

	//��������
	if (::CreateProcess(NULL, szCmdInfo, NULL, NULL, FALSE, dwCreationFlags, NULL, NULL, &si, &pi))
	{
		::CloseHandle(pi.hThread);
		::CloseHandle(pi.hProcess);
		return pi.dwProcessId;
	}

	return 0L;
}

//���ҽ���
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

//��������
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

//����ģ��
bool CNetworkModule::StartupNetwork()
{
	WSAData wsaData;
	int nErrMsg = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nErrMsg != NO_ERROR)
	{
		Logger_Error(TEXT("WSAStartupʧ��=>����ֵ:%d, ������:%d"), nErrMsg, GetLastError());
		return false;
	}

	return true;
}

//����ģ��
bool CNetworkModule::CleanupNetwork()
{
	WSACleanup();

	return true;
}

//��������
bool CNetworkModule::RunNetwork()
{
	//���з���
	char ch = 0;
	while (ch != 'x' && ch != 'X')
	{
		ch = std::cin.get();
	}

	return true;
}

//��������
bool CNetworkModule::CreateService(IProxyService ** ppProxyService)
{
	//����У��
	if (ppProxyService == NULL) return false;
	if (*ppProxyService != NULL) return false;

	//��������
	IProxyService * pProxyService = new(std::nothrow) CProxyInstance();
	if (pProxyService)
	{
		//������ֵ
		*ppProxyService = pProxyService;

		return true;
	}

	return false;
}

//���ٴ���
bool CNetworkModule::DestroyService(IProxyService ** ppProxyService)
{
	//����У��
	if (ppProxyService == NULL) return false;
	if (*ppProxyService == NULL) return false;

	//����ת��
	CProxyInstance * pProxyService = dynamic_cast<CProxyInstance *>(*ppProxyService);
	if (pProxyService)
	{
		//�ͷŶ���
		delete pProxyService;
		pProxyService = NULL;
		*ppProxyService = NULL;

		return true;
	}

	return false;
}

//��������
bool CNetworkModule::CreateService(IServerService ** ppServerService)
{
	//����У��
	if (ppServerService == NULL) return false;
	if (*ppServerService != NULL) return false;

	//��������
	IServerService * pServerService = new(std::nothrow) CServerInstance();
	if (pServerService)
	{
		//������ֵ
		*ppServerService = pServerService;

		return true;
	}

	return false;
}

//���ٷ���
bool CNetworkModule::DestroyService(IServerService ** ppServerService)
{
	//����У��
	if (ppServerService == NULL) return false;
	if (*ppServerService == NULL) return false;

	//����ת��
	CServerInstance * pServerService = dynamic_cast<CServerInstance *>(*ppServerService);
	if (pServerService)
	{
		//�ͷŶ���
		delete pServerService;
		pServerService = NULL;
		*ppServerService = NULL;

		return true;
	}

	return false;
}

//��������
bool CNetworkModule::CreateService(IClientService ** ppClientService)
{
	//����У��
	if (ppClientService == NULL) return false;
	if (*ppClientService != NULL) return false;

	//��������
	IClientService * pClientService = new(std::nothrow) CClientInstance();
	if (pClientService)
	{
		//������ֵ
		*ppClientService = pClientService;

		return true;
	}

	return false;
}

//��������
bool CNetworkModule::DestroyService(IClientService ** ppClientService)
{
	//����У��
	if (ppClientService == NULL) return false;
	if (*ppClientService == NULL) return false;

	//����ת��
	CClientInstance * pClientService = dynamic_cast<CClientInstance *>(*ppClientService);
	if (pClientService)
	{
		//�ͷŶ���
		delete pClientService;
		pClientService = NULL;
		*ppClientService = NULL;

		return true;
	}

	return false;
}

//�����첽
bool CNetworkModule::CreateService(IAsynchronismService ** ppAsynchronismService)
{
	//����У��
	if (ppAsynchronismService == NULL) return false;
	if (*ppAsynchronismService != NULL) return false;

	//��������
	IAsynchronismService * pAsynchronismService = new(std::nothrow) CAsynchronismInstance();
	if (pAsynchronismService)
	{
		//������ֵ
		*ppAsynchronismService = pAsynchronismService;

		return true;
	}

	return false;
}

//�����첽
bool CNetworkModule::DestroyService(IAsynchronismService ** ppAsynchronismService)
{
	//����У��
	if (ppAsynchronismService == NULL) return false;
	if (*ppAsynchronismService == NULL) return false;

	//����ת��
	CAsynchronismInstance * pAsynchronismService = dynamic_cast<CAsynchronismInstance *>(*ppAsynchronismService);
	if (pAsynchronismService)
	{
		//�ͷŶ���
		delete pAsynchronismService;
		pAsynchronismService = NULL;
		*ppAsynchronismService = NULL;

		return true;
	}

	return false;
}

//����ʱ��
bool CNetworkModule::CreateService(ITimerService ** ppTimerService)
{
	//����У��
	if (ppTimerService == NULL) return false;
	if (*ppTimerService != NULL) return false;

	//��������
	CTimerInstance * pTimerService = new(std::nothrow) CTimerInstance();
	if (pTimerService)
	{
		//������ֵ
		*ppTimerService = pTimerService;

		return true;
	}

	return false;
}

//����ʱ��
bool CNetworkModule::DestroyService(ITimerService ** ppTimerService)
{
	//����У��
	if (ppTimerService == NULL) return false;
	if (*ppTimerService == NULL) return false;

	//����ת��
	CTimerInstance * pTimerService = dynamic_cast<CTimerInstance *>(*ppTimerService);
	if (pTimerService)
	{
		//�ͷŶ���
		delete pTimerService;
		pTimerService = NULL;
		*ppTimerService = NULL;

		return true;
	}

	return false;
}

//��������
bool CNetworkModule::CreateService(IAttemperService ** ppAttemperService)
{
	//����У��
	if (ppAttemperService == NULL) return false;
	if (*ppAttemperService != NULL) return false;

	//��������
	CAttemperInstance * pAttemperService = new(std::nothrow) CAttemperInstance();
	if (pAttemperService)
	{
		//������ֵ
		*ppAttemperService = pAttemperService;

		return true;
	}

	return false;
}

//���ٵ���
bool CNetworkModule::DestroyService(IAttemperService ** ppAttemperService)
{
	//����У��
	if (ppAttemperService == NULL) return false;
	if (*ppAttemperService == NULL) return false;

	//����ת��
	CAttemperInstance * pAttemperService = dynamic_cast<CAttemperInstance *>(*ppAttemperService);
	if (pAttemperService)
	{
		//�ͷŶ���
		delete pAttemperService;
		pAttemperService = NULL;
		*ppAttemperService = NULL;

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
