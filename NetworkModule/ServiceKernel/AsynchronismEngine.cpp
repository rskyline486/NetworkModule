#include "stdafx.h"
#include "AsynchronismEngine.h"

//////////////////////////////////////////////////////////////////////////

CAsynchronismThread::CAsynchronismThread(WORD wThreadID)
{
	m_wThreadID = wThreadID;
	m_hCompletionPort = NULL;
	m_pAsynchronismEventSink = NULL;
	ZeroMemory(m_cbBuffer, sizeof(m_cbBuffer));
}

CAsynchronismThread::~CAsynchronismThread(void)
{
}

//���ú���
bool CAsynchronismThread::InitThread(HANDLE hCompletionPort, IAsynchronismEventSink * pAsynchronismEventSink)
{
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("��ɶ˿ڶ���Ϊ��"));
		return false;
	}

	if (pAsynchronismEventSink == NULL)
	{
		Logger_Error(TEXT("�ص��ӿ�Ϊ��"));
		return false;
	}

	//���ñ���
	m_hCompletionPort = hCompletionPort;
	m_pAsynchronismEventSink = pAsynchronismEventSink;

	return true;
}

//��ʼ�¼�
bool CAsynchronismThread::OnEventThreadStrat()
{
	//��ʼ�¼�
	m_pAsynchronismEventSink->OnEventAsynchronismStrat();

	Logger_Info(TEXT("�첽�߳�����[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//�����¼�
bool CAsynchronismThread::OnEventThreadStop()
{
	//�����¼�
	m_pAsynchronismEventSink->OnEventAsynchronismStop();

	Logger_Info(TEXT("�첽�߳��˳�[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//���к���
bool CAsynchronismThread::OnEventThreadRun()
{
	DWORD dwThancferred = 0;
	OVERLAPPED * pOverLapped = NULL;
	CDataQueue * pDataQueue = NULL;

	//��ȡ��ɶ���
	if (!GetQueuedCompletionStatus(m_hCompletionPort, &dwThancferred, (PULONG_PTR)&pDataQueue, &pOverLapped, INFINITE))
	{
		Logger_Info(TEXT("�첽ģ���쳣,��ȡ��ɶ���ʧ��,������:%d"), WSAGetLastError());
		return false;
	}

	//�˳��ж�
	if (pDataQueue == NULL)
	{
		return false;
	}

	//��ȡ����
	tagDataHead DataHead;
	pDataQueue->DistillData(DataHead, m_cbBuffer, sizeof(m_cbBuffer));

	//���ݴ���
	m_pAsynchronismEventSink->OnEventAsynchronismData(DataHead.wIdentifier, m_cbBuffer, DataHead.wDataSize);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CAsynchronismEngine::CAsynchronismEngine()
{
	m_hCompletionPort = NULL;
}

//��������
CAsynchronismEngine::~CAsynchronismEngine(void)
{
}

//��ʼ���첽����
bool CAsynchronismEngine::Init(IAsynchronismEventSink * pAsynchronismEventSink, WORD wThreadCount)
{
	if (m_hCompletionPort != NULL)
	{
		Logger_Error(TEXT("ģ���ѳ�ʼ��"));
		return false;
	}

	//ϵͳ��Ϣ
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	//�����߳���Ŀ
	wThreadCount = (WORD)(wThreadCount < SystemInfo.dwNumberOfProcessors ? wThreadCount : SystemInfo.dwNumberOfProcessors);
	wThreadCount = wThreadCount > 0 ? wThreadCount : 1;

	//��ɶ˿�
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, wThreadCount);
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("������ɶ˿�ʧ��"));
		return false;
	}

	//���������������߳�
	m_WorkerThreadPtrList.clear();
	for (WORD i = 0; i < wThreadCount; i++)
	{
		//�����̶߳���
		CAsynchronismThread * pWorkerThread = new(std::nothrow) CAsynchronismThread(i);
		if (pWorkerThread == NULL)
		{
			Logger_Error(TEXT("���빤���߳�ʧ��"));
			break;
		}

		//��ʼ���̶߳���
		if (pWorkerThread->InitThread(m_hCompletionPort, pAsynchronismEventSink) == false)
		{
			Logger_Error(TEXT("��ʼ�������߳�ʧ��"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//�����߳�
		if (pWorkerThread->StartThread() == false)
		{
			Logger_Error(TEXT("���������߳�ʧ��"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//�����̳߳��б�
		m_WorkerThreadPtrList.push_back(pWorkerThread);
	}

	//�����߳�У��
	if (m_WorkerThreadPtrList.empty())
	{
		Logger_Error(TEXT("�첽ģ������ʧ��,�����߳���:0"));
		Release();
		return false;
	}

	//��ʼ�����д�С(16M)
	m_DataQueue.InitSize(16384 * 1024);

	Logger_Info(TEXT("�첽ģ�������ɹ�,�����߳���:%d"), wThreadCount);

	return true;
}

//�ͷ��첽����
bool CAsynchronismEngine::Release()
{
	//֪ͨ�����߳̽���
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	}

	//ֹͣ�����߳�
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;

		m_WorkerThreadPtrList[i]->StopThread(INFINITE);
		delete m_WorkerThreadPtrList[i];
		m_WorkerThreadPtrList[i] = NULL;
	}
	m_WorkerThreadPtrList.clear();

	//�رն���
	if (m_hCompletionPort != NULL)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}

	Logger_Info(TEXT("�첽ģ��ֹͣ"));

	return true;
}

//������Ϣ
void CAsynchronismEngine::GetAsynchronismBurthenInfo(tagBurthenInfo & BurthenInfo)
{
	m_DataQueue.GetBurthenInfo(BurthenInfo);
}

//��������
bool CAsynchronismEngine::PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	//�����ж�
	if (m_hCompletionPort == NULL) return false;

	//��������
	if (m_DataQueue.InsertData(wIdentifier, pData, wDataSize) == false)
	{
		Logger_Error(TEXT("��������ʧ��,��ʶ��Ϣ:%u,���ݴ�С:%u"), wIdentifier, wDataSize);
		return false;
	}

	//Ͷ��֪ͨ
	PostQueuedCompletionStatus(m_hCompletionPort, wDataSize, (ULONG_PTR)&m_DataQueue, NULL);

	return true;
}

//////////////////////////////////////////////////////////////////////////
