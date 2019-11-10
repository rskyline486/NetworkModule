#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//服务核心
class CServerKernel : protected INetworkEvent
{
public:
	CServerKernel();
	virtual ~CServerKernel();

public:
	//启动内核
	bool StartKernel();
	//停止内核
	bool StopKernel();

protected:
	//绑定事件
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//读取事件
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

protected:
	CServer							m_Server;							//服务对象
};

//////////////////////////////////////////////////////////////////////////

//服务实例
class CServerInstance : protected CServerKernel, public IServerService
{
public:
	CServerInstance();
	virtual ~CServerInstance();

public:
	//启动服务
	virtual bool StartServer(tagServerOption ServerOption, IServerEvent * pServerEvent);
	//停止服务
	virtual bool StopServer();

public:
	//发送数据
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//发送数据
	virtual bool SendDataBatch(VOID * pData, WORD wDataSize);
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID);

protected:
	//绑定事件
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//读取事件
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	IServerEvent *					m_pServerEvent;						//事件接口
};

//////////////////////////////////////////////////////////////////////////
