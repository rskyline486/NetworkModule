#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//服务核心
class CAsynchronismKernel : protected IAsynchronismEventSink
{
public:
	CAsynchronismKernel();
	virtual ~CAsynchronismKernel();

public:
	//启动内核
	bool StartKernel();
	//停止内核
	bool StopKernel();

protected:
	//异步开始
	virtual bool OnEventAsynchronismStrat() = 0;
	//异步结束
	virtual bool OnEventAsynchronismStop() = 0;
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	CAsynchronismEngine				m_AsynchronismEngine;				//异步对象
};

//////////////////////////////////////////////////////////////////////////

//服务实例
class CAsynchronismInstance : protected CAsynchronismKernel, public IAsynchronismService
{
public:
	CAsynchronismInstance();
	virtual ~CAsynchronismInstance();

public:
	//启动服务
	virtual bool StartServer(WORD wThreadCount, IAsynchronismEvent * pAsynchronismEvent);
	//停止服务
	virtual bool StopServer();

public:
	//发送数据
	virtual bool PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//异步开始
	virtual bool OnEventAsynchronismStrat();
	//异步结束
	virtual bool OnEventAsynchronismStop();
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

private:
	IAsynchronismEvent *			m_pAsynchronismEvent;				//事件接口
};

//////////////////////////////////////////////////////////////////////////
