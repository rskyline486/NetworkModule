#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//�������
class CAttemperKernel : protected IAsynchronismEventSink, protected ITimerEventSink, protected ISocketEvent, protected INetworkEvent
{
public:
	CAttemperKernel();
	virtual ~CAttemperKernel();

public:
	//�����ں�
	bool StartKernel();
	//ֹͣ�ں�
	bool StopKernel();

protected:
	//�첽��ʼ
	virtual bool OnEventAsynchronismStrat();
	//�첽����
	virtual bool OnEventAsynchronismStop();
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//ʱ���¼�
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

protected:
	//�����¼�
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	//���¼�
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	CAsynchronismEngine				m_AsynchronismEngine;				//�첽����
	CTimerEngine					m_TimerEngine;						//ʱ�����
	CClient							m_Client;							//���Ӷ���
	CServer							m_Server;							//�������
	CMutex							m_Mutex;							//ͬ������
	BYTE							m_cbBuffer[ASYNCHRONISM_BUFFER];	//��ʱ����
};

//////////////////////////////////////////////////////////////////////////

//����ʵ��
class CAttemperInstance : protected CAttemperKernel, public IAttemperService
{
public:
	CAttemperInstance();
	virtual ~CAttemperInstance();

public:
	//��������
	virtual bool StartServer(tagAttemperOption AttemperOption, IAttemperEvent * pAttemperEvent);
	//ֹͣ����
	virtual bool StopServer();

public:
	//���ӷ���
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//��������
	virtual bool SendData(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize);
	//Ⱥ������
	virtual bool SendDataBatch(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize);
	//�ر�����
	virtual bool CloseSocket(DWORD dwSocketID);
	//��ȫ�ر�
	virtual bool ShutDownSocket(DWORD dwSocketID);

public:
	//���ö�ʱ��
	virtual bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter);
	//ɾ����ʱ��
	virtual bool KillTimer(DWORD dwTimerID);
	//ɾ����ʱ��
	virtual bool KillAllTimer();

public:
	//Ͷ���¼�
	virtual bool PostCustomEvent(DWORD dwCustomID, VOID * pData, WORD wDataSize);
	//Ͷ���¼�
	virtual bool PostCustomEventEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize);

protected:
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//ʱ���¼�
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

protected:
	//�����¼�
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	//���¼�
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	IAttemperEvent *				m_pAttemperEvent;					//�¼��ӿ�
	CCryptoManager					m_CryptoManager;					//�������
};

//////////////////////////////////////////////////////////////////////////
