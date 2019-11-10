#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//�������
class CClientKernel : protected ISocketEvent
{
public:
	CClientKernel();
	virtual ~CClientKernel();

public:
	//�����ں�
	bool StartKernel();
	//ֹͣ�ں�
	bool StopKernel();

protected:
	//�����¼�
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	CClient							m_Client;							//���Ӷ���
};

//////////////////////////////////////////////////////////////////////////

//����ʵ��
class CClientInstance : protected CClientKernel, public IClientService
{
public:
	CClientInstance();
	virtual ~CClientInstance();

public:
	//��������
	virtual bool StartServer(tagClientOption ClientOption, IClientEvent * pClientEvent);
	//ֹͣ����
	virtual bool StopServer();
	//���ӷ���
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//��������
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//��������
	virtual bool SendDataBatch(VOID * pData, WORD wDataSize);
	//�ر�����
	virtual bool CloseSocket(DWORD dwSocketID);

protected:
	//�����¼�
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	IClientEvent *					m_pClientEvent;						//�¼��ӿ�
};

//////////////////////////////////////////////////////////////////////////
