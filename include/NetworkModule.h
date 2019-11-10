#pragma once

//////////////////////////////////////////////////////////////////////////

#include <windows.h>

//////////////////////////////////////////////////////////////////////////

//数组维数
#define CountArray(Array)			(sizeof(Array)/sizeof(Array[0]))
//存储长度
#define CountStringBuffer(String)	((UINT)((lstrlen(String)+1)*sizeof(TCHAR)))
//字符长度
#define CountString(String)			(UINT)((lstrlen(String)+1))

//常量定义
#define INVALID_SOCKETID			0L									//无效句柄
#define SOCKET_BUFFER				16384								//缓冲大小
#define ASYNCHRONISM_BUFFER			(SOCKET_BUFFER+24)					//异步数据(拓展24字节的头部信息)

//计时器常量
#define TIMES_INFINITY				DWORD(-1)							//无限次数

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//=============================代理接口部分=============================//
//////////////////////////////////////////////////////////////////////////

//代理参数
struct tagProxyOption
{
	WORD							wListenPort;						//监听端口
	WORD							wThreadCount;						//工作线程
	DWORD							dwDetectTime;						//检测间隔
	WORD							wProtocol;							//协议类型
	WORD							wMaxAcceptCount;					//监听对象
	WORD							wMaxSocketCount;					//网络对象
	WORD							wMaxConnectCount;					//连接对象
	TCHAR							szDefaultAddress[33];				//连接地址
	WORD							wDefaultPort;						//连接端口
};

//////////////////////////////////////////////////////////////////////////

//代理事件
class IProxyEvent
{
public:
	//连接事件
	virtual bool OnProxyServerLink(DWORD dwSocketID, DWORD dwUserData) = 0;
	//关闭事件
	virtual bool OnProxyServerShut(DWORD dwSocketID, DWORD dwUserData) = 0;
	//读取事件
	virtual int OnProxyServerRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData) = 0;

public:
	//绑定事件
	virtual bool OnProxyClientBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwUserData) = 0;
	//关闭事件
	virtual bool OnProxyClientShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime, DWORD dwUserData) = 0;
	//读取事件
	virtual int OnProxyClientRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData) = 0;
};

//////////////////////////////////////////////////////////////////////////

//代理接口 
class IProxyService
{
public:
	//启动服务
	virtual bool StartServer(tagProxyOption ProxyOption, IProxyEvent * pProxyEvent) = 0;
	//停止服务
	virtual bool StopServer() = 0;
	//连接服务
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch) = 0;

public:
	//代理连接
	virtual DWORD ProxyConnect(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol) = 0;
	//设置数据
	virtual bool SetUserData(DWORD dwSocketID, DWORD dwUserData) = 0;
	//发送数据
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize) = 0;
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID) = 0;
};

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//=============================服务接口部分=============================//
//////////////////////////////////////////////////////////////////////////

//服务参数
struct tagServerOption
{
	WORD							wListenPort;						//监听端口
	WORD							wThreadCount;						//工作线程
	DWORD							dwDetectTime;						//检测间隔
	WORD							wProtocol;							//协议类型
	WORD							wMaxAcceptCount;					//监听对象
	WORD							wMaxSocketCount;					//网络对象
	LPVOID							pHeartbeatData;						//心跳数据
	WORD							wHeartbeatDataSize;					//心跳大小
};

//////////////////////////////////////////////////////////////////////////

//服务事件
class IServerEvent
{
public:
	//绑定事件
	virtual bool OnServerNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort) = 0;
	//关闭事件
	virtual bool OnServerNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime) = 0;
	//读取事件
	virtual int OnServerNetworkRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize) = 0;
};

//////////////////////////////////////////////////////////////////////////

//服务接口 
class IServerService
{
public:
	//启动服务
	virtual bool StartServer(tagServerOption ServerOption, IServerEvent * pServerEvent) = 0;
	//停止服务
	virtual bool StopServer() = 0;

public:
	//发送数据
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize) = 0;
	//发送数据
	virtual bool SendDataBatch(VOID * pData, WORD wDataSize) = 0;
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID) = 0;
};

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//=============================连接接口部分=============================//
//////////////////////////////////////////////////////////////////////////

//连接参数
struct tagClientOption
{
	WORD							wThreadCount;						//工作线程
	DWORD							dwDetectTime;						//检测间隔
	WORD							wMaxConnectCount;					//连接对象
};

//////////////////////////////////////////////////////////////////////////

//连接事件
class IClientEvent
{
public:
	//连接事件
	virtual bool OnClientSocketLink(DWORD dwSocketID, DWORD dwUserData) = 0;
	//关闭事件
	virtual bool OnClientSocketShut(DWORD dwSocketID, DWORD dwUserData) = 0;
	//读取事件
	virtual int OnClientSocketRead(DWORD dwSocketID, VOID * pData, DWORD dwDataSize, DWORD dwUserData) = 0;
};

//////////////////////////////////////////////////////////////////////////

//连接接口 
class IClientService
{
public:
	//启动服务
	virtual bool StartServer(tagClientOption ClientOption, IClientEvent * pClientEvent) = 0;
	//停止服务
	virtual bool StopServer() = 0;
	//连接服务
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch) = 0;

public:
	//发送数据
	virtual bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize) = 0;
	//发送数据
	virtual bool SendDataBatch(VOID * pData, WORD wDataSize) = 0;
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID) = 0;
};

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//=============================异步接口部分=============================//
//////////////////////////////////////////////////////////////////////////

//异步事件
class IAsynchronismEvent
{
public:
	//异步开始
	virtual bool OnAsynchronismStart() = 0;
	//异步结束
	virtual bool OnAsynchronismStop() = 0;
	//异步事件
	virtual bool OnAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize) = 0;
};

