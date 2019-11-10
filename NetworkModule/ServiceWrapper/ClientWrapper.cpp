#include "stdafx.h"
#include "ClientWrapper.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CClientKernel::CClientKernel()
{
}

//��������
CClientKernel::~CClientKernel()
{
}

//�����ں�
bool CClientKernel::StartKernel()
{
	//��ʼ�����ӷ���
	if (m_Client.Init(this, 4, 10000L, 512) == false) return false;

	return true;
}

//ֹͣ�ں�
bool CClientKernel::StopKernel()
{
	//ֹͣ����
	m_Client.Release();

	return true;
}

//�����¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CClientKernel::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect�����¼�=>���ӱ�ʶ:%u, �û�����:%u"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("��������=>test data"));
	m_Client.SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));

	return true;
}

//�ر��¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CClientKernel::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect�ر��¼�=>���ӱ�ʶ:%u, �û�����:%u"), dwSocketID, dwUserData);

	return true;
}

//��ȡ�¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
int CClientKernel::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect��ȡ�¼�=>���ӱ�ʶ:%u, �û�����:%u, ���ݴ�С:%u"), dwSocketID, dwUserData, dwDataSize);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);

	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CClientInstance::CClientInstance()
{
	m_pClientEvent = NULL;
}

//��������
CClientInstance::~CClientInstance()
{
}

//��������
bool CClientInstance::StartServer(tagClientOption ClientOption, IClientEvent * pClientEvent)
{
	//����У��
	//if (pServerEvent == NULL) return false;

	//��ʼ�����ӷ���
	if (m_Client.Init(this, ClientOption.wThreadCount, ClientOption.dwDetectTime, ClientOption.wMaxConnectCount) == false) return false;

	//���ýӿ�
	m_pClientEvent = pClientEvent;

	return true;
}

//ֹͣ����
bool CClientInstance::StopServer()
{
	//ֹͣ����
	__super::StopKernel();

	//���ýӿ�
	m_pClientEvent = NULL;

	return true;
}

//���ӷ���
bool CClientInstance::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return (m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch) != INVALID_SOCKETID);
}

//��������
bool CClientInstance::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_Client.SendData(dwSocketID, pData, wDataSize);
}

//��������
bool CClientInstance::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_Client.SendDataBatch(pData, wDataSize);
}

//�ر�����
bool CClientInstance::CloseSocket(DWORD dwSocketID)
{
	return m_Client.CloseSocket(dwSocketID);
}

//�����¼�
bool CClientInstance::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//�ص�����
	if (m_pClientEvent)
	{
		return m_pClientEvent->OnClientSocketLink(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketLink(dwSocketID, dwUserData);
}

//�ر��¼�
bool CClientInstance::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	//�ص�����
	if (m_pClientEvent)
	{
		return m_pClientEvent->OnClientSocketShut(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketShut(dwSocketID, dwUserData);
}

//��ȡ�¼�
int CClientInstance::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//�ص�����
	if (m_pClientEvent)
	{
		return m_pClientEvent->OnClientSocketRead(dwSocketID, pData, dwDataSize, dwUserData);
	}

	return __super::OnEventSocketRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//////////////////////////////////////////////////////////////////////////
