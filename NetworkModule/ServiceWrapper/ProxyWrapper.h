#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//�������
class CProxyKernel : protected ISocketEvent, protected INetworkEvent
{
public:
	CProxyKernel();
	virtual ~CProxyKernel();

public:
	//�����ں�
	bool StartKernel(WORD wListenPort, LPCTSTR pszDefaultAddress, WORD wDefaultPort);
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
	//���¼�
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	CServer							m_Server;							//�������
	CClient							m_Client;							//���Ӷ���
	TCHAR							m_szDefaultAddress[33];				//���ӵ�ַ
	WORD							m_wDefaultPort;						//���Ӷ˿�
	WORD							m_wDefaultProtocol;					//����Э��
};

//////////////////////////////////////////////////////////////////////////

//����ʵ��
class CProxyInstance : protected CProxyKernel, public IProxyService
{
public:
	CProxyInstance();
	virtual ~CProxyInstance();

public:
	//��������
	virtual bool StartServer(tagProxyOption ProxyOption, IProxyEvent * pProxyEvent);
	//ֹͣ����
	virtual bool StopServer();
	//���ӷ���
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//��������
	virtual DWORD ProxyConnect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol);
	//��������
	virtual bool SetUserData(DWORD dwSocketID, DWORD dwUserData);
	//��������
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//�ر�����
	virtual bool CloseSocket(DWORD dwSocketID);

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
	IProxyEvent *					m_pProxyEvent;						//�¼��ӿ�
};

//////////////////////////////////////////////////////////////////////////
