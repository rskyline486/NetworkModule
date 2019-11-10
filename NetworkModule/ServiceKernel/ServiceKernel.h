#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"
#include "AcceptItem.h"
#include "SocketItem.h"
#include "ConnectItem.h"
#include "WorkerThread.h"
#include "DetectThread.h"
#include "AsynchronismEngine.h"
#include "TimerEngine.h"
#include "CryptoCore.h"

//////////////////////////////////////////////////////////////////////////

//�����¼�
class ISocketEvent
{
public:
	//�����¼�
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData) = 0;
	//�ر��¼�
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData) = 0;
	//��ȡ�¼�
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData) = 0;
};

//////////////////////////////////////////////////////////////////////////

//�����¼�
class INetworkEvent
{
public:
	//���¼�
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData) = 0;
	//�ر��¼�
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData) = 0;
	//��ȡ�¼�
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData) = 0;
};

//////////////////////////////////////////////////////////////////////////

//�������
class CServiceKernel : protected IAcceptItemSink, protected ISocketItemSink, protected IConnectItemSink, protected IDetectEventSink, protected IAsynchronismEventSink
{
public:
	CServiceKernel();
	virtual ~CServiceKernel();

public:
	//��ʼ������
	bool InitService(WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount, WORD wMaxConnectCount);
	//�ͷŷ���
	bool ReleaseService();

protected:
	//�����˿�
	DWORD ListenPort(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch);
	//���ӷ���
	DWORD ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

protected:
	//���ͺ���
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//Ⱥ������
	bool SendDataBatch(VOID * pData, WORD wDataSize);
	//�ر�����
	bool CloseSocket(DWORD dwSocketID);
	//���ü��
	bool SetDetect(DWORD dwSocketID, bool bDetect);
	//��������
	bool SetUserData(DWORD dwSocketID, DWORD dwUserData);
	//���ùر�
	bool ShutDownSocket(DWORD dwSocketID);

protected:
	//����¼�
	virtual VOID OnEventDetectBeat();

protected:
	//�첽��ʼ
	virtual bool OnEventAsynchronismStrat();
	//�첽����
	virtual bool OnEventAsynchronismStop();
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//��ʼ����
	virtual VOID OnEventListenStart(CNativeInfo* pNativeInfo);
	//�����¼�
	virtual VOID OnEventListenAccept(CNativeInfo* pNativeInfo, SOCKET hSocket);
	//��������
	virtual VOID OnEventListenStop(CNativeInfo* pNativeInfo);

protected:
	//���¼�
	virtual VOID OnEventSocketBind(CNativeInfo* pNativeInfo);
	//��ȡ�¼�
	virtual DWORD OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//�ر��¼�
	virtual VOID OnEventSocketShut(CNativeInfo* pNativeInfo);

protected:
	//�����¼�
	virtual VOID OnEventConnectLink(CNativeInfo* pNativeInfo);
	//��ȡ�¼�
	virtual DWORD OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//�ر��¼�
	virtual VOID OnEventConnectShut(CNativeInfo* pNativeInfo);

protected:
	HANDLE							m_hCompletionPort;					//��ɶ˿�
	std::vector<CThread *>			m_WorkerThreadPtrList;				//�����߳�
	CDetectThread					m_DetectThread;						//����߳�
	CAcceptItemManager				m_AcceptItemManager;				//��������
	CSocketItemManager				m_SocketItemManager;				//�������
	CConnectItemManager				m_ConnectItemManager;				//���Ӷ���
	CAsynchronismEngine				m_AsynchronismEngine;				//�첽����
	CMutex							m_Mutex;							//ͬ������
	BYTE							m_cbBuffer[ASYNCHRONISM_BUFFER];	//��ʱ����
};

//////////////////////////////////////////////////////////////////////////

//����ģ��
class CServer : protected CServiceKernel, protected INetworkEvent
{
public:
	CServer();
	~CServer();

public:
	//��ʼ������
	bool Init(INetworkEvent * pNetworkEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount);
	//�ͷŷ���
	bool Release();
	//����������
	bool SetHeartbeatPacket(VOID * pData, WORD wDataSize);
	//�����˿�
	DWORD Listen(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch);
	//��ȡ��ַ
	DWORD GetClientIP(DWORD dwSocketID);

public:
	//��������
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//Ⱥ������
	bool SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//�ر�����
	bool CloseSocket(DWORD dwSocketID, bool bSynchronize = false);
	//���ü��
	bool SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize = false);
	//��������
	bool SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize = false);
	//���ùر�
	bool ShutDownSocket(DWORD dwSocketID, bool bSynchronize = false);

protected:
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//���¼�
	virtual VOID OnEventSocketBind(CNativeInfo* pNativeInfo);
	//��ȡ�¼�
	virtual DWORD OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//�ر��¼�
	virtual VOID OnEventSocketShut(CNativeInfo* pNativeInfo);

protected:
	//���¼�
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	INetworkEvent *					m_pNetworkEvent;					//�ص��ӿ�
};

//////////////////////////////////////////////////////////////////////////

//����ģ��
class CClient : protected CServiceKernel, protected ISocketEvent
{
public:
	CClient();
	~CClient();

public:
	//��ʼ������
	bool Init(ISocketEvent * pISocketEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxConnectCount);
	//�ͷŷ���
	bool Release();
	//����������
	bool SetHeartbeatPacket(VOID * pData, WORD wDataSize);
	//���ӷ���
	DWORD Connect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//��������
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//Ⱥ������
	bool SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//�ر�����
	bool CloseSocket(DWORD dwSocketID, bool bSynchronize = false);
	//���ü��
	bool SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize = false);
	//��������
	bool SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize = false);
	//���ùر�
	bool ShutDownSocket(DWORD dwSocketID, bool bSynchronize = false);

protected:
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//�����¼�
	virtual VOID OnEventConnectLink(CNativeInfo* pNativeInfo);
	//��ȡ�¼�
	virtual DWORD OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//�ر��¼�
	virtual VOID OnEventConnectShut(CNativeInfo* pNativeInfo);

protected:
	//�����¼�
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	ISocketEvent *					m_pISocketEvent;					//�ص��ӿ�
};

//////////////////////////////////////////////////////////////////////////
