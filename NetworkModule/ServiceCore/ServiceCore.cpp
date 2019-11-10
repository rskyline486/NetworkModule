#include "stdafx.h"
#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
COverLapped::COverLapped(enOperationType OperationType, DWORD dwBufferSize)
{
	//���ñ���
	ZeroMemory(&m_WSABuffer, sizeof(m_WSABuffer));
	ZeroMemory(&m_OverLapped, sizeof(m_OverLapped));
	m_OperationType = OperationType;
	m_bHandleIng = false;
	if (!m_Buffer.Init(dwBufferSize))
	{
		Logger_Error(TEXT("��ʼ���������ʧ��,dwBufferSize:%u"), dwBufferSize);
	}
}

//��������
COverLapped::~COverLapped()
{
	m_Buffer.Release();
}

//����ָ��
COverLapped::operator LPWSABUF()
{
	return &m_WSABuffer;
}

//�ص��ṹ
COverLapped::operator LPWSAOVERLAPPED()
{
	return &m_OverLapped;
}

//�����ַ
LPVOID COverLapped::GetBufferAddr()
{
	return m_Buffer.GetData();
}

//�����С
DWORD COverLapped::GetBufferSize()
{
	return m_Buffer.GetDataSize();
}

//��������
enOperationType COverLapped::GetOperationType()
{
	return m_OperationType;
}

//��������
void COverLapped::SetOperationType(enOperationType OperationType)
{
	m_OperationType = OperationType;
}

//���ñ�ʶ
void COverLapped::SetHandleIng(bool bHandleIng)
{
	m_bHandleIng = bHandleIng;
}

//��ȡ��ʶ
bool COverLapped::GetHandleIng()
{
	return m_bHandleIng;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
COverLappedSend::COverLappedSend() : COverLapped(enOperationType_Send, SOCKET_BUFFER)
{
}

//��������
COverLappedSend::~COverLappedSend()
{
}

//�ָ�����
bool COverLappedSend::ResumeBuffer()
{
	DWORD dwSendSize = m_Buffer.GetDataSize();
	if (dwSendSize == 0)
	{
		return false;
	}

	if (dwSendSize > SOCKET_BUFFER)
	{
		dwSendSize = SOCKET_BUFFER;
	}

	m_WSABuffer.len = dwSendSize;
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetData();

	return true;
}

//��������
void COverLappedSend::ResetData()
{
	m_Buffer.Init(SOCKET_BUFFER);
}

//��������
bool COverLappedSend::SendData(LPVOID lpData, DWORD dwDataSize)
{
	//����У��
	if (lpData == NULL || dwDataSize == 0)
	{
		return false;
	}

	//��ȡ����������β��ַ
	LPVOID lpBuffer = m_Buffer.GetDeliverData(dwDataSize);
	if (lpBuffer == NULL)
	{
		return false;
	}

	//��������
	memcpy(lpBuffer, lpData, dwDataSize);

	//���³���
	return m_Buffer.RecvSize(dwDataSize);
}

//�������
bool COverLappedSend::SendCompleted(DWORD dwSendSize)
{
	return m_Buffer.DealSize(dwSendSize);
}

//////////////////////////////////////////////////////////////////////////

//���캯��
COverLappedRecv::COverLappedRecv() : COverLapped(enOperationType_Recv, SOCKET_BUFFER)
{
}

//��������
COverLappedRecv::~COverLappedRecv()
{
}

//�ָ�����
bool COverLappedRecv::ResumeBuffer()
{
	m_WSABuffer.len = SOCKET_BUFFER;
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetDeliverData(m_WSABuffer.len);

	return (m_WSABuffer.buf != NULL);
}

//��������
void COverLappedRecv::ResetData()
{
	m_Buffer.Init(SOCKET_BUFFER);
}

//�������
bool COverLappedRecv::RecvCompleted(DWORD dwRecvSize)
{
	return m_Buffer.RecvSize(dwRecvSize);
}

//�������
bool COverLappedRecv::DealCompleted(DWORD dwDealSize)
{
	return m_Buffer.DealSize(dwDealSize);
}

//////////////////////////////////////////////////////////////////////////

//���캯��
COverLappedAccept::COverLappedAccept() : COverLapped(enOperationType_Accept, (sizeof(sockaddr_storage) + 16) * 2)
{
	//��������
	m_Buffer.Init((sizeof(sockaddr_storage) + 16) * 2);
	//��������
	m_Buffer.RecvSize((sizeof(sockaddr_storage) + 16) * 2);
	//���û���
	m_WSABuffer.len = m_Buffer.GetDataSize();
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetData();

	//���þ��
	m_hSocket = INVALID_SOCKET;
}

//��������
COverLappedAccept::~COverLappedAccept()
{
}

//�ָ�����
bool COverLappedAccept::ResumeBuffer()
{
	return true;
}

//��������
void COverLappedAccept::ResetData()
{
	//��������
	m_Buffer.Init((sizeof(sockaddr_storage) + 16) * 2);
	//��������
	m_Buffer.RecvSize((sizeof(sockaddr_storage) + 16) * 2);
	//���û���
	m_WSABuffer.len = m_Buffer.GetDataSize();
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetData();
}

//�洢����
bool COverLappedAccept::StoreSocket(SOCKET hSocket)
{
	if (m_hSocket != INVALID_SOCKET)
	{
		return false;
	}

	m_hSocket = hSocket;
	return true;
}

//ȡ������
SOCKET COverLappedAccept::GetOutSocket()
{
	SOCKET hSocket = m_hSocket;
	m_hSocket = INVALID_SOCKET;
	return hSocket;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CNativeInfo::CNativeInfo(WORD wIndex)
{
	m_dwActiveTime = 0L;
	m_dwRealIPV4 = 0L;

	m_dwUserData = 0L;
	m_hSocket = INVALID_SOCKET;
	m_wIndex = wIndex;
	m_wRountID = 1;
	m_bUsed = false;
	m_bDetect = false;
	m_bShutDown = false;
}

//��������
CNativeInfo::~CNativeInfo(void)
{
}

//���ü��
bool CNativeInfo::SetDetect(WORD wRountID, bool bDetect)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//״̬�ж�
	if (m_wRountID != wRountID) return false;

	if (bDetect)
	{
		//���ü��
		SetDetect();
	}
	else
	{
		//ȡ�����
		CancelDetect();
	}

	return true;
}

//��������
bool CNativeInfo::SetUserData(WORD wRountID, DWORD dwUserData)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//״̬�ж�
	if (m_wRountID != wRountID) return false;

	m_dwUserData = dwUserData;

	return true;
}

