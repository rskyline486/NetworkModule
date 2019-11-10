#include "stdafx.h"
#include "TestAttemperClient.h"
#include <time.h>

CTestAttemperClient::CTestAttemperClient()
{
	m_pAttemperService = NULL;
}

CTestAttemperClient::~CTestAttemperClient()
{
}

//��������
bool CTestAttemperClient::StartClient()
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
	//AttemperOption.pServerOption = &ServerOption;
	AttemperOption.pClientOption = &ClientOption;

	//��������
	return m_pAttemperService->StartServer(AttemperOption, this);
}

//ֹͣ����
bool CTestAttemperClient::StopClient()
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

//���ӷ���
bool CTestAttemperClient::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//������ʱ��
	m_pAttemperService->SetTimer(5555, 8 * 1000, 10, 8888);
	return m_pAttemperService->ConnectServer(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//��������
bool CTestAttemperClient::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_pAttemperService->SendData(dwSocketID, 100, 200, pData, wDataSize);
}

//Ⱥ������
bool CTestAttemperClient::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_pAttemperService->PostCustomEvent(22, pData, wDataSize);
	//return m_pAttemperService->SendDataBatch(200, 800, pData, wDataSize);
}

//ʱ���¼�
bool CTestAttemperClient::OnAttemperTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	_tprintf(TEXT("ʱ���¼�=>ʱ���ʶ:%u, ������:%u\n"), dwTimerID, dwBindParameter);
	//��������
	TCHAR szTestData[1024] = TEXT("");
	_sntprintf_s(szTestData, CountArray(szTestData), TEXT("��ʱ����Ϣ"));
	m_pAttemperService->SendDataBatch(55, 800, szTestData, CountStringBuffer(szTestData));
	return true;
}

//�Զ��¼�
bool CTestAttemperClient::OnAttemperCustom(DWORD dwCustomID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("�Զ��¼�=>�Զ���ʶ:%u, ���ݴ�С:%u\n"), dwCustomID, wDataSize);
	m_pAttemperService->SendDataBatch(22, 800, pData, wDataSize);
	return true;
}

//�Զ��¼�
bool CTestAttemperClient::OnAttemperCustomEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("�Զ��¼�=>�Զ���ʶ:%u, ���ݱ�ʶ:%u, ���ݴ�С:%u\n"), dwCustomID, dwContextID, wDataSize);
	return true;
}

//���Ӱ�
bool CTestAttemperClient::OnAttemperSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("���Ӱ�=>���ӱ�ʶ:%u, �û�����:%u\n"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("��������=>test data"));
	SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));
	return true;
}

//���ӹر�
bool CTestAttemperClient::OnAttemperSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("���ӹر�=>���ӱ�ʶ:%u, �û�����:%u\n"), dwSocketID, dwUserData);
	return true;
}

//���Ӷ�ȡ
bool CTestAttemperClient::OnAttemperSocketRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwUserData)
{
	_tprintf(TEXT("���Ӷ�ȡ=>���ӱ�ʶ:%u, �û�����:%u, ����Ϣ��:%u, ����Ϣ��:%u, ���ݴ�С:%u\n"), dwSocketID, dwUserData, wMainCmdID, wSubCmdID, wDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return true;
}

//�����
bool CTestAttemperClient::OnAttemperNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort)
{
	_tprintf(TEXT("�����=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d\n"), dwSocketID, pszClientIP, wClientPort);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("��������=>The server has received your connection, ����IP��ַΪ:%s, �˿�Ϊ:%d"), pszClientIP, wClientPort);
	SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	return true;
}

//����ر�
bool CTestAttemperClient::OnAttemperNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime)
{
	_tprintf(TEXT("����ر�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, ����ʱ��:%u, ����ʱ��:%u��\n"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime));
	return true;
}

//�����ȡ
bool CTestAttemperClient::OnAttemperNetworkRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	_tprintf(TEXT("�����ȡ=>���ӱ�ʶ:%u, ����Ϣ��:%u, ����Ϣ��:%u, ���ݴ�С:%u\n"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return true;
}

//���з���
#include <iostream>
int CTestAttemperClient::Run(int argc, TCHAR* argv[])
{
	//��������
	CTestAttemperClient TestAttemperClient;
	if (TestAttemperClient.StartClient() == true)
	{
		//���ӷ���
		TestAttemperClient.ConnectServer(TEXT("127.0.0.1"), 9999, 1234, 0, true);

		//��������
		TCHAR szTestData[1024] = TEXT("");
		_sntprintf_s(szTestData, CountArray(szTestData), TEXT("�ͻ�������=>��������cesgagd154545��˹�ٷ��͵���asdf1545asdf45a4sd5f"));

		//���з���
		char ch = 0;
		while (ch != 'x' && ch != 'X')
		{
			ch = std::cin.get();
			//Ⱥ������
			TestAttemperClient.SendDataBatch(szTestData, CountStringBuffer(szTestData));
		}
	}

	//ֹͣ����
	TestAttemperClient.StopClient();

	//��������
	system("pause");

	return 0;
}