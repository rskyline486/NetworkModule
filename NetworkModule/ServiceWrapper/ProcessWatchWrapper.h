#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//进程结构
struct PROCESS_INFO
{
	DWORD							dwProcessID;						//进程ID
	TCHAR							szProcessPath[256];					//进程路径
	TCHAR							szConsoleTitle[64];					//窗口标题
	TCHAR							szCommandLine[256];					//命令参数
	bool							bCreateNewWindow;					//创建窗口
	bool							bShowWindow;						//显示窗口
};

//////////////////////////////////////////////////////////////////////////

//服务核心
class CProcessWatchKernel : protected CThread
{
public:
	CProcessWatchKernel();
	virtual ~CProcessWatchKernel();

public:
	//启动内核
	bool StartKernel();
	//停止内核
	bool StopKernel();

public:
	//添加进程
	bool AddProcessInfo(LPCTSTR pszProcessPath, LPCTSTR pszConsoleTitle, LPCTSTR pszCommandLine, bool bCreateNewWindow, bool bShowWindow);

protected:
	//开始事件
	virtual bool OnEventThreadStrat();
	//结束事件
	virtual bool OnEventThreadStop();
	//运行函数
	virtual bool OnEventThreadRun();

protected:
	std::vector<PROCESS_INFO *>		m_ProcessInfoPtrList;				//进程参数
};

//////////////////////////////////////////////////////////////////////////

//服务实例
class CProcessWatchInstance
{
public:
	CProcessWatchInstance();
	virtual ~CProcessWatchInstance();

public:
	//启动服务
	virtual bool StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent);
	//停止服务
	virtual bool StopServer();

private:
	CProcessWatchKernel				m_ProcessWatchKernel;				//守护对象
};

//////////////////////////////////////////////////////////////////////////