//���ùر�
bool CNativeInfo::ShutDownSocket(WORD wRountID)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//״̬�ж�
	if (m_wRountID != wRountID) return false;

	//���ñ�ʶ
	m_bShutDown = true;

	return true;
}

//��ȡ����
DWORD CNativeInfo::GetUserData()
{
	//ͬ������
	//�˴��������,����ȷ���˴˴��ĵ��ò���Ϊ�ص��߳��ڲ�
	//�ص��߳��ڲ��Ѿ�����������,�˴��ٴ�����,���ޱ�Ҫ
	//�ϸ�����˵,Ӧ�ü���(ͬ�߳��ڶ�μ���,ֻ���������ü���,��Ӧ�ͷż���)
	//CLocker Locker(m_Mutex);

	return m_dwUserData;
}

//���ü��
void CNativeInfo::SetDetect()
{
	m_bDetect = true;
}

//ȡ�����
void CNativeInfo::CancelDetect()
{
	m_bDetect = false;
}

//��ȡ���
bool CNativeInfo::GetDetect()
{
	return m_bDetect;
}

//�ж�ʹ��
bool CNativeInfo::IsUsed()
{
	//bool����Ϊԭ�ӱ���,�������м�ֵ
	//ֻҪȷ����ֵ��ʱ��,���ɲ��ؼ���

	return m_bUsed;
}

//��ȡ����
WORD CNativeInfo::GetIndex()
{
	return m_wIndex;
}

//��ȡ����
WORD CNativeInfo::GetRountID()
{
	return m_wRountID;
}

//��ȡ��ʶ
DWORD CNativeInfo::GetSocketID()
{
	return MAKELONG(m_wIndex, m_wRountID);
}

//����ʱ��
DWORD CNativeInfo::GetActiveTime()
{
	return m_dwActiveTime;
}

//��ȡ��ַ
bool CNativeInfo::GetLocalAddress(TCHAR * pszBuffer, DWORD dwBufferLength, LPWORD pwPort)
{
	*pwPort = CNetwork::GetPort(m_LocalAddress);
	return CNetwork::GetIP(m_LocalAddress, pszBuffer, dwBufferLength);
}

