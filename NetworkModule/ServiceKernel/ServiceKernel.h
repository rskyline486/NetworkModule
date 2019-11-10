#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"
#include "AcceptItem.h"
#include "SocketItem.h"
#include "ConnectItem.h"
#include "WorkerThread.h"
#include "DetectThread.h"
#include "AsynchronismEngine.h"
#include "TimerEngine.h"
#include "CryptoCore.h"

//////////////////////////////////////////////////////////////////////////

//网络事件
class ISocketEvent
{
public:
	//连接事件
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData) = 0;
	//关闭事件
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData) = 0;
	//读取事件
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData) = 0;
};

//////////////////////////////////////////////////////////////////////////

//网络事件
class INetworkEvent
{
public:
	//绑定事件
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData) = 0;
	//关闭事件
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData) = 0;
	//读取事件
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData) = 0;
};

//////////////////////////////////////////////////////////////////////////

//服务核心
class CServiceKernel : protected IAcceptItemSink, protected ISocketItemSink, protected IConnectItemSink, protected IDetectEventSink, protected IAsynchronismEventSink
{
public:
	CServiceKernel();
	virtual ~CServiceKernel();

public:
	//初始化服务
	bool InitService(WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount, WORD wMaxConnectCount);
	//释放服务
	bool ReleaseService();

protected:
	//监听端口
	DWORD ListenPort(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch);
	//连接服务
	DWORD ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

protected:
	//发送函数
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//群发数据
	bool SendDataBatch(VOID * pData, WORD wDataSize);
	//关闭连接
	bool CloseSocket(DWORD dwSocketID);
	//设置监测
	bool SetDetect(DWORD dwSocketID, bool bDetect);
	//设置数据
	bool SetUserData(DWORD dwSocketID, DWORD dwUserData);
	//设置关闭
	bool ShutDownSocket(DWORD dwSocketID);

protected:
	//检测事件
	virtual VOID OnEventDetectBeat();

protected:
	//异步开始
	virtual bool OnEventAsynchronismStrat();
	//异步结束
	virtual bool OnEventAsynchronismStop();
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//开始监听
	virtual VOID OnEventListenStart(CNativeInfo* pNativeInfo);
	//接收事件
	virtual VOID OnEventListenAccept(CNativeInfo* pNativeInfo, SOCKET hSocket);
	//结束监听
	virtual VOID OnEventListenStop(CNativeInfo* pNativeInfo);

protected:
	//绑定事件
	virtual VOID OnEventSocketBind(CNativeInfo* pNativeInfo);
	//读取事件
	virtual DWORD OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//关闭事件
	virtual VOID OnEventSocketShut(CNativeInfo* pNativeInfo);

protected:
	//连接事件
	virtual VOID OnEventConnectLink(CNativeInfo* pNativeInfo);
	//读取事件
	virtual DWORD OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//关闭事件
	virtual VOID OnEventConnectShut(CNativeInfo* pNativeInfo);

protected:
	HANDLE							m_hCompletionPort;					//完成端口
	std::vector<CThread *>			m_WorkerThreadPtrList;				//工作线程
	CDetectThread					m_DetectThread;						//检测线程
	CAcceptItemManager				m_AcceptItemManager;				//监听对象
	CSocketItemManager				m_SocketItemManager;				//网络对象
	CConnectItemManager				m_ConnectItemManager;				//连接对象
	CAsynchronismEngine				m_AsynchronismEngine;				//异步对象
	CMutex							m_Mutex;							//同步对象
	BYTE							m_cbBuffer[ASYNCHRONISM_BUFFER];	//临时缓冲
};

//////////////////////////////////////////////////////////////////////////

//服务模块
class CServer : protected CServiceKernel, protected INetworkEvent
{
public:
	CServer();
	~CServer();

public:
	//初始化服务
	bool Init(INetworkEvent * pNetworkEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxAcceptCount, WORD wMaxSocketCount);
	//释放服务
	bool Release();
	//设置心跳包
	bool SetHeartbeatPacket(VOID * pData, WORD wDataSize);
	//监听端口
	DWORD Listen(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch);
	//获取地址
	DWORD GetClientIP(DWORD dwSocketID);

public:
	//发送数据
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//群发数据
	bool SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//关闭连接
	bool CloseSocket(DWORD dwSocketID, bool bSynchronize = false);
	//设置监测
	bool SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize = false);
	//设置数据
	bool SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize = false);
	//设置关闭
	bool ShutDownSocket(DWORD dwSocketID, bool bSynchronize = false);

protected:
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//绑定事件
	virtual VOID OnEventSocketBind(CNativeInfo* pNativeInfo);
	//读取事件
	virtual DWORD OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//关闭事件
	virtual VOID OnEventSocketShut(CNativeInfo* pNativeInfo);

protected:
	//绑定事件
	virtual bool OnEventNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData);
	//读取事件
	virtual int OnEventNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	INetworkEvent *					m_pNetworkEvent;					//回调接口
};

//////////////////////////////////////////////////////////////////////////

//连接模块
class CClient : protected CServiceKernel, protected ISocketEvent
{
public:
	CClient();
	~CClient();

public:
	//初始化服务
	bool Init(ISocketEvent * pISocketEvent, WORD wThreadCount, DWORD dwDetectTime, WORD wMaxConnectCount);
	//释放服务
	bool Release();
	//设置心跳包
	bool SetHeartbeatPacket(VOID * pData, WORD wDataSize);
	//连接服务
	DWORD Connect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//发送数据
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//群发数据
	bool SendDataBatch(VOID * pData, WORD wDataSize, bool bSynchronize = false);
	//关闭连接
	bool CloseSocket(DWORD dwSocketID, bool bSynchronize = false);
	//设置监测
	bool SetDetect(DWORD dwSocketID, bool bDetect, bool bSynchronize = false);
	//设置数据
	bool SetUserData(DWORD dwSocketID, DWORD dwUserData, bool bSynchronize = false);
	//设置关闭
	bool ShutDownSocket(DWORD dwSocketID, bool bSynchronize = false);

protected:
	//异步事件
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//连接事件
	virtual VOID OnEventConnectLink(CNativeInfo* pNativeInfo);
	//读取事件
	virtual DWORD OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize);
	//关闭事件
	virtual VOID OnEventConnectShut(CNativeInfo* pNativeInfo);

protected:
	//连接事件
	virtual bool OnEventSocketLink(DWORD dwSocketID, DWORD dwUserData);
	//关闭事件
	virtual bool OnEventSocketShut(DWORD dwSocketID, DWORD dwUserData);
	//读取事件
	virtual int OnEventSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData);

private:
	ISocketEvent *					m_pISocketEvent;					//回调接口
};

//////////////////////////////////////////////////////////////////////////
