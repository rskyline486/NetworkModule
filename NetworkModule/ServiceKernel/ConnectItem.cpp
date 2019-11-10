#include "stdafx.h"
#include "ConnectItem.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CConnectItem::CConnectItem(WORD wIndex, IConnectItemSink * pConnectItemSink)
	: CNativeInfo(wIndex), m_pConnectItemSink(pConnectItemSink)
{
	ZeroMemory(m_szConnectAddress, sizeof(m_szConnectAddress));
	m_wConnectPort = 9999;
	m_Protocol = EnableIPv4;
}

//��������
CConnectItem::~CConnectItem(void)
{
}

//���Ӷ���
DWORD CConnectItem::Connect(HANDLE hCompletionPort, LPCTSTR pszConnectAddress, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch)
{
	//����У��
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("��ɶ˿ڶ���Ϊ��"));
		return INVALID_SOCKETID;
	}

	//��¼����
	lstrcpyn(m_szConnectAddress, pszConnectAddress, CountArray(m_szConnectAddress));
	m_wConnectPort = wPort;
	m_Protocol = Protocol;

	//��ȡ��ַ
	Address serverAddr = CNetwork::GetAddress(m_szConnectAddress, wPort, Protocol, true);
	if (CNetwork::IsValidAddress(serverAddr) == false)
	{
		Logger_Error(TEXT("��ȡ��ַʧ��,���ӵ�ַ:%s,���Ӷ˿�:%u,Э��汾:%d,������:%d"), m_szConnectAddress, wPort, Protocol, WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//����SOCKET
	m_hSocket = CNetwork::CreateSocket(serverAddr, Protocol);
	if (m_hSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("����SOCKETʧ��,������:%d"), WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//��ȡConnectEx������ַ
	LPFN_CONNECTEX fnConnectEx = NULL;
	GUID GuidConnectEx = WSAID_CONNECTEX;
	DWORD dwBytes;
	if (WSAIoctl(m_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidConnectEx, sizeof(GuidConnectEx), &fnConnectEx, sizeof(fnConnectEx), &dwBytes, NULL, NULL) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("��ȡConnectEx������ַʧ��,������:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//�󶨵�ַ
	Address bindAddr = CNetwork::GetAddress(serverAddr, 0);
	bindAddr = CNetwork::Bind(m_hSocket, bindAddr);
	if (CNetwork::IsValidAddress(bindAddr) == false)
	{
		Logger_Error(TEXT("������˿�ʧ��,������:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//�󶨵���ɶ˿�
	if (CreateIoCompletionPort((HANDLE)m_hSocket, hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		Logger_Error(TEXT("�󶨵���ɶ˿�ʧ��,������:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//��������
	__super::InitData();
	//�û�����
	m_dwUserData = dwUserData;
	//�����ػ�
	if (bWatch) __super::SetDetect();

	//ͬ������
	//��ֹConnectEx����Ͷ�ݽ��պ�,��δ���ü����ý��ձ�ʶ,�ʹ����˽�����ɻص�
	//��ɻص����ý��ձ�ʶ��,�˴��������ִ��,���������ý��ձ�ʶ
	//�Ӷ����º���Ͷ�ݽ�������ʱ��������Ϊ��һ�ν��ջ�δ����
	//��˳����ȴ�����,�������¸ñ�ʶ��Զ�����ᱻ����
	//��:�˴�������������Ͷ�ݱ�ʶ,��ô��������ɻص���δ����ʱ,�Ϳ���ֱ�������ر�����
	//�Ӷ����º����ö���ᱻ�������������,�������²���Ԥ�ϵ��������(��Ϊ��һ�εĻص���δ����)
	CLocker Locker(m_Mutex);

	//Ͷ������
	m_OverLappedRecv.SetOperationType(enOperationType_Connect);
	if (!fnConnectEx(m_hSocket, &serverAddr.sa, CNetwork::GetAddressSize(serverAddr), 0, 0, 0, m_OverLappedRecv))
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Error(TEXT("ConnectEx����ʧ��,������:%d"), WSAGetLastError());
			CNetwork::CloseSocket(m_hSocket);
			m_hSocket = INVALID_SOCKET;

			//��������
			__super::ResetData();

			return INVALID_SOCKETID;
		}
	}

	//���ñ�ʶ
	m_OverLappedRecv.SetHandleIng(true);

	return __super::GetSocketID();
}

//�ָ�����
DWORD CConnectItem::ResumeConnect(HANDLE hCompletionPort)
{
	//�����ж�
	if (__super::IsUsed()) return __super::GetSocketID();

	//����У��
	if (hCompletionPort == NULL) return INVALID_SOCKETID;

	return Connect(hCompletionPort, m_szConnectAddress, m_wConnectPort, m_dwUserData, m_Protocol, true);
}

//��������
void CConnectItem::DealAsync(COverLapped * pOverLapped, DWORD dwThancferred)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//���ñ�ʶ
	pOverLapped->SetHandleIng(false);

	//�жϹر�
	if (m_hSocket == INVALID_SOCKET)
	{
		Logger_Debug(TEXT("SOCKET�����Ч,�ر�����"));
		CloseSocket();
		return;
	}

	//����У��
	if (static_cast<int>(dwThancferred) == SOCKET_ERROR)
	{
		Logger_Debug(TEXT("�����쳣,�ر�����"));
		CloseSocket();
		return;
	}

	switch (pOverLapped->GetOperationType())
	{
	case enOperationType_Connect:
		{
			//��������
			if (setsockopt(m_hSocket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0) == SOCKET_ERROR)
			{
				Logger_Error(TEXT("��������[SO_UPDATE_CONNECT_CONTEXT]ʧ��,������:%d,�ر�����"), WSAGetLastError());
				CloseSocket();
				return;
			}

			//����Ϊ��������
			pOverLapped->SetOperationType(enOperationType_Recv);

			//����֪ͨ
			m_pConnectItemSink->OnEventConnectLink(this);

			//Ͷ������
			PostRecv();
		}
		break;
	case enOperationType_Recv:
		{
			//����У��
			if (dwThancferred == 0)
			{
				CloseSocket();
				return;
			}
			//���ճ���
			if (m_OverLappedRecv.RecvCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("�������ݴ���ʧ��,dwThancferred:%u,�ر�����"), dwThancferred);
				CloseSocket();
				return;
			}
			//��ȡ����
			DWORD dwDealSize = m_pConnectItemSink->OnEventConnectRead(this, pOverLapped->GetBufferAddr(), pOverLapped->GetBufferSize());
			if (static_cast<int>(dwDealSize) == SOCKET_ERROR || m_OverLappedRecv.DealCompleted(dwDealSize) == false)
			{
				Logger_Debug(TEXT("�������ݴ���ʧ��,dwDealSize:%u,�ر�����"), dwDealSize);
				CloseSocket();
				return;
			}
			//Ͷ������
			PostRecv();
		}
		break;
	case enOperationType_Send:
		{
			//����У��
			if (dwThancferred == 0)
			{
				CloseSocket();
				return;
			}
			//���ͳ���
			if (m_OverLappedSend.SendCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("�������ݴ���ʧ��,dwThancferred:%u,�ر�����"), dwThancferred);
				CloseSocket();
				return;
			}
			//Ͷ������
			PostSend();
		}
		break;
	default:
		{
			Logger_Error(TEXT("�쳣֪ͨ����???,OperationType:%d,dwThancferred:%u,�ر�����"), pOverLapped->GetOperationType(), dwThancferred);
			CloseSocket();
		}
		break;
	}
}

//���ͺ���
bool CConnectItem::SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//�����ж�
	if (SendVerdict(wRountID) == false) return false;

	//��������
	if (lpData != NULL && dwDataSize != 0)
	{
		if (m_OverLappedSend.SendData(lpData, dwDataSize) == false)
		{
			Logger_Error(TEXT("����ʧ��,��������ʧ��,dwDataSize:%u"), dwDataSize);
			return false;
		}
	}

	//״̬�ж�
	if (m_OverLappedRecv.GetOperationType() == enOperationType_Connect)
	{
		return true;
	}

	//Ͷ�ݷ���
	PostSend();

	return true;
}

//�ر�����
bool CConnectItem::CloseSocket(WORD wRountID)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//״̬�ж�
	if (m_wRountID != wRountID) return false;

	//�ظ�У��(��ֹ�ڹرջص��ӿ����ٴε��øýӿڶ�������ѭ��)
	if (m_hSocket == INVALID_SOCKET) return true;

	CloseSocket();
	return true;
}

//�ر�����
void CConnectItem::CloseSocket()
{
	//�ر�socket
	if (m_hSocket != INVALID_SOCKET)
	{
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	//�жϹر�
	if ((m_OverLappedRecv.GetHandleIng() == false) && (m_OverLappedSend.GetHandleIng() == false))
	{
		if (__super::IsUsed())
		{
			//�ر�֪ͨ
			m_pConnectItemSink->OnEventConnectShut(this);
			//�ָ�����
			ResumeData();
		}
	}
}

//�ָ�����
void CConnectItem::ResumeData()
{
	//��������
	m_OverLappedRecv.ResetData();
	//��������
	m_OverLappedSend.ResetData();
	//��������
	__super::ResetData();
}

//Ͷ������
void CConnectItem::PostRecv()
{
	//Ͷ��У��
	if (m_OverLappedRecv.GetHandleIng() == true)
	{
		//Logger_Debug(TEXT("�Ѵ��ڽ�������"));
		return;
	}

	//�ָ�����
	if (!m_OverLappedRecv.ResumeBuffer())
	{
		Logger_Debug(TEXT("�����ص�����ʧ��,�ر�����"));
		CloseSocket();
		return;
	}

	//��������
	DWORD dwThancferred = 0, dwFlags = 0;
	if (WSARecv(m_hSocket, m_OverLappedRecv, 1, &dwThancferred, &dwFlags, m_OverLappedRecv, NULL) == SOCKET_ERROR)
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Debug(TEXT("WSARecv����ʧ��,������:%d,�ر�����"), WSAGetLastError());
			CloseSocket();
			return;
		}
	}

	//���ñ�ʶ
	m_OverLappedRecv.SetHandleIng(true);
}

//Ͷ������
void CConnectItem::PostSend()
{
	//Ͷ��У��
	if (m_OverLappedSend.GetHandleIng() == true)
	{
		//Logger_Debug(TEXT("�Ѵ��ڷ�������"));
		return;
	}

	//�ָ�����
	if (!m_OverLappedSend.ResumeBuffer())
	{
		//Logger_Debug(TEXT("�������"));
		return;
	}

	//��������
	DWORD dwThancferred = 0;
	if (WSASend(m_hSocket, m_OverLappedSend, 1, &dwThancferred, 0, m_OverLappedSend, NULL) == SOCKET_ERROR)
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Debug(TEXT("WSASend����ʧ��,������:%d,�ر�����"), WSAGetLastError());
			CloseSocket();
			return;
		}
	}

	//���ñ�ʶ
	m_OverLappedSend.SetHandleIng(true);
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CConnectItemManager::CConnectItemManager()
{
	m_pConnectItemSink = NULL;
}

//��������
CConnectItemManager::~CConnectItemManager()
{
	__super::ReleaseNativeInfo(true);
}

//��ʼ���������
bool CConnectItemManager::Init(HANDLE hCompletionPort, IConnectItemSink * pConnectItemSink, WORD wMaxItemCount)
{
	m_pConnectItemSink = pConnectItemSink;
	__super::SetParameter(hCompletionPort, INDEX_CONNECT, wMaxItemCount);
	return true;
}

//�ͷŹ������
bool CConnectItemManager::Release()
{
	return __super::ReleaseNativeInfo(false);
}

//�������
DWORD CConnectItemManager::ActiveConnectItem(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//��ȡ����
	CConnectItem * pConnectItem = dynamic_cast<CConnectItem *>(__super::GetFreeNativeInfo());
	if (pConnectItem == NULL)
	{
		if (m_NativeInfoPtrList.size() < m_wMaxNativeItemCount)
		{
			pConnectItem = new(std::nothrow) CConnectItem((WORD)(m_NativeInfoPtrList.size() + m_wStartIndex), m_pConnectItemSink);
			if (pConnectItem)
			{
				//��ӵ����
				m_NativeInfoPtrList.push_back(pConnectItem);
			}
		}
		else
		{
			Logger_Error(TEXT("���Ӷ����Ѵ�����:%u"), m_wMaxNativeItemCount);
		}
	}

	//����У��
	if (pConnectItem == NULL)
	{
		Logger_Error(TEXT("���ӷ���[%s:%d]ʧ��=>�������Ӷ���ʧ��"), pszConnectAddress, wConnectPort);
		return INVALID_SOCKETID;
	}

	//Э������
	ProtocolSupport Protocol = EnableIPv4;
	if (wProtocol == EnableIPv6) Protocol = EnableIPv6;
	if (wProtocol == EnableBoth) Protocol = EnableBoth;

	//���ӷ���
	return pConnectItem->Connect(m_hCompletionPort, pszConnectAddress, wConnectPort, dwUserData, Protocol, bWatch);
}

//������
bool CConnectItemManager::DetectItem()
{
	CConnectItem * pConnectItem = NULL;
	//��������
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//����������,����Ҫ�����
		if (m_NativeInfoPtrList[i]->IsUsed() == false && m_NativeInfoPtrList[i]->GetDetect() == true)
		{
			pConnectItem = dynamic_cast<CConnectItem *>(m_NativeInfoPtrList[i]);
			pConnectItem->ResumeConnect(m_hCompletionPort);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
