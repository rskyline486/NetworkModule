#include "stdafx.h"
#include "ServiceKernel.h"

//////////////////////////////////////////////////////////////////////////

//��������
#define ASYNCHRONISM_SEND_DATA		1									//���ͱ�ʶ
#define ASYNCHRONISM_SEND_BATCH		2									//Ⱥ�巢��
#define ASYNCHRONISM_CLOSE_SOCKET	3									//�ر�����
#define ASYNCHRONISM_SET_DETECT		4									//���ü��
#define ASYNCHRONISM_SET_USER_DATA	5									//������
#define ASYNCHRONISM_SHUT_DOWN		6									//��ȫ�ر�

//////////////////////////////////////////////////////////////////////////

//��������
struct tagSendDataRequest
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	WORD							wDataSize;							//���ݴ�С
	BYTE							cbSendBuffer[SOCKET_BUFFER];		//���ͻ���
};

//Ⱥ������
struct tagBatchSendRequest
{
	WORD							wDataSize;							//���ݴ�С
	BYTE                            cbBatchMask;                        //��������
	BYTE							cbSendBuffer[SOCKET_BUFFER];		//���ͻ���
};

//�ر�����
struct tagCloseSocket
{
	DWORD							dwSocketID;							//���ӱ�ʶ
};

//���ü��
struct tagSetDetect
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	bool							bDetect;							//����ʶ
};

//��������
struct tagSetUserData
{
	DWORD							dwSocketID;							//���ӱ�ʶ
	DWORD							dwUserData;							//�û�����
};

//��ȫ�ر�
struct tagShutDownSocket
{
	DWORD							dwSocketID;							//���ӱ�ʶ
};

//////////////////////////////////////////////////////////////////////////

//���캯��
CServiceKernel::CServiceKernel()
{
	m_hCompletionPort = NULL;
}

//��������
CServiceKernel::~CServiceKernel()
{
}

//��ʼ������
bool CServiceKernel::InitService(WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount, WORD wMaxConnectCount)
{
	if (m_hCompletionPort != NULL)
	{
		Logger_Error(TEXT("�����ѳ�ʼ��"));
		return false;
	}

	//ϵͳ��Ϣ
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);

	//�����߳���Ŀ
	wThreadCount = (WORD)(wThreadCount < SystemInfo.dwNumberOfProcessors ? wThreadCount : SystemInfo.dwNumberOfProcessors);
	wThreadCount = wThreadCount > 0 ? wThreadCount : 1;

	//��ɶ˿�
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, wThreadCount);
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("������ɶ˿�ʧ��"));
		return false;
	}

	//���������������߳�
	m_WorkerThreadPtrList.clear();
	for (WORD i = 0; i < wThreadCount; i++)
	{
		//�����̶߳���
		CWorkerThread * pWorkerThread = new(std::nothrow) CWorkerThread(i);
		if (pWorkerThread == NULL)
		{
			Logger_Error(TEXT("���빤���߳�ʧ��"));
			break;
		}

		//��ʼ���̶߳���
		if (pWorkerThread->InitThread(m_hCompletionPort) == false)
		{
			Logger_Error(TEXT("��ʼ�������߳�ʧ��"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//�����߳�
		if (pWorkerThread->StartThread() == false)
		{
			Logger_Error(TEXT("���������߳�ʧ��"));
			delete pWorkerThread;
			pWorkerThread = NULL;
			break;
		}

		//�����̳߳��б�
		m_WorkerThreadPtrList.push_back(pWorkerThread);
	}

	//�����߳�У��
	if (m_WorkerThreadPtrList.empty())
	{
		Logger_Error(TEXT("��������ʧ��,�����߳���:0"));
		ReleaseService();
		return false;
	}

	//��ʼ������߳�
	if (m_DetectThread.InitThread(this, dwDetectTime) == false)
	{
		Logger_Error(TEXT("��ʼ������߳�ʧ��"));
		ReleaseService();
		return false;
	}
	//�����߳�
	if (m_DetectThread.StartThread() == false)
	{
		Logger_Error(TEXT("��������߳�ʧ��"));
		ReleaseService();
		return false;
	}

	//��ʼ����������
	m_AcceptItemManager.Init(m_hCompletionPort, this, wMaxAcceptCount);
	//��ʼ���������
	m_SocketItemManager.Init(m_hCompletionPort, this, wMaxSocketCount);
	//��ʼ�����Ӷ���
	m_ConnectItemManager.Init(m_hCompletionPort, this, wMaxConnectCount);
	//��ʼ���첽����
	m_AsynchronismEngine.Init(this, 1);

	Logger_Info(TEXT("���������ɹ�,�����߳���:%d"), wThreadCount);

	return true;
}

//�ͷŷ���
bool CServiceKernel::ReleaseService()
{
	//����У��
	if (m_hCompletionPort == NULL) return true;

	//ֹͣ����߳�
	m_DetectThread.StopThread(INFINITE);

	//�رռ�������
	m_AcceptItemManager.Release();
	//�ر��������
	m_SocketItemManager.Release();
	//�ر����Ӷ���
	m_ConnectItemManager.Release();

	//֪ͨ�����߳̽���
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
	}

	//ֹͣ�����߳�
	for (WORD i = 0; i < m_WorkerThreadPtrList.size(); i++)
	{
		if (m_WorkerThreadPtrList[i] == NULL) continue;

		m_WorkerThreadPtrList[i]->StopThread(INFINITE);
		delete m_WorkerThreadPtrList[i];
		m_WorkerThreadPtrList[i] = NULL;
	}
	m_WorkerThreadPtrList.clear();

	//ֹͣ�첽����
	m_AsynchronismEngine.Release();

	//�رն���
	if (m_hCompletionPort != NULL)
	{
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}

	Logger_Info(TEXT("����ֹͣ"));

	return true;
}

//�����˿�
DWORD CServiceKernel::ListenPort(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//У������
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("�����˿�[%d]ʧ��=>������δ��ʼ��"), wListenPort);
		return INVALID_SOCKETID;
	}

	//��������
	return m_AcceptItemManager.ActiveAcceptItem(wListenPort, dwUserData, wProtocol, bWatch);
}

