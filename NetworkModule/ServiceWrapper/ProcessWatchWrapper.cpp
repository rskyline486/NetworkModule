#include "stdafx.h"
#include "ProcessWatchWrapper.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CProcessWatchKernel::CProcessWatchKernel()
{
}

//��������
CProcessWatchKernel::~CProcessWatchKernel()
{
}

//�����ں�
bool CProcessWatchKernel::StartKernel()
{
	//�ļ����ƶ���
	TCHAR szFileName[MAX_PATH];
	ZeroMemory(szFileName, sizeof(szFileName));

	//��ȡ�ļ�·���Լ��ļ�����
	if (1)
	{
		TCHAR szFilePath[MAX_PATH];
		ZeroMemory(szFilePath, sizeof(szFilePath));
		GetModuleFileName(NULL, szFilePath, CountArray(szFilePath));
		for (INT i = lstrlen(szFilePath) - 1; i >= 0; i--)
		{
			if (szFilePath[i] == TEXT('\\'))
			{
				//���㳤��(ȥ����б��,ȥ����׺)
				INT nNameCount = (lstrlen(szFilePath) - i) - 1 - 4;
				for (INT k = 0; k < nNameCount; k++) szFileName[k] = szFilePath[i + 1 + k];
				break;
			}
		}
	}

	//���ñ���
	TCHAR szTitleName[128];
	if (lstrlen(szFileName) > 0)
	{
		_stprintf_s(szTitleName, CountArray(szTitleName), TEXT("%s - [�ػ�����]-[%u]"), szFileName, GetCurrentProcessId());
	}
	else
	{
		_stprintf_s(szTitleName, CountArray(szTitleName), TEXT("[�ػ�����]-[%u]"), GetCurrentProcessId());
	}
	
	//���ñ���
	::SetConsoleTitle(szTitleName);

	//��־����
	Logger_SetFileName(TEXT("�ػ�����"));

	//�����߳�
	if (__super::StartThread() == false)
	{
		Logger_Error(TEXT("�����ػ��߳�ʧ��"));
		return false;
	}

	return true;
}

//ֹͣ�ں�
bool CProcessWatchKernel::StopKernel()
{
	//ֹͣ�ػ��߳�
	__super::StopThread(INFINITE);

	//�ͷ��ڴ�
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		delete m_ProcessInfoPtrList[i];
		m_ProcessInfoPtrList[i] = NULL;
	}
	m_ProcessInfoPtrList.clear();

	return true;
}

//��ӽ���
bool CProcessWatchKernel::AddProcessInfo(LPCTSTR pszProcessPath, LPCTSTR pszConsoleTitle, LPCTSTR pszCommandLine, bool bCreateNewWindow, bool bShowWindow)
{
	//����У��
	if (pszProcessPath == NULL) return false;
	//����У��
	if (__super::IsRuning() == true) return false;

	//�������
	PROCESS_INFO * pProcessInfo = new(std::nothrow) PROCESS_INFO;
	if (pProcessInfo == NULL) return false;

	//������ֵ
	pProcessInfo->dwProcessID = 0;
	lstrcpyn(pProcessInfo->szProcessPath, pszProcessPath, CountArray(pProcessInfo->szProcessPath));
	lstrcpyn(pProcessInfo->szConsoleTitle, pszConsoleTitle, CountArray(pProcessInfo->szConsoleTitle));
	lstrcpyn(pProcessInfo->szCommandLine, pszCommandLine, CountArray(pProcessInfo->szCommandLine));
	pProcessInfo->bCreateNewWindow = bCreateNewWindow;
	pProcessInfo->bShowWindow = bShowWindow;

	/*
	//��������
	if (0)
	{
		//��������
		pProcessInfo->dwProcessID = CNetworkModule::StartProcess(pProcessInfo->szProcessPath, pProcessInfo->szConsoleTitle, pProcessInfo->szCommandLine, pProcessInfo->bCreateNewWindow, pProcessInfo->bShowWindow);
		if (pProcessInfo->dwProcessID == 0)
		{
			//�ͷŶ���
			delete pProcessInfo;
			pProcessInfo = NULL;

			Logger_Info(TEXT("��������ʧ��=>��������:%s"), pProcessInfo->szConsoleTitle);
			return false;
		}

		Logger_Info(TEXT("�������̳ɹ�=>��������:%s-[%u]"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
	}
	*/

	//����ػ�
	m_ProcessInfoPtrList.push_back(pProcessInfo);

	return true;
}

