#pragma once

class CTestAttemperClient : public IAttemperEvent
{
public:
	CTestAttemperClient();
	~CTestAttemperClient();

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
	//ʱ���¼�
	virtual bool OnAttemperTimer(DWORD dwTimerID, WPARAM dwBindParameter);
	//�Զ��¼�
	virtual bool OnAttemperCustom(DWORD dwCustomID, VOID * pData, WORD wDataSize);
	//�Զ��¼�
	virtual bool OnAttemperCustomEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize);

public:
	//���Ӱ�
	virtual bool OnAttemperSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//���ӹر�
	virtual bool OnAttemperSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//���Ӷ�ȡ
	virtual bool OnAttemperSocketRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwUserData);

public:
	//�����
	virtual bool OnAttemperNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort);
	//����ر�
	virtual bool OnAttemperNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime);
	//�����ȡ
	virtual bool OnAttemperNetworkRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize);

public:
	//���з���
	static int Run(int argc, TCHAR* argv[]);

private:
	IAttemperService *				m_pAttemperService;					//����ģ��
};

