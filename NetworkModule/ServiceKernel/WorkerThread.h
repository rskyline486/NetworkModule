#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//�����߳�
class CWorkerThread : public CThread
{
public:
	CWorkerThread(WORD wThreadID);
	virtual ~CWorkerThread(void);

public:
	//���ú���
	bool InitThread(HANDLE hCompletionPort);

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
};

//////////////////////////////////////////////////////////////////////////