//���ӷ���
DWORD CServiceKernel::ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	//У������
	if (m_hCompletionPort == NULL)
	{
		Logger_Error(TEXT("���ӷ���[%s:%d]ʧ��=>������δ��ʼ��"), pszConnectAddress, wConnectPort);
		return INVALID_SOCKETID;
	}

	//���ӷ���
	return m_ConnectItemManager.ActiveConnectItem(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//���ͺ���(֮���Ե���Ϊ�첽,����Ϊ�ڻص��߳��е�����Щ�������ײ�������-[����,����socketͬʱ���Է���������])
bool CServiceKernel::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	//У������
	if (wDataSize > SOCKET_BUFFER) return false;

	//ͬ������
	CLocker Locker(m_Mutex);
	tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)m_cbBuffer;

	//��������
	pSendDataRequest->dwSocketID = dwSocketID;
	pSendDataRequest->wDataSize = wDataSize;
	if (wDataSize > 0)
	{
		CopyMemory(pSendDataRequest->cbSendBuffer, pData, wDataSize);
	}

	//��������
	WORD wSendSize = sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + wDataSize;
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SEND_DATA, m_cbBuffer, wSendSize);
}

//Ⱥ������(֮���Ե���Ϊ�첽,����Ϊ�ڻص��߳��е�����Щ�������ײ�������-[����,����socketͬʱ���Է���������])
bool CServiceKernel::SendDataBatch(VOID * pData, WORD wDataSize)
{
	//У������
	if (wDataSize > SOCKET_BUFFER) return false;

	//ͬ������
	CLocker Locker(m_Mutex);
	tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)m_cbBuffer;

	//��������
	pBatchSendRequest->wDataSize = wDataSize;
	pBatchSendRequest->cbBatchMask = 0;
	if (wDataSize > 0)
	{
		CopyMemory(pBatchSendRequest->cbSendBuffer, pData, wDataSize);
	}

	//��������
	WORD wSendSize = sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + wDataSize;
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SEND_BATCH, m_cbBuffer, wSendSize);
}

