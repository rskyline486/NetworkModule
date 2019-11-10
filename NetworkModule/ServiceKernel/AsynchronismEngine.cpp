#include "stdafx.h"
#include "AsynchronismEngine.h"

//////////////////////////////////////////////////////////////////////////

CAsynchronismThread::CAsynchronismThread(WORD wThreadID)
{
	m_wThreadID = wThreadID;
	m_hCompletionPort = NULL;
	m_pAsynchronismEventSink = NULL;
	ZeroMemory(m_cbBuffer, sizeof(m_cbBuffer));
}

CAsynchronismThread::~CAsynchronismThread(void)
{
}

//配置函数
bool CAsynchronismThread::InitThread(HANDLE hCompletionPort, IAsynchronismEventSink * pAsynchronismEventSink)
{
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("完成端口对象为空"));
		return false;
	}

	if (pAsynchronismEventSink == NULL)
	{
		Logger_Error(TEXT("回调接口为空"));
		return false;
	}

	//设置变量
	m_hCompletionPort = hCompletionPort;
	m_pAsynchronismEventSink = pAsynchronismEventSink;

	return true;
}

//开始事件
bool CAsynchronismThread::OnEventThreadStrat()
{
	//开始事件
	m_pAsynchronismEventSink->OnEventAsynchronismStrat();

	Logger_Info(TEXT("异步线程启动[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//结束事件
bool CAsynchronismThread::OnEventThreadStop()
{
	//结束事件
	m_pAsynchronismEventSink->OnEventAsynchronismStop();

	Logger_Info(TEXT("异步线程退出[wThreadID:%u, dwCurrentThreadId:%u]"), m_wThreadID, GetCurrentThreadId());
	return true;
}

//运行函数
bool CAsynchronismThread::OnEventThreadRun()
{
	DWORD dwThancferred = 0;
	OVERLAPPED * pOverLapped = NULL;
	CDataQueue * pDataQueue = NULL;

	//获取完成对象
	if (!GetQueuedCompletionStatus(m_hCompletionPort, &dwThancferred, (PULONG_PTR)&pDataQueue, &pOverLapped, INFINITE))
	{
		Logger_Info(TEXT("异步模块异常,获取完成对象失败,错误码:%d"), WSAGetLastError());
		return false;
	}

	//退出判断
	if (pDataQueue == NULL)
	{
		return false;
	}

	//提取数据
	tagDataHead DataHead;
	pDataQueue->DistillData(DataHead, m_cbBuffer, sizeof(m_cbBuffer));

	//数据处理
	m_pAsynchronismEventSink->OnEventAsynchronismData(DataHead.wIdentifier, m_cbBuffer, DataHead.wDataSize);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CAsynchronismEngine::CAsynchronismEngine()
{
	m_hCompletionPort = NULL;
}

//析构函数
CAsynchronismEngine::~CAsynchronismEngine(void)
{
}

//初始化异步对象
bool CAsynchronismEngine::Init(IAsynchronismEventSink * pAsynchronismEventSink, WORD wThreadCount)
{
	if (m_hCompletionPort != NULL)
	{
		Logger_Error(TEXT("模块已初始化"));
		return false;
	}

	//系统信息
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	//调整线程数目
	wThreadCount = (WORD)(wThreadCount < SystemInfo.dwNumberOfProcessors ? wThreadCount : SystemInfo.dwNumberOfProcessors);
	wThreadCount = wThreadCount > 0 ? wThreadCount : 1;

	//完成端口
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, wThreadCount);
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("创建完成端口失败"));
		return false;
	}

	//创建并启动工作线程
	m_WorkerThreadPtrList.clear();
	for (WORD i = 0; i < wThreadCount; i++)
	{
		//申请线程对象
		CAsynchronismThread * pWorkerThread = new(std::nothrow) CAsynchronismThread(i);
		if (pWorkerThread == NULL)
		{
			Logger_Error(TEXT("申请工作线程失败"));
			break;
		}

		//初始化线程对象
		if (pWorkerThread->InitThread(m_hCompletionPort, pAsynchronismEventSink) == false)
		{
			Logger_Error(TEXT("初始化工作线程失败"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//启动线程
		if (pWorkerThread->StartThread() == false)
		{
			Logger_Error(TEXT("启动工作线程失败"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//加入线程池列表
		m_WorkerThreadPtrList.push_back(pWorkerThread);
	}

	//工作线程校验
	if (m_WorkerThreadPtrList.empty())
	{
		Logger_Error(TEXT("异步模块启动失败,工作线程数:0"));
		Release();
		return false;
	}

	//初始化队列大小(16M)
	m_DataQueue.InitSize(16384 * 1024);

	Logger_Info(TEXT("异步模块启动成功,工作线程数:%d"), wThreadCount);

	return true;
}

//释放异步对象
bool CAsynchronismEngine::Release()
{
	//通知工作线程结束
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	}

	//停止工作线程
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;

		m_WorkerThreadPtrList[i]->StopThread(INFINITE);
		delete m_WorkerThreadPtrList[i];
		m_WorkerThreadPtrList[i] = NULL;
	}
	m_WorkerThreadPtrList.clear();

	//关闭对象
	if (m_hCompletionPort != NULL)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}

	Logger_Info(TEXT("异步模块停止"));

	return true;
}

//负荷信息
void CAsynchronismEngine::GetAsynchronismBurthenInfo(tagBurthenInfo & BurthenInfo)
{
	m_DataQueue.GetBurthenInfo(BurthenInfo);
}

//发送数据
bool CAsynchronismEngine::PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	//运行判断
	if (m_hCompletionPort == NULL) return false;

	//插入数据
	if (m_DataQueue.InsertData(wIdentifier, pData, wDataSize) == false)
	{
		Logger_Error(TEXT("插入数据失败,标识信息:%u,数据大小:%u"), wIdentifier, wDataSize);
		return false;
	}

	//投递通知
	PostQueuedCompletionStatus(m_hCompletionPort, wDataSize, (ULONG_PTR)&m_DataQueue, NULL);

	return true;
}

//////////////////////////////////////////////////////////////////////////