//��ʼ�¼�
bool CProcessWatchKernel::OnEventThreadStrat()
{
	Logger_Info(TEXT("�ػ��߳�����[dwCurrentThreadId:%u]"), GetCurrentThreadId());

	//��������
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		//�����ж�
		if (m_ProcessInfoPtrList[i]->dwProcessID != 0) continue;

		//��������
		PROCESS_INFO * pProcessInfo = m_ProcessInfoPtrList[i];
		pProcessInfo->dwProcessID = CNetworkModule::StartProcess(pProcessInfo->szProcessPath, pProcessInfo->szConsoleTitle, pProcessInfo->szCommandLine, pProcessInfo->bCreateNewWindow, pProcessInfo->bShowWindow);
		if (pProcessInfo->dwProcessID == 0)
		{
			Logger_Info(TEXT("��������ʧ��=>��������:%s"), pProcessInfo->szConsoleTitle);
		}
		else
		{
			Logger_Info(TEXT("�������̳ɹ�=>��������:%s-[%u]"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
		}

		//��ʱ5��
		Sleep(5000);
	}

	//�˳�У��
	if (m_ProcessInfoPtrList.size() < 1) return false;

	return true;
}

//�����¼�
bool CProcessWatchKernel::OnEventThreadStop()
{
	//�رս���
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		CNetworkModule::KillProcess(m_ProcessInfoPtrList[i]->dwProcessID);
		Logger_Info(TEXT("�رս���=>��������:%s-[%u]"), m_ProcessInfoPtrList[i]->szConsoleTitle, m_ProcessInfoPtrList[i]->dwProcessID);
	}

	Logger_Info(TEXT("�ػ��߳��˳�[dwCurrentThreadId:%u]"), GetCurrentThreadId());
	return true;
}

//���к���
bool CProcessWatchKernel::OnEventThreadRun()
{
	//��������
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		//���ҽ���
		PROCESS_INFO * pProcessInfo = m_ProcessInfoPtrList[i];
		if (CNetworkModule::FindProcess(pProcessInfo->dwProcessID) == false)
		{
			Logger_Info(TEXT("�Ҳ�������=>��������:%s-[%u], ׼����������"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
			//��������
			pProcessInfo->dwProcessID = CNetworkModule::StartProcess(pProcessInfo->szProcessPath, pProcessInfo->szConsoleTitle, pProcessInfo->szCommandLine, pProcessInfo->bCreateNewWindow, pProcessInfo->bShowWindow);
			if (pProcessInfo->dwProcessID == 0)
			{
				Logger_Info(TEXT("��������ʧ��=>��������:%s"), pProcessInfo->szConsoleTitle);
			}
			else
			{
				Logger_Info(TEXT("�������̳ɹ�=>��������:%s-[%u]"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
			}

			//��ʱ5��
			Sleep(5000);
		}
	}

	//�����
	Sleep(1000);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CProcessWatchInstance::CProcessWatchInstance()
{
}

//��������
CProcessWatchInstance::~CProcessWatchInstance()
{
}

//��������
bool CProcessWatchInstance::StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent)
{
	//��Ӷ���
	//m_ProcessWatchKernel.AddProcessInfo();

	//��������
	m_ProcessWatchKernel.StartKernel();

	return true;
}

//ֹͣ����
bool CProcessWatchInstance::StopServer()
{
	//ֹͣ����
	m_ProcessWatchKernel.StopKernel();

	return true;
}

//////////////////////////////////////////////////////////////////////////
