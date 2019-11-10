#pragma once

//////////////////////////////////////////////////////////////////////////

//�̶߳���
class CThread
{
public:
	CThread(void);
	virtual ~CThread(void);

public:
	//�����߳�
	bool StartThread();
	//ֹͣ�߳�
	bool StopThread(DWORD dwMillSeconds = INFINITE);
	//�ж��߳��Ƿ�������
	bool IsRuning();

protected:
	//��ʼ�¼�
	virtual bool OnEventThreadStrat();
	//�����¼�
	virtual bool OnEventThreadRun();
	//�����¼�
	virtual bool OnEventThreadStop();

private:
	//�̺߳���
	static unsigned int __stdcall ThreadFunction(void * pThreadData);

protected:
	volatile bool m_bRun;
	unsigned int m_uThreadID;
	HANDLE m_hThreadHandle;
};

//////////////////////////////////////////////////////////////////////////
