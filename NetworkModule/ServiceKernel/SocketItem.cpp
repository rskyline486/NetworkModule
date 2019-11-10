#include "stdafx.h"
#include "SocketItem.h"

//////////////////////////////////////////////////////////////////////////

//ϵ������
#define DEAD_QUOTIETY				0									//����ϵ��
#define DANGER_QUOTIETY				1									//Σ��ϵ��
#define SAFETY_QUOTIETY				2									//��ȫϵ��

//////////////////////////////////////////////////////////////////////////

//���캯��
CSocketItem::CSocketItem(WORD wIndex, ISocketItemSink * pSocketItemSink)
	: CNativeInfo(wIndex), m_pSocketItemSink(pSocketItemSink)
{
	m_dwRecvDataCount = 0L;
	m_dwRecvDataCount = 0L;
	m_wSurvivalTime = 0;
}

//��������
CSocketItem::~CSocketItem(void)
{
}

//�󶨶���
DWORD CSocketItem::Attach(HANDLE hCompletionPort, SOCKET hSocket)
{
	//����У��
	if (hSocket == INVALID_SOCKET)
	{
		Logger_Error(TEXT("���Ӿ����Ч"));
		return INVALID_SOCKETID;
	}
	if (hCompletionPort == NULL)
	{
		Logger_Error(TEXT("��ɶ˿ڶ���Ϊ��"));
		CNetwork::CloseSocket(hSocket);
		return INVALID_SOCKETID;
	}

	//�󶨵���ɶ˿�
	if (CreateIoCompletionPort((HANDLE)hSocket, hCompletionPort, (ULONG_PTR)this, 0) == NULL)
	{
		Logger_Error(TEXT("�󶨵���ɶ˿�ʧ��,������:%d"), WSAGetLastError());
		CNetwork::CloseSocket(hSocket);
		return INVALID_SOCKETID;
	}

	//���ñ���
	m_hSocket = hSocket;
	m_wSurvivalTime = SAFETY_QUOTIETY;

	//��������
	__super::InitData();
	//���ü��
	__super::SetDetect();

	//����֪ͨ
	m_pSocketItemSink->OnEventSocketBind(this);

	//ͬ������
	//��ֹPostRecv����Ͷ�ݽ��պ�,��δ���ü����ý��ձ�ʶ,�ʹ����˽�����ɻص�
	//��ɻص����ý��ձ�ʶ��,PostRecv�������������ý��ձ�ʶ
	//�Ӷ����º���Ͷ�ݽ�������ʱ��������Ϊ��һ�ν��ջ�δ����
	//��˳����ȴ�����,�������¸ñ�ʶ��Զ�����ᱻ����
	CLocker Locker(m_Mutex);

	//Ͷ������
	PostRecv();

	return __super::GetSocketID();
}

//��ȡ����
DWORD CSocketItem::GetRecvDataCount()
{
	return InterlockedExchangeAdd(&m_dwRecvDataCount, 0);
}

//��ȡ����
DWORD CSocketItem::GetSendDataCount()
{
	return InterlockedExchangeAdd(&m_dwSendDataCount, 0);
}

//�������
bool CSocketItem::Heartbeat(DWORD dwNowTime)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//�����ж�
	if (m_dwRecvDataCount > 0)
	{
		if (m_wSurvivalTime == 0)
		{
			Logger_Debug(TEXT("��ʱ�䲻����,�ر�����[dwSocketID:%u, RecvIng:%u, SendIng:%u][wIndex:%u, wRountID:%u, bUsed:%u, bDetect:%u, bShutDown:%u]")
				, GetSocketID(), m_OverLappedRecv.GetHandleIng(), m_OverLappedSend.GetHandleIng(), m_wIndex, m_wRountID, m_bUsed, m_bDetect, m_bShutDown);
			CloseSocket();
			return false;
		}
	}
	else
	{
		//�رղ���������
		if ((m_dwActiveTime + 4) <= dwNowTime)
		{
			Logger_Debug(TEXT("���Ӳ�����,�ر�����[dwSocketID:%u, RecvIng:%u, SendIng:%u][wIndex:%u, wRountID:%u, bUsed:%u, bDetect:%u, bShutDown:%u]")
				, GetSocketID(), m_OverLappedRecv.GetHandleIng(), m_OverLappedSend.GetHandleIng(), m_wIndex, m_wRountID, m_bUsed, m_bDetect, m_bShutDown);
			CloseSocket();
			return false;
		}
	}

	//���ݼ�
	m_wSurvivalTime--;
	return (m_wSurvivalTime == 0);
}

