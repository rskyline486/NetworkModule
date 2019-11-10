#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//����¼��ص��ӿ�
class IDetectEventSink
{
public:
	//����¼�
	virtual VOID OnEventDetectBeat() = 0;
};

//////////////////////////////////////////////////////////////////////////

//����߳�
class CDetectThread : public CThread
{
public:
	CDetectThread();
	virtual ~CDetectThread(void);

public:
	//���ú���
	bool InitThread(IDetectEventSink * pDetectEventSink, DWORD dwDetectTime);

protected:
	//��ʼ�¼�
	virtual bool OnEventThreadStrat();
	//�����¼�
	virtual bool OnEventThreadStop();
	//���к���
	virtual bool OnEventThreadRun();

private:
	IDetectEventSink *				m_pDetectEventSink;					//�ص��ӿ�
	DWORD							m_dwPileTime;						//����ʱ��
	DWORD							m_dwDetectTime;						//���ʱ��
};

//////////////////////////////////////////////////////////////////////////
