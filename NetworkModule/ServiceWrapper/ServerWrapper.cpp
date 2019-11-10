#include "stdafx.h"
#include "ServerWrapper.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CServerKernel::CServerKernel()
{
}

//��������
CServerKernel::~CServerKernel()
{
}

//�����ں�
bool CServerKernel::StartKernel()
{
	//��ʼ����������
	if (m_Server.Init(this, 4, 10000L, 1, 512) == false) return false;
	//�����˿�
	if (m_Server.Listen(9999, 0L, EnableIPv4, true) == false) return false;

	return true;
}

//ֹͣ�ں�
bool CServerKernel::StopKernel()
{
	//ֹͣ����
	m_Server.Release();

	return true;
}

//���¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CServerKernel::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket���¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, �û�����:%u"), dwSocketID, pszClientIP, wClientPort, dwUserData);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("��������=>The server has received your connection, ����IP��ַΪ:%s, �˿�Ϊ:%d"), pszClientIP, wClientPort);
	m_Server.SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	
	return true;
}

//�ر��¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CServerKernel::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket�ر��¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, ����ʱ��:%u, ����ʱ��:%u��, �û�����:%u"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime), dwUserData);
	
	return true;
}

//��ȡ�¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
int CServerKernel::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket��ȡ�¼�=>���ӱ�ʶ:%u, ���ݴ�С:%u, �û�����:%u"), dwSocketID, dwDataSize, dwUserData);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);

	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CServerInstance::CServerInstance()
{
	m_pServerEvent = NULL;
}

//��������
CServerInstance::~CServerInstance()
{
}

//��������
bool CServerInstance::StartServer(tagServerOption ServerOption, IServerEvent * pServerEvent)
{
	//����У��
	//if (pServerEvent == NULL) return false;

	//��ʶ��Ϣ
	DWORD dwUserData = ServerOption.wListenPort;
	//�ػ���ʶ
	bool bWatch = true;

	//��ʼ����������
	if (m_Server.Init(this, ServerOption.wThreadCount, ServerOption.dwDetectTime, ServerOption.wMaxAcceptCount, ServerOption.wMaxSocketCount) == false) return false;
	//������������
	if (ServerOption.pHeartbeatData != NULL && ServerOption.wHeartbeatDataSize > 0)
	{
		if (m_Server.SetHeartbeatPacket(ServerOption.pHeartbeatData, ServerOption.wHeartbeatDataSize) == false) return false;
	}
	//�����˿�
	if (m_Server.Listen(ServerOption.wListenPort, dwUserData, ServerOption.wProtocol, bWatch) == false) return false;

	//���ýӿ�
	m_pServerEvent = pServerEvent;

	return true;
}

//ֹͣ����
bool CServerInstance::StopServer()
{
	//ֹͣ����
	__super::StopKernel();

	//���ýӿ�
	m_pServerEvent = NULL;

	return true;
}

//��������
bool CServerInstance::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	return m_Server.SendData(dwSocketID, pData, wDataSize);
}

//��������
bool CServerInstance::SendDataBatch(VOID * pData, WORD wDataSize)
{
	return m_Server.SendDataBatch(pData, wDataSize);
}

//�ر�����
bool CServerInstance::CloseSocket(DWORD dwSocketID)
{
	return m_Server.CloseSocket(dwSocketID);
}

//���¼�
bool CServerInstance::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	//�ص�����
	if (m_pServerEvent)
	{
		return m_pServerEvent->OnServerNetworkBind(dwSocketID, pszClientIP, wClientPort);
	}

	return __super::OnEventNetworkBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
}

//�ر��¼�
bool CServerInstance::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	//�ص�����
	if (m_pServerEvent)
	{
		return m_pServerEvent->OnServerNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime);
	}

	return __super::OnEventNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
}

//��ȡ�¼�
int CServerInstance::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//�ص�����
	if (m_pServerEvent)
	{
		return m_pServerEvent->OnServerNetworkRead(dwSocketID, pData, dwDataSize);
	}

	return __super::OnEventNetworkRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//////////////////////////////////////////////////////////////////////////
