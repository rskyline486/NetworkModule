#include "stdafx.h"
#include "DetectThread.h"

//////////////////////////////////////////////////////////////////////////

CDetectThread::CDetectThread()
{
	m_pDetectEventSink = NULL;
	m_dwPileTime = 0L;
	m_dwDetectTime = 10000L;
}

CDetectThread::~CDetectThread(void)
{
}

//配置函数
bool CDetectThread::InitThread(IDetectEventSink * pDetectEventSink, DWORD dwDetectTime)
{
	if (pDetectEventSink == NULL)
	{
		Logger_Error(TEXT("回调接口为空"));
		return false;
	}

	//设置变量
	m_pDetectEventSink = pDetectEventSink;
	m_dwDetectTime = dwDetectTime;
	m_dwPileTime = 0L;

	return true;
}

//开始事件
bool CDetectThread::OnEventThreadStrat()
{
	Logger_Info(TEXT("检测线程启动[dwDetectTime:%u毫秒, dwCurrentThreadId:%u]"), m_dwDetectTime, GetCurrentThreadId());
	return true;
}

//结束事件
bool CDetectThread::OnEventThreadStop()
{
	Logger_Info(TEXT("检测线程退出[dwDetectTime:%u毫秒, dwCurrentThreadId:%u]"), m_dwDetectTime, GetCurrentThreadId());
	return true;
}

//运行函数
bool CDetectThread::OnEventThreadRun()
{
	//设置间隔
	Sleep(200);
	m_dwPileTime += 200L;

	//检测连接
	if (m_dwPileTime >= m_dwDetectTime)
	{
		m_dwPileTime = 0L;
		m_pDetectEventSink->OnEventDetectBeat();
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
