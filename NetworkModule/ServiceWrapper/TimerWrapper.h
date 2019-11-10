#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//�������
class CTimerKernel : protected ITimerEventSink
{
public:
	CTimerKernel();
	virtual ~CTimerKernel();

public:
	//�����ں�
	bool StartKernel();
	//ֹͣ�ں�
	bool StopKernel();

protected:
	//ʱ���¼�
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

protected:
	CTimerEngine					m_TimerEngine;						//ʱ�����
};

//////////////////////////////////////////////////////////////////////////

//����ʵ��
class CTimerInstance : protected CTimerKernel, public ITimerService
{
public:
	CTimerInstance();
	virtual ~CTimerInstance();

public:
	//��������
	virtual bool StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent);
	//ֹͣ����
	virtual bool StopServer();

public:
	//���ö�ʱ��
	virtual bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter);
	//ɾ����ʱ��
	virtual bool KillTimer(DWORD dwTimerID);
	//ɾ����ʱ��
	virtual bool KillAllTimer();

protected:
	//ʱ���¼�
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

private:
	ITimerEvent *					m_pTimerEvent;						//�¼��ӿ�
};

//////////////////////////////////////////////////////////////////////////
