#include "stdafx.h"
#include "TimerEngine.h"

//////////////////////////////////////////////////////////////////////////

//宏定义
#define NO_TIME_LEAVE				DWORD(-1)							//不响应时间

//////////////////////////////////////////////////////////////////////////

CTimerThread::CTimerThread(void)
{
	m_dwTimerSpace = 30L;
	m_dwLastTickCount = 0L;
}

CTimerThread::~CTimerThread(void)
{
}

//配置函数
bool CTimerThread::InitThread(DWORD dwTimerSpace)
{
	//参数调整
	if (dwTimerSpace < 10) dwTimerSpace = 10;
	if (dwTimerSpace > 100) dwTimerSpace = 100;

	//设置变量
	m_dwTimerSpace = dwTimerSpace;
	m_dwLastTickCount = 0L;

	return true;
}

//开始事件
bool CTimerThread::OnEventThreadStrat()
{
	Logger_Info(TEXT("计时器线程启动[ dwCurrentThreadId:%u]"), GetCurrentThreadId());
	return true;
}

//结束事件
bool CTimerThread::OnEventThreadStop()
{
	Logger_Info(TEXT("计时器线程退出[dwCurrentThreadId:%u]"), GetCurrentThreadId());
	return true;
}

//运行函数
bool CTimerThread::OnEventThreadRun()
{
	//获取当前计时(GetTickCount()返回从操作系统启动所经过的毫秒数,可存储的最大时间大致为49.71天,之后就会归0,重新计时)
	//因为是无符型整数,因此即便计时重新开始,也不会有影响

	//获取时间
	DWORD dwTimerSpace = m_dwTimerSpace;
	DWORD dwNowTickCount = GetTickCount();

	//等待调整
	if ((m_dwLastTickCount != 0L) && (dwNowTickCount > m_dwLastTickCount))
	{
		DWORD dwHandleTickCount = dwNowTickCount - m_dwLastTickCount;
		dwTimerSpace = (dwTimerSpace > dwHandleTickCount) ? (dwTimerSpace - dwHandleTickCount) : 0L;
	}

	//定时处理
	if (dwTimerSpace > 0L) Sleep(dwTimerSpace);
	//更新时间
	m_dwLastTickCount = GetTickCount();

	//定时通知
	OnTimerEventLoop();

	return true;
}

//定时通知
void CTimerThread::OnTimerEventLoop()
{
	Logger_Info(TEXT("计时器线程轮询[dwCurrentThreadId:%u]"), GetCurrentThreadId());
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CTimerEngine::CTimerEngine(void)
{
	m_pTimerEventSink = NULL;
	m_dwTimePass = 0L;
	m_dwTimeLeave = NO_TIME_LEAVE;
}

//析构函数
CTimerEngine::~CTimerEngine(void)
{
}

//初始化时间对象
bool CTimerEngine::Init(ITimerEventSink * pTimerEventSink, DWORD dwTimerSpace)
{
	if (pTimerEventSink == NULL)
	{
		Logger_Error(TEXT("回调接口为空"));
		return false;
	}

	//设置变量
	m_pTimerEventSink = pTimerEventSink;
	m_dwTimePass = 0L;
	m_dwTimeLeave = NO_TIME_LEAVE;

	//初始化计时器线程
	if (__super::InitThread(dwTimerSpace) == false)
	{
		Logger_Error(TEXT("初始化计时器线程失败"));
		return false;
	}
	//启动计时器线程
	if (__super::StartThread() == false)
	{
		Logger_Error(TEXT("启动计时器线程失败"));
		return false;
	}

	return true;
}

//释放时间对象
bool CTimerEngine::Release()
{
	//停止计时器线程
	__super::StopThread(INFINITE);

	//结束定时器
	KillAllTimer();

	//释放内存
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		delete m_TimerItemPtrActiveList[i];
		m_TimerItemPtrActiveList[i] = NULL;
	}
	m_TimerItemPtrActiveList.clear();

	//释放内存
	for (size_t i = 0; i < m_TimerItemPtrFreeList.size(); i++)
	{
		delete m_TimerItemPtrFreeList[i];
		m_TimerItemPtrFreeList[i] = NULL;
	}
	m_TimerItemPtrFreeList.clear();

	return true;
}

//设置定时器
bool CTimerEngine::SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter)
{
	//数据校验
	if (dwRepeat == 0) return false;

	//调整时间(调整到时间片的整数倍)
	dwElapse = (dwElapse + m_dwTimerSpace - 1) / m_dwTimerSpace*m_dwTimerSpace;
	if (dwElapse == 0) dwElapse = m_dwTimerSpace * 1;

	//同步控制
	CLocker Locker(m_Mutex);

	//激活对象
	tagTimerItem * pTimerItem = ActiveTimerItem(dwTimerID);
	if (pTimerItem)
	{
		//设置参数
		pTimerItem->dwElapse = dwElapse;
		pTimerItem->dwTimerID = dwTimerID;
		pTimerItem->dwTimeLeave = dwElapse + m_dwTimePass;
		pTimerItem->dwRepeatTimes = dwRepeat;
		pTimerItem->dwBindParameter = dwBindParameter;

		//激活定时器
		m_dwTimeLeave = min(m_dwTimeLeave, dwElapse);

		return true;
	}

	return false;
}

