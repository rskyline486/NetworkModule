#include "stdafx.h"
#include "Logger.h"
#include <locale.h>
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////

#pragma warning(disable:4996)

//////////////////////////////////////////////////////////////////////////

//日志级别辅助类
class CLoggerLevelHelper
{
public:
	//构造函数
	CLoggerLevelHelper(enLoggerLevel LoggerLevel)
	{
		//获取控制台输出句柄
		m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_hConsole != INVALID_HANDLE_VALUE)
		{
			//获取当前配置信息
			if (!GetConsoleScreenBufferInfo(m_hConsole, &m_oldInfo))
			{
				m_hConsole = INVALID_HANDLE_VALUE;
			}
		}

		if (m_hConsole != INVALID_HANDLE_VALUE)
		{
			//设置颜色
			SetConsoleTextAttribute(m_hConsole, GetConsoleTextColor(LoggerLevel));
		}
	}

	//析构函数
	~CLoggerLevelHelper()
	{
		if (m_hConsole != INVALID_HANDLE_VALUE)
		{
			//恢复配置
			SetConsoleTextAttribute(m_hConsole, m_oldInfo.wAttributes);
		}
	}

protected:
	//颜色信息
	WORD GetConsoleTextColor(enLoggerLevel LoggerLevel)
	{
		switch (LoggerLevel)
		{
		case LoggerLevel_Trace:
			return (FOREGROUND_INTENSITY);
			break;
		case LoggerLevel_Debug:
			return (FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case LoggerLevel_Info:
			//return (FOREGROUND_GREEN | FOREGROUND_RED);
			return (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			break;
		case LoggerLevel_Warn:
			return (FOREGROUND_RED | FOREGROUND_INTENSITY);
			break;
		case LoggerLevel_Error:
			return (FOREGROUND_RED);
			break;
		case LoggerLevel_Fatal:
			return (FOREGROUND_BLUE);
			break;
		case LoggerLevel_Count:
			break;
		default:
			break;
		}

		return (FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
	}

private:
	HANDLE							m_hConsole;
	CONSOLE_SCREEN_BUFFER_INFO		m_oldInfo;
};

//////////////////////////////////////////////////////////////////////////

//构造函数
CLogger::CLogger()
{
	//设置语言环境
	_tsetlocale(LC_ALL, TEXT("chs"));
	//初始化临界区
	InitializeCriticalSection(&m_Mutex);
	//申请缓冲
	m_dwOutputBufferSize = 102400 * sizeof(TCHAR);
	m_pszOutputBuffer = reinterpret_cast<TCHAR*>(::malloc(m_dwOutputBufferSize));
	if (m_pszOutputBuffer == NULL)
	{
		//重新申请
		m_dwOutputBufferSize = 10240 * sizeof(TCHAR);
		m_pszOutputBuffer = reinterpret_cast<TCHAR*>(::malloc(m_dwOutputBufferSize));
		if (m_pszOutputBuffer == NULL)
		{
			m_dwOutputBufferSize = 0;
			_tprintf(TEXT("CLogger=>[构造函数]:申请缓冲失败\r\n"));
		}
	}
	m_pILogger = this;
	m_dwLoggerCount = 0;

	//获取文件路径以及文件名称
	ZeroMemory(m_szFilePath, sizeof(m_szFilePath));
	ZeroMemory(m_szFileName, sizeof(m_szFileName));
	GetModuleFileName(NULL, m_szFilePath, sizeof(m_szFilePath)/sizeof(TCHAR));
	for (INT i = lstrlen(m_szFilePath) - 1; i >= 0; i--)
	{
		if (m_szFilePath[i] == TEXT('\\'))
		{
			//计算长度(去掉反斜杠,去掉后缀)
			INT nNameCount = (lstrlen(m_szFilePath) - i) - 1 - 4;
			for (INT k = 0; k < nNameCount; k++) m_szFileName[k] = m_szFilePath[i + 1 + k];

			//附加路径
			m_szFilePath[i] = 0;
			lstrcat(m_szFilePath, TEXT("\\log\\"));
			break;
		}
	}
}

//析构函数
CLogger::~CLogger()
{
	//释放临界区
	DeleteCriticalSection(&m_Mutex);

	//释放缓冲
	if (m_pszOutputBuffer)
	{
		::free(m_pszOutputBuffer);
		m_pszOutputBuffer = NULL;
	}
	m_dwOutputBufferSize = 0;

	//重置变量
	m_pILogger = NULL;
	m_dwLoggerCount = 0;
	ZeroMemory(m_szFilePath, sizeof(m_szFilePath));
}

//设置名称
void CLogger::SetFileName(LPCTSTR pszFileName)
{
	//进入临界区
	EnterCriticalSection(&m_Mutex);

	//设置名称
	lstrcpyn(m_szFileName, pszFileName, (sizeof(m_szFileName) / sizeof(m_szFileName[0])));

	//离开临界区
	LeaveCriticalSection(&m_Mutex);
}

//设置接口
bool CLogger::SetLogger(ILogger * pILogger)
{
	//限定只能在程序初始化阶段设置接口(否则需要考虑加锁)
	if (m_dwLoggerCount != 0) return false;

	if (pILogger != NULL)
	{
		m_pILogger = pILogger;
	}
	else
	{
		m_pILogger = this;
	}

	return true;
}

//获取接口
ILogger * CLogger::GetLogger()
{
	return m_pILogger;
}

//打印日志
void CLogger::Logger(enLoggerLevel LoggerLevel, LPCTSTR pszFile, DWORD dwLine, LPCTSTR pszFormat, ...)
{
	//进入临界区
	EnterCriticalSection(&m_Mutex);

	//日志级别辅助类
	CLoggerLevelHelper LoggerLevelHelper(LoggerLevel);

	//解析参数
	va_list ap;
	va_start(ap, pszFormat);

	//打印日志
	if (m_pszOutputBuffer)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		//获取文件名
		LPCTSTR pszFileName = &pszFile[lstrlen(pszFile) - 1];
		while (pszFileName && (*pszFileName != TEXT('\\')) && (pszFileName >= pszFile)) pszFileName--; pszFileName++;
		_stprintf(m_pszOutputBuffer, TEXT("%02d:%02d:%02d|%05d:%05d|%s:%05u|"), st.wHour, st.wMinute, st.wSecond, GetCurrentProcessId(), GetCurrentThreadId(), pszFileName, dwLine);
		_vstprintf(&m_pszOutputBuffer[lstrlen(m_pszOutputBuffer)], pszFormat, ap);
		_tprintf(m_pszOutputBuffer); _tprintf(TEXT("\n"));

		//更新log文件名
		for (INT i = lstrlen(m_szFilePath) - 1; i >= 0; i--)
		{
			if (m_szFilePath[i] == TEXT('\\'))
			{
				//定位到目录
				m_szFilePath[i] = 0;

				//如果目录不存在
				if (_taccess(m_szFilePath, 0) != 0)
				{
					::CreateDirectory(m_szFilePath, NULL);
				}

				_stprintf(&m_szFilePath[i], TEXT("\\[%s][%4d-%02d-%02d]-[%05d].log"), m_szFileName, st.wYear, st.wMonth, st.wDay, GetCurrentProcessId());
				break;
			}
		}

		//记录LOG
		FILE * fp = _tfopen(m_szFilePath, TEXT("a"));
		if (fp)
		{
			_ftprintf(fp, m_pszOutputBuffer); _ftprintf(fp, TEXT("\n"));
			fflush(fp);
			fclose(fp);
		}
	}
	else
	{
		//打印日志信息到控制台
		_vtprintf(pszFormat, ap); _tprintf(TEXT("\n"));
	}

	//结束解析
	va_end(ap);

	//累加计数
	m_dwLoggerCount++;

	//离开临界区
	LeaveCriticalSection(&m_Mutex);
}

//////////////////////////////////////////////////////////////////////////
