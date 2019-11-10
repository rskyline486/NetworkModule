#include "stdafx.h"
#include "TestAttemperServer.h"
#include <time.h>

CTestAttemperServer::CTestAttemperServer()
{
	m_pAttemperService = NULL;
}

CTestAttemperServer::~CTestAttemperServer()
{
}

//��������
bool CTestAttemperServer::StartServer()
{
	//����ģ��
	CNetworkModule::StartupNetwork();

	//��������
	if (CNetworkModule::CreateService(&m_pAttemperService) == false) return false;

	//�������
	TCHAR szHeartbeat[128] = TEXT("");
	_sntprintf_s(szHeartbeat, CountArray(szHeartbeat), TEXT("������������"));

	//���ò���
	tagServerOption ServerOption;
	ServerOption.wListenPort = 9999;
	ServerOption.wThreadCount = 4;
	ServerOption.dwDetectTime = 10000;
	ServerOption.wProtocol = 0;
	ServerOption.wMaxAcceptCount = 1;
	ServerOption.wMaxSocketCount = 512;
	ServerOption.pHeartbeatData = szHeartbeat;
	ServerOption.wHeartbeatDataSize = CountStringBuffer(szHeartbeat);

	//���ò���
	tagClientOption ClientOption;
	ClientOption.wThreadCount = 4;
	ClientOption.dwDetectTime = 10000;
	ClientOption.wMaxConnectCount = 512;

	//���Ȳ���
	tagAttemperOption AttemperOption;
	ZeroMemory(&AttemperOption, sizeof(AttemperOption));
	AttemperOption.pServerOption = &ServerOption;
	//AttemperOption.pClientOption = &ClientOption;

	//��������
	return m_pAttemperService->StartServer(AttemperOption, this);
}

//ֹͣ����
bool CTestAttemperServer::StopServer()
{
	//ֹͣ����
	if (m_pAttemperService)
	{
		m_pAttemperService->StopServer();
		CNetworkModule::DestroyService(&m_pAttemperService);
	}

	//����ģ��
	CNetworkModule::CleanupNetwork();

	return true;
}

//��������
bool CTestAttemperServer::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_pAttemperService->SendData(dwSocketID, 100, 200, pData, wDataSize);
}

//Ⱥ������
bool CTestAttemperServer::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_pAttemperService->SendDataBatch(200, 800, pData, wDataSize);
}

//ʱ���¼�
bool CTestAttemperServer::OnAttemperTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	_tprintf(TEXT("ʱ���¼�=>ʱ���ʶ:%u, ������:%u\n"), dwTimerID, dwBindParameter);
	return true;
}

//�Զ��¼�
bool CTestAttemperServer::OnAttemperCustom(DWORD dwCustomID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("�Զ��¼�=>�Զ���ʶ:%u, ���ݴ�С:%u\n"), dwCustomID, wDataSize);
	return true;
}

//�Զ��¼�
bool CTestAttemperServer::OnAttemperCustomEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("�Զ��¼�=>�Զ���ʶ:%u, ���ݱ�ʶ:%u, ���ݴ�С:%u\n"), dwCustomID, dwContextID, wDataSize);
	return true;
}

//���Ӱ�
bool CTestAttemperServer::OnAttemperSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("���Ӱ�=>���ӱ�ʶ:%u, �û�����:%u\n"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("��������=>test data"));
	SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));
	return true;
}

//���ӹر�
bool CTestAttemperServer::OnAttemperSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("���ӹر�=>���ӱ�ʶ:%u, �û�����:%u\n"), dwSocketID, dwUserData);
	return true;
}

//���Ӷ�ȡ
bool CTestAttemperServer::OnAttemperSocketRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwUserData)
{
	_tprintf(TEXT("���Ӷ�ȡ=>���ӱ�ʶ:%u, �û�����:%u, ����Ϣ��:%u, ����Ϣ��:%u, ���ݴ�С:%u\n"), dwSocketID, dwUserData, wMainCmdID, wSubCmdID, wDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return true;
}

//�����
bool CTestAttemperServer::OnAttemperNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort)
{
	_tprintf(TEXT("�����=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d\n"), dwSocketID, pszClientIP, wClientPort);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("��������=>The server has received your connection, ����IP��ַΪ:%s, �˿�Ϊ:%d"), pszClientIP, wClientPort);
	SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	return true;
}

//����ر�
bool CTestAttemperServer::OnAttemperNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime)
{
	_tprintf(TEXT("����ر�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, ����ʱ��:%u, ����ʱ��:%u��\n"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime));
	return true;
}

//�����ȡ
bool CTestAttemperServer::OnAttemperNetworkRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("�����ȡ=>���ӱ�ʶ:%u, ����Ϣ��:%u, ����Ϣ��:%u, ���ݴ�С:%u\n"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return true;
}

//���з���
#include <iostream>
int CTestAttemperServer::Run(int argc, TCHAR* argv[])
{
	//��������
	CTestAttemperServer TestAttemperServer;
	if (TestAttemperServer.StartServer() == true)
	{
		//��������
		TCHAR szTestData[1024] = TEXT("");
		_sntprintf_s(szTestData, CountArray(szTestData), TEXT("���������=>��������cesgagd154545��˹�ٷ��͵���asdf1545asdf45a4sd5f"));

		//���з���
		char ch = 0;
		while (ch != 'x' && ch != 'X')
		{
			ch = std::cin.get();
			//Ⱥ������
			TestAttemperServer.SendDataBatch(szTestData, CountStringBuffer(szTestData));
		}
	}

	//ֹͣ����
	TestAttemperServer.StopServer();

	//��������
	system("pause");

	return 0;
}