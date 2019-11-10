#pragma once

class CTestClient : public IClientEvent
{
public:
	CTestClient();
	~CTestClient();

public:
	//启动服务
	bool StartClient();
	//停止服务
	bool StopClient();
	//连接服务
	bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//发送数据
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//群发数据
	bool SendDataBatch(VOID * pData, WORD wDataSize);

public:
	//连接事件
	virtual bool OnClientSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//关闭事件
	virtual bool OnClientSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//读取事件
	virtual int OnClientSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

public:
	//运行方法
	static int Run(int argc, TCHAR* argv[]);

private:
	IClientService *				m_pClientService;					//连接模块
};

