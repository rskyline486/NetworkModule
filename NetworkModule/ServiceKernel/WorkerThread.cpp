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

//���ú���
bool CWorkerThread::InitThread(HANDLE hCompletionPort)
{
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("��ɶ˿ڶ���Ϊ��"));
		return false;
	}

	//���ñ���
	m_hCompletionPort = hCompletionPort;

	return true;
}

//��ʼ�¼�
bool CWorkerThread::OnEventThreadStrat()
{
	Logger_Info(TEXT("�����߳�����[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//�����¼�
bool CWorkerThread::OnEventThreadStop()
{
	Logger_Info(TEXT("�����߳��˳�[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//���к���
bool CWorkerThread::OnEventThreadRun()
{
	DWORD dwThancferred = 0;
	CNativeInfo * pNativeInfo = NULL;
	OVERLAPPED * pOverLapped = NULL;

	//��ȡ��ɶ���
	if (!GetQueuedCompletionStatus(m_hCompletionPort, &dwThancferred, (PULONG_PTR)&pNativeInfo, &pOverLapped, INFINITE))
	{
		int err = WSAGetLastError();
		if (pOverLapped == NULL)
		{
			if (err == WAIT_TIMEOUT)
			{
				Logger_Error(TEXT("��ȡ��ɶ���ʱ,������:%d"), err);
				return true;
			}
			Logger_Error(TEXT("��ȡ��ɶ���ʧ��,�ص�����Ϊ��,������:%d"), err);
			Sleep(100);
			return true;
		}

		if (pNativeInfo == NULL)
		{
			Logger_Error(TEXT("��ȡ��ɶ���ʧ��,�󶨶���Ϊ��,������:%d"), err);
			return true;
		}

		Logger_Info(TEXT("��ȡ��ɶ���ʧ��,������:%d"), err);

		//��������
		COverLapped * pSocketLapped = CONTAINING_RECORD(pOverLapped, COverLapped, m_OverLapped);
		dwThancferred = SOCKET_ERROR;
		pNativeInfo->DealAsync(pSocketLapped, dwThancferred);

		return true;
	}

	//�˳��ж�
	if ((pNativeInfo == NULL) && (pOverLapped == NULL))
	{
		return false;
	}

	//����У��
	if ((pNativeInfo == NULL) || (pOverLapped == NULL))
	{
		Logger_Error(TEXT("��ȡ��ɶ���ɹ�,���ص�����[0x%p]��֪ͨ����[0x%p]Ϊ��"), pNativeInfo, pOverLapped);
		return true;
	}

	COverLapped * pSocketLapped = CONTAINING_RECORD(pOverLapped, COverLapped, m_OverLapped);
	pNativeInfo->DealAsync(pSocketLapped, dwThancferred);

	return true;
}

//////////////////////////////////////////////////////////////////////////
