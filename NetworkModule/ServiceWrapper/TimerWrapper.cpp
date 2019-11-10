#include "stdafx.h"
#include "TimerWrapper.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CTimerKernel::CTimerKernel()
{
}

//��������
CTimerKernel::~CTimerKernel()
{
}

//�����ں�
bool CTimerKernel::StartKernel()
{
	//��ʼ����ʱ������
	if (m_TimerEngine.Init(this, 30) == false) return false;

	return true;
}

//ֹͣ�ں�
bool CTimerKernel::StopKernel()
{
	//ֹͣ��ʱ������
	m_TimerEngine.Release();

	return true;
}

//ʱ���¼�
bool CTimerKernel::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	Logger_Info(TEXT("ʱ���¼�=>ʱ���ʶ:%u, ������:%u"), dwTimerID, dwBindParameter);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CTimerInstance::CTimerInstance()
{
	m_pTimerEvent = NULL;
}

//��������
CTimerInstance::~CTimerInstance()
{
}

//��������
bool CTimerInstance::StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent)
{
	//��ʼ����ʱ������
	if (m_TimerEngine.Init(this, dwTimerSpace) == false) return false;

	//���ýӿ�
	m_pTimerEvent = pTimerEvent;

	return true;
}

//ֹͣ����
bool CTimerInstance::StopServer()
{
	//ֹͣ����
	__super::StopKernel();

	//���ýӿ�
	m_pTimerEvent = NULL;

	return true;
}

//���ö�ʱ��
bool CTimerInstance::SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter)
{
	return m_TimerEngine.SetTimer(dwTimerID, dwElapse, dwRepeat, dwBindParameter);
}

//ɾ����ʱ��
bool CTimerInstance::KillTimer(DWORD dwTimerID)
{
	return m_TimerEngine.KillTimer(dwTimerID);
}

//ɾ����ʱ��
bool CTimerInstance::KillAllTimer()
{
	return m_TimerEngine.KillAllTimer();
}

//ʱ���¼�
bool CTimerInstance::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	//�ص�����
	if (m_pTimerEvent)
	{
		return m_pTimerEvent->OnTimerEvent(dwTimerID, dwBindParameter);
	}

	return __super::OnEventTimer(dwTimerID, dwBindParameter);
}

//////////////////////////////////////////////////////////////////////////
