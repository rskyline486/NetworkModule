#pragma once

class CTestAttemperClient : public IAttemperEvent
{
public:
	CTestAttemperClient();
	~CTestAttemperClient();

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
	//时间事件
	virtual bool OnAttemperTimer(DWORD dwTimerID, WPARAM dwBindParameter);
	//自定事件
	virtual bool OnAttemperCustom(DWORD dwCustomID, VOID * pData, WORD wDataSize);
	//自定事件
	virtual bool OnAttemperCustomEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize);

public:
	//连接绑定
	virtual bool OnAttemperSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//连接关闭
	virtual bool OnAttemperSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//连接读取
	virtual bool OnAttemperSocketRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwUserData);

public:
	//网络绑定
	virtual bool OnAttemperNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort);
	//网络关闭
	virtual bool OnAttemperNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime);
	//网络读取
	virtual bool OnAttemperNetworkRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize);

public:
	//运行方法
	static int Run(int argc, TCHAR* argv[]);

private:
	IAttemperService *				m_pAttemperService;					//服务模块
};

