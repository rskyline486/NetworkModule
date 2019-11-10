#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//服务核心
class CClientKernel : protected ISocketEvent
{
public:
	CClientKernel();
	virtual ~CClientKernel();

public:
	//启动内核
	bool StartKernel();
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
	CClient							m_Client;							//连接对象
};

//////////////////////////////////////////////////////////////////////////

//服务实例
class CClientInstance : protected CClientKernel, public IClientService
{
public:
	CClientInstance();
	virtual ~CClientInstance();

public:
	//启动服务
	virtual bool StartServer(tagClientOption ClientOption, IClientEvent * pClientEvent);
	//停止服务
	virtual bool StopServer();
	//连接服务
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//发送数据
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//发送数据
	virtual bool SendDataBatch(VOID * pData, WORD wDataSize);
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID);

protected:
	//连接事件
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//读取事件
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	IClientEvent *					m_pClientEvent;						//事件接口
};

//////////////////////////////////////////////////////////////////////////
