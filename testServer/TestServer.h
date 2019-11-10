#pragma once

class CTestServer : public IServerEvent
{
public:
	CTestServer();
	~CTestServer();

public:
	//启动服务
	bool StartServer();
	//停止服务
	bool StopServer();

public:
	//发送数据
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//群发数据
	bool SendDataBatch(VOID * pData, WORD wDataSize);

public:
	//绑定事件
	virtual bool OnServerNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort);
	//关闭事件
	virtual bool OnServerNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime);
	//读取事件
	virtual int OnServerNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize);

public:
	//运行方法
	static int Run(int argc, TCHAR* argv[]);

private:
	IServerService *				m_pServerService;					//服务模块
};