//�ر�����(֮���Ե���Ϊ�첽,����Ϊ�ڻص��߳��е�����Щ�������ײ�������-[����,����socketͬʱ�رնԷ�])
bool CServiceKernel::CloseSocket(DWORD dwSocketID)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	tagCloseSocket * pCloseSocket = (tagCloseSocket *)m_cbBuffer;

	//��������
	pCloseSocket->dwSocketID = dwSocketID;

	//��������
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_CLOSE_SOCKET, m_cbBuffer, sizeof(tagCloseSocket));
}

//���ü��(֮���Ե���Ϊ�첽,����Ϊ�ڻص��߳��е�����Щ�������ײ�������-[����,����socketͬʱ���öԷ�����])
bool CServiceKernel::SetDetect(DWORD dwSocketID, bool bDetect)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	tagSetDetect * pSetDetect = (tagSetDetect *)m_cbBuffer;

	//��������
	pSetDetect->dwSocketID = dwSocketID;
	pSetDetect->bDetect = bDetect;

	//��������
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SET_DETECT, m_cbBuffer, sizeof(tagSetDetect));
}

//��������(֮���Ե���Ϊ�첽,����Ϊ�ڻص��߳��е�����Щ�������ײ�������-[����,����socketͬʱ���öԷ�����])
bool CServiceKernel::SetUserData(DWORD dwSocketID, DWORD dwUserData)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	tagSetUserData * pSetUserData = (tagSetUserData *)m_cbBuffer;

	//��������
	pSetUserData->dwSocketID = dwSocketID;
	pSetUserData->dwUserData = dwUserData;

	//��������
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SET_USER_DATA, m_cbBuffer, sizeof(tagSetUserData));
}

//���ùر�
bool CServiceKernel::ShutDownSocket(DWORD dwSocketID)
{
	//ͬ������
	CLocker Locker(m_Mutex);
	tagShutDownSocket * pShutDownSocket = (tagShutDownSocket *)m_cbBuffer;

	//��������
	pShutDownSocket->dwSocketID = dwSocketID;

	//��������
	return m_AsynchronismEngine.PostAsynchronismData(ASYNCHRONISM_SHUT_DOWN, m_cbBuffer, sizeof(tagShutDownSocket));
}

//����¼�
VOID CServiceKernel::OnEventDetectBeat()
{
	//��ʾ���и���
	//if (1)
	//{
	//	tagBurthenInfo BurthenInfo;
	//	m_AsynchronismEngine.GetAsynchronismBurthenInfo(BurthenInfo);
	//	Logger_Info(TEXT("���и�����Ϣ,���峤��:%u,���ݴ�С:%u,���ݰ���:%u"), BurthenInfo.dwBufferSize, BurthenInfo.dwDataSize, BurthenInfo.dwPacketCount);
	//}

	m_AcceptItemManager.DetectItem();
	m_ConnectItemManager.DetectItem();
	m_SocketItemManager.DetectItem();
}

//�첽��ʼ
bool CServiceKernel::OnEventAsynchronismStrat()
{
	return true;
}

//�첽����
bool CServiceKernel::OnEventAsynchronismStop()
{
	return true;
}

