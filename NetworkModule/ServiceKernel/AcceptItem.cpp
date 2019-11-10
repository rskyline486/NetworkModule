#include "stdafx.h"
#include "AcceptItem.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CAcceptItem::CAcceptItem(WORD wIndex, IAcceptItemSink * pAcceptItemSink)
	: CNativeInfo(wIndex), m_pAcceptItemSink(pAcceptItemSink)
{
	m_wListenPort = 9999;
	m_Protocol = EnableIPv4;
	m_fnAcceptEx = NULL;
}

//��������
CAcceptItem::~CAcceptItem(void)
{
}

//��������
DWORD CAcceptItem::Listen(HANDLE hCompletionPort, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch)
{
	//У�����
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("��ɶ˿ڶ���Ϊ��"));
		return INVALID_SOCKETID;
	}

	//��¼����
	m_wListenPort = wPort;
	m_Protocol = Protocol;

	//��ȡ��ַ
	const_cast<Address&>(m_LocalAddress) = CNetwork::GetAddress(NULL, wPort, Protocol, true);
	if (CNetwork::IsValidAddress(m_LocalAddress) == false)
	{
		Logger_Error(TEXT("��ȡ��ַʧ��,������:%d"), WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//����SOCKET
	m_hSocket = CNetwork::CreateSocket(m_LocalAddress, Protocol);
	if (m_hSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("����SOCKETʧ��,������:%d"), WSAGetLastError());
		return INVALID_SOCKETID;
	}

	//��ȡAcceptEx������ַ
	GUID GuidAcceptEx = WSAID_ACCEPTEX;
	DWORD dwBytes;
	if (WSAIoctl(m_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidAcceptEx, sizeof(GuidAcceptEx), &m_fnAcceptEx, sizeof(m_fnAcceptEx), &dwBytes, NULL, NULL) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("��ȡAcceptEx������ַʧ��,������:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		return INVALID_SOCKETID;
	}

	//�󶨵�ַ
	const_cast<Address&>(m_LocalAddress) = CNetwork::Bind(m_hSocket, m_LocalAddress);
	if (CNetwork::IsValidAddress(m_LocalAddress) == false)
	{
		Logger_Error(TEXT("�󶨶˿�ʧ��,�˿ں�:%u,������:%d"), wPort, WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		m_fnAcceptEx = NULL;
		return INVALID_SOCKETID;
	}

	//�����˿�
	if (CNetwork::Listen(m_hSocket, 200) == false)
	{
		Logger_Error(TEXT("�����˿�ʧ��,�˿ں�:%u,������:%d"), wPort, WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		m_fnAcceptEx = NULL;
		return INVALID_SOCKETID;
	}

	//�󶨵���ɶ˿�
	if (CreateIoCompletionPort((HANDLE)m_hSocket, hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		Logger_Error(TEXT("�󶨵���ɶ˿�ʧ��,������:%d"), WSAGetLastError());
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
		m_fnAcceptEx = NULL;
		return INVALID_SOCKETID;
	}

	//��������
	__super::InitData();
	//�û�����
	m_dwUserData = dwUserData;
	//�����ػ�
	if (bWatch) __super::SetDetect();

	//�¼�֪ͨ
	m_pAcceptItemSink->OnEventListenStart(this);

	//ͬ������
	//��ֹPostAccept����Ͷ�ݽ��պ�,��δ���ü����ý��ձ�ʶ,�ʹ����˽�����ɻص�
	//��ɻص����ý��ձ�ʶ��,PostAccept�������������ý��ձ�ʶ
	//�Ӷ����º���Ͷ�ݽ�������ʱ��������Ϊ��һ�ν��ջ�δ����
	//��˳����ȴ�����,�������¸ñ�ʶ��Զ�����ᱻ����
	CLocker Locker(m_Mutex);

	//Ͷ������
	PostAccept();

	return __super::GetSocketID();
}

//�ָ�����
DWORD CAcceptItem::ResumeListen(HANDLE hCompletionPort)
{
	//�����ж�
	if (__super::IsUsed()) return __super::GetSocketID();

	//����У��
	if (hCompletionPort == NULL) return INVALID_SOCKETID;

	return Listen(hCompletionPort, m_wListenPort, m_dwUserData, m_Protocol, true);
}

//��������
void CAcceptItem::DealAsync(COverLapped * pOverLapped, DWORD dwThancferred)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//���ñ�ʶ
	pOverLapped->SetHandleIng(false);

	//��ȡ���
	SOCKET hAcceptSocket = m_OverLappedAccept.GetOutSocket();

	//�жϹر�
	if (m_hSocket == INVALID_SOCKET || hAcceptSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("SOCKET�����Ч:m_hSocket:[0x%p],hAcceptSocket:[0x%p],�رռ���"), m_hSocket, hAcceptSocket);
		CloseSocket();
		return;
	}

	//����У��
	if (static_cast<int>(dwThancferred) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("�����쳣,�رռ���"));
		CloseSocket();
		return;
	}

	//��������
	if (setsockopt(hAcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (CHAR*)&hAcceptSocket, sizeof(hAcceptSocket)) == SOCKET_ERROR)
	{
		Logger_Error(TEXT("��������[SO_UPDATE_ACCEPT_CONTEXT]ʧ��,������:%d,�رռ���"), WSAGetLastError());
		CloseSocket();
		return;
	}

	//�¼�֪ͨ
	m_pAcceptItemSink->OnEventListenAccept(this, hAcceptSocket);

	//Ͷ������
	PostAccept();
}

//���ͺ���
bool CAcceptItem::SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID)
{
	//����У��
	if (lpData == NULL || dwDataSize == 0) return false;

	//ͬ������
	CLocker Locker(m_Mutex);

	//�����ж�
	if (SendVerdict(wRountID) == false) return false;

	return false;
}

