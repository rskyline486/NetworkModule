#include "stdafx.h"
#include "AttemperWrapper.h"

//////////////////////////////////////////////////////////////////////////

//�¼�����
#define EVENT_TIMER					0x0001								//ʱ���¼�
#define EVENT_CUSTOMIZE				0x0002								//�Զ��¼�
#define EVENT_CUSTOMIZE_EX			0x0003								//�Զ��¼�

#define EVENT_SOCKET_LINK			0x0004								//�����¼�
#define EVENT_SOCKET_SHUT			0x0005								//�ر��¼�
#define EVENT_SOCKET_READ			0x0006								//��ȡ�¼�

#define EVENT_NETWORK_ACCEPT		0x0007								//Ӧ���¼�
#define EVENT_NETWORK_SHUT			0x0008								//�ر��¼�
#define EVENT_NETWORK_READ			0x0009								//��ȡ�¼�

//////////////////////////////////////////////////////////////////////////

//��ʱ���¼�
struct NTY_TimerEvent
{
	DWORD							dwTimerID;							//ʱ���ʶ
	WPARAM							dwBindParameter;					//�󶨲���
};

//�Զ��¼�
struct NTY_CustomEvent
{
	DWORD							dwCustomID;							//�Զ���ʶ
};

//�Զ��¼�
struct NTY_CustomEventEx
{
	DWORD							dwCustomID;							//�Զ���ʶ
	DWORD							dwContextID;						//�����ʶ
};

//�����¼�
struct NTY_SocketLinkEvent
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	DWORD							dwUserData;							//�û�����
};

//�ر��¼�
struct NTY_SocketShutEvent
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	DWORD							dwUserData;							//�û�����
};

//��ȡ�¼�
struct NTY_SocketReadEvent
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	DWORD							dwUserData;							//�û�����
	WORD							wDataSize;							//���ݴ�С
	NT_Command						Command;							//������Ϣ
};

//Ӧ���¼�
struct NTY_NetworkAcceptEvent
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	TCHAR							szClientAddress[MAX_ADDRSTRLEN];	//�û���ַ
	WORD							wClientPort;						//�û��˿�
};

//�ر��¼�
struct NTY_NetworkShutEvent
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	TCHAR							szClientAddress[MAX_ADDRSTRLEN];	//�û���ַ
	WORD							wClientPort;						//�û��˿�
	DWORD							dwActiveTime;						//����ʱ��
};

//��ȡ�¼�
struct NTY_NetworkReadEvent
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	WORD							wDataSize;							//���ݴ�С
	NT_Command						Command;							//������Ϣ
};

//////////////////////////////////////////////////////////////////////////

//���캯��
CAttemperKernel::CAttemperKernel()
{
}

//��������
CAttemperKernel::~CAttemperKernel()
{
}

//�����ں�
bool CAttemperKernel::StartKernel()
{
	//��ʼ���첽����
	if (m_AsynchronismEngine.Init(this, 1) == false) return false;

	//��ʼ����ʱ������
	if (m_TimerEngine.Init(this, 30) == false) return false;

	//��ʼ�����ӷ���
	if (m_Client.Init(this, 4, 10000L, 512) == false) return false;

	//��ʼ����������
	if (m_Server.Init(this, 4, 10000L, 1, 512) == false) return false;
	//�����˿�
	if (m_Server.Listen(9999, 0L, EnableIPv4, true) == false) return false;

	return true;
}

//ֹͣ�ں�
bool CAttemperKernel::StopKernel()
{
	//ֹͣ����
	m_Server.Release();

	//ֹͣ����
	m_Client.Release();

	//ֹͣ��ʱ������
	m_TimerEngine.Release();

	//ֹͣ�첽����
	m_AsynchronismEngine.Release();

	return true;
}

//�첽��ʼ
bool CAttemperKernel::OnEventAsynchronismStrat()
{
	return true;
}

//�첽����
bool CAttemperKernel::OnEventAsynchronismStop()
{
	return true;
}

//�첽�¼�
bool CAttemperKernel::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	Logger_Info(TEXT("�첽�¼�=>��ʶ��Ϣ:%u, ���ݴ�С:%u"), wIdentifier, wDataSize);
	return true;
}

