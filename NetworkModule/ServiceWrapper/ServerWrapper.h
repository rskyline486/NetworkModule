#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//�������
class CServerKernel : protected INetworkEvent
{
public:
	CServerKernel();
	virtual ~CServerKernel();

public:
	//�����ں�
	bool StartKernel();
	//ֹͣ�ں�
	bool StopKernel();

protected:
	//���¼�
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	CServer							m_Server;							//�������
};

//////////////////////////////////////////////////////////////////////////

//����ʵ��
class CServerInstance : protected CServerKernel, public IServerService
{
public:
	CServerInstance();
	virtual ~CServerInstance();

public:
	//��������
	virtual bool StartServer(tagServerOption ServerOption, IServerEvent * pServerEvent);
	//ֹͣ����
	virtual bool StopServer();

public:
	//��������
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//��������
	virtual bool SendDataBatch(VOID * pData, WORD wDataSize);
	//�ر�����
	virtual bool CloseSocket(DWORD dwSocketID);

protected:
	//���¼�
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	IServerEvent *					m_pServerEvent;						//�¼��ӿ�
};

//////////////////////////////////////////////////////////////////////////