//�ر�����
bool CAcceptItem::CloseSocket(WORD wRountID)
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
void CAcceptItem::CloseSocket()
{
	//�رռ���
	if (m_hSocket != INVALID_SOCKET)
	{
		CNetwork::CloseSocket(m_hSocket);
		m_hSocket = INVALID_SOCKET;
	}

	//�ر�����
	SOCKET hAcceptSocket = m_OverLappedAccept.GetOutSocket();
	if (hAcceptSocket != INVALID_SOCKET)
	{
		CNetwork::CloseSocket(hAcceptSocket);
		hAcceptSocket = INVALID_SOCKET;
	}

	//�жϹر�
	if (m_OverLappedAccept.GetHandleIng() == false)
	{
		if (__super::IsUsed())
		{
			//�¼�֪ͨ
			m_pAcceptItemSink->OnEventListenStop(this);
			//�ָ�����
			ResumeData();
		}
	}
}

//�ָ�����
void CAcceptItem::ResumeData()
{
	//���ýӿ�
	m_fnAcceptEx = NULL;
	//��������
	m_OverLappedAccept.ResetData();
	//��������
	__super::ResetData();
}

//Ͷ������
void CAcceptItem::PostAccept()
{
	//Ͷ��У��
	if (m_OverLappedAccept.GetHandleIng() == true)
	{
		//Logger_Error(TEXT("�Ѵ���Ͷ������"));
		return;
	}

	//�ָ�����
	if (!m_OverLappedAccept.ResumeBuffer())
	{
		Logger_Error(TEXT("�����ص�����ʧ��,�رռ���"));
		CloseSocket();
		return;
	}

	//��������
	SOCKET hAcceptSocket = CNetwork::CreateSocket(m_LocalAddress);
	if (hAcceptSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("�������Ӷ���ʧ��,hAcceptSocket:%u,������:%d,�رռ���"), hAcceptSocket, WSAGetLastError());
		CloseSocket();
		return;
	}

	//�洢����
	if (!m_OverLappedAccept.StoreSocket(hAcceptSocket))
	{
		Logger_Error(TEXT("�洢����ʧ��???"));
		//ȡ�����ͷ�����(�����ϲ����ߵ�����)
		SOCKET hTempSocket = m_OverLappedAccept.GetOutSocket();
		if (hTempSocket)
		{
			CNetwork::CloseSocket(hTempSocket);
			hTempSocket = INVALID_SOCKET;
		}
		m_OverLappedAccept.StoreSocket(hAcceptSocket);
	}

	//��������
	DWORD dwRecv = 0;
	DWORD dwLocalAddrSize = m_OverLappedAccept.GetBufferSize() / 2;
	DWORD dwRemoteAddrSize = m_OverLappedAccept.GetBufferSize() / 2;
	if (!m_fnAcceptEx(m_hSocket, hAcceptSocket, m_OverLappedAccept.GetBufferAddr(), 0, dwLocalAddrSize, dwRemoteAddrSize, &dwRecv, m_OverLappedAccept))
	{
		if (!CNetwork::WouldBlock())
		{
			Logger_Error(TEXT("AcceptEx����ʧ��,������:%d,�رռ���"), WSAGetLastError());
			CloseSocket();
			return;
		}
	}

	//���ñ�ʶ
	m_OverLappedAccept.SetHandleIng(true);
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CAcceptItemManager::CAcceptItemManager()
{
	m_pAcceptItemSink = NULL;
}

