#include "stdafx.h"
#include "TestServer.h"
#include <time.h>

CTestServer::CTestServer()
{
	m_pServerService = NULL;
}

CTestServer::~CTestServer()
{
}

//��������
bool CTestServer::StartServer()
{
	//����ģ��
	CNetworkModule::StartupNetwork();

	//��������
	if (CNetworkModule::CreateService(&m_pServerService) == false) return false;

	//�������
	TCHAR szHeartbeat[128] = TEXT("");
	_sntprintf_s(szHeartbeat, CountArray(szHeartbeat), TEXT("������������"));

	//���ò���
	tagServerOption ServerOption;
	ServerOption.wListenPort = 9999;
	ServerOption.wThreadCount = 4;
	ServerOption.dwDetectTime = 8000;
	ServerOption.wProtocol = 0;
	ServerOption.wMaxAcceptCount = 1;
	ServerOption.wMaxSocketCount = 512;
	ServerOption.pHeartbeatData = szHeartbeat;
	ServerOption.wHeartbeatDataSize = CountStringBuffer(szHeartbeat);

	//��������
	return m_pServerService->StartServer(ServerOption, this);
}

//ֹͣ����
bool CTestServer::StopServer()
{
	//ֹͣ����
	if (m_pServerService)
	{
		m_pServerService->StopServer();
		CNetworkModule::DestroyService(&m_pServerService);
	}

	//����ģ��
	CNetworkModule::CleanupNetwork();

	return true;
}

//��������
bool CTestServer::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_pServerService->SendData(dwSocketID, pData, wDataSize);
}

//Ⱥ������
bool CTestServer::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_pServerService->SendDataBatch( pData, wDataSize);
}

//���¼�
bool CTestServer::OnServerNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort)
{
	_tprintf(TEXT("���¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d\n"), dwSocketID, pszClientIP, wClientPort);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("��������=>The server has received your connection, ����IP��ַΪ:%s, �˿�Ϊ:%d"), pszClientIP, wClientPort);
	m_pServerService->SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	return true;
}

//�ر��¼�
bool CTestServer::OnServerNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime)
{
	_tprintf(TEXT("�ر��¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, ����ʱ��:%u, ����ʱ��:%u��\n"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime));
	return true;
}

//��ȡ�¼�
int CTestServer::OnServerNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize)
{
	_tprintf(TEXT("��ȡ�¼�=>���ӱ�ʶ:%u, ���ݴ�С:%u\n"), dwSocketID, dwDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return dwDataSize;
}

//���з���
#include <iostream>
int CTestServer::Run(int argc, TCHAR* argv[])
{
	//��������
	CTestServer TestServer;
	if (TestServer.StartServer() == true)
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
			TestServer.SendDataBatch(szTestData, CountStringBuffer(szTestData));
		}
	}

	//ֹͣ����
	TestServer.StopServer();

	//��������
	system("pause");

	return 0;
}