//ʱ���¼�
bool CAttemperKernel::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_TimerEvent * pTimerEvent = (NTY_TimerEvent *)m_cbBuffer;

	//��������
	pTimerEvent->dwTimerID = dwTimerID;
	pTimerEvent->dwBindParameter = dwBindParameter;

	//Ͷ������
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_TIMER, m_cbBuffer, sizeof(NTY_TimerEvent));
}

//�����¼�
bool CAttemperKernel::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_SocketLinkEvent * pConnectEvent = (NTY_SocketLinkEvent *)m_cbBuffer;

	//��������
	pConnectEvent->dwSocketID = dwSocketID;
	pConnectEvent->dwUserData = dwUserData;

	//Ͷ������
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_SOCKET_LINK, m_cbBuffer, sizeof(NTY_SocketLinkEvent));
}

//�ر��¼�
bool CAttemperKernel::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_SocketShutEvent * pCloseEvent = (NTY_SocketShutEvent *)m_cbBuffer;

	//��������
	pCloseEvent->dwSocketID = dwSocketID;
	pCloseEvent->dwUserData = dwUserData;

	//Ͷ������
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_SOCKET_SHUT, m_cbBuffer, sizeof(NTY_SocketShutEvent));
}

//��ȡ�¼�
int CAttemperKernel::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//У������
	if (dwDataSize == 0 || pData == NULL) return -1;
	if (dwDataSize > SOCKET_BUFFER) return -1;

	//������ȡ
	if (dwDataSize < sizeof(NT_Command)) return -1;
	NT_Command * pCommand = (NT_Command *)pData;

	//��������
	pData = pCommand + 1;
	dwDataSize = dwDataSize - sizeof(NT_Command);

	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_SocketReadEvent * pReadEvent = (NTY_SocketReadEvent *)m_cbBuffer;

	//��������
	pReadEvent->dwSocketID = dwSocketID;
	pReadEvent->dwUserData = dwUserData;
	pReadEvent->wDataSize = (WORD)dwDataSize;
	pReadEvent->Command = *pCommand;

	//��������
	if (dwDataSize > 0)
	{
		CopyMemory(m_cbBuffer + sizeof(NTY_SocketReadEvent), pData, dwDataSize);
	}

	//Ͷ������
	if (m_AsynchronismEngine.PostAsynchronismData(EVENT_SOCKET_READ, m_cbBuffer, sizeof(NTY_SocketReadEvent) + (WORD)dwDataSize) == true)
	{
		return dwUserData;
	}

	return -1;
}

//���¼�
bool CAttemperKernel::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_NetworkAcceptEvent * pAcceptEvent = (NTY_NetworkAcceptEvent *)m_cbBuffer;

	//��������
	pAcceptEvent->dwSocketID = dwSocketID;
	lstrcpyn(pAcceptEvent->szClientAddress, pszClientIP, CountArray(pAcceptEvent->szClientAddress));
	pAcceptEvent->wClientPort = wClientPort;

	//Ͷ������
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_NETWORK_ACCEPT, m_cbBuffer, sizeof(NTY_NetworkAcceptEvent));
}

//�ر��¼�
bool CAttemperKernel::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_NetworkShutEvent * pCloseEvent = (NTY_NetworkShutEvent *)m_cbBuffer;

	//��������
	pCloseEvent->dwSocketID = dwSocketID;
	lstrcpyn(pCloseEvent->szClientAddress, pszClientIP, CountArray(pCloseEvent->szClientAddress));
	pCloseEvent->wClientPort = wClientPort;
	pCloseEvent->dwActiveTime = dwActiveTime;

	//Ͷ������
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_NETWORK_SHUT, m_cbBuffer, sizeof(NTY_NetworkShutEvent));
}

