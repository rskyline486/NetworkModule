#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//�첽�¼��ص��ӿ�
class IAsynchronismEventSink
{
public:
	//�첽��ʼ
	virtual bool OnEventAsynchronismStrat() = 0;
	//�첽����
	virtual bool OnEventAsynchronismStop() = 0;
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize) = 0;
};

//////////////////////////////////////////////////////////////////////////

//�첽�߳�
class CAsynchronismThread : public CThread
{
public:
	CAsynchronismThread(WORD wThreadID);
	virtual ~CAsynchronismThread(void);

public:
	//���ú���
	bool InitThread(HANDLE hCompletionPort, IAsynchronismEventSink * pAsynchronismEventSink);

protected:
	//��ʼ�¼�
	virtual bool OnEventThreadStrat();
	//�����¼�
	virtual bool OnEventThreadStop();
	//���к���
	virtual bool OnEventThreadRun();

private:
	WORD							m_wThreadID;						//�̱߳�ʶ
	HANDLE							m_hCompletionPort;					//��ɶ˿�
	IAsynchronismEventSink *		m_pAsynchronismEventSink;			//�ص��ӿ�
	BYTE							m_cbBuffer[ASYNCHRONISM_BUFFER];	//���ջ���
};

//////////////////////////////////////////////////////////////////////////

//�첽����
class CAsynchronismEngine
{
public:
	CAsynchronismEngine();
	virtual ~CAsynchronismEngine(void);

public:
	//��ʼ���첽����
	bool Init(IAsynchronismEventSink * pAsynchronismEventSink, WORD wThreadCount);
	//�ͷ��첽����
	bool Release();

public:
	//������Ϣ
	void GetAsynchronismBurthenInfo(tagBurthenInfo & BurthenInfo);
	//��������
	bool PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

private:
	HANDLE							m_hCompletionPort;					//��ɶ˿�
	std::vector<CThread *>			m_WorkerThreadPtrList;				//�����߳�
	CDataQueue						m_DataQueue;						//���ݶ���
};

//////////////////////////////////////////////////////////////////////////
