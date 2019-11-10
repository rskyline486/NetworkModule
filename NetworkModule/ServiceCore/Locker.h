#pragma once

//////////////////////////////////////////////////////////////////////////

//互斥对象
class CMutex
{
public:
	CMutex();
	~CMutex();

public:
	//锁定对象
	void Lock();
	//解锁对象
	void UnLock();

private:
	mutable CRITICAL_SECTION m_Mutex;
};

//////////////////////////////////////////////////////////////////////////

//锁定对象
class CLocker
{
public:
	CLocker(CMutex& mutex);
	~CLocker(void);

private:
	CLocker(const CLocker&);

private:
	CMutex& m_Mutex;
};

//////////////////////////////////////////////////////////////////////////