//��ȡ�¼�
int CAttemperKernel::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//У������
	if (dwDataSize == 0 || pData == NULL) return -1;
	if (dwDataSize > SOCKET_BUFFER) return -1;

	//������ȡ
	if (dwDataSize < sizeof(NT_Command)) return -1;
	NT_Command * pCommand = (NT_Command *)pData;

	//��������
	pData = pCommand + 1;
	dwDataSize = dwDataSize - sizeof(NT_Command);

	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_NetworkReadEvent * pReadEvent = (NTY_NetworkReadEvent *)m_cbBuffer;

	//��������
	pReadEvent->dwSocketID = dwSocketID;
	pReadEvent->wDataSize = (WORD)dwDataSize;
	pReadEvent->Command = *pCommand;

	if (dwDataSize > 0)
	{
		//��������
		CopyMemory(m_cbBuffer + sizeof(NTY_NetworkReadEvent), pData, dwDataSize);
	}

	//Ͷ������
	if (m_AsynchronismEngine.PostAsynchronismData(EVENT_NETWORK_READ, m_cbBuffer, sizeof(NTY_NetworkReadEvent) + (WORD)dwDataSize) == true)
	{
		return dwUserData;
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CAttemperInstance::CAttemperInstance()
{
	m_pAttemperEvent = NULL;
}

//��������
CAttemperInstance::~CAttemperInstance()
{
}

//��������
bool CAttemperInstance::StartServer(tagAttemperOption AttemperOption, IAttemperEvent * pAttemperEvent)
{
	//����У��
	if (pAttemperEvent == NULL) return false;
	if (AttemperOption.pClientOption == NULL && AttemperOption.pServerOption == NULL) return false;

	//��ʼ���������
	if (m_CryptoManager.Init() == false) return false;

	//��ʼ���첽����
	if (m_AsynchronismEngine.Init(this, 1) == false) return false;

	//��ʼ����ʱ������
	if (m_TimerEngine.Init(this, 30) == false) return false;

	//����пͻ���
	if (AttemperOption.pClientOption)
	{
		//��ʼ�����ӷ���
		if (m_Client.Init(this, AttemperOption.pClientOption->wThreadCount, AttemperOption.pClientOption->dwDetectTime, AttemperOption.pClientOption->wMaxConnectCount) == false) return false;
	}

	//����з����
	if (AttemperOption.pServerOption)
	{
		//��ʶ��Ϣ
		DWORD dwUserData = AttemperOption.pServerOption->wListenPort;
		//�ػ���ʶ
		bool bWatch = true;
		//��ʼ����������
		if (m_Server.Init(this, AttemperOption.pServerOption->wThreadCount, AttemperOption.pServerOption->dwDetectTime, AttemperOption.pServerOption->wMaxAcceptCount, AttemperOption.pServerOption->wMaxSocketCount) == false) return false;

		//���ܶ���
		CCryptoHelper CryptoHelper(m_CryptoManager);
		//��������
		tagEncryptData EncryptData;
		if (CryptoHelper.Encrypt(MDM_NT_COMMAND, SUB_NT_DETECT_SOCKET, NULL, 0, EncryptData) == false) return false;
		//������������
		if (m_Server.SetHeartbeatPacket(EncryptData.pDataBuffer, EncryptData.wDataSize) == false) return false;

		//�����˿�
		if (m_Server.Listen(AttemperOption.pServerOption->wListenPort, dwUserData, AttemperOption.pServerOption->wProtocol, bWatch) == false) return false;
	}

	//���ýӿ�
	m_pAttemperEvent = pAttemperEvent;

	return true;
}

//ֹͣ����
bool CAttemperInstance::StopServer()
{
	//ֹͣ����
	__super::StopKernel();

	//�ͷŹ������
	m_CryptoManager.Release();

	return true;
}

//���ӷ���
bool CAttemperInstance::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return (m_Client.Connect(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch) != INVALID_SOCKETID);
}

