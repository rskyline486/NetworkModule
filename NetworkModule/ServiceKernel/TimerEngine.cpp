#include "stdafx.h"
#include "TimerEngine.h"

//////////////////////////////////////////////////////////////////////////

//�궨��
#define NO_TIME_LEAVE				DWORD(-1)							//����Ӧʱ��

//////////////////////////////////////////////////////////////////////////

CTimerThread::CTimerThread(void)
{
	m_dwTimerSpace = 30L;
	m_dwLastTickCount = 0L;
}

CTimerThread::~CTimerThread(void)
{
}

//���ú���
bool CTimerThread::InitThread(DWORD dwTimerSpace)
{
	//��������
	if (dwTimerSpace < 10) dwTimerSpace = 10;
	if (dwTimerSpace > 100) dwTimerSpace = 100;

	//���ñ���
	m_dwTimerSpace = dwTimerSpace;
	m_dwLastTickCount = 0L;

	return true;
}

//��ʼ�¼�
bool CTimerThread::OnEventThreadStrat()
{
	Logger_Info(TEXT("��ʱ���߳�����[ dwCurrentThreadId:%u]"), GetCurrentThreadId());
	return true;
}

//�����¼�
bool CTimerThread::OnEventThreadStop()
{
	Logger_Info(TEXT("��ʱ���߳��˳�[dwCurrentThreadId:%u]"), GetCurrentThreadId());
	return true;
}

//���к���
bool CTimerThread::OnEventThreadRun()
{
	//��ȡ��ǰ��ʱ(GetTickCount()���شӲ���ϵͳ�����������ĺ�����,�ɴ洢�����ʱ�����Ϊ49.71��,֮��ͻ��0,���¼�ʱ)
	//��Ϊ���޷�������,��˼����ʱ���¿�ʼ,Ҳ������Ӱ��

	//��ȡʱ��
	DWORD dwTimerSpace = m_dwTimerSpace;
	DWORD dwNowTickCount = GetTickCount();

	//�ȴ�����
	if ((m_dwLastTickCount != 0L) && (dwNowTickCount > m_dwLastTickCount))
	{
		DWORD dwHandleTickCount = dwNowTickCount - m_dwLastTickCount;
		dwTimerSpace = (dwTimerSpace > dwHandleTickCount) ? (dwTimerSpace - dwHandleTickCount) : 0L;
	}

	//��ʱ����
	if (dwTimerSpace > 0L) Sleep(dwTimerSpace);
	//����ʱ��
	m_dwLastTickCount = GetTickCount();

	//��ʱ֪ͨ
	OnTimerEventLoop();

	return true;
}

//��ʱ֪ͨ
void CTimerThread::OnTimerEventLoop()
{
	Logger_Info(TEXT("��ʱ���߳���ѯ[dwCurrentThreadId:%u]"), GetCurrentThreadId());
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CTimerEngine::CTimerEngine(void)
{
	m_pTimerEventSink = NULL;
	m_dwTimePass = 0L;
	m_dwTimeLeave = NO_TIME_LEAVE;
}

//��������
CTimerEngine::~CTimerEngine(void)
{
}

//��ʼ��ʱ�����
bool CTimerEngine::Init(ITimerEventSink * pTimerEventSink, DWORD dwTimerSpace)
{
	if (pTimerEventSink == NULL)
	{
		Logger_Error(TEXT("�ص��ӿ�Ϊ��"));
		return false;
	}

	//���ñ���
	m_pTimerEventSink = pTimerEventSink;
	m_dwTimePass = 0L;
	m_dwTimeLeave = NO_TIME_LEAVE;

	//��ʼ����ʱ���߳�
	if (__super::InitThread(dwTimerSpace) == false)
	{
		Logger_Error(TEXT("��ʼ����ʱ���߳�ʧ��"));
		return false;
	}
	//������ʱ���߳�
	if (__super::StartThread() == false)
	{
		Logger_Error(TEXT("������ʱ���߳�ʧ��"));
		return false;
	}

	return true;
}

//�ͷ�ʱ�����
bool CTimerEngine::Release()
{
	//ֹͣ��ʱ���߳�
	__super::StopThread(INFINITE);

	//������ʱ��
	KillAllTimer();

	//�ͷ��ڴ�
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		delete m_TimerItemPtrActiveList[i];
		m_TimerItemPtrActiveList[i] = NULL;
	}
	m_TimerItemPtrActiveList.clear();

	//�ͷ��ڴ�
	for (size_t i = 0; i < m_TimerItemPtrFreeList.size(); i++)
	{
		delete m_TimerItemPtrFreeList[i];
		m_TimerItemPtrFreeList[i] = NULL;
	}
	m_TimerItemPtrFreeList.clear();

	return true;
}

