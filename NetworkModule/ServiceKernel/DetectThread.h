#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//检测事件回调接口
class IDetectEventSink
{
public:
	//检测事件
	virtual VOID OnEventDetectBeat() = 0;
};

//////////////////////////////////////////////////////////////////////////

//检测线程
class CDetectThread : public CThread
{
public:
	CDetectThread();
	virtual ~CDetectThread(void);

public:
	//配置函数
	bool InitThread(IDetectEventSink * pDetectEventSink, DWORD dwDetectTime);

protected:
	//开始事件
	virtual bool OnEventThreadStrat();
	//结束事件
	virtual bool OnEventThreadStop();
	//运行函数
	virtual bool OnEventThreadRun();

private:
	IDetectEventSink *				m_pDetectEventSink;					//回调接口
	DWORD							m_dwPileTime;						//积累时间
	DWORD							m_dwDetectTime;						//检测时间
};

//////////////////////////////////////////////////////////////////////////
