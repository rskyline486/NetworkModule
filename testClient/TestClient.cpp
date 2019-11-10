#include "stdafx.h"
#include "TestClient.h"

CTestClient::CTestClient()
{
	m_pClientService = NULL;
}

CTestClient::~CTestClient()
{
}

//��������
bool CTestClient::StartClient()
{
	//����ģ��
	CNetworkModule::StartupNetwork();

	//��������
	if (CNetworkModule::CreateService(&m_pClientService) == false) return false;

	//���ò���
	tagClientOption ClientOption;
	ClientOption.wThreadCount = 4;
	ClientOption.dwDetectTime = 8000;
	ClientOption.wMaxConnectCount = 512;

	//��������
	return m_pClientService->StartServer(ClientOption, this);
}

//ֹͣ����
bool CTestClient::StopClient()
{
	//ֹͣ����
	if (m_pClientService)
	{
		m_pClientService->StopServer();
		CNetworkModule::DestroyService(&m_pClientService);
	}

	//����ģ��
	CNetworkModule::CleanupNetwork();

	return true;
}

//���ӷ���
bool CTestClient::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return m_pClientService->ConnectServer(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//��������
bool CTestClient::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_pClientService->SendData(dwSocketID, pData, wDataSize);
}

//Ⱥ������
bool CTestClient::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_pClientService->SendDataBatch(pData, wDataSize);
}

//�����¼�
bool CTestClient::OnClientSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("�����¼�=>���ӱ�ʶ:%u, �û�����:%u\n"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("��������=>test data"));
	m_pClientService->SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));
	return true;
}

//�ر��¼�
bool CTestClient::OnClientSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	_tprintf(TEXT("�ر��¼�=>���ӱ�ʶ:%u, �û�����:%u\n"), dwSocketID, dwUserData);
	return true;
}

//��ȡ�¼�
int CTestClient::OnClientSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	_tprintf(TEXT("��ȡ�¼�=>���ӱ�ʶ:%u, �û�����:%u, ���ݴ�С:%u\n"), dwSocketID, dwUserData, dwDataSize);
	_tprintf(TEXT("%s\n"), (TCHAR*)pData);
	return dwDataSize;
}

//���з���
#include <iostream>
int CTestClient::Run(int argc, TCHAR* argv[])
{
	//��������
	CTestClient TestClient;
	if (TestClient.StartClient() == true)
	{
		//���ӷ���
		TestClient.ConnectServer(TEXT("192.168.29.224"), 9999, 1234, 0, true);

		//��������
		TCHAR szTestData[1024] = TEXT("");
		_sntprintf_s(szTestData, CountArray(szTestData), TEXT("�ͻ�������=>��������cesgagd154545��˹�ٷ��͵���asdf1545asdf45a4sd5f"));

		//���з���
		char ch = 0;
		while (ch != 'x' && ch != 'X')
		{
			ch = std::cin.get();
			//Ⱥ������
			TestClient.SendDataBatch(szTestData, CountStringBuffer(szTestData));
		}
	}

	//ֹͣ����
	TestClient.StopClient();

	//��������
	system("pause");

	return 0;
}