#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//工作线程
class CWorkerThread : public CThread
{
public:
	CWorkerThread(WORD wThreadID);
	virtual ~CWorkerThread(void);

public:
	//配置函数
	bool InitThread(HANDLE hCompletionPort);

protected:
	//开始事件
	virtual bool OnEventThreadStrat();
	//结束事件
	virtual bool OnEventThreadStop();
	//运行函数
	virtual bool OnEventThreadRun();

private:
	WORD							m_wThreadID;						//线程标识
	HANDLE							m_hCompletionPort;					//完成端口
};

//////////////////////////////////////////////////////////////////////////