//�첽�¼�
bool CServiceKernel::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case ASYNCHRONISM_SEND_DATA:		//��������
		{
			//Ч������
			tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)pData;
			if (wDataSize < (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + pSendDataRequest->wDataSize)) return false;
			
			//��ȡ����
			WORD wIndex = SOCKET_INDEX(pSendDataRequest->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
			}

			return m_SocketItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
		}
	case ASYNCHRONISM_SEND_BATCH:		//Ⱥ������
		{
			//Ч������
			tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)pData;
			if (wDataSize < (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + pBatchSendRequest->wDataSize)) return false;

			return false;
		}
	case ASYNCHRONISM_CLOSE_SOCKET:		//�ر�����
		{
			//Ч������
			tagCloseSocket * pCloseSocket = (tagCloseSocket *)pData;
			if (wDataSize != sizeof(tagCloseSocket)) return false;

			//��ȡ����
			WORD wIndex = SOCKET_INDEX(pCloseSocket->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.CloseSocket(pCloseSocket->dwSocketID);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.CloseSocket(pCloseSocket->dwSocketID);
			}

			return m_SocketItemManager.CloseSocket(pCloseSocket->dwSocketID);
		}
	case ASYNCHRONISM_SET_DETECT:		//���ü��
		{
			//Ч������
			tagSetDetect * pSetDetect = (tagSetDetect *)pData;
			if (wDataSize != sizeof(tagSetDetect)) return false;

			//��ȡ����
			WORD wIndex = SOCKET_INDEX(pSetDetect->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
			}

			return m_SocketItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
		}
	case ASYNCHRONISM_SET_USER_DATA:	//������
		{
			//Ч������
			tagSetUserData * pSetUserData = (tagSetUserData *)pData;
			if (wDataSize != sizeof(tagSetUserData)) return false;

			//��ȡ����
			WORD wIndex = SOCKET_INDEX(pSetUserData->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
			}

			return m_SocketItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
		}
	case ASYNCHRONISM_SHUT_DOWN:		//��ȫ�ر�
		{
			//Ч������
			tagShutDownSocket * pShutDownSocket = (tagShutDownSocket *)pData;
			if (wDataSize != sizeof(tagShutDownSocket)) return false;

			//��ȡ����
			WORD wIndex = SOCKET_INDEX(pShutDownSocket->dwSocketID);
			if (wIndex >= INDEX_LISTEN)
			{
				return m_AcceptItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
			}
			else if (wIndex >= INDEX_CONNECT)
			{
				return m_ConnectItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
			}

			return m_SocketItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
		}
	}

	return false;
}

//��ʼ����
VOID CServiceKernel::OnEventListenStart(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	Logger_Info(TEXT("��ʼ����=>%s:%d"), szLocalAddress, wLocalPort);
}

//�����¼�
VOID CServiceKernel::OnEventListenAccept(CNativeInfo* pNativeInfo, SOCKET hSocket)
{
	//��������
	m_SocketItemManager.ActiveSocketItem(hSocket);
}

//��������
VOID CServiceKernel::OnEventListenStop(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	Logger_Info(TEXT("��������=>%s:%d"), szLocalAddress, wLocalPort);
}

//���¼�
VOID CServiceKernel::OnEventSocketBind(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Socket���¼�=>���ص�ַ:%s:%d, �Զ˵�ַ:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//��ȡ�¼�
DWORD CServiceKernel::OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	Logger_Info(TEXT("Socket��ȡ�¼�=>dwSocketID:%u, ���ݴ�С:%u"), pNativeInfo->GetSocketID(), dwDataSize);
	return dwDataSize;
}

//�ر��¼�
VOID CServiceKernel::OnEventSocketShut(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Socket�ر��¼�=>���ص�ַ:%s:%d, �Զ˵�ַ:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//�����¼�
VOID CServiceKernel::OnEventConnectLink(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Connect�����¼�=>���ص�ַ:%s:%d, �Զ˵�ַ:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//��ȡ�¼�
DWORD CServiceKernel::OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	Logger_Info(TEXT("Connect��ȡ�¼�=>dwSocketID:%u, ���ݴ�С:%u"), pNativeInfo->GetSocketID(), dwDataSize);
	return dwDataSize;
}

//�ر��¼�
VOID CServiceKernel::OnEventConnectShut(CNativeInfo* pNativeInfo)
{
	TCHAR szLocalAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wLocalPort = 0;
	pNativeInfo->GetLocalAddress(szLocalAddress, CountArray(szLocalAddress), &wLocalPort);
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	Logger_Info(TEXT("Connect�ر��¼�=>���ص�ַ:%s:%d, �Զ˵�ַ:%s:%d"), szLocalAddress, wLocalPort, szRemoteAddress, wRemotePort);
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CServer::CServer()
{
	m_pNetworkEvent = this;
}

//��������
CServer::~CServer()
{
	//
}

//��ʼ������
bool CServer::Init(INetworkEvent * pNetworkEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount)
{
	if (__super::InitService(wThreadCount, dwDetectTime, wMaxAcceptCount, wMaxSocketCount, 0))
	{
		m_pNetworkEvent = (pNetworkEvent != NULL) ? pNetworkEvent : this;
		return true;
	}
	
	return false;
}

//�ͷŷ���
bool CServer::Release()
{
	__super::ReleaseService();
	m_pNetworkEvent = this;
	return true;
}

//����������
bool CServer::SetHeartbeatPacket(VOID * pData, WORD wDataSize)
{
	return m_SocketItemManager.SetHeartbeatData(pData, wDataSize);
}

//�����˿�
DWORD CServer::Listen(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return __super::ListenPort(wListenPort, dwUserData, wProtocol, bWatch);
}

//��ȡ��ַ
DWORD CServer::GetClientIP(DWORD dwSocketID)
{
	return m_SocketItemManager.GetClientIP(dwSocketID);
}

//��������
bool CServer::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SendData(dwSocketID, pData, wDataSize);
	}

	return __super::SendData(dwSocketID, pData, wDataSize);
}

