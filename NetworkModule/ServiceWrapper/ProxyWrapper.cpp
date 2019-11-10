#include "stdafx.h"
#include "ProxyWrapper.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CProxyKernel::CProxyKernel()
{
	ZeroMemory(&m_szDefaultAddress, sizeof(m_szDefaultAddress));
	m_wDefaultPort = 0;
	m_wDefaultProtocol = 0;
}

//��������
CProxyKernel::~CProxyKernel()
{
}

//�����ں�
bool CProxyKernel::StartKernel(WORD wListenPort, LPCTSTR pszDefaultAddress, WORD wDefaultPort)
{
	//��ʼ����������
	if (m_Server.Init(this, 4, 10000L, 1, 512) == false) return false;
	//�����˿�
	if (m_Server.Listen(wListenPort, 0L, EnableIPv4, true) == false) return false;

	//��ʼ�����ӷ���
	if (m_Client.Init(this, 4, 10000L, 512) == false) return false;

	//���ò���
	lstrcpyn(m_szDefaultAddress, pszDefaultAddress, CountArray(m_szDefaultAddress));
	m_wDefaultPort = wDefaultPort;
	m_wDefaultProtocol = EnableIPv4;

	return true;
}

//ֹͣ�ں�
bool CProxyKernel::StopKernel()
{
	//ֹͣ����
	m_Server.Release();
	//ֹͣ����
	m_Client.Release();

	//���ò���
	ZeroMemory(&m_szDefaultAddress, sizeof(m_szDefaultAddress));
	m_wDefaultPort = 0;
	m_wDefaultProtocol = 0;

	return true;
}

//�����¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CProxyKernel::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect�����¼�=>���ӱ�ʶ:%u, �û�����:%u"), dwSocketID, dwUserData);
	
	//����һ�λ���
	m_Client.SendData(dwSocketID, NULL, 0);
	
	return true;
}

//�ر��¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CProxyKernel::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect�ر��¼�=>���ӱ�ʶ:%u, �û�����:%u"), dwSocketID, dwUserData);

	//�ر�����
	if (CHECK_SOCKETID(dwUserData))
	{
		//�첽�ر�
		m_Server.CloseSocket(dwUserData);
	}
	
	return true;
}

//��ȡ�¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
int CProxyKernel::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect��ȡ�¼�=>���ӱ�ʶ:%u, �û�����:%u, ���ݴ�С:%u"), dwSocketID, dwUserData, dwDataSize);
	
	//ת������
	if (CHECK_SOCKETID(dwUserData))
	{
		//��ΧУ��
		if (dwDataSize > SOCKET_BUFFER)
		{
			Logger_Error(TEXT("ת�����ݵ��ͻ���ʧ��,���ݴ�С�쳣,���ӱ�ʶ:%u, �û�����:%u, ���ݴ�С:%u"), dwSocketID, dwUserData, dwDataSize);
			return -1;
		}

		//ת������
		if (m_Server.SendData(dwUserData, pData, (WORD)dwDataSize) == true)
		{
			return dwDataSize;
		}
	}

	return -1;
}

//���¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CProxyKernel::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket���¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, �û�����:%u"), dwSocketID, pszClientIP, wClientPort, dwUserData);
	
	return true;
}

//�ر��¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
bool CProxyKernel::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket�ر��¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, ����ʱ��:%u, ����ʱ��:%u��, �û�����:%u"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime), dwUserData);
	
	//�ر�����
	if (CHECK_SOCKETID(dwUserData))
	{
		//�첽�ر�
		m_Client.CloseSocket(dwUserData);
	}

	return true;
}

