#pragma once

class CTestServer : public IServerEvent
{
public:
	CTestServer();
	~CTestServer();

public:
	//��������
	bool StartServer();
	//ֹͣ����
	bool StopServer();

public:
	//��������
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//Ⱥ������
	bool SendDataBatch(VOID * pData, WORD wDataSize);

public:
	//���¼�
	virtual bool OnServerNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort);
	//�ر��¼�
	virtual bool OnServerNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime);
	//��ȡ�¼�
	virtual int OnServerNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize);

public:
	//���з���
	static int Run(int argc, TCHAR* argv[]);

private:
	IServerService *				m_pServerService;					//����ģ��
};

