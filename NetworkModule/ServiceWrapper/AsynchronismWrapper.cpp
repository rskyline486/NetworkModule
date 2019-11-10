#include "stdafx.h"
#include "AsynchronismWrapper.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CAsynchronismKernel::CAsynchronismKernel()
{
}

//析构函数
CAsynchronismKernel::~CAsynchronismKernel()
{
}

//启动内核
bool CAsynchronismKernel::StartKernel()
{
	//初始化异步对象
	if (m_AsynchronismEngine.Init(this, 1) == false) return false;

	return true;
}

//停止内核
bool CAsynchronismKernel::StopKernel()
{
	//停止异步对象
	m_AsynchronismEngine.Release();

	return true;
}

//异步开始
bool CAsynchronismKernel::OnEventAsynchronismStrat()
{
	return true;
}

//异步结束
bool CAsynchronismKernel::OnEventAsynchronismStop()
{
	return true;
}

//异步事件
bool CAsynchronismKernel::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	Logger_Info(TEXT("异步事件=>标识信息:%u, 数据大小:%u"), wIdentifier, wDataSize);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CAsynchronismInstance::CAsynchronismInstance()
{
	m_pAsynchronismEvent = NULL;
}

//析构函数
CAsynchronismInstance::~CAsynchronismInstance()
{
}

//启动服务
bool CAsynchronismInstance::StartServer(WORD wThreadCount, IAsynchronismEvent * pAsynchronismEvent)
{
	//设置接口
	m_pAsynchronismEvent = pAsynchronismEvent;

	//初始化异步对象
	if (m_AsynchronismEngine.Init(this, wThreadCount) == false)
	{
		m_pAsynchronismEvent = NULL;
		return false;
	}

	return true;
}

//停止服务
bool CAsynchronismInstance::StopServer()
{
	//停止服务
	__super::StopKernel();

	//重置接口
	m_pAsynchronismEvent = NULL;

	return true;
}

//发送数据
bool CAsynchronismInstance::PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	return m_AsynchronismEngine.PostAsynchronismData(wIdentifier, pData, wDataSize);
}

//异步开始
bool CAsynchronismInstance::OnEventAsynchronismStrat()
{
	//回调处理
	if (m_pAsynchronismEvent)
	{
		return m_pAsynchronismEvent->OnAsynchronismStart();
	}

	return __super::OnEventAsynchronismStrat();
}

//异步结束
bool CAsynchronismInstance::OnEventAsynchronismStop()
{
	//回调处理
	if (m_pAsynchronismEvent)
	{
		return m_pAsynchronismEvent->OnAsynchronismStop();
	}

	return __super::OnEventAsynchronismStop();
}

//异步事件
bool CAsynchronismInstance::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	//回调处理
	if (m_pAsynchronismEvent)
	{
		return m_pAsynchronismEvent->OnAsynchronismData(wIdentifier, pData, wDataSize);
	}

	return __super::OnEventAsynchronismData(wIdentifier, pData, wDataSize);
}

//////////////////////////////////////////////////////////////////////////