//删除定时器
bool CTimerEngine::KillTimer(DWORD dwTimerID)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//遍历列表
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		if (m_TimerItemPtrActiveList[i]->dwTimerID == dwTimerID)
		{
			//加入空闲列表
			m_TimerItemPtrFreeList.push_back(m_TimerItemPtrActiveList[i]);
			//移除数据
			int nMoveCount = m_TimerItemPtrActiveList.size() - i - 1;
			if (nMoveCount > 0) MoveMemory(&m_TimerItemPtrActiveList[i], &m_TimerItemPtrActiveList[i + 1], nMoveCount * sizeof(tagTimerItem *));
			m_TimerItemPtrActiveList.pop_back();

			//停止计数
			if (m_TimerItemPtrActiveList.size() == 0)
			{
				m_dwTimePass = 0L;
				m_dwTimeLeave = NO_TIME_LEAVE;
			}

			return true;
		}
	}

	return false;
}

//删除定时器
bool CTimerEngine::KillAllTimer()
{
	//同步控制
	CLocker Locker(m_Mutex);

	//遍历列表
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		//加入空闲列表
		m_TimerItemPtrFreeList.push_back(m_TimerItemPtrActiveList[i]);
	}
	m_TimerItemPtrActiveList.clear();

	//设置变量
	m_dwTimePass = 0L;
	m_dwTimeLeave = NO_TIME_LEAVE;

	return true;
}

//定时通知(如果在时间回调同线程中添加或者删除计时器,可能会出现不可预料的问题,因此计时器事件应投递给一个单独的线程)
void CTimerEngine::OnTimerEventLoop()
{
	//同步控制
	CLocker Locker(m_Mutex);

	//倒计时间
	if (m_dwTimeLeave == NO_TIME_LEAVE)
	{
		/*
		//理论上(m_TimerItemPtrActiveList.size() == 0)
		if (m_TimerItemPtrActiveList.size() > 0)
		{
			Logger_Error(TEXT("计时器内部异常,活动计时器数目:%u"), m_TimerItemPtrActiveList.size());
		}
		*/

		return;
	}

	//减少时间
	m_dwTimePass += m_dwTimerSpace;
	m_dwTimeLeave -= m_dwTimerSpace;

	//查询定时器
	if (m_dwTimeLeave == 0)
	{
		//变量定义
		bool bKillTimer = false;
		DWORD dwTimeLeave = NO_TIME_LEAVE;

		//遍历列表
		for (size_t i = 0; i < m_TimerItemPtrActiveList.size();/* i++*/)
		{
			//获取对象
			tagTimerItem * pTimerItem = m_TimerItemPtrActiveList[i];

			//设置变量
			bKillTimer = false;
			pTimerItem->dwTimeLeave -= m_dwTimePass;

			//通知判断
			if (pTimerItem->dwTimeLeave == 0L)
			{
				//事件通知(如果在时间回调同线程中添加或者删除计时器,可能会出现不可预料的问题,因此计时器事件应投递给一个单独的线程)
				m_pTimerEventSink->OnEventTimer(pTimerItem->dwTimerID, pTimerItem->dwBindParameter);
				//设置时间
				pTimerItem->dwTimeLeave = pTimerItem->dwElapse;

				//设置次数
				if (pTimerItem->dwRepeatTimes != TIMES_INFINITY)
				{
					if (pTimerItem->dwRepeatTimes == 1L)
					{
						bKillTimer = true;

						//移除数据
						int nMoveCount = m_TimerItemPtrActiveList.size() - i - 1;
						if (nMoveCount > 0) MoveMemory(&m_TimerItemPtrActiveList[i], &m_TimerItemPtrActiveList[i + 1], nMoveCount * sizeof(tagTimerItem *));
						m_TimerItemPtrActiveList.pop_back();

						//加入空闲列表
						m_TimerItemPtrFreeList.push_back(pTimerItem);
					}
					else
					{
						pTimerItem->dwRepeatTimes--;
					}
				}
			}

			//增加索引
			if (bKillTimer == false)
			{
				dwTimeLeave = min(dwTimeLeave, pTimerItem->dwTimeLeave);
				i++;

				/*
				//理论上(dwTimeLeave%m_dwTimerSpace == 0)
				if ((dwTimeLeave%m_dwTimerSpace) != 0)
				{
					Logger_Error(TEXT("计时器内部异常,剩余时间(%u)%%时间片(%u)=%u"), dwTimeLeave, m_dwTimerSpace, (dwTimeLeave%m_dwTimerSpace));
				}
				*/
			}
		}

		//设置响应
		m_dwTimePass = 0L;
		m_dwTimeLeave = dwTimeLeave;
	}
}

//获取对象
tagTimerItem * CTimerEngine::ActiveTimerItem(DWORD dwTimerID)
{
	//遍历列表
	for (size_t i = 0; i < m_TimerItemPtrActiveList.size(); i++)
	{
		if (m_TimerItemPtrActiveList[i]->dwTimerID == dwTimerID)
		{
			return m_TimerItemPtrActiveList[i];
		}
	}

	//定义对象
	tagTimerItem * pTimerItem = NULL;

	//获取空闲
	if (m_TimerItemPtrFreeList.size() > 0)
	{
		//取出最后一个
		pTimerItem = m_TimerItemPtrFreeList[m_TimerItemPtrFreeList.size() - 1];
		m_TimerItemPtrFreeList.pop_back();
	}
	
	//创建对象
	if (pTimerItem == NULL)
	{
		pTimerItem = new(std::nothrow) tagTimerItem;
	}

	//添加对象
	if (pTimerItem)
	{
		m_TimerItemPtrActiveList.push_back(pTimerItem);
	}

	return pTimerItem;
}

//////////////////////////////////////////////////////////////////////////