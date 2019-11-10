#include "stdafx.h"
#include "TimerWrapper.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CTimerKernel::CTimerKernel()
{
}

//析构函数
CTimerKernel::~CTimerKernel()
{
}

//启动内核
bool CTimerKernel::StartKernel()
{
	//初始化计时器对象
	if (m_TimerEngine.Init(this, 30) == false) return false;

	return true;
}

//停止内核
bool CTimerKernel::StopKernel()
{
	//停止计时器对象
	m_TimerEngine.Release();

	return true;
}

//时间事件
bool CTimerKernel::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	Logger_Info(TEXT("时间事件=>时间标识:%u, 绑定数据:%u"), dwTimerID, dwBindParameter);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CTimerInstance::CTimerInstance()
{
	m_pTimerEvent = NULL;
}

//析构函数
CTimerInstance::~CTimerInstance()
{
}

//启动服务
bool CTimerInstance::StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent)
{
	//初始化计时器对象
	if (m_TimerEngine.Init(this, dwTimerSpace) == false) return false;

	//设置接口
	m_pTimerEvent = pTimerEvent;

	return true;
}

//停止服务
bool CTimerInstance::StopServer()
{
	//停止服务
	__super::StopKernel();

	//重置接口
	m_pTimerEvent = NULL;

	return true;
}

//设置定时器
bool CTimerInstance::SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter)
{
	return m_TimerEngine.SetTimer(dwTimerID, dwElapse, dwRepeat, dwBindParameter);
}

//删除定时器
bool CTimerInstance::KillTimer(DWORD dwTimerID)
{
	return m_TimerEngine.KillTimer(dwTimerID);
}

//删除定时器
bool CTimerInstance::KillAllTimer()
{
	return m_TimerEngine.KillAllTimer();
}

//时间事件
bool CTimerInstance::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	//回调处理
	if (m_pTimerEvent)
	{
		return m_pTimerEvent->OnTimerEvent(dwTimerID, dwBindParameter);
	}

	return __super::OnEventTimer(dwTimerID, dwBindParameter);
}

//////////////////////////////////////////////////////////////////////////
