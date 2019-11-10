#pragma once

//////////////////////////////////////////////////////////////////////////

#include "Singleton.h"

//////////////////////////////////////////////////////////////////////////

//日志级别
enum enLoggerLevel
{
	LoggerLevel_Trace,				//打印信息
	LoggerLevel_Debug,				//调试信息
	LoggerLevel_Info,				//提示信息
	LoggerLevel_Warn,				//警告信息
	LoggerLevel_Error,				//错误信息
	LoggerLevel_Fatal,				//严重信息
	LoggerLevel_Count				//类型数量
};

//////////////////////////////////////////////////////////////////////////

//日志接口
class ILogger
{
public:
	virtual void Logger(enLoggerLevel LoggerLevel, LPCTSTR pszFile, DWORD dwLine, LPCTSTR pszFormat, ...) = 0;
};

//////////////////////////////////////////////////////////////////////////

//日志对象
class CLogger : public ILogger
{
public:
	CLogger();
	virtual ~CLogger();

public:
	//设置名称
	void SetFileName(LPCTSTR pszFileName);
	//设置接口
	bool SetLogger(ILogger * pILogger);

public:
	//获取接口
	ILogger * GetLogger();

public:
	//打印日志
	virtual void Logger(enLoggerLevel LoggerLevel, LPCTSTR pszFile, DWORD dwLine, LPCTSTR pszFormat, ...);

private:
	CRITICAL_SECTION				m_Mutex;							//临界对象
	DWORD							m_dwOutputBufferSize;				//缓冲大小
	TCHAR *							m_pszOutputBuffer;					//输出缓冲
	ILogger *						m_pILogger;							//日志接口
	DWORD							m_dwLoggerCount;					//日志计数
	TCHAR							m_szFilePath[MAX_PATH];				//文件路径
	TCHAR							m_szFileName[MAX_PATH];				//文件名称
};

//////////////////////////////////////////////////////////////////////////

//参数设置
#define Logger_SetFileName(NAME)	INSTANCE(CLogger).SetFileName(NAME)
#define Logger_SetLogger(LOGCLASS)	INSTANCE(CLogger).SetLogger(LOGCLASS)
//打印日志
#define Logger_Trace(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Trace, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Debug(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Debug, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Info(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Info,  TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Warn(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Warn,  TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Error(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Error, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Fatal(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Fatal, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////