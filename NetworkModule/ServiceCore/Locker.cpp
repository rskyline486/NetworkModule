#include "stdafx.h"
#include "Locker.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CMutex::CMutex()
{
	InitializeCriticalSection(&m_Mutex);
}

//析构函数
CMutex::~CMutex()
{
	DeleteCriticalSection(&m_Mutex);
}

//锁定对象
void CMutex::Lock()
{
	EnterCriticalSection(&m_Mutex);
}

//解锁对象
void CMutex::UnLock()
{
	LeaveCriticalSection(&m_Mutex);
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CLocker::CLocker(CMutex& mutex) : m_Mutex(mutex)
{
	m_Mutex.Lock();
}

//析构函数
CLocker::~CLocker(void)
{
	m_Mutex.UnLock();
}

//////////////////////////////////////////////////////////////////////////