//��ȡ��ַ
bool CNativeInfo::GetRemoteAddress(TCHAR * pszBuffer, DWORD dwBufferLength, LPWORD pwPort)
{
	*pwPort = CNetwork::GetPort(m_RemoteAddress);
	return CNetwork::GetIP(m_RemoteAddress, pszBuffer, dwBufferLength);
}

//��ȡIPV4
DWORD CNativeInfo::GetClientIPV4()
{
	if (m_dwRealIPV4 > 0L) return m_dwRealIPV4;
	return m_RemoteAddress.saIn.sin_addr.S_un.S_addr;
}

//��������
void CNativeInfo::InitData()
{
	//����ʹ��
	m_bUsed = true;
	//���ñ�ʶ
	m_bShutDown = false;
	//��¼ʱ��(��1970��1��1��0ʱ0��0�뵽���ڵ�����)(������������п��ܳ�����Χ)
	m_dwActiveTime = (DWORD)time(NULL);
	//��ȡ��ַ
	m_LocalAddress = CNetwork::GetLocalAddress(m_hSocket);
	m_RemoteAddress = CNetwork::GetRemoteAddress(m_hSocket);
	m_dwRealIPV4 = 0L;
}

//�����ж�
bool CNativeInfo::SendVerdict(WORD wRountID)
{
	//״̬�ж�
	if ((m_wRountID != wRountID) || (m_bShutDown == true)) return false;
	if (m_hSocket == INVALID_SOCKET) return false;

	return true;
}

//��������
void CNativeInfo::ResetData()
{
	m_dwActiveTime = 0L;
	//���õ�ַ
	memset(&m_LocalAddress.saStorage, 0, sizeof(sockaddr_storage));
	memset(&m_RemoteAddress.saStorage, 0, sizeof(sockaddr_storage));
	m_dwRealIPV4 = 0L;
	//�����û�����
	if (m_bDetect == false) m_dwUserData = 0L;
	//���þ��
	m_hSocket = INVALID_SOCKET;
	//�ָ�����(����д����bug,��ѭ������֮��,���ǻ��Ϊ0)
	//m_wRountID = max(1, m_wRountID + 1);
	m_wRountID = m_wRountID + 1;
	if (m_wRountID == 0) m_wRountID = 1;
	//���ñ�ʶ
	m_bShutDown = false;
	//���ÿ���
	m_bUsed = false;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CNativeInfoManager::CNativeInfoManager()
{
	m_hCompletionPort = NULL;
	m_wStartIndex = 0;
	m_wMaxNativeItemCount = MAX_SOCKET;
	ZeroMemory(&m_HeartbeatPacket, sizeof(m_HeartbeatPacket));
}

//��������
CNativeInfoManager::~CNativeInfoManager()
{
}

//��ȡ��С
WORD CNativeInfoManager::GetTotalNativeCount()
{
	//ͬ������
	CLocker Locker(m_Mutex);
	return (WORD)m_NativeInfoPtrList.size();
}

//��ȡ��С
WORD CNativeInfoManager::GetActiveNativeCount()
{
	//�����
	WORD wActiveCount = 0;

	//��������
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		if (m_NativeInfoPtrList[i]->IsUsed() == true)
		{
			wActiveCount++;
		}
	}

	return wActiveCount;
}

//������������
bool CNativeInfoManager::SetHeartbeatData(VOID * pData, WORD wDataSize)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//����Ѿ�������,�����޸�����
	if (m_NativeInfoPtrList.size() > 0)
	{
		return false;
	}

	//����У��
	if (pData == NULL || wDataSize == 0)
	{
		ZeroMemory(&m_HeartbeatPacket, sizeof(m_HeartbeatPacket));
		m_HeartbeatPacket.wDataSize = 0;
		return true;
	}

	//��СУ��
	if (wDataSize > sizeof(m_HeartbeatPacket.cbSendBuffer))
	{
		return false;
	}

	//��������
	CopyMemory(m_HeartbeatPacket.cbSendBuffer, pData, wDataSize);
	m_HeartbeatPacket.wDataSize = wDataSize;

	return true;
}

//��������
bool CNativeInfoManager::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	//��ȡ����
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//��ΧУ��
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->SendData(pData, wDataSize, wRountID);
	}

	return false;
}

//Ⱥ������
bool CNativeInfoManager::SendDataBatch(VOID * pData, WORD wDataSize)
{
	//��������
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		if (m_NativeInfoPtrList[i]->IsUsed() == false) continue;
		m_NativeInfoPtrList[i]->SendData(pData, wDataSize, m_NativeInfoPtrList[i]->GetRountID());
	}

	return true;
}

