#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//时间事件回调接口
class ITimerEventSink
{
public:
	//异步事件
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter) = 0;
};

//////////////////////////////////////////////////////////////////////////

//计时对象
struct tagTimerItem
{
	DWORD							dwElapse;							//倒数时间
	DWORD							dwTimerID;							//时间标识
	DWORD							dwTimeLeave;						//剩余时间
	DWORD							dwRepeatTimes;						//重复次数
	WPARAM							dwBindParameter;					//绑定参数
};

//////////////////////////////////////////////////////////////////////////

//时间线程
class CTimerThread : public CThread
{
public:
	CTimerThread(void);
	virtual ~CTimerThread(void);

public:
	//配置函数
	bool InitThread(DWORD dwTimerSpace);

protected:
	//开始事件
	virtual bool OnEventThreadStrat();
	//结束事件
	virtual bool OnEventThreadStop();
	//运行函数
	virtual bool OnEventThreadRun();

protected:
	//定时通知
	virtual void OnTimerEventLoop();

protected:
	DWORD							m_dwTimerSpace;						//时间间隔
	DWORD							m_dwLastTickCount;					//处理时间
};

//定时器引擎
class CTimerEngine : public CTimerThread
{
public:
	CTimerEngine(void);
	virtual ~CTimerEngine(void);

public:
	//初始化时间对象
	bool Init(ITimerEventSink * pTimerEventSink, DWORD dwTimerSpace);
	//释放时间对象
	bool Release();

public:
	//设置定时器
	bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter);
	//删除定时器
	bool KillTimer(DWORD dwTimerID);
	//删除定时器
	bool KillAllTimer();

protected:
	//定时通知
	virtual void OnTimerEventLoop();

private:
	//获取对象
	tagTimerItem * ActiveTimerItem(DWORD dwTimerID);

private:
	CMutex							m_Mutex;							//同步对象
	ITimerEventSink *				m_pTimerEventSink;					//回调接口
	DWORD							m_dwTimePass;						//经过时间
	DWORD							m_dwTimeLeave;						//倒计时间
	std::vector<tagTimerItem *>		m_TimerItemPtrActiveList;			//活动列表
	std::vector<tagTimerItem *>		m_TimerItemPtrFreeList;				//空闲列表
};


//////////////////////////////////////////////////////////////////////////