//Ⱥ������
bool CServer::SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SendDataBatch(pData, wDataSize);
	}

	return __super::SendDataBatch(pData, wDataSize);
}

//�ر�����
bool CServer::CloseSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.CloseSocket(dwSocketID);
	}

	return __super::CloseSocket(dwSocketID);
}

//���ü��
bool CServer::SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SetDetect(dwSocketID, bDetect);
	}

	return __super::SetDetect(dwSocketID, bDetect);
}

//��������
bool CServer::SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.SetUserData(dwSocketID, dwUserData);
	}

	return __super::SetUserData(dwSocketID, dwUserData);
}

//���ùر�
bool CServer::ShutDownSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_SocketItemManager.ShutDownSocket(dwSocketID);
	}

	return __super::ShutDownSocket(dwSocketID);
}

//�첽�¼�
bool CServer::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case ASYNCHRONISM_SEND_DATA:		//��������
		{
			//Ч������
			tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)pData;
			if (wDataSize < (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + pSendDataRequest->wDataSize)) return false;

			return m_SocketItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
		}
	case ASYNCHRONISM_SEND_BATCH:		//Ⱥ������
		{
			//Ч������
			tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)pData;
			if (wDataSize < (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + pBatchSendRequest->wDataSize)) return false;

			return m_SocketItemManager.SendDataBatch(pBatchSendRequest->cbSendBuffer, pBatchSendRequest->wDataSize);
		}
	case ASYNCHRONISM_CLOSE_SOCKET:		//�ر�����
		{
			//Ч������
			tagCloseSocket * pCloseSocket = (tagCloseSocket *)pData;
			if (wDataSize != sizeof(tagCloseSocket)) return false;

			return m_SocketItemManager.CloseSocket(pCloseSocket->dwSocketID);
		}
	case ASYNCHRONISM_SET_DETECT:		//���ü��
		{
			//Ч������
			tagSetDetect * pSetDetect = (tagSetDetect *)pData;
			if (wDataSize != sizeof(tagSetDetect)) return false;

			return m_SocketItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
		}
	case ASYNCHRONISM_SET_USER_DATA:	//������
		{
			//Ч������
			tagSetUserData * pSetUserData = (tagSetUserData *)pData;
			if (wDataSize != sizeof(tagSetUserData)) return false;

			return m_SocketItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
		}
	case ASYNCHRONISM_SHUT_DOWN:		//��ȫ�ر�
		{
			//Ч������
			tagShutDownSocket * pShutDownSocket = (tagShutDownSocket *)pData;
			if (wDataSize != sizeof(tagShutDownSocket)) return false;

			return m_SocketItemManager.ShutDownSocket(pShutDownSocket->dwSocketID);
		}
	}

	return false;
}

//���¼�
VOID CServer::OnEventSocketBind(CNativeInfo* pNativeInfo)
{
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	m_pNetworkEvent->OnEventNetworkBind(pNativeInfo->GetSocketID(), szRemoteAddress, wRemotePort, pNativeInfo->GetUserData());
}