//��������
void CSocketItem::DealAsync(COverLapped * pOverLapped, DWORD dwThancferred)
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
	if (static_cast<int>(dwThancferred) == SOCKET_ERROR || dwThancferred == 0)
	{
		Logger_Debug(TEXT("�����쳣,�ر�����"));
		CloseSocket();
		return;
	}

	switch (pOverLapped->GetOperationType())
	{
	case enOperationType_Recv:
		{
			//���ճ���
			if (m_OverLappedRecv.RecvCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("�������ݴ���ʧ��,dwThancferred:%u,�ر�����"), dwThancferred);
				CloseSocket();
				return;
			}

			//�ж��ж�
			if (m_bShutDown == true)
			{
				//��������
				m_OverLappedRecv.DealCompleted(pOverLapped->GetBufferSize());

				//Ͷ������
				//PostRecv();
				return;
			}

			//������ʵIP
			if (m_dwRecvDataCount == 0L && m_dwRealIPV4 == 0L)
			{
				//���Դ�nginx���ݰ��н������û���ʵIP��Ϣ
				DWORD dwNginxPacketSize = PraseClientIP((CHAR *)pOverLapped->GetBufferAddr(), pOverLapped->GetBufferSize());
				if (dwNginxPacketSize > 0L)
				{
					//��������
					m_OverLappedRecv.DealCompleted(dwNginxPacketSize);

					//У������
					if (pOverLapped->GetBufferSize() == 0)
					{
						m_wSurvivalTime = SAFETY_QUOTIETY;

						//Ͷ������
						PostRecv();
						return;
					}
				}
			}

			//��ȡ����
			DWORD dwDealSize = m_pSocketItemSink->OnEventSocketRead(this, pOverLapped->GetBufferAddr(), pOverLapped->GetBufferSize());
			if (static_cast<int>(dwDealSize) == SOCKET_ERROR || m_OverLappedRecv.DealCompleted(dwDealSize) == false)
			{
				Logger_Debug(TEXT("�������ݴ���ʧ��,dwDealSize:%u,�ر�����"), dwDealSize);
				CloseSocket();
				return;
			}

			//��������(ԭ�Ӳ���)
			InterlockedExchangeAdd(&m_dwRecvDataCount, dwDealSize);
			m_wSurvivalTime = SAFETY_QUOTIETY;

			//Ͷ������
			PostRecv();
		}
		break;
	case enOperationType_Send:
		{
			//���ͳ���
			if (m_OverLappedSend.SendCompleted(dwThancferred) == false)
			{
				Logger_Debug(TEXT("�������ݴ���ʧ��,dwThancferred:%u,�ر�����"), dwThancferred);
				CloseSocket();
				return;
			}

			//��������(ԭ�Ӳ���)
			InterlockedExchangeAdd(&m_dwSendDataCount, dwThancferred);
			m_wSurvivalTime = SAFETY_QUOTIETY;

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
bool CSocketItem::SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID)
{
	//����У��
	if (lpData == NULL || dwDataSize == 0) return false;

	//ͬ������
	CLocker Locker(m_Mutex);

	//�����ж�
	if (SendVerdict(wRountID) == false) return false;

	//��������
	if (m_OverLappedSend.SendData(lpData, dwDataSize) == false)
	{
		Logger_Error(TEXT("����ʧ��,��������ʧ��,dwDataSize:%u"), dwDataSize);
		return false;
	}

	//Ͷ�ݷ���
	PostSend();

	return true;
}

