#include "stdafx.h"
#include "Thread.h"
#include <process.h>
#include <stdlib.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////////

//��������
struct tagThreadParameter
{
	bool bSuccess;
	HANDLE hEventFinish;
	CThread	* pServiceThread;
};

//////////////////////////////////////////////////////////////////////////

//���캯��
CThread::CThread(void)
{
	m_bRun = false;
	m_uThreadID = 0;
	m_hThreadHandle = NULL;
}

//��������
CThread::~CThread(void)
{
	StopThread(INFINITE);
}

//�����߳�
bool CThread::StartThread()
{
	if (IsRuning() == true) return false;
	if (m_hThreadHandle != NULL)
	{
		CloseHandle(m_hThreadHandle);
		m_uThreadID = 0;
		m_hThreadHandle = NULL;
	}

	tagThreadParameter ThreadParameter;
	ZeroMemory(&ThreadParameter, sizeof(ThreadParameter));
	ThreadParameter.bSuccess = false;
	ThreadParameter.pServiceThread = this;
	ThreadParameter.hEventFinish = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (ThreadParameter.hEventFinish == NULL) return false;

	//�����߳�
	m_bRun = true;
	m_hThreadHandle = (HANDLE)::_beginthreadex(NULL, 0, ThreadFunction, &ThreadParameter, 0, &m_uThreadID);
	if (m_hThreadHandle == INVALID_HANDLE_VALUE)
	{
		CloseHandle(ThreadParameter.hEventFinish);
		return false;
	}

	//�ȴ��߳�����
	WaitForSingleObject(ThreadParameter.hEventFinish, INFINITE);
	CloseHandle(ThreadParameter.hEventFinish);
	if (ThreadParameter.bSuccess == false)
	{
		StopThread(INFINITE);
		return false;
	}

	return true;
}

//ֹͣ�߳�
bool CThread::StopThread(DWORD dwMillSeconds /* = INFINITE */)
{
	if (IsRuning() == true)
	{
		m_bRun = false;
		if (WaitForSingleObject(m_hThreadHandle, dwMillSeconds) == WAIT_TIMEOUT)
		{
			return false;
		}
	}

	if (m_hThreadHandle != NULL)
	{
		CloseHandle(m_hThreadHandle);
		m_uThreadID = 0;
		m_hThreadHandle = NULL;
	}

	return true;
}

//�ж��߳��Ƿ�������
bool CThread::IsRuning()
{
	if (m_hThreadHandle == NULL) return false;
	if (WaitForSingleObject(m_hThreadHandle, 0) != WAIT_TIMEOUT) return false;
	return true;
}

//��ʼ�¼�
bool CThread::OnEventThreadStrat()
{
	return true;
}
//�����¼�
bool CThread::OnEventThreadRun()
{
	return true;
}
//�����¼�
bool CThread::OnEventThreadStop()
{
	return true;
}

//�̺߳���
unsigned int __stdcall CThread::ThreadFunction(void * pThreadData)
{
	//�������
	srand((DWORD)time(NULL));

	//��ȡ����
	tagThreadParameter * pThreadParameter = (tagThreadParameter *)pThreadData;
	CThread * pServiceThread = pThreadParameter->pServiceThread;

	//��ʼ֪ͨ
	try
	{
		pThreadParameter->bSuccess = pServiceThread->OnEventThreadStrat();
	}
	catch (...)
	{
		pThreadParameter->bSuccess = false;
	}

	bool bSuccess = pThreadParameter->bSuccess;

	//֪ͨ���÷��߳�������
	if (pThreadParameter->hEventFinish != NULL)
	{
		SetEvent(pThreadParameter->hEventFinish);
	}

	//�̴߳���
	if (bSuccess == true)
	{
		while (pServiceThread->m_bRun)
		{
			try
			{
				if (pServiceThread->OnEventThreadRun() == false)
				{
					break;
				}
			}
			catch (...)
			{
				//
			}
		}

		//ֹ֪ͣͨ
		try
		{
			pServiceThread->OnEventThreadStop();
		}
		catch (...)
		{
			//
		}
	}

	//��ֹ�߳�
	_endthreadex(0L);

	return 0;
}

//////////////////////////////////////////////////////////////////////////
