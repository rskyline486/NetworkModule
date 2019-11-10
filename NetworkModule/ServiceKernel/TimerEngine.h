#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//ʱ���¼��ص��ӿ�
class ITimerEventSink
{
public:
	//�첽�¼�
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter) = 0;
};

//////////////////////////////////////////////////////////////////////////

//��ʱ����
struct tagTimerItem
{
	DWORD							dwElapse;							//����ʱ��
	DWORD							dwTimerID;							//ʱ���ʶ
	DWORD							dwTimeLeave;						//ʣ��ʱ��
	DWORD							dwRepeatTimes;						//�ظ�����
	WPARAM							dwBindParameter;					//�󶨲���
};

//////////////////////////////////////////////////////////////////////////

//ʱ���߳�
class CTimerThread : public CThread
{
public:
	CTimerThread(void);
	virtual ~CTimerThread(void);

public:
	//���ú���
	bool InitThread(DWORD dwTimerSpace);

protected:
	//��ʼ�¼�
	virtual bool OnEventThreadStrat();
	//�����¼�
	virtual bool OnEventThreadStop();
	//���к���
	virtual bool OnEventThreadRun();

protected:
	//��ʱ֪ͨ
	virtual void OnTimerEventLoop();

protected:
	DWORD							m_dwTimerSpace;						//ʱ����
	DWORD							m_dwLastTickCount;					//����ʱ��
};

//��ʱ������
class CTimerEngine : public CTimerThread
{
public:
	CTimerEngine(void);
	virtual ~CTimerEngine(void);

public:
	//��ʼ��ʱ�����
	bool Init(ITimerEventSink * pTimerEventSink, DWORD dwTimerSpace);
	//�ͷ�ʱ�����
	bool Release();

public:
	//���ö�ʱ��
	bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter);
	//ɾ����ʱ��
	bool KillTimer(DWORD dwTimerID);
	//ɾ����ʱ��
	bool KillAllTimer();

protected:
	//��ʱ֪ͨ
	virtual void OnTimerEventLoop();

private:
	//��ȡ����
	tagTimerItem * ActiveTimerItem(DWORD dwTimerID);

private:
	CMutex							m_Mutex;							//ͬ������
	ITimerEventSink *				m_pTimerEventSink;					//�ص��ӿ�
	DWORD							m_dwTimePass;						//����ʱ��
	DWORD							m_dwTimeLeave;						//����ʱ��
	std::vector<tagTimerItem *>		m_TimerItemPtrActiveList;			//��б�
	std::vector<tagTimerItem *>		m_TimerItemPtrFreeList;				//�����б�
};


//////////////////////////////////////////////////////////////////////////
