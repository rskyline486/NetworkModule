#pragma once

//////////////////////////////////////////////////////////////////////////

//线程对象
class CThread
{
public:
	CThread(void);
	virtual ~CThread(void);

public:
	//启动线程
	bool StartThread();
	//停止线程
	bool StopThread(DWORD dwMillSeconds = INFINITE);
	//判断线程是否在运行
	bool IsRuning();

protected:
	//开始事件
	virtual bool OnEventThreadStrat();
	//运行事件
	virtual bool OnEventThreadRun();
	//结束事件
	virtual bool OnEventThreadStop();

private:
	//线程函数
	static unsigned int __stdcall ThreadFunction(void * pThreadData);

protected:
	volatile bool m_bRun;
	unsigned int m_uThreadID;
	HANDLE m_hThreadHandle;
};

//////////////////////////////////////////////////////////////////////////