//////////////////////////////////////////////////////////////////////////

//异步接口 
class IAsynchronismService
{
public:
	//启动服务
	virtual bool StartServer(WORD wThreadCount, IAsynchronismEvent * pAsynchronismEvent) = 0;
	//停止服务
	virtual bool StopServer() = 0;

public:
	//发送数据
	virtual bool PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize) = 0;
};

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//=============================时间接口部分=============================//
//////////////////////////////////////////////////////////////////////////

//时间事件
class ITimerEvent
{
public:
	//时间事件
	virtual bool OnTimerEvent(DWORD dwTimerID, WPARAM dwBindParameter) = 0;
};

//////////////////////////////////////////////////////////////////////////

//时间接口 
class ITimerService
{
public:
	//启动服务
	virtual bool StartServer(DWORD dwTimerSpace, ITimerEvent * pTimerEvent) = 0;
	//停止服务
	virtual bool StopServer() = 0;

public:
	//设置定时器
	virtual bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter) = 0;
	//删除定时器
	virtual bool KillTimer(DWORD dwTimerID) = 0;
	//删除定时器
	virtual bool KillAllTimer() = 0;
};

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//=============================调度接口部分=============================//
//////////////////////////////////////////////////////////////////////////

//调度参数
struct tagAttemperOption
{
	tagServerOption	*				pServerOption;						//服务参数
	tagClientOption	*				pClientOption;						//连接参数
};

//////////////////////////////////////////////////////////////////////////

//调度事件
class IAttemperEvent
{
public:
	//时间事件
	virtual bool OnAttemperTimer(DWORD dwTimerID, WPARAM dwBindParameter) = 0;
	//自定事件
	virtual bool OnAttemperCustom(DWORD dwCustomID, VOID * pData, WORD wDataSize) = 0;
	//自定事件
	virtual bool OnAttemperCustomEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize) = 0;

public:
	//连接绑定
	virtual bool OnAttemperSocketLink(DWORD dwSocketID, DWORD dwUserData) = 0;
	//连接关闭
	virtual bool OnAttemperSocketShut(DWORD dwSocketID, DWORD dwUserData) = 0;
	//连接读取
	virtual bool OnAttemperSocketRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, DWORD dwUserData) = 0;

public:
	//网络绑定
	virtual bool OnAttemperNetworkBind(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort) = 0;
	//网络关闭
	virtual bool OnAttemperNetworkShut(DWORD dwSocketID, LPCTSTR pszClientIP, WORD wClientPort, DWORD dwActiveTime) = 0;
	//网络读取
	virtual bool OnAttemperNetworkRead(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize) = 0;
};

//////////////////////////////////////////////////////////////////////////

//调度接口 
class IAttemperService
{
public:
	//启动服务
	virtual bool StartServer(tagAttemperOption AttemperOption, IAttemperEvent * pAttemperEvent) = 0;
	//停止服务
	virtual bool StopServer() = 0;

public:
	//连接服务
	virtual bool ConnectServer(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch) = 0;

public:
	//发送数据
	virtual bool SendData(DWORD dwSocketID, WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize) = 0;
	//群发数据
	virtual bool SendDataBatch(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize) = 0;
	//关闭连接
	virtual bool CloseSocket(DWORD dwSocketID) = 0;
	//安全关闭
	virtual bool ShutDownSocket(DWORD dwSocketID) = 0;

public:
	//设置定时器
	virtual bool SetTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter) = 0;
	//删除定时器
	virtual bool KillTimer(DWORD dwTimerID) = 0;
	//删除定时器
	virtual bool KillAllTimer() = 0;

public:
	//投递事件
	virtual bool PostCustomEvent(DWORD dwCustomID, VOID * pData, WORD wDataSize) = 0;
	//投递事件
	virtual bool PostCustomEventEx(DWORD dwCustomID, DWORD dwContextID, VOID * pData, WORD wDataSize) = 0;
};

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//=============================模块管理部分=============================//
//////////////////////////////////////////////////////////////////////////

//网络模块
class CNetworkModule
{
public:
	//启动进程
	static DWORD StartProcess(LPCTSTR pszProcessPath, LPCTSTR pszWindownTitle, LPCTSTR pszCommandLine, bool bCreateNewWindow, bool bHideWindow);
	//查找进程
	static bool FindProcess(DWORD dwProcesID);
	//结束进程
	static bool KillProcess(DWORD dwProcesID);

public:
	//启动模块
	static bool StartupNetwork();
	//清理模块
	static bool CleanupNetwork();
	//运行网络
	static bool RunNetwork();

public:
	//创建代理
	static bool CreateService(IProxyService ** ppProxyService);
	//销毁代理
	static bool DestroyService(IProxyService ** ppProxyService);

public:
	//创建服务
	static bool CreateService(IServerService ** ppServerService);
	//销毁服务
	static bool DestroyService(IServerService ** ppServerService);

public:
	//创建连接
	static bool CreateService(IClientService ** ppClientService);
	//销毁连接
	static bool DestroyService(IClientService ** ppClientService);

public:
	//创建异步
	static bool CreateService(IAsynchronismService ** ppAsynchronismService);
	//销毁异步
	static bool DestroyService(IAsynchronismService ** ppAsynchronismService);

public:
	//创建时间
	static bool CreateService(ITimerService ** ppTimerService);
	//销毁时间
	static bool DestroyService(ITimerService ** ppTimerService);

public:
	//创建调度
	static bool CreateService(IAttemperService ** ppAttemperService);
	//销毁调度
	static bool DestroyService(IAttemperService ** ppAttemperService);
};

//////////////////////////////////////////////////////////////////////////
