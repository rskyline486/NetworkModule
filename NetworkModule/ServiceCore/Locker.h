#pragma once

//////////////////////////////////////////////////////////////////////////

//�������
class CMutex
{
public:
	CMutex();
	~CMutex();

public:
	//��������
	void Lock();
	//��������
	void UnLock();

private:
	mutable CRITICAL_SECTION m_Mutex;
};

//////////////////////////////////////////////////////////////////////////

//��������
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