//���ö�ʱ��
bool CTimerEngine::SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter)
{
	//����У��
	if (dwRepeat == 0) return false;

	//����ʱ��(������ʱ��Ƭ��������)
	dwElapse = (dwElapse + m_dwTimerSpace - 1) / m_dwTimerSpace*m_dwTimerSpace;
	if (dwElapse == 0) dwElapse = m_dwTimerSpace * 1;

	//ͬ������
	CLocker Locker(m_Mutex);

	//�������
	tagTimerItem * pTimerItem = ActiveTimerItem(dwTimerID);
	if (pTimerItem)
	{
		//���ò���
		pTimerItem->dwElapse = dwElapse;
		pTimerItem->dwTimerID = dwTimerID;
		pTimerItem->dwTimeLeave = dwElapse + m_dwTimePass;
		pTimerItem->dwRepeatTimes = dwRepeat;
		pTimerItem->dwBindParameter = dwBindParameter;

		//���ʱ��
		m_dwTimeLeave = min(m_dwTimeLeave, dwElapse);

		return true;
	}

	return false;
}

//ɾ����ʱ��
bool CTimerEngine::KillTimer(DWORD dwTimerID)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//�����б�
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		if (m_TimerItemPtrActiveList[i]->dwTimerID == dwTimerID)
		{
			//��������б�
			m_TimerItemPtrFreeList.push_back(m_TimerItemPtrActiveList[i]);
			//�Ƴ�����
			int nMoveCount = m_TimerItemPtrActiveList.size() - i - 1;
			if (nMoveCount > 0) MoveMemory(&m_TimerItemPtrActiveList[i], &m_TimerItemPtrActiveList[i + 1], nMoveCount * sizeof(tagTimerItem *));
			m_TimerItemPtrActiveList.pop_back();

			//ֹͣ����
			if (m_TimerItemPtrActiveList.size() == 0)
			{
				m_dwTimePass = 0L;
				m_dwTimeLeave = NO_TIME_LEAVE;
			}

			return true;
		}
	}

	return false;
}

//ɾ����ʱ��
bool CTimerEngine::KillAllTimer()
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//�����б�
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		//��������б�
		m_TimerItemPtrFreeList.push_back(m_TimerItemPtrActiveList[i]);
	}
	m_TimerItemPtrActiveList.clear();

	//���ñ���
	m_dwTimePass = 0L;
	m_dwTimeLeave = NO_TIME_LEAVE;

	return true;
}

