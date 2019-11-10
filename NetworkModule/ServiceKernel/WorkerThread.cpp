#include "stdafx.h"
#include "WorkerThread.h"

//////////////////////////////////////////////////////////////////////////

CWorkerThread::CWorkerThread(WORD wThreadID)
{
	m_wThreadID = wThreadID;
	m_hCompletionPort = NULL;
}

CWorkerThread::~CWorkerThread(void)
{
}

//配置函数
bool CWorkerThread::InitThread(HANDLE hCompletionPort)
{
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("完成端口对象为空"));
		return false;
	}

	//设置变量
	m_hCompletionPort = hCompletionPort;

	return true;
}

//开始事件
bool CWorkerThread::OnEventThreadStrat()
{
	Logger_Info(TEXT("工作线程启动[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//结束事件
bool CWorkerThread::OnEventThreadStop()
{
	Logger_Info(TEXT("工作线程退出[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//运行函数
bool CWorkerThread::OnEventThreadRun()
{
	DWORD dwThancferred = 0;
	CNativeInfo * pNativeInfo = NULL;
	OVERLAPPED * pOverLapped = NULL;

	//获取完成对象
	if (!GetQueuedCompletionStatus(m_hCompletionPort, &dwThancferred, (PULONG_PTR)&pNativeInfo, &pOverLapped, INFINITE))
	{
		int err = WSAGetLastError();
		if (pOverLapped == NULL)
		{
			if (err == WAIT_TIMEOUT)
			{
				Logger_Error(TEXT("获取完成对象超时,错误码:%d"), err);
				return true;
			}
			Logger_Error(TEXT("获取完成对象失败,重叠对象为空,错误码:%d"), err);
			Sleep(100);
			return true;
		}

		if (pNativeInfo == NULL)
		{
			Logger_Error(TEXT("获取完成对象失败,绑定对象为空,错误码:%d"), err);
			return true;
		}

		Logger_Info(TEXT("获取完成对象失败,错误码:%d"), err);

		//变量定义
		COverLapped * pSocketLapped = CONTAINING_RECORD(pOverLapped, COverLapped, m_OverLapped);
		dwThancferred = SOCKET_ERROR;
		pNativeInfo->DealAsync(pSocketLapped, dwThancferred);

		return true;
	}

	//退出判断
	if ((pNativeInfo == NULL) && (pOverLapped == NULL))
	{
		return false;
	}

	//数据校验
	if ((pNativeInfo == NULL) || (pOverLapped == NULL))
	{
		Logger_Error(TEXT("获取完成对象成功,但重叠对象[0x%p]或通知对象[0x%p]为空"), pNativeInfo, pOverLapped);
		return true;
	}

	COverLapped * pSocketLapped = CONTAINING_RECORD(pOverLapped, COverLapped, m_OverLapped);
	pNativeInfo->DealAsync(pSocketLapped, dwThancferred);

	return true;
}

//////////////////////////////////////////////////////////////////////////
