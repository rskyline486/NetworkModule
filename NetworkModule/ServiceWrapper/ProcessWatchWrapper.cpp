#include "stdafx.h"
#include "ProcessWatchWrapper.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
CProcessWatchKernel::CProcessWatchKernel()
{
}

//析构函数
CProcessWatchKernel::~CProcessWatchKernel()
{
}

//启动内核
bool CProcessWatchKernel::StartKernel()
{
	//文件名称定义
	TCHAR szFileName[MAX_PATH];
	ZeroMemory(szFileName, sizeof(szFileName));

	//获取文件路径以及文件名称
	if (1)
	{
		TCHAR szFilePath[MAX_PATH];
		ZeroMemory(szFilePath, sizeof(szFilePath));
		GetModuleFileName(NULL, szFilePath, CountArray(szFilePath));
		for (INT i = lstrlen(szFilePath) - 1; i >= 0; i--)
		{
			if (szFilePath[i] == TEXT('\\'))
			{
				//计算长度(去掉反斜杠,去掉后缀)
				INT nNameCount = (lstrlen(szFilePath) - i) - 1 - 4;
				for (INT k = 0; k < nNameCount; k++) szFileName[k] = szFilePath[i + 1 + k];
				break;
			}
		}
	}

	//设置标题
	TCHAR szTitleName[128];
	if (lstrlen(szFileName) > 0)
	{
		_stprintf_s(szTitleName, CountArray(szTitleName), TEXT("%s - [守护进程]-[%u]"), szFileName, GetCurrentProcessId());
	}
	else
	{
		_stprintf_s(szTitleName, CountArray(szTitleName), TEXT("[守护进程]-[%u]"), GetCurrentProcessId());
	}
	
	//设置标题
	::SetConsoleTitle(szTitleName);

	//日志名称
	Logger_SetFileName(TEXT("守护进程"));

	//启动线程
	if (__super::StartThread() == false)
	{
		Logger_Error(TEXT("启动守护线程失败"));
		return false;
	}

	return true;
}

//停止内核
bool CProcessWatchKernel::StopKernel()
{
	//停止守护线程
	__super::StopThread(INFINITE);

	//释放内存
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		delete m_ProcessInfoPtrList[i];
		m_ProcessInfoPtrList[i] = NULL;
	}
	m_ProcessInfoPtrList.clear();

	return true;
}

//添加进程
bool CProcessWatchKernel::AddProcessInfo(LPCTSTR pszProcessPath, LPCTSTR pszConsoleTitle, LPCTSTR pszCommandLine, bool bCreateNewWindow, bool bShowWindow)
{
	//数据校验
	if (pszProcessPath == NULL) return false;
	//运行校验
	if (__super::IsRuning() == true) return false;

	//申请对象
	PROCESS_INFO * pProcessInfo = new(std::nothrow) PROCESS_INFO;
	if (pProcessInfo == NULL) return false;

	//变量赋值
	pProcessInfo->dwProcessID = 0;
	lstrcpyn(pProcessInfo->szProcessPath, pszProcessPath, CountArray(pProcessInfo->szProcessPath));
	lstrcpyn(pProcessInfo->szConsoleTitle, pszConsoleTitle, CountArray(pProcessInfo->szConsoleTitle));
	lstrcpyn(pProcessInfo->szCommandLine, pszCommandLine, CountArray(pProcessInfo->szCommandLine));
	pProcessInfo->bCreateNewWindow = bCreateNewWindow;
	pProcessInfo->bShowWindow = bShowWindow;

	/*
	//立即启动
	if (0)
	{
		//启动进程
		pProcessInfo->dwProcessID = CNetworkModule::StartProcess(pProcessInfo->szProcessPath, pProcessInfo->szConsoleTitle, pProcessInfo->szCommandLine, pProcessInfo->bCreateNewWindow, pProcessInfo->bShowWindow);
		if (pProcessInfo->dwProcessID == 0)
		{
			//释放对象
			delete pProcessInfo;
			pProcessInfo = NULL;

			Logger_Info(TEXT("启动进程失败=>进程名称:%s"), pProcessInfo->szConsoleTitle);
			return false;
		}

		Logger_Info(TEXT("启动进程成功=>进程名称:%s-[%u]"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
	}
	*/

	//添加守护
	m_ProcessInfoPtrList.push_back(pProcessInfo);

	return true;
}

