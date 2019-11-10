#pragma once

//////////////////////////////////////////////////////////////////////////

#include "Logger.h"
#include "Thread.h"
#include "Locker.h"
#include "Buffer.h"
#include "DataQueue.h"
#include "Network.h"

//////////////////////////////////////////////////////////////////////////

//操作类型
enum enOperationType
{
	enOperationType_Send,			//发送类型
	enOperationType_Recv,			//接收类型
	enOperationType_Accept,			//接受类型
	enOperationType_Connect,		//连接类型
	enOperationType_Count			//类型数量
};

//////////////////////////////////////////////////////////////////////////

//重叠结构类
class COverLapped
{
public:
	COverLapped(enOperationType OperationType, DWORD dwBufferSize);
	virtual ~COverLapped();

public:
	//数据指针
	operator LPWSABUF();
	//重叠结构
	operator LPWSAOVERLAPPED();

public:
	//恢复缓冲
	virtual bool ResumeBuffer() = 0;
	//重置数据
	virtual void ResetData() = 0;

public:
	//缓冲地址
	LPVOID GetBufferAddr();
	//缓冲大小
	DWORD GetBufferSize();

public:
	//操作类型
	enOperationType GetOperationType();
	//设置类型
	void SetOperationType(enOperationType OperationType);

public:
	//设置标识
	void SetHandleIng(bool bHandleIng);
	//获取标识
	bool GetHandleIng();

protected:
	friend class CWorkerThread;

	WSABUF							m_WSABuffer;						//数据指针
	OVERLAPPED						m_OverLapped;						//重叠结构
	enOperationType					m_OperationType;					//操作类型
	bool							m_bHandleIng;						//投递标识
	CBuffer							m_Buffer;							//缓冲数据
};

//////////////////////////////////////////////////////////////////////////

//发送重叠结构
class COverLappedSend : public COverLapped
{
public:
	COverLappedSend();
	virtual ~COverLappedSend();

public:
	//恢复缓冲
	virtual bool ResumeBuffer();
	//重置数据
	virtual void ResetData();

public:
	//发送数据
	bool SendData(LPVOID lpData, DWORD dwDataSize);
	//发送完毕
	bool SendCompleted(DWORD dwSendSize);
};

//////////////////////////////////////////////////////////////////////////

//接收重叠结构
class COverLappedRecv : public COverLapped
{
public:
	COverLappedRecv();
	virtual ~COverLappedRecv();

public:
	//恢复缓冲
	virtual bool ResumeBuffer();
	//重置数据
	virtual void ResetData();

public:
	//接收完毕
	bool RecvCompleted(DWORD dwRecvSize);
	//处理完毕
	bool DealCompleted(DWORD dwDealSize);
};

//////////////////////////////////////////////////////////////////////////

//监听重叠结构
class COverLappedAccept : public COverLapped
{
public:
	COverLappedAccept();
	virtual ~COverLappedAccept();

public:
	//恢复缓冲
	virtual bool ResumeBuffer();
	//重置数据
	virtual void ResetData();

public:
	//存储对象
	bool StoreSocket(SOCKET hSocket);
	//取出对象
	SOCKET GetOutSocket();

private:
	SOCKET							m_hSocket;							//连接句柄
};

//////////////////////////////////////////////////////////////////////////

//连接对象基类
class CNativeInfo
{
public:
	CNativeInfo(WORD wIndex);
	virtual ~CNativeInfo(void);

public:
	//处理请求
	virtual void DealAsync(COverLapped * pOverLapped, DWORD dwThancferred) = 0;
	//发送函数
	virtual bool SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID) = 0;
	//关闭连接
	virtual bool CloseSocket(WORD wRountID) = 0;

public:
	//设置监测
	virtual bool SetDetect(WORD wRountID, bool bDetect);
	//设置数据
	virtual bool SetUserData(WORD wRountID, DWORD dwUserData);
	//设置关闭
	virtual bool ShutDownSocket(WORD wRountID);

public:
	//获取数据
	DWORD GetUserData();
	//设置监测
	void SetDetect();
	//取消监测
	void CancelDetect();
	//获取监测
	bool GetDetect();

public:
	//判断使用
	bool IsUsed();
	//获取索引
	WORD GetIndex();
	//获取计数
	WORD GetRountID();
	//获取标识
	DWORD GetSocketID();
	//激活时间
	DWORD GetActiveTime();

public:
	//获取地址
	bool GetLocalAddress(TCHAR * pszIBuffer, DWORD dwBufferLength, LPWORD pwPort);
	//获取地址
	bool GetRemoteAddress(TCHAR * pszBuffer, DWORD dwBufferLength, LPWORD pwPort);
	//获取IPV4
	DWORD GetClientIPV4();

