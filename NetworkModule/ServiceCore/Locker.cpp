#include "stdafx.h"
#include "Locker.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CMutex::CMutex()
{
	InitializeCriticalSection(&m_Mutex);
}

//��������
CMutex::~CMutex()
{
	DeleteCriticalSection(&m_Mutex);
}

//��������
void CMutex::Lock()
{
	EnterCriticalSection(&m_Mutex);
}

//��������
void CMutex::UnLock()
{
	LeaveCriticalSection(&m_Mutex);
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CLocker::CLocker(CMutex& mutex) : m_Mutex(mutex)
{
	m_Mutex.Lock();
}

//��������
CLocker::~CLocker(void)
{
	m_Mutex.UnLock();
}

//////////////////////////////////////////////////////////////////////////
