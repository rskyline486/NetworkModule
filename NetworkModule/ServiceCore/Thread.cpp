#include "stdafx.h"
#include "Thread.h"
#include <process.h>
#include <stdlib.h>
#include <time.h>

//////////////////////////////////////////////////////////////////////////

//启动参数
struct tagThreadParameter
{
	bool bSuccess;
	HANDLE hEventFinish;
	CThread	* pServiceThread;
};

//////////////////////////////////////////////////////////////////////////

//构造函数
CThread::CThread(void)
{
	m_bRun = false;
	m_uThreadID = 0;
	m_hThreadHandle = NULL;
}

//析构函数
CThread::~CThread(void)
{
	StopThread(INFINITE);
}

//启动线程
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

	//启动线程
	m_bRun = true;
	m_hThreadHandle = (HANDLE)::_beginthreadex(NULL, 0, ThreadFunction, &ThreadParameter, 0, &m_uThreadID);
	if (m_hThreadHandle == INVALID_HANDLE_VALUE)
	{
		CloseHandle(ThreadParameter.hEventFinish);
		return false;
	}

	//等待线程启动
	WaitForSingleObject(ThreadParameter.hEventFinish, INFINITE);
	CloseHandle(ThreadParameter.hEventFinish);
	if (ThreadParameter.bSuccess == false)
	{
		StopThread(INFINITE);
		return false;
	}

	return true;
}

//停止线程
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

//判断线程是否在运行
bool CThread::IsRuning()
{
	if (m_hThreadHandle == NULL) return false;
	if (WaitForSingleObject(m_hThreadHandle, 0) != WAIT_TIMEOUT) return false;
	return true;
}

//开始事件
bool CThread::OnEventThreadStrat()
{
	return true;
}
//运行事件
bool CThread::OnEventThreadRun()
{
	return true;
}
//结束事件
bool CThread::OnEventThreadStop()
{
	return true;
}

//线程函数
unsigned int __stdcall CThread::ThreadFunction(void * pThreadData)
{
	//随机种子
	srand((DWORD)time(NULL));

	//获取参数
	tagThreadParameter * pThreadParameter = (tagThreadParameter *)pThreadData;
	CThread * pServiceThread = pThreadParameter->pServiceThread;

	//开始通知
	try
	{
		pThreadParameter->bSuccess = pServiceThread->OnEventThreadStrat();
	}
	catch (...)
	{
		pThreadParameter->bSuccess = false;
	}

	bool bSuccess = pThreadParameter->bSuccess;

	//通知调用方线程已启动
	if (pThreadParameter->hEventFinish != NULL)
	{
		SetEvent(pThreadParameter->hEventFinish);
	}

	//线程处理
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

		//停止通知
		try
		{
			pServiceThread->OnEventThreadStop();
		}
		catch (...)
		{
			//
		}
	}

	//中止线程
	_endthreadex(0L);

	return 0;
}

//////////////////////////////////////////////////////////////////////////
