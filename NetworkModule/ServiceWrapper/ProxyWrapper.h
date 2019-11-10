#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//代理核心
class CProxyKernel : protected ISocketEvent, protected INetworkEvent
{
public:
	CProxyKernel();
	virtual ~CProxyKernel();

public:
	//启动内核
	bool StartKernel(WORD wListenPort, LPCTSTR pszDefaultAddress, WORD wDefaultPort);
	//停止内核
	bool StopKernel();

protected:
	//连接事件
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//读取事件
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	//绑定事件
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//读取事件
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	CServer							m_Server;							//服务对象
	CClient							m_Client;							//连接对象
	TCHAR							m_szDefaultAddress[33];				//连接地址
	WORD							m_wDefaultPort;						//连接端口
	WORD							m_wDefaultProtocol;					//连接协议
};

//////////////////////////////////////////////////////////////////////////

//代理实例
class CProxyInstance : protected CProxyKernel, public IProxyService
{
public:
	CProxyInstance();
	virtual ~CProxyInstance();

public:
	//启动服务
	virtual bool StartServer(tagProxyOption ProxyOption, IProxyEvent * pProxyEvent);
	//停止服务
	virtual bool StopServer();
	//连接服务
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//代理连接
	virtual DWORD ProxyConnect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol);
	//设置数据
	virtual bool SetUserData(DWORD dwSocketID, DWORD dwUserData);
	//发送数据
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID);

protected:
	//连接事件
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//读取事件
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	//绑定事件
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//读取事件
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	IProxyEvent *					m_pProxyEvent;						//事件接口
};

//////////////////////////////////////////////////////////////////////////
