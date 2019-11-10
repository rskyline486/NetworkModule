#include "stdafx.h"
#include "Logger.h"
#include <locale.h>
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////

#pragma warning(disable:4996)

//////////////////////////////////////////////////////////////////////////

//��־��������
class CLoggerLevelHelper
{
public:
	//���캯��
	CLoggerLevelHelper(enLoggerLevel LoggerLevel)
	{
		//��ȡ����̨������
		m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		if (m_hConsole != INVALID_HANDLE_VALUE)
		{
			//��ȡ��ǰ������Ϣ
			if (!GetConsoleScreenBufferInfo(m_hConsole, &m_oldInfo))
			{
				m_hConsole = INVALID_HANDLE_VALUE;
			}
		}

		if (m_hConsole != INVALID_HANDLE_VALUE)
		{
			//������ɫ
			SetConsoleTextAttribute(m_hConsole, GetConsoleTextColor(LoggerLevel));
		}
	}

	//��������
	~CLoggerLevelHelper()
	{
		if (m_hConsole != INVALID_HANDLE_VALUE)
		{
			//�ָ�����
			SetConsoleTextAttribute(m_hConsole, m_oldInfo.wAttributes);
		}
	}

protected:
	//��ɫ��Ϣ
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

//���캯��
CLogger::CLogger()
{
	//�������Ի���
	_tsetlocale(LC_ALL, TEXT("chs"));
	//��ʼ���ٽ���
	InitializeCriticalSection(&m_Mutex);
	//���뻺��
	m_dwOutputBufferSize = 102400 * sizeof(TCHAR);
	m_pszOutputBuffer = reinterpret_cast<TCHAR*>(::malloc(m_dwOutputBufferSize));
	if (m_pszOutputBuffer == NULL)
	{
		//��������
		m_dwOutputBufferSize = 10240 * sizeof(TCHAR);
		m_pszOutputBuffer = reinterpret_cast<TCHAR*>(::malloc(m_dwOutputBufferSize));
		if (m_pszOutputBuffer == NULL)
		{
			m_dwOutputBufferSize = 0;
			_tprintf(TEXT("CLogger=>[���캯��]:���뻺��ʧ��\r\n"));
		}
	}
	m_pILogger = this;
	m_dwLoggerCount = 0;

	//��ȡ�ļ�·���Լ��ļ�����
	ZeroMemory(m_szFilePath, sizeof(m_szFilePath));
	ZeroMemory(m_szFileName, sizeof(m_szFileName));
	GetModuleFileName(NULL, m_szFilePath, sizeof(m_szFilePath)/sizeof(TCHAR));
	for (INT i = lstrlen(m_szFilePath) - 1; i >= 0; i--)
	{
		if (m_szFilePath[i] == TEXT('\\'))
		{
			//���㳤��(ȥ����б��,ȥ����׺)
			INT nNameCount = (lstrlen(m_szFilePath) - i) - 1 - 4;
			for (INT k = 0; k < nNameCount; k++) m_szFileName[k] = m_szFilePath[i + 1 + k];

			//����·��
			m_szFilePath[i] = 0;
			lstrcat(m_szFilePath, TEXT("\\log\\"));
			break;
		}
	}
}

//��������
CLogger::~CLogger()
{
	//�ͷ��ٽ���
	DeleteCriticalSection(&m_Mutex);

	//�ͷŻ���
	if (m_pszOutputBuffer)
	{
		::free(m_pszOutputBuffer);
		m_pszOutputBuffer = NULL;
	}
	m_dwOutputBufferSize = 0;

	//���ñ���
	m_pILogger = NULL;
	m_dwLoggerCount = 0;
	ZeroMemory(m_szFilePath, sizeof(m_szFilePath));
}

//��������
void CLogger::SetFileName(LPCTSTR pszFileName)
{
	//�����ٽ���
	EnterCriticalSection(&m_Mutex);

	//��������
	lstrcpyn(m_szFileName, pszFileName, (sizeof(m_szFileName) / sizeof(m_szFileName[0])));

	//�뿪�ٽ���
	LeaveCriticalSection(&m_Mutex);
}

//���ýӿ�
bool CLogger::SetLogger(ILogger * pILogger)
{
	//�޶�ֻ���ڳ����ʼ���׶����ýӿ�(������Ҫ���Ǽ���)
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

//��ȡ�ӿ�
ILogger * CLogger::GetLogger()
{
	return m_pILogger;
}

//��ӡ��־
void CLogger::Logger(enLoggerLevel LoggerLevel, LPCTSTR pszFile, DWORD dwLine, LPCTSTR pszFormat, ...)
{
	//�����ٽ���
	EnterCriticalSection(&m_Mutex);

	//��־��������
	CLoggerLevelHelper LoggerLevelHelper(LoggerLevel);

	//��������
	va_list ap;
	va_start(ap, pszFormat);

	//��ӡ��־
	if (m_pszOutputBuffer)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);

		//��ȡ�ļ���
		LPCTSTR pszFileName = &pszFile[lstrlen(pszFile) - 1];
		while (pszFileName && (*pszFileName != TEXT('\\')) && (pszFileName >= pszFile)) pszFileName--; pszFileName++;
		_stprintf(m_pszOutputBuffer, TEXT("%02d:%02d:%02d|%05d:%05d|%s:%05u|"), st.wHour, st.wMinute, st.wSecond, GetCurrentProcessId(), GetCurrentThreadId(), pszFileName, dwLine);
		_vstprintf(&m_pszOutputBuffer[lstrlen(m_pszOutputBuffer)], pszFormat, ap);
		_tprintf(m_pszOutputBuffer); _tprintf(TEXT("\n"));

		//����log�ļ���
		for (INT i = lstrlen(m_szFilePath) - 1; i >= 0; i--)
		{
			if (m_szFilePath[i] == TEXT('\\'))
			{
				//��λ��Ŀ¼
				m_szFilePath[i] = 0;

				//���Ŀ¼������
				if (_taccess(m_szFilePath, 0) != 0)
				{
					::CreateDirectory(m_szFilePath, NULL);
				}

				_stprintf(&m_szFilePath[i], TEXT("\\[%s][%4d-%02d-%02d]-[%05d].log"), m_szFileName, st.wYear, st.wMonth, st.wDay, GetCurrentProcessId());
				break;
			}
		}

		//��¼LOG
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
		//��ӡ��־��Ϣ������̨
		_vtprintf(pszFormat, ap); _tprintf(TEXT("\n"));
	}

	//��������
	va_end(ap);

	//�ۼӼ���
	m_dwLoggerCount++;

	//�뿪�ٽ���
	LeaveCriticalSection(&m_Mutex);
}

//////////////////////////////////////////////////////////////////////////