protected:
	//设置数据
	void InitData();
	//发送判断
	bool SendVerdict(WORD wRountID);
	//重置数据
	void ResetData();

protected:
	Address							m_LocalAddress;						//本地地址
	Address							m_RemoteAddress;					//对端地址
	DWORD							m_dwActiveTime;						//激活时间
	DWORD							m_dwRealIPV4;						//真实IP信息

protected:
	CMutex							m_Mutex;							//同步对象
	DWORD							m_dwUserData;						//用户数据
	SOCKET							m_hSocket;							//连接句柄
	WORD							m_wIndex;							//连接索引
	WORD							m_wRountID;							//循环索引
	bool							m_bUsed;							//使用标识
	bool							m_bDetect;							//监测标识
	bool							m_bShutDown;						//关闭标志
};

//////////////////////////////////////////////////////////////////////////

//常量定义
#define MAX_ADDRSTRLEN				INET6_ADDRSTRLEN					//地址长度
#define INVALID_SOCKETID			0L									//无效句柄
#define MAX_SOCKET					512									//最大连接
#define SOCKET_BUFFER				16384								//缓冲大小
#define ASYNCHRONISM_BUFFER			(SOCKET_BUFFER+24)					//异步数据(拓展24字节的头部信息)

//计时器常量
#define TIMES_INFINITY				DWORD(-1)							//无限次数

//索引定义
#define INDEX_SOCKET				(WORD)(0x0000)						//网络索引
#define INDEX_CONNECT				(WORD)(0x4000)						//连接索引
#define INDEX_LISTEN				(WORD)(0x8000)						//监听索引

//索引辅助
#define SOCKET_INDEX(dwSocketID)	LOWORD(dwSocketID)					//位置索引
#define SOCKET_ROUNTID(dwSocketID)	HIWORD(dwSocketID)					//循环索引

//句柄检测
#define CHECK_SOCKETID(dwSocketID)	(HIWORD(dwSocketID) > 0)			//检测句柄

//心跳数据包定义
struct tagHeartbeatPacket
{
	WORD							wDataSize;							//数据大小
	BYTE							cbSendBuffer[SOCKET_BUFFER];		//发送缓冲
};

//////////////////////////////////////////////////////////////////////////

//连接对象管理类
class CNativeInfoManager
{
public:
	CNativeInfoManager();
	virtual ~CNativeInfoManager();

public:
	//获取大小
	WORD GetTotalNativeCount();
	//获取大小
	WORD GetActiveNativeCount();

public:
	//设置心跳数据
	bool SetHeartbeatData(VOID * pData, WORD wDataSize);

public:
	//发送数据
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
	//获取地址
	DWORD GetClientIP(DWORD dwSocketID);

protected:
	//获取空闲
	CNativeInfo * GetFreeNativeInfo();
	//获取索引
	WORD GetIndex(DWORD dwSocketID);
	//获取计数
	WORD GetRountID(DWORD dwSocketID);

protected:
	//设置参数
	bool SetParameter(HANDLE hCompletionPort, WORD wStartIndex, WORD wMaxNativeItemCount);
	//释放连接
	bool ReleaseNativeInfo(bool bFreeMemroy);

protected:
	CMutex							m_Mutex;							//同步对象
	HANDLE							m_hCompletionPort;					//完成端口
	WORD							m_wStartIndex;						//开始索引
	std::vector<CNativeInfo *>		m_NativeInfoPtrList;				//操作对象
	WORD							m_wMaxNativeItemCount;				//最大连接
	tagHeartbeatPacket				m_HeartbeatPacket;					//心跳数据
};

//////////////////////////////////////////////////////////////////////////

//数组维数
#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))
//存储长度
#define CountStringBuffer(String) ((UINT)((lstrlen(String)+1)*sizeof(TCHAR)))

//////////////////////////////////////////////////////////////////////////

//服务核心
class CServiceCore
{
public:
	CServiceCore();
	~CServiceCore();

public:
	static bool W2A(CHAR * pszBuffer, DWORD dwBufferLength, LPCWSTR pszSource);
	static bool A2W(WCHAR * pszBuffer, DWORD dwBufferLength, LPCSTR pszSource);
	static bool T2A(CHAR * pszBuffer, DWORD dwBufferLength, LPCTSTR pszSource);
	static bool A2T(TCHAR * pszBuffer, DWORD dwBufferLength, LPCSTR pszSource);
};

//////////////////////////////////////////////////////////////////////////
