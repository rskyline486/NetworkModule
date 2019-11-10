#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//服务核心
class CAttemperKernel : protected IAsynchronismEventSink, protected ITimerEventSink, protected ISocketEvent, protected INetworkEvent
{
public:
	CAttemperKernel();
	virtual ~CAttemperKernel();

public:
	//启动内核
	bool StartKernel();
	//停止内核
	bool StopKernel();

protected:
	//异步开始
	virtual bool OnEventAsynchronismStrat();
	//异步结束
	virtual bool OnEventAsynchronismStop();
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//时间事件
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

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
	CAsynchronismEngine				m_AsynchronismEngine;				//异步对象
	CTimerEngine					m_TimerEngine;						//时间对象
	CClient							m_Client;							//连接对象
	CServer							m_Server;							//服务对象
	CMutex							m_Mutex;							//同步对象
	BYTE							m_cbBuffer[ASYNCHRONISM_BUFFER];	//临时缓冲
};

//////////////////////////////////////////////////////////////////////////

//服务实例
class CAttemperInstance : protected CAttemperKernel, public IAttemperService
{
public:
	CAttemperInstance();
	virtual ~CAttemperInstance();

public:
	//启动服务
	virtual bool StartServer(tagAttemperOption AttemperOption, IAttemperEvent * pAttemperEvent);
	//停止服务
	virtual bool StopServer();

public:
	//连接服务
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//发送数据
	virtual bool SendData(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize);
	//群发数据
	virtual bool SendDataBatch(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize);
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID);
	//安全关闭
	virtual bool ShutDownSocket(DWORD dwSocketID);

public:
	//设置定时器
	virtual bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter);
	//删除定时器
	virtual bool KillTimer(DWORD dwTimerID);
	//删除定时器
	virtual bool KillAllTimer();

public:
	//投递事件
	virtual bool PostCustomEvent(DWORD dwCustomID, VOID * pData, WORD wDataSize);
	//投递事件
	virtual bool PostCustomEventEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize);

protected:
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//时间事件
	virtual bool OnEventTimer(DWORD dwTimerID, WPARAM dwBindParameter);

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
	IAttemperEvent *				m_pAttemperEvent;					//事件接口
	CCryptoManager					m_CryptoManager;					//密码管理
};

//////////////////////////////////////////////////////////////////////////
