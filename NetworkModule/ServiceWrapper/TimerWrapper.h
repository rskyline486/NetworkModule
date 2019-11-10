#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//服务核心
class CTimerKernel : protected ITimerEventSink
{
public:
	CTimerKernel();
	virtual ~CTimerKernel();

public:
	//启动内核
	bool StartKernel();
	//停止内核
	bool StopKernel();

protected:
	//时间事件
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

protected:
	CTimerEngine					m_TimerEngine;						//时间对象
};

//////////////////////////////////////////////////////////////////////////

//服务实例
class CTimerInstance : protected CTimerKernel, public ITimerService
{
public:
	CTimerInstance();
	virtual ~CTimerInstance();

public:
	//启动服务
	virtual bool StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent);
	//停止服务
	virtual bool StopServer();

public:
	//设置定时器
	virtual bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter);
	//删除定时器
	virtual bool KillTimer(DWORD dwTimerID);
	//删除定时器
	virtual bool KillAllTimer();

protected:
	//时间事件
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

private:
	ITimerEvent *					m_pTimerEvent;						//事件接口
};

//////////////////////////////////////////////////////////////////////////