//�ر�����
bool CSocketItem::CloseSocket(WORD wRountID)
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
void CSocketItem::CloseSocket()
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
			m_pSocketItemSink->OnEventSocketShut(this);
			//�ָ�����
			ResumeData();
		}
	}
}

//�ָ�����
void CSocketItem::ResumeData()
{
	//��������
	m_OverLappedRecv.ResetData();
	//��������
	m_OverLappedSend.ResetData();
	//���ý���
	m_dwRecvDataCount = 0L;
	//���÷���
	m_dwSendDataCount = 0L;
	//���ô��
	m_wSurvivalTime = 0;
	//ȡ�����
	__super::CancelDetect();
	//��������
	__super::ResetData();
}

//Ͷ������
void CSocketItem::PostRecv()
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
void CSocketItem::PostSend()
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

//������ַ(��������Ҫճ�������,������ݰ����Ȳ���,�����nginx����Э����Ϣ)
DWORD CSocketItem::PraseClientIP(CHAR * pszBuffer, DWORD dwBufferSize)
{
	//����У��
	if (m_dwRecvDataCount != 0L || m_dwRealIPV4 != 0L) return 0L;

	DWORD dwReadSize = 0;
	//Logger_Debug(TEXT("���Դ����ݰ��н�����nginx����IP��Ϣ[dwSocketID:%u,wIndex:%u,wRountID:%u],���ݰ�����:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
	//����Э��
	if ((dwBufferSize - dwReadSize) >= 6 && memcmp("PROXY ", &pszBuffer[dwReadSize], 6) == 0)
	{
		dwReadSize += 6;
		//IPV4
		if ((dwBufferSize - dwReadSize) >= 5 && memcmp("TCP4 ", &pszBuffer[dwReadSize], 5) == 0)
		{
			dwReadSize += 5;
			//����У��
			if ((dwBufferSize - dwReadSize) >= 15)
			{
				CHAR szIPInfo[16] = { 0 };
				int nDotCount = 0;
				for (int i = 0; i < 15; i++)
				{
					if (pszBuffer[dwReadSize + i] == ' ') break;
					if (pszBuffer[dwReadSize + i] != '.' && (pszBuffer[dwReadSize + i] < '0' || pszBuffer[dwReadSize + i] > '9')) break;
					szIPInfo[i] = pszBuffer[dwReadSize + i];
					if (pszBuffer[dwReadSize + i] == '.') nDotCount++;
				}

				if (nDotCount == 3)
				{
					if (inet_pton(AF_INET, szIPInfo, &m_dwRealIPV4) <= 0)
					{
						//Logger_Debug(TEXT("nginx����Э��,������ʵIPʧ��,szIPInfo:%hs"), szIPInfo);
						m_dwRealIPV4 = 0L;
					}
					else
					{
						//Logger_Debug(TEXT("nginx����Э��,������ʵIP�ɹ�,szIPInfo:%hs"), szIPInfo);
					}
				}
			}
		}
		else
		{
			//Logger_Debug(TEXT("ò��nginx���ݰ���IP��Ϣ����IPV4[dwSocketID:%u,wIndex:%u,wRountID:%u],���ݰ�����:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
		}

		//���ֽ�
		CHAR * pszEnd = strstr(pszBuffer, "\r\n");
		if (pszEnd)
		{
			//����nginxIP���ݰ�����
			dwReadSize = (pszEnd - pszBuffer) + 2;

			//����У��
			if (dwBufferSize < dwReadSize)
			{
				//Logger_Debug(TEXT("ò�����ݰ��а���nginx���ݰ���Ϣ,�����ݰ������쳣[dwSocketID:%u,wIndex:%u,wRountID:%u],���ݰ�����:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
				return 0L;
			}

			return dwReadSize;
		}
		else
		{
			//Logger_Debug(TEXT("ò��nginx���ݰ���������Э���쳣[dwSocketID:%u,wIndex:%u,wRountID:%u],���ݰ�����:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
		}
	}
	else
	{
		//Logger_Debug(TEXT("ò�����ݰ��в���nginx����IP��Ϣ[dwSocketID:%u,wIndex:%u,wRountID:%u],���ݰ�����:%u"), GetSocketID(), m_wIndex, m_wRountID, dwBufferSize);
	}

	return 0L;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CSocketItemManager::CSocketItemManager()
{
	m_pSocketItemSink = NULL;
}

//��������
CSocketItemManager::~CSocketItemManager()
{
	__super::ReleaseNativeInfo(true);
}

//��ʼ���������
bool CSocketItemManager::Init(HANDLE hCompletionPort, ISocketItemSink * pSocketItemSink, WORD wMaxItemCount)
{
	m_pSocketItemSink = pSocketItemSink;
	__super::SetParameter(hCompletionPort, INDEX_SOCKET, wMaxItemCount);
	return true;
}

//�ͷŹ������
bool CSocketItemManager::Release()
{
	return __super::ReleaseNativeInfo(false);
}

//�������
DWORD CSocketItemManager::ActiveSocketItem(SOCKET hSocket)
{
	//ͬ������
	CLocker Locker(m_Mutex);

	//��ȡ����
	CSocketItem * pSocketItem = dynamic_cast<CSocketItem *>(__super::GetFreeNativeInfo());
	if (pSocketItem == NULL)
	{
		if (m_NativeInfoPtrList.size() < m_wMaxNativeItemCount)
		{
			pSocketItem = new(std::nothrow) CSocketItem((WORD)(m_NativeInfoPtrList.size() + m_wStartIndex), m_pSocketItemSink);
			if (pSocketItem)
			{
				//��ӵ����
				m_NativeInfoPtrList.push_back(pSocketItem);
			}
		}
		else
		{
			Logger_Error(TEXT("��������Ѵ�����:%u"), m_wMaxNativeItemCount);
		}
	}

	//����У��
	if (pSocketItem == NULL)
	{
		//��ȡ���ص�ַ
		Address LocalAddr = CNetwork::GetLocalAddress(hSocket);
		TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 };
		WORD wLocalPort = CNetwork::GetPort(LocalAddr);
		CNetwork::GetIP(LocalAddr, szLocalAddress, CountArray(szLocalAddress));
		//��ȡ�Զ˵�ַ
		Address RemoteAddr = CNetwork::GetRemoteAddress(hSocket);
		TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 };
		WORD wRemotePort = CNetwork::GetPort(RemoteAddr);
		CNetwork::GetIP(RemoteAddr, szRemoteAddress, CountArray(szRemoteAddress));
		Logger_Error(TEXT("�����������ʧ��, ���ص�ַ:%s:%d, �Զ˵�ַ:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
		//�������Ӷ���ʧ��
		CNetwork::CloseSocket(hSocket);
		return INVALID_SOCKETID;
	}

	//�󶨶���
	return pSocketItem->Attach(m_hCompletionPort, hSocket);
}

//������
bool CSocketItemManager::DetectItem()
{
	DWORD dwNowTime = (DWORD)time(NULL);
	CSocketItem * pSocketItem = NULL;
	//��������
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//�����������,����Ҫ�����
		if (m_NativeInfoPtrList[i]->IsUsed() == true && m_NativeInfoPtrList[i]->GetDetect() == true)
		{
			pSocketItem = dynamic_cast<CSocketItem *>(m_NativeInfoPtrList[i]);
			if (pSocketItem->Heartbeat(dwNowTime))
			{
				//����������
				if (m_HeartbeatPacket.wDataSize > 0)
				{
					pSocketItem->SendData(m_HeartbeatPacket.cbSendBuffer, m_HeartbeatPacket.wDataSize, pSocketItem->GetRountID());
				}
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