//��ȡ�¼�
DWORD CServer::OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	int nReadSize = m_pNetworkEvent->OnEventNetworkRead(pNativeInfo->GetSocketID(), pData, dwDataSize, pNativeInfo->GetUserData());
	if (nReadSize < 0) return SOCKET_ERROR;
	return nReadSize;
}

//�ر��¼�
VOID CServer::OnEventSocketShut(CNativeInfo* pNativeInfo)
{
	TCHAR szRemoteAddress[MAX_ADDRSTRLEN] = { 0 }; WORD wRemotePort = 0;
	pNativeInfo->GetRemoteAddress(szRemoteAddress, CountArray(szRemoteAddress), &wRemotePort);
	m_pNetworkEvent->OnEventNetworkShut(pNativeInfo->GetSocketID(), szRemoteAddress, wRemotePort, pNativeInfo->GetActiveTime(), pNativeInfo->GetUserData());
}

//���¼�
bool CServer::OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket���¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, �û�����:%u"), dwSocketID, pszClientIP, wClientPort, dwUserData);
	TCHAR szResponseText[128] = TEXT("");
	_sntprintf_s(szResponseText, CountArray(szResponseText), TEXT("��������=>The server has received your connection, ����IP��ַΪ:%s, �˿�Ϊ:%d"), pszClientIP, wClientPort);
	SendData(dwSocketID, szResponseText, CountStringBuffer(szResponseText));
	return true;
}

//�ر��¼�
bool CServer::OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket�ر��¼�=>���ӱ�ʶ:%u, �Զ˵�ַ:%s:%d, ����ʱ��:%u, ����ʱ��:%u��, �û�����:%u"), dwSocketID, pszClientIP, wClientPort, dwActiveTime, ((DWORD)time(NULL) - dwActiveTime), dwUserData);
	return true;
}

//��ȡ�¼�
int CServer::OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Socket��ȡ�¼�=>���ӱ�ʶ:%u, ���ݴ�С:%u, �û�����:%u"), dwSocketID, dwDataSize, dwUserData);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);
	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CClient::CClient()
{
	m_pISocketEvent = this;
}

//��������
CClient::~CClient()
{
	//
}

//��ʼ������
bool CClient::Init(ISocketEvent * pISocketEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxConnectCount)
{
	if (__super::InitService(wThreadCount, dwDetectTime, 0, 0, wMaxConnectCount))
	{
		m_pISocketEvent = (pISocketEvent != NULL) ? pISocketEvent : this;
		return true;
	}

	return false;
}

//�ͷŷ���
bool CClient::Release()
{
	__super::ReleaseService();
	m_pISocketEvent = this;
	return true;
}

//����������
bool CClient::SetHeartbeatPacket(VOID * pData, WORD wDataSize)
{
	return m_ConnectItemManager.SetHeartbeatData(pData, wDataSize);
}

//�����˿�
DWORD CClient::Connect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch)
{
	return __super::ConnectServer(pszConnectAddress, wConnectPort, dwUserData, wProtocol, bWatch);
}

//��������
bool CClient::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SendData(dwSocketID, pData, wDataSize);
	}

	return __super::SendData(dwSocketID, pData, wDataSize);
}

//Ⱥ������
bool CClient::SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SendDataBatch(pData, wDataSize);
	}

	return __super::SendDataBatch(pData, wDataSize);
}

//�ر�����
bool CClient::CloseSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.CloseSocket(dwSocketID);
	}

	return __super::CloseSocket(dwSocketID);
}

//���ü��
bool CClient::SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SetDetect(dwSocketID, bDetect);
	}

	return __super::SetDetect(dwSocketID, bDetect);
}

//��������
bool CClient::SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.SetUserData(dwSocketID, dwUserData);
	}

	return __super::SetUserData(dwSocketID, dwUserData);
}

//���ùر�
bool CClient::ShutDownSocket(DWORD dwSocketID, bool bSynchronize /* = false */)
{
	if (bSynchronize)
	{
		return m_ConnectItemManager.ShutDownSocket(dwSocketID);
	}

	return __super::ShutDownSocket(dwSocketID);
}

