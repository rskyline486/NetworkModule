#pragma once

class CTestClient : public IClientEvent
{
public:
	CTestClient();
	~CTestClient();

public:
	//��������
	bool StartClient();
	//ֹͣ����
	bool StopClient();
	//���ӷ���
	bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//��������
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//Ⱥ������
	bool SendDataBatch(VOID * pData, WORD wDataSize);

public:
	//�����¼�
	virtual bool OnClientSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//�ر��¼�
	virtual bool OnClientSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//��ȡ�¼�
	virtual int OnClientSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

public:
	//���з���
	static int Run(int argc, TCHAR* argv[]);

private:
	IClientService *				m_pClientService;					//����ģ��
};