//���ü��
bool CNativeInfoManager::SetDetect(DWORD dwSocketID, bool bDetect)
{
	//��ȡ����
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//��ΧУ��
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->SetDetect(wRountID, bDetect);
	}

	return false;
}

//��������
bool CNativeInfoManager::SetUserData(DWORD dwSocketID, DWORD dwUserData)
{
	//��ȡ����
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//��ΧУ��
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->SetUserData(wRountID, dwUserData);
	}

	return false;
}

//���ùر�
bool CNativeInfoManager::ShutDownSocket(DWORD dwSocketID)
{
	//��ȡ����
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//��ΧУ��
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->ShutDownSocket(wRountID);
	}

	return false;
}

//��ȡ��ַ
DWORD CNativeInfoManager::GetClientIP(DWORD dwSocketID)
{
	//��ȡ����
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//��ΧУ��
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->GetClientIPV4();
	}

	return 0L;
}

//�ر�����
bool CNativeInfoManager::CloseSocket(DWORD dwSocketID)
{
	//��ȡ����
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//��ΧУ��
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->CloseSocket(wRountID);
	}

	return false;
}

//��ȡ����
CNativeInfo * CNativeInfoManager::GetFreeNativeInfo()
{
	//��������
	WORD wNativeInfoCount = (WORD)m_NativeInfoPtrList.size();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//����������,�Ҳ���Ҫ�����
		if (m_NativeInfoPtrList[i]->IsUsed() == false && m_NativeInfoPtrList[i]->GetDetect() == false)
		{
			return m_NativeInfoPtrList[i];
		}
	}

	return NULL;
}

//��ȡ����
WORD CNativeInfoManager::GetIndex(DWORD dwSocketID)
{
	return LOWORD(dwSocketID) - m_wStartIndex;
}

//��ȡ����
WORD CNativeInfoManager::GetRountID(DWORD dwSocketID)
{
	return HIWORD(dwSocketID);
}

//���ò���
bool CNativeInfoManager::SetParameter(HANDLE hCompletionPort, WORD wStartIndex, WORD wMaxNativeItemCount)
{
	m_hCompletionPort = hCompletionPort;
	m_wStartIndex = wStartIndex;
	m_wMaxNativeItemCount = wMaxNativeItemCount;
	
	//��������
	ZeroMemory(&m_HeartbeatPacket, sizeof(m_HeartbeatPacket));
	m_HeartbeatPacket.wDataSize = 0;

	return true;
}

//�ͷ�����
bool CNativeInfoManager::ReleaseNativeInfo(bool bFreeMemroy)
{
	if (bFreeMemroy == true)
	{
		//�ͷ�����
		WORD wNativeInfoCount = (WORD)m_NativeInfoPtrList.size();
		for (WORD i = 0; i < wNativeInfoCount; i++)
		{
			m_NativeInfoPtrList[i]->CloseSocket(m_NativeInfoPtrList[i]->GetRountID());
			delete m_NativeInfoPtrList[i];
			m_NativeInfoPtrList[i] = NULL;
		}
		m_NativeInfoPtrList.clear();
	}
	else
	{
		//�ر�����
		WORD wNativeInfoCount = (WORD)m_NativeInfoPtrList.size();
		for (WORD i = 0; i < wNativeInfoCount; i++)
		{
			m_NativeInfoPtrList[i]->CloseSocket(m_NativeInfoPtrList[i]->GetRountID());
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CServiceCore::CServiceCore()
{
}

//��������
CServiceCore::~CServiceCore()
{
}

bool CServiceCore::W2A(CHAR * pszBuffer, DWORD dwBufferLength, LPCWSTR pszSource)
{
	_snprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, "%ws", pszSource);
	return true;
}

bool CServiceCore::A2W(WCHAR * pszBuffer, DWORD dwBufferLength, LPCSTR pszSource)
{
	_snwprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, L"%hs", pszSource);
	return true;
}

bool CServiceCore::T2A(CHAR * pszBuffer, DWORD dwBufferLength, LPCTSTR pszSource)
{
#ifdef _UNICODE
	_snprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, "%ws", pszSource);
#else
	_snprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, "%s", pszSource);
#endif // _UNICODE
	return true;
}

bool CServiceCore::A2T(TCHAR * pszBuffer, DWORD dwBufferLength, LPCSTR pszSource)
{
	_sntprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, TEXT("%hs"), pszSource);
	return true;
}

//////////////////////////////////////////////////////////////////////////
