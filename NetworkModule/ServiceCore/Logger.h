#pragma once

//////////////////////////////////////////////////////////////////////////

#include "Singleton.h"

//////////////////////////////////////////////////////////////////////////

//��־����
enum enLoggerLevel
{
	LoggerLevel_Trace,				//��ӡ��Ϣ
	LoggerLevel_Debug,				//������Ϣ
	LoggerLevel_Info,				//��ʾ��Ϣ
	LoggerLevel_Warn,				//������Ϣ
	LoggerLevel_Error,				//������Ϣ
	LoggerLevel_Fatal,				//������Ϣ
	LoggerLevel_Count				//��������
};

//////////////////////////////////////////////////////////////////////////

//��־�ӿ�
class ILogger
{
public:
	virtual void Logger(enLoggerLevel LoggerLevel, LPCTSTR pszFile, DWORD dwLine, LPCTSTR pszFormat, ...) = 0;
};

//////////////////////////////////////////////////////////////////////////

//��־����
class CLogger : public ILogger
{
public:
	CLogger();
	virtual ~CLogger();

public:
	//��������
	void SetFileName(LPCTSTR pszFileName);
	//���ýӿ�
	bool SetLogger(ILogger * pILogger);

public:
	//��ȡ�ӿ�
	ILogger * GetLogger();

public:
	//��ӡ��־
	virtual void Logger(enLoggerLevel LoggerLevel, LPCTSTR pszFile, DWORD dwLine, LPCTSTR pszFormat, ...);

private:
	CRITICAL_SECTION				m_Mutex;							//�ٽ����
	DWORD							m_dwOutputBufferSize;				//�����С
	TCHAR *							m_pszOutputBuffer;					//�������
	ILogger *						m_pILogger;							//��־�ӿ�
	DWORD							m_dwLoggerCount;					//��־����
	TCHAR							m_szFilePath[MAX_PATH];				//�ļ�·��
	TCHAR							m_szFileName[MAX_PATH];				//�ļ�����
};

//////////////////////////////////////////////////////////////////////////

//��������
#define Logger_SetFileName(NAME)	INSTANCE(CLogger).SetFileName(NAME)
#define Logger_SetLogger(LOGCLASS)	INSTANCE(CLogger).SetLogger(LOGCLASS)
//��ӡ��־
#define Logger_Trace(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Trace, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Debug(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Debug, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Info(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Info,  TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Warn(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Warn,  TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Error(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Error, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)
#define Logger_Fatal(FRM, ...)		INSTANCE(CLogger).GetLogger()->Logger(LoggerLevel_Fatal, TEXT(__FILE__), __LINE__, FRM, __VA_ARGS__)

//////////////////////////////////////////////////////////////////////////