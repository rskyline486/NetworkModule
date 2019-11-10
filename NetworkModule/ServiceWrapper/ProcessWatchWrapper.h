#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//���̽ṹ
struct PROCESS_INFO
{
	DWORD							dwProcessID;						//����ID
	TCHAR							szProcessPath[256];					//����·��
	TCHAR							szConsoleTitle[64];					//���ڱ���
	TCHAR							szCommandLine[256];					//�������
	bool							bCreateNewWindow;					//��������
	bool							bShowWindow;						//��ʾ����
};

//////////////////////////////////////////////////////////////////////////

//�������
class CProcessWatchKernel : protected CThread
{
public:
	CProcessWatchKernel();
	virtual ~CProcessWatchKernel();

public:
	//�����ں�
	bool StartKernel();
	//ֹͣ�ں�
	bool StopKernel();

public:
	//��ӽ���
	bool AddProcessInfo(LPCTSTR pszProcessPath, LPCTSTR pszConsoleTitle, LPCTSTR pszCommandLine, bool bCreateNewWindow, bool bShowWindow);

protected:
	//��ʼ�¼�
	virtual bool OnEventThreadStrat();
	//�����¼�
	virtual bool OnEventThreadStop();
	//���к���
	virtual bool OnEventThreadRun();

protected:
	std::vector<PROCESS_INFO *>		m_ProcessInfoPtrList;				//���̲���
};

//////////////////////////////////////////////////////////////////////////

//����ʵ��
class CProcessWatchInstance
{
public:
	CProcessWatchInstance();
	virtual ~CProcessWatchInstance();

public:
	//��������
	virtual bool StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent);
	//ֹͣ����
	virtual bool StopServer();

private:
	CProcessWatchKernel				m_ProcessWatchKernel;				//�ػ�����
};

//////////////////////////////////////////////////////////////////////////