//��ȡ�¼�(�˴��ص�Ϊ�����̻߳ص�,����ʹ��ͬ���ķ�ʽ�������������Ӷ���,���������������)
int CProxyKernel::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket��ȡ�¼�=>���ӱ�ʶ:%u, ���ݴ�С:%u, �û�����:%u"), dwSocketID, dwDataSize, dwUserData);
	
	//�����ж�
	if (dwUserData == INVALID_SOCKETID)
	{
		//���ӷ���
		DWORD dwConnectSocketID = m_Client.Connect(m_szDefaultAddress, m_wDefaultPort, dwSocketID, m_wDefaultProtocol, false);
		if (dwConnectSocketID == INVALID_SOCKETID)
		{
			Logger_Error(TEXT("���ӷ���ʧ��=>szServerAddr:%s, wServerPort:%u"), m_szDefaultAddress, m_wDefaultPort);
			return -1;
		}

		//�󶨾��(ͬ�������Լ�)
		m_Server.SetUserData(dwSocketID, dwConnectSocketID, true);
		//��������
		dwUserData = dwConnectSocketID;
	}

	//��ΧУ��
	if (dwDataSize > SOCKET_BUFFER)
	{
		Logger_Error(TEXT("ת�����ݵ�������ʧ��,���ݴ�С�쳣,���ӱ�ʶ:%u, �û�����:%u, ���ݴ�С:%u"), dwSocketID, dwUserData, dwDataSize);
		return -1;
	}

	//ת������
	if (m_Client.SendData(dwUserData, pData, (WORD)dwDataSize) == true)
	{
		return dwDataSize;
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CProxyInstance::CProxyInstance()
{
	m_pProxyEvent = NULL;
}

//��������
CProxyInstance::~CProxyInstance()
{
}

//��������
bool CProxyInstance::StartServer(tagProxyOption ProxyOption, IProxyEvent * pProxyEvent)
{
	//����У��
	//if (pProxyEvent == NULL) return false;

	//��ʶ��Ϣ
	DWORD dwUserData = ProxyOption.wListenPort;
	//�ػ���ʶ
	bool bWatch = true;

	//��ʼ����������
	if (m_Server.Init(this, ProxyOption.wThreadCount, ProxyOption.dwDetectTime, ProxyOption.wMaxAcceptCount, ProxyOption.wMaxSocketCount) == false) return false;
	//�����˿�
	if (m_Server.Listen(ProxyOption.wListenPort, dwUserData, ProxyOption.wProtocol, bWatch) == false) return false;

	//��ʼ�����ӷ���
	if (m_Client.Init(this, ProxyOption.wThreadCount, ProxyOption.dwDetectTime, ProxyOption.wMaxConnectCount) == false) return false;

	//���ýӿ�
	m_pProxyEvent = pProxyEvent;

	//���ò���
	lstrcpyn(m_szDefaultAddress, ProxyOption.szDefaultAddress, CountArray(m_szDefaultAddress));
	m_wDefaultPort = ProxyOption.wDefaultPort;
	m_wDefaultProtocol = ProxyOption.wProtocol;

	return true;
}

//ֹͣ����
bool CProxyInstance::StopServer()
{
	//ֹͣ����
	__super::StopKernel();

	//���ýӿ�
	m_pProxyEvent = NULL;

	return true;
}

//���ӷ���
bool CProxyInstance::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return (m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch) != INVALID_SOCKETID);
}

//��������
DWORD CProxyInstance::ProxyConnect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol)
{
	return m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, false);
}

//��������
bool CProxyInstance::SetUserData(DWORD dwSocketID, DWORD dwUserData)
{
	//��ʶ���
	if (CHECK_SOCKETID(dwSocketID) == false) return false;

	//��ȡ����
	WORD wIndex = SOCKET_INDEX(dwSocketID);
	if (wIndex >= INDEX_LISTEN)
	{
		return false;
	}
	else if (wIndex >= INDEX_CONNECT)
	{
		return m_Client.SetUserData(dwSocketID, dwUserData, true);
	}

	return m_Server.SetUserData(dwSocketID, dwUserData, true);
}

//��������
bool CProxyInstance::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	//��ʶ���
	if (CHECK_SOCKETID(dwSocketID) == false) return false;

	//��ȡ����
	WORD wIndex = SOCKET_INDEX(dwSocketID);
	if (wIndex >= INDEX_LISTEN)
	{
		return false;
	}
	else if (wIndex >= INDEX_CONNECT)
	{
		return m_Client.SendData(dwSocketID, pData, wDataSize);
	}

	return m_Server.SendData(dwSocketID, pData, wDataSize);
}

//�ر�����
bool CProxyInstance::CloseSocket(DWORD dwSocketID)
{
	//��ʶ���
	if (CHECK_SOCKETID(dwSocketID) == false) return false;

	//��ȡ����
	WORD wIndex = SOCKET_INDEX(dwSocketID);
	if (wIndex >= INDEX_LISTEN)
	{
		return false;
	}
	else if (wIndex >= INDEX_CONNECT)
	{
		return m_Client.CloseSocket(dwSocketID);
	}

	return m_Server.CloseSocket(dwSocketID);
}

//�����¼�
bool CProxyInstance::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//�ص�����
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyServerLink(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketLink(dwSocketID, dwUserData);
}

//�ر��¼�
bool CProxyInstance::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	//�ص�����
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyServerShut(dwSocketID, dwUserData);
	}

	return __super::OnEventSocketShut(dwSocketID, dwUserData);
}

//��ȡ�¼�
int CProxyInstance::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//�ص�����
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyServerRead(dwSocketID, pData, dwDataSize, dwUserData);
	}

	return __super::OnEventSocketRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//���¼�
bool CProxyInstance::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	//�ص�����
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyClientBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
	}

	return __super::OnEventNetworkBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
}

//�ر��¼�
bool CProxyInstance::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	//�ص�����
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyClientShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
	}

	return __super::OnEventNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
}

//��ȡ�¼�
int CProxyInstance::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//�ص�����
	if (m_pProxyEvent)
	{
		return m_pProxyEvent->OnProxyClientRead(dwSocketID, pData, dwDataSize, dwUserData);
	}

	return __super::OnEventNetworkRead(dwSocketID, pData, dwDataSize, dwUserData);
}

//////////////////////////////////////////////////////////////////////////