//��������
CAcceptItemManager::~CAcceptItemManager()
{
	__super::ReleaseNativeInfo(true);
}

//��ʼ���������
bool CAcceptItemManager::Init(HANDLE hCompletionPort, IAcceptItemSink * pAcceptItemSink, WORD wMaxItemCount)
{
	m_pAcceptItemSink = pAcceptItemSink;
	__super::SetParameter(hCompletionPort, INDEX_LISTEN, wMaxItemCount);
	return true;
}

//�ͷŹ������
bool CAcceptItemManager::Release()
{
	return __super::ReleaseNativeInfo(false);
}

//�������
DWORD CAcceptItemManager::ActiveAcceptItem(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//��ȡ����
	CAcceptItem * pAcceptItem = dynamic_cast<CAcceptItem *>(__super::GetFreeNativeInfo());
	if (pAcceptItem == NULL)
	{
		if (m_NativeInfoPtrList.size() < m_wMaxNativeItemCount)
		{
			pAcceptItem = new(std::nothrow) CAcceptItem((WORD)(m_NativeInfoPtrList.size() + m_wStartIndex), m_pAcceptItemSink);
			if (pAcceptItem)
			{
				//��ӵ����
				m_NativeInfoPtrList.push_back(pAcceptItem);
			}
		}
		else
		{
			Logger_Error(TEXT("���������Ѵ�����:%u"), m_wMaxNativeItemCount);
		}
	}

	//����У��
	if (pAcceptItem == NULL)
	{
		Logger_Error(TEXT("�����˿�[%d]ʧ��=>�����������ʧ��"), wListenPort);
		return INVALID_SOCKETID;
	}

	//Э������
	ProtocolSupport Protocol = EnableIPv4;
	if (wProtocol == EnableIPv6) Protocol = EnableIPv6;
	if (wProtocol == EnableBoth) Protocol = EnableBoth;

	//��������
	return pAcceptItem->Listen(m_hCompletionPort, wListenPort, dwUserData, Protocol, bWatch);
}

//������
bool CAcceptItemManager::DetectItem()
{
	CAcceptItem * pAcceptItem = NULL;
	//��������
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//����������,����Ҫ�����
		if (m_NativeInfoPtrList[i]->IsUsed() == false && m_NativeInfoPtrList[i]->GetDetect() == true)
		{
			pAcceptItem = dynamic_cast<CAcceptItem *>(m_NativeInfoPtrList[i]);
			pAcceptItem->ResumeListen(m_hCompletionPort);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
