#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//异步事件回调接口
class IAsynchronismEventSink
{
public:
	//异步开始
	virtual bool OnEventAsynchronismStrat() = 0;
	//异步结束
	virtual bool OnEventAsynchronismStop() = 0;
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize) = 0;
};

//////////////////////////////////////////////////////////////////////////

//异步线程
class CAsynchronismThread : public CThread
{
public:
	CAsynchronismThread(WORD wThreadID);
	virtual ~CAsynchronismThread(void);

public:
	//配置函数
	bool InitThread(HANDLE hCompletionPort, IAsynchronismEventSink * pAsynchronismEventSink);

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
	IAsynchronismEventSink *		m_pAsynchronismEventSink;			//回调接口
	BYTE							m_cbBuffer[ASYNCHRONISM_BUFFER];	//接收缓冲
};

//////////////////////////////////////////////////////////////////////////

//异步引擎
class CAsynchronismEngine
{
public:
	CAsynchronismEngine();
	virtual ~CAsynchronismEngine(void);

public:
	//初始化异步对象
	bool Init(IAsynchronismEventSink * pAsynchronismEventSink, WORD wThreadCount);
	//释放异步对象
	bool Release();

public:
	//负荷信息
	void GetAsynchronismBurthenInfo(tagBurthenInfo & BurthenInfo);
	//发送数据
	bool PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

private:
	HANDLE							m_hCompletionPort;					//完成端口
	std::vector<CThread *>			m_WorkerThreadPtrList;				//工作线程
	CDataQueue						m_DataQueue;						//数据队列
};

//////////////////////////////////////////////////////////////////////////