//��ʱ֪ͨ(�����ʱ��ص�ͬ�߳�����ӻ���ɾ����ʱ��,���ܻ���ֲ���Ԥ�ϵ�����,��˼�ʱ���¼�ӦͶ�ݸ�һ���������߳�)
void CTimerEngine::OnTimerEventLoop()
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//����ʱ��
	if (m_dwTimeLeave == NO_TIME_LEAVE)
	{
		/*
		//������(m_TimerItemPtrActiveList.size() == 0)
		if (m_TimerItemPtrActiveList.size() > 0)
		{
			Logger_Error(TEXT("��ʱ���ڲ��쳣,���ʱ����Ŀ:%u"), m_TimerItemPtrActiveList.size());
		}
		*/

		return;
	}

	//����ʱ��
	m_dwTimePass += m_dwTimerSpace;
	m_dwTimeLeave -= m_dwTimerSpace;

	//��ѯ��ʱ��
	if (m_dwTimeLeave == 0)
	{
		//��������
		bool bKillTimer = false;
		DWORD dwTimeLeave = NO_TIME_LEAVE;

		//�����б�
		for (size_t i = 0; i < m_TimerItemPtrActiveList.size();/* i++*/)
		{
			//��ȡ����
			tagTimerItem * pTimerItem = m_TimerItemPtrActiveList[i];

			//���ñ���
			bKillTimer = false;
			pTimerItem->dwTimeLeave -= m_dwTimePass;

			//֪ͨ�ж�
			if (pTimerItem->dwTimeLeave == 0L)
			{
				//�¼�֪ͨ(�����ʱ��ص�ͬ�߳�����ӻ���ɾ����ʱ��,���ܻ���ֲ���Ԥ�ϵ�����,��˼�ʱ���¼�ӦͶ�ݸ�һ���������߳�)
				m_pTimerEventSink->OnEventTimer(pTimerItem->dwTimerID, pTimerItem->dwBindParameter);
				//����ʱ��
				pTimerItem->dwTimeLeave = pTimerItem->dwElapse;

				//���ô���
				if (pTimerItem->dwRepeatTimes != TIMES_INFINITY)
				{
					if (pTimerItem->dwRepeatTimes == 1L)
					{
						bKillTimer = true;

						//�Ƴ�����
						int nMoveCount = m_TimerItemPtrActiveList.size() - i - 1;
						if (nMoveCount > 0) MoveMemory(&m_TimerItemPtrActiveList[i], &m_TimerItemPtrActiveList[i + 1], nMoveCount * sizeof(tagTimerItem *));
						m_TimerItemPtrActiveList.pop_back();

						//��������б�
						m_TimerItemPtrFreeList.push_back(pTimerItem);
					}
					else
					{
						pTimerItem->dwRepeatTimes--;
					}
				}
			}

			//��������
			if (bKillTimer == false)
			{
				dwTimeLeave = min(dwTimeLeave, pTimerItem->dwTimeLeave);
				i++;

				/*
				//������(dwTimeLeave%m_dwTimerSpace == 0)
				if ((dwTimeLeave%m_dwTimerSpace) != 0)
				{
					Logger_Error(TEXT("��ʱ���ڲ��쳣,ʣ��ʱ��(%u)%%ʱ��Ƭ(%u)=%u"), dwTimeLeave, m_dwTimerSpace, (dwTimeLeave%m_dwTimerSpace));
				}
				*/
			}
		}

		//������Ӧ
		m_dwTimePass = 0L;
		m_dwTimeLeave = dwTimeLeave;
	}
}

//��ȡ����
tagTimerItem * CTimerEngine::ActiveTimerItem(DWORD dwTimerID)
{
	//�����б�
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		if (m_TimerItemPtrActiveList[i]->dwTimerID == dwTimerID)
		{
			return m_TimerItemPtrActiveList[i];
		}
	}

	//�������
	tagTimerItem * pTimerItem = NULL;

	//��ȡ����
	if (m_TimerItemPtrFreeList.size() > 0)
	{
		//ȡ�����һ��
		pTimerItem = m_TimerItemPtrFreeList[m_TimerItemPtrFreeList.size() - 1];
		m_TimerItemPtrFreeList.pop_back();
	}
	
	//��������
	if (pTimerItem == NULL)
	{
		pTimerItem = new(std::nothrow) tagTimerItem;
	}

	//��Ӷ���
	if (pTimerItem)
	{
		m_TimerItemPtrActiveList.push_back(pTimerItem);
	}

	return pTimerItem;
}

//////////////////////////////////////////////////////////////////////////