//��������
bool CAttemperInstance::SendData(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	//��ʶ���
	if (CHECK_SOCKETID(dwSocketID) == false) return false;

	//��ȡ����
	WORD wIndex = SOCKET_INDEX(dwSocketID);
	if (wIndex >= INDEX_LISTEN)
	{
		Logger_Info(TEXT("��������=>�쳣������Ϣ,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize);
		return false;
	}

	//���ܶ���
	CCryptoHelper CryptoHelper(m_CryptoManager);

	//��������
	tagEncryptData EncryptData;
	if (CryptoHelper.Encrypt(wMainCmdID, wSubCmdID, pData, wDataSize, EncryptData) == false)
	{
		Logger_Info(TEXT("��������=>��������ʧ��,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize);
		return false;
	}

	//���ӷ���
	if (wIndex >= INDEX_CONNECT)
	{
		if (m_Client.SendData(dwSocketID, EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
		{
			Logger_Info(TEXT("���ӷ���=>��������ʧ��,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
			return false;
		}
	}
	
	//������
	if (m_Server.SendData(dwSocketID, EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
	{
		Logger_Info(TEXT("������=>��������ʧ��,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), dwSocketID, wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
		return false;
	}

	return true;
}

//Ⱥ������
bool CAttemperInstance::SendDataBatch(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize)
{
	//���ܶ���
	CCryptoHelper CryptoHelper(m_CryptoManager);

	//��������
	tagEncryptData EncryptData;
	if (CryptoHelper.Encrypt(wMainCmdID, wSubCmdID, pData, wDataSize, EncryptData) == false)
	{
		Logger_Info(TEXT("Ⱥ������=>��������ʧ��,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), wMainCmdID, wSubCmdID, wDataSize);
		return false;
	}

	//��������
	if (m_Client.SendDataBatch(EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
	{
		//Logger_Info(TEXT("����Ⱥ��=>��������ʧ��,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
	}

	//��������
	if (m_Server.SendDataBatch(EncryptData.pDataBuffer, EncryptData.wDataSize) == false)
	{
		//Logger_Info(TEXT("����Ⱥ��=>��������ʧ��,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u,EncryptDataSize:%u"), wMainCmdID, wSubCmdID, wDataSize, EncryptData.wDataSize);
	}

	return true;
}

//�ر�����
bool CAttemperInstance::CloseSocket(DWORD dwSocketID)
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

//��ȫ�ر�
bool CAttemperInstance::ShutDownSocket(DWORD dwSocketID)
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
		return m_Client.ShutDownSocket(dwSocketID);
	}

	return m_Server.ShutDownSocket(dwSocketID);
}

//���ö�ʱ��
bool CAttemperInstance::SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter)
{
	return m_TimerEngine.SetTimer(dwTimerID, dwElapse, dwRepeat, dwBindParameter);
}

//ɾ����ʱ��
bool CAttemperInstance::KillTimer(DWORD dwTimerID)
{
	return m_TimerEngine.KillTimer(dwTimerID);
}

//ɾ����ʱ��
bool CAttemperInstance::KillAllTimer()
{
	return m_TimerEngine.KillAllTimer();
}

//Ͷ�ݿ���
bool CAttemperInstance::PostCustomEvent(DWORD dwCustomID, VOID * pData, WORD wDataSize)
{
	//У������
	if (wDataSize > SOCKET_BUFFER) return false;

	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_CustomEvent * pCustomEvent = (NTY_CustomEvent *)m_cbBuffer;

	//��������
	pCustomEvent->dwCustomID = dwCustomID;

	//��������
	if (wDataSize>0)
	{
		CopyMemory(m_cbBuffer + sizeof(NTY_CustomEvent), pData, wDataSize);
	}

	//Ͷ������
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_CUSTOMIZE, m_cbBuffer, sizeof(NTY_CustomEvent) + wDataSize);
}

//Ͷ���¼�
bool CAttemperInstance::PostCustomEventEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize)
{
	//У������
	if (wDataSize > SOCKET_BUFFER) return false;

	//ͬ������
	CLocker Locker(m_Mutex);
	NTY_CustomEventEx * pCustomEventEx = (NTY_CustomEventEx *)m_cbBuffer;

	//��������
	pCustomEventEx->dwCustomID = dwCustomID;
	pCustomEventEx->dwContextID = dwContextID;

	//��������
	if (wDataSize > 0)
	{
		CopyMemory(m_cbBuffer + sizeof(NTY_CustomEventEx), pData, wDataSize);
	}

	//Ͷ������
	return m_AsynchronismEngine.PostAsynchronismData(EVENT_CUSTOMIZE_EX, m_cbBuffer, sizeof(NTY_CustomEventEx) + wDataSize);
}

//�첽�¼�
bool CAttemperInstance::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	//�ں��¼�
	switch (wIdentifier)
	{
	case EVENT_TIMER:				//ʱ���¼�
		{
			//����У��
			if (wDataSize!=sizeof(NTY_TimerEvent)) return false;

			//������Ϣ
			NTY_TimerEvent * pTimerEvent=(NTY_TimerEvent *)pData;
			m_pAttemperEvent->OnAttemperTimer(pTimerEvent->dwTimerID,pTimerEvent->dwBindParameter);

			return true;
		}
	case EVENT_CUSTOMIZE:			//�Զ��¼�
		{
			//����У��
			if (wDataSize<sizeof(NTY_CustomEvent)) return false;

			//������Ϣ
			NTY_CustomEvent * pCustomEvent=(NTY_CustomEvent *)pData;
			m_pAttemperEvent->OnAttemperCustom(pCustomEvent->dwCustomID, pCustomEvent +1,wDataSize-sizeof(NTY_CustomEvent));

			return true;
		}
	case EVENT_CUSTOMIZE_EX:		//�Զ��¼�
		{
			//����У��
			if (wDataSize<sizeof(NTY_CustomEventEx)) return false;

			//������Ϣ
			NTY_CustomEventEx * pCustomEventEx=(NTY_CustomEventEx *)pData;
			m_pAttemperEvent->OnAttemperCustomEx(pCustomEventEx->dwCustomID, pCustomEventEx->dwContextID, pCustomEventEx +1,wDataSize-sizeof(NTY_CustomEventEx));

			return true;
		}
	case EVENT_SOCKET_LINK:			//�����¼�
		{
			//����У��
			if (wDataSize!=sizeof(NTY_SocketLinkEvent)) return false;

			//������Ϣ
			NTY_SocketLinkEvent * pConnectEvent=(NTY_SocketLinkEvent *)pData;
			m_pAttemperEvent->OnAttemperSocketLink(pConnectEvent->dwSocketID,pConnectEvent->dwUserData);

			return true;
		}
	case EVENT_SOCKET_SHUT:			//�ر��¼�
		{
			//����У��
			if (wDataSize!=sizeof(NTY_SocketShutEvent)) return false;

			//������Ϣ
			NTY_SocketShutEvent * pCloseEvent=(NTY_SocketShutEvent *)pData;
			m_pAttemperEvent->OnAttemperSocketShut(pCloseEvent->dwSocketID,pCloseEvent->dwUserData);

			return true;
		}
	case EVENT_SOCKET_READ:			//��ȡ�¼�
		{
			//��������
			NTY_SocketReadEvent * pReadEvent=(NTY_SocketReadEvent *)pData;

			//����У��
			if (wDataSize<sizeof(NTY_SocketReadEvent)) return false;
			if (wDataSize!=(sizeof(NTY_SocketReadEvent)+pReadEvent->wDataSize)) return false;

			//������Ϣ
			m_pAttemperEvent->OnAttemperSocketRead(pReadEvent->dwSocketID,pReadEvent->Command.wMainCmdID,pReadEvent->Command.wSubCmdID,pReadEvent+1,pReadEvent->wDataSize,pReadEvent->dwUserData);

			return true;
		}
	case EVENT_NETWORK_ACCEPT:		//Ӧ���¼�
		{
			//����У��
			if (wDataSize!=sizeof(NTY_NetworkAcceptEvent)) return false;

			//��������
			bool bSuccess=false;
			NTY_NetworkAcceptEvent * pAcceptEvent=(NTY_NetworkAcceptEvent *)pData;

			//������Ϣ
			try
			{ 
				bSuccess= m_pAttemperEvent->OnAttemperNetworkBind(pAcceptEvent->dwSocketID, pAcceptEvent->szClientAddress, pAcceptEvent->wClientPort);
			}
			catch (...)	{ }

			//ʧ�ܴ���
			if (bSuccess == false)
			{
				Logger_Warn(TEXT("�����=>�����쳣,�ر�����,dwSocketID:%u,szClientAddress:%s,wClientPort:%u"), pAcceptEvent->dwSocketID, pAcceptEvent->szClientAddress, pAcceptEvent->wClientPort);
				m_Server.CloseSocket(pAcceptEvent->dwSocketID);
			}

			return true;
		}
	case EVENT_NETWORK_SHUT:		//�ر��¼�
		{
			//����У��
			if (wDataSize!=sizeof(NTY_NetworkShutEvent)) return false;

			//������Ϣ
			NTY_NetworkShutEvent * pCloseEvent=(NTY_NetworkShutEvent *)pData;
			m_pAttemperEvent->OnAttemperNetworkShut(pCloseEvent->dwSocketID,pCloseEvent->szClientAddress,pCloseEvent->wClientPort,pCloseEvent->dwActiveTime);

			return true;
		}
	case EVENT_NETWORK_READ:		//��ȡ�¼�
		{
			//Ч���С
			NTY_NetworkReadEvent * pReadEvent=(NTY_NetworkReadEvent *)pData;

			//����У��
			if (wDataSize<sizeof(NTY_NetworkReadEvent)) return false;
			if (wDataSize!=(sizeof(NTY_NetworkReadEvent)+pReadEvent->wDataSize)) return false;

			//������Ϣ
			bool bSuccess=false;
			try
			{ 
				bSuccess= m_pAttemperEvent->OnAttemperNetworkRead(pReadEvent->dwSocketID,pReadEvent->Command.wMainCmdID,pReadEvent->Command.wSubCmdID,pReadEvent+1,pReadEvent->wDataSize);
			}
			catch (...)	{ }

			//ʧ�ܴ���
			if (bSuccess == false)
			{
				Logger_Warn(TEXT("�����ȡ=>�����쳣,�ر�����,dwSocketID:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), pReadEvent->dwSocketID, pReadEvent->Command.wMainCmdID, pReadEvent->Command.wSubCmdID, pReadEvent->wDataSize);
				m_Server.CloseSocket(pReadEvent->dwSocketID);
			}

			return true;
		}
	}

	return __super::OnEventAsynchronismData(wIdentifier, pData, wDataSize);
}

//ʱ���¼�
bool CAttemperInstance::OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter)
{
	return __super::OnEventTimer(dwTimerID, dwBindParameter);
}

//�����¼�
bool CAttemperInstance::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	//������֤
	NT_Validate Validate;
	ZeroMemory(&Validate, sizeof(Validate));
	lstrcpyn(Validate.szValidateKey, szValidateSecretKey, CountArray(Validate.szValidateKey));
	SendData(dwSocketID, MDM_NT_COMMAND, SUB_NT_VALIDATE_SOCKET, &Validate, sizeof(Validate));

	return __super::OnEventSocketLink(dwSocketID, dwUserData);
}

//�ر��¼�
bool CAttemperInstance::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	return __super::OnEventSocketShut(dwSocketID, dwUserData);
}

//��ȡ�¼�
int CAttemperInstance::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//��������
	int nReadSize = 0;
	while ((dwDataSize - nReadSize) >= sizeof(NT_Head))
	{
		//��������
		NT_Info * pInfo = (NT_Info *)((BYTE*)pData + nReadSize);

		//����У��
		if ((pInfo->wPacketSize < sizeof(NT_Info)) || (pInfo->wPacketSize > SOCKET_BUFFER))
		{
			Logger_Error(TEXT("���Ӷ�ȡ=>���ݰ������쳣,dwSocketID:%u,dwDataSize:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwDataSize, dwUserData, pInfo->wPacketSize);
			return -1;
		}

		//�����ж�
		if ((dwDataSize - nReadSize) < pInfo->wPacketSize)
		{
			return nReadSize;
		}

		//��������
		WORD wBufferDataSize = pInfo->wPacketSize;

		//���ܶ���
		CCryptoHelper CryptoHelper(m_CryptoManager);

		//��������
		tagDecryptData DecryptData;
		if (CryptoHelper.Decrypt((LPBYTE)pData + nReadSize, wBufferDataSize, DecryptData) == false)
		{
			Logger_Error(TEXT("���Ӷ�ȡ=>�������ݰ��쳣,dwSocketID:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwUserData, wBufferDataSize);
			return -1;
		}

		//���´���
		nReadSize += wBufferDataSize;

		//������
		if ((DecryptData.Command.wMainCmdID == MDM_NT_COMMAND) && (DecryptData.Command.wSubCmdID == SUB_NT_DETECT_SOCKET))
		{
			//Logger_Info(TEXT("���Ӷ�ȡ=>��������,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
			//SendData(dwSocketID, MDM_NT_COMMAND, SUB_NT_DETECT_SOCKET, NULL, 0);
			continue;
		}

		//�����ж�
		if (DecryptData.Command.wMainCmdID != MDM_NT_COMMAND)
		{
			//��������
			if (__super::OnEventSocketRead(dwSocketID, DecryptData.pDataBuffer, DecryptData.wDataSize, dwUserData) < 0)
			{
				Logger_Error(TEXT("���Ӷ�ȡ=>�������ݰ��쳣,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
				return -1;
			}
		}
	}

	return nReadSize;
}

//���¼�
bool CAttemperInstance::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	return __super::OnEventNetworkBind(dwSocketID, pszClientIP, wClientPort, dwUserData);
}

//�ر��¼�
bool CAttemperInstance::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	return __super::OnEventNetworkShut(dwSocketID, pszClientIP, wClientPort, dwActiveTime, dwUserData);
}

//��ȡ�¼�
int CAttemperInstance::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	//��������
	int nReadSize = 0;
	while ((dwDataSize - nReadSize) >= sizeof(NT_Head))
	{
		//��������
		NT_Info * pInfo = (NT_Info *)((BYTE*)pData + nReadSize);

		//����У��
		if ((pInfo->wPacketSize < sizeof(NT_Info)) || (pInfo->wPacketSize > SOCKET_BUFFER))
		{
			Logger_Error(TEXT("�����ȡ=>���ݰ������쳣,dwSocketID:%u,dwDataSize:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwDataSize, dwUserData, pInfo->wPacketSize);
			return -1;
		}

		//�����ж�
		if ((dwDataSize - nReadSize) < pInfo->wPacketSize)
		{
			return nReadSize;
		}

		//��������
		WORD wBufferDataSize = pInfo->wPacketSize;

		//���ܶ���
		CCryptoHelper CryptoHelper(m_CryptoManager);

		//��������
		tagDecryptData DecryptData;
		if (CryptoHelper.Decrypt((LPBYTE)pData + nReadSize, wBufferDataSize, DecryptData) == false)
		{
			Logger_Error(TEXT("�����ȡ=>�������ݰ��쳣,dwSocketID:%u,dwUserData:%u,wPacketSize:%u"), dwSocketID, dwUserData, wBufferDataSize);
			return -1;
		}

		//���´���
		nReadSize += wBufferDataSize;

		//��֤�ж�
		if (dwUserData == 0)
		{
			if (DecryptData.Command.wMainCmdID != MDM_NT_COMMAND || DecryptData.Command.wSubCmdID != SUB_NT_VALIDATE_SOCKET || (DecryptData.wDataSize - sizeof(NT_Command)) != sizeof(NT_Validate))
			{
				Logger_Error(TEXT("�����ȡ=>��֤��Ϣ�쳣,wMainCmdID:[%d],wSubCmdID:[%d],wDataSize:[%d]"), DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
				return -1;
			}

			//��ȡ��Ȩ�ṹ
			NT_Validate * pValidate = (NT_Validate *)((LPBYTE)DecryptData.pDataBuffer + sizeof(NT_Command));
			pValidate->szValidateKey[CountArray(pValidate->szValidateKey) - 1] = 0;
			if (memcmp(pValidate->szValidateKey, szValidateSecretKey, sizeof(szValidateSecretKey)) != 0)
			{
				Logger_Error(TEXT("�����ȡ=>�쳣��Ȩ��=>%s"), pValidate->szValidateKey);
				return -1;
			}

			//�����û�����(ͬ������,ע��:����ֻ��ͬ�������Լ�,����������socket���������ײ�������)
			dwUserData = 1;
			m_Server.SetUserData(dwSocketID, dwUserData, true);
			continue;
		}

		//������
		if ((DecryptData.Command.wMainCmdID == MDM_NT_COMMAND) && (DecryptData.Command.wSubCmdID == SUB_NT_DETECT_SOCKET))
		{
			//Logger_Info(TEXT("�����ȡ=>��������,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
			//SendData(dwSocketID, MDM_NT_COMMAND, SUB_NT_DETECT_SOCKET, NULL, 0);
			continue;
		}

		//�����ж�
		if (DecryptData.Command.wMainCmdID != MDM_NT_COMMAND)
		{
			//��������
			if (__super::OnEventNetworkRead(dwSocketID, DecryptData.pDataBuffer, DecryptData.wDataSize, dwUserData) < 0)
			{
				Logger_Error(TEXT("�����ȡ=>�������ݰ��쳣,dwSocketID:%u,dwUserData:%u,wMainCmdID:%u,wSubCmdID:%u,wDataSize:%u"), dwSocketID, dwUserData, DecryptData.Command.wMainCmdID, DecryptData.Command.wSubCmdID, DecryptData.wDataSize - sizeof(NT_Command));
				return -1;
			}
		}
	}

	return nReadSize;
}

//////////////////////////////////////////////////////////////////////////