//开始事件
bool CProcessWatchKernel::OnEventThreadStrat()
{
	Logger_Info(TEXT("守护线程启动[dwCurrentThreadId:%u]"), GetCurrentThreadId());

	//遍历进程
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		//启动判断
		if (m_ProcessInfoPtrList[i]->dwProcessID != 0) continue;

		//启动进程
		PROCESS_INFO * pProcessInfo = m_ProcessInfoPtrList[i];
		pProcessInfo->dwProcessID = CNetworkModule::StartProcess(pProcessInfo->szProcessPath, pProcessInfo->szConsoleTitle, pProcessInfo->szCommandLine, pProcessInfo->bCreateNewWindow, pProcessInfo->bShowWindow);
		if (pProcessInfo->dwProcessID == 0)
		{
			Logger_Info(TEXT("启动进程失败=>进程名称:%s"), pProcessInfo->szConsoleTitle);
		}
		else
		{
			Logger_Info(TEXT("启动进程成功=>进程名称:%s-[%u]"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
		}

		//延时5秒
		Sleep(5000);
	}

	//退出校验
	if (m_ProcessInfoPtrList.size() < 1) return false;

	return true;
}

//结束事件
bool CProcessWatchKernel::OnEventThreadStop()
{
	//关闭进程
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		CNetworkModule::KillProcess(m_ProcessInfoPtrList[i]->dwProcessID);
		Logger_Info(TEXT("关闭进程=>进程名称:%s-[%u]"), m_ProcessInfoPtrList[i]->szConsoleTitle, m_ProcessInfoPtrList[i]->dwProcessID);
	}

	Logger_Info(TEXT("守护线程退出[dwCurrentThreadId:%u]"), GetCurrentThreadId());
	return true;
}

//运行函数
bool CProcessWatchKernel::OnEventThreadRun()
{
	//遍历进程
	for (size_t i = 0; i < m_ProcessInfoPtrList.size(); i++)
	{
		//查找进程
		PROCESS_INFO * pProcessInfo = m_ProcessInfoPtrList[i];
		if (CNetworkModule::FindProcess(pProcessInfo->dwProcessID) == false)
		{
			Logger_Info(TEXT("找不到进程=>进程名称:%s-[%u], 准备重新启动"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
			//启动进程
			pProcessInfo->dwProcessID = CNetworkModule::StartProcess(pProcessInfo->szProcessPath, pProcessInfo->szConsoleTitle, pProcessInfo->szCommandLine, pProcessInfo->bCreateNewWindow, pProcessInfo->bShowWindow);
			if (pProcessInfo->dwProcessID == 0)
			{
				Logger_Info(TEXT("重启进程失败=>进程名称:%s"), pProcessInfo->szConsoleTitle);
			}
			else
			{
				Logger_Info(TEXT("重启进程成功=>进程名称:%s-[%u]"), pProcessInfo->szConsoleTitle, pProcessInfo->dwProcessID);
			}

			//延时5秒
			Sleep(5000);
		}
	}

	//检测间隔
	Sleep(1000);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CProcessWatchInstance::CProcessWatchInstance()
{
}

//析构函数
CProcessWatchInstance::~CProcessWatchInstance()
{
}

//启动服务
bool CProcessWatchInstance::StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent)
{
	//添加对象
	//m_ProcessWatchKernel.AddProcessInfo();

	//启动服务
	m_ProcessWatchKernel.StartKernel();

	return true;
}

//停止服务
bool CProcessWatchInstance::StopServer()
{
	//停止服务
	m_ProcessWatchKernel.StopKernel();

	return true;
}

//////////////////////////////////////////////////////////////////////////
