#include "stdafx.h"
#include "DetectThread.h"

//////////////////////////////////////////////////////////////////////////

CDetectThread::CDetectThread()
{
	m_pDetectEventSink = NULL;
	m_dwPileTime = 0L;
	m_dwDetectTime = 10000L;
}

CDetectThread::~CDetectThread(void)
{
}

//���ú���
bool CDetectThread::InitThread(IDetectEventSink * pDetectEventSink, DWORD dwDetectTime)
{
	if (pDetectEventSink == NULL)
	{
		Logger_Error(TEXT("�ص��ӿ�Ϊ��"));
		return false;
	}

	//���ñ���
	m_pDetectEventSink = pDetectEventSink;
	m_dwDetectTime = dwDetectTime;
	m_dwPileTime = 0L;

	return true;
}

//��ʼ�¼�
bool CDetectThread::OnEventThreadStrat()
{
	Logger_Info(TEXT("����߳�����[dwDetectTime:%u����, dwCurrentThreadId:%u]"), m_dwDetectTime, GetCurrentThreadId());
	return true;
}

//�����¼�
bool CDetectThread::OnEventThreadStop()
{
	Logger_Info(TEXT("����߳��˳�[dwDetectTime:%u����, dwCurrentThreadId:%u]"), m_dwDetectTime, GetCurrentThreadId());
	return true;
}

//���к���
bool CDetectThread::OnEventThreadRun()
{
	//���ü��
	Sleep(200);
	m_dwPileTime += 200L;

	//�������
	if (m_dwPileTime >= m_dwDetectTime)
	{
		m_dwPileTime = 0L;
		m_pDetectEventSink->OnEventDetectBeat();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