//�첽�¼�
bool CClient::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	switch (wIdentifier)
	{
	case ASYNCHRONISM_SEND_DATA:		//��������
		{
			//Ч������
			tagSendDataRequest * pSendDataRequest = (tagSendDataRequest *)pData;
			if (wDataSize < (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagSendDataRequest) - sizeof(pSendDataRequest->cbSendBuffer) + pSendDataRequest->wDataSize)) return false;

			return m_ConnectItemManager.SendData(pSendDataRequest->dwSocketID, pSendDataRequest->cbSendBuffer, pSendDataRequest->wDataSize);
		}
	case ASYNCHRONISM_SEND_BATCH:		//Ⱥ������
		{
			//Ч������
			tagBatchSendRequest * pBatchSendRequest = (tagBatchSendRequest *)pData;
			if (wDataSize < (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer))) return false;
			if (wDataSize != (sizeof(tagBatchSendRequest) - sizeof(pBatchSendRequest->cbSendBuffer) + pBatchSendRequest->wDataSize)) return false;

			return m_ConnectItemManager.SendDataBatch(pBatchSendRequest->cbSendBuffer, pBatchSendRequest->wDataSize);
		}
	case ASYNCHRONISM_CLOSE_SOCKET:		//�ر�����
		{
			//Ч������
			tagCloseSocket * pCloseSocket = (tagCloseSocket *)pData;
			if (wDataSize != sizeof(tagCloseSocket)) return false;

			return m_ConnectItemManager.CloseSocket(pCloseSocket->dwSocketID);
		}
	case ASYNCHRONISM_SET_DETECT:		//���ü��
		{
			//Ч������
			tagSetDetect * pSetDetect = (tagSetDetect *)pData;
			if (wDataSize != sizeof(tagSetDetect)) return false;

			return m_ConnectItemManager.SetDetect(pSetDetect->dwSocketID, pSetDetect->bDetect);
		}
	case ASYNCHRONISM_SET_USER_DATA:	//������
		{
			//Ч������
			tagSetUserData * pSetUserData = (tagSetUserData *)pData;
			if (wDataSize != sizeof(tagSetUserData)) return false;

			return m_ConnectItemManager.SetUserData(pSetUserData->dwSocketID, pSetUserData->dwUserData);
		}
	}

	return false;
}

//�����¼�
VOID CClient::OnEventConnectLink(CNativeInfo* pNativeInfo)
{
	m_pISocketEvent->OnEventSocketLink(pNativeInfo->GetSocketID(), pNativeInfo->GetUserData());
}

//��ȡ�¼�
DWORD CClient::OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize)
{
	int nReadSize = m_pISocketEvent->OnEventSocketRead(pNativeInfo->GetSocketID(), pData, dwDataSize, pNativeInfo->GetUserData());
	if (nReadSize < 0) return SOCKET_ERROR;
	return nReadSize;
}

//�ر��¼�
VOID CClient::OnEventConnectShut(CNativeInfo* pNativeInfo)
{
	m_pISocketEvent->OnEventSocketShut(pNativeInfo->GetSocketID(), pNativeInfo->GetUserData());
}

//�����¼�
bool CClient::OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect�����¼�=>���ӱ�ʶ:%u, �û�����:%u"), dwSocketID, dwUserData);
	TCHAR szRequestText[128] = TEXT("");
	_sntprintf_s(szRequestText, CountArray(szRequestText), TEXT("��������=>test data"));
	SendData(dwSocketID, szRequestText, CountStringBuffer(szRequestText));
	return true;
}

//�ر��¼�
bool CClient::OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect�ر��¼�=>���ӱ�ʶ:%u, �û�����:%u"), dwSocketID, dwUserData);
	return true;
}

//��ȡ�¼�
int CClient::OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData)
{
	Logger_Info(TEXT("Connect��ȡ�¼�=>���ӱ�ʶ:%u, �û�����:%u, ���ݴ�С:%u"), dwSocketID, dwUserData, dwDataSize);
	Logger_Info(TEXT("%s"), (TCHAR*)pData);
	return dwDataSize;
}

//////////////////////////////////////////////////////////////////////////
