#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//网络对象回调接口
class ISocketItemSink
{
public:
	//绑定事件
	virtual VOID OnEventSocketBind(CNativeInfo* pNativeInfo) = 0;
	//读取事件
	virtual DWORD OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize) = 0;
	//关闭事件
	virtual VOID OnEventSocketShut(CNativeInfo* pNativeInfo) = 0;
};

//////////////////////////////////////////////////////////////////////////

//网络对象
class CSocketItem : public CNativeInfo
{
public:
	CSocketItem(WORD wIndex, ISocketItemSink * pSocketItemSink);
	virtual ~CSocketItem(void);

public:
	//绑定对象
	DWORD Attach(HANDLE hCompletionPort, SOCKET hSocket);

public:
	//获取接收
	DWORD GetRecvDataCount();
	//获取发送
	DWORD GetSendDataCount();

public:
	//心跳检测
	bool Heartbeat(DWORD dwNowTime);

public:
	//处理请求
	virtual void DealAsync(COverLapped * pOverLapped, DWORD dwThancferred);
	//发送函数
	virtual bool SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID);
	//关闭连接
	virtual bool CloseSocket(WORD wRountID);

private:
	//关闭连接
	void CloseSocket();
	//恢复数据
	void ResumeData();
	//投递请求
	void PostRecv();
	//投递请求
	void PostSend();

private:
	//解析地址
	DWORD PraseClientIP(CHAR * pszBuffer, DWORD dwBufferSize);

private:
	ISocketItemSink *				m_pSocketItemSink;					//回调接口
	COverLappedRecv					m_OverLappedRecv;					//重叠结构
	COverLappedSend					m_OverLappedSend;					//重叠结构
	DWORD							m_dwRecvDataCount;					//接收流量
	DWORD							m_dwSendDataCount;					//发送流量
	WORD							m_wSurvivalTime;					//生存时间
};

//////////////////////////////////////////////////////////////////////////

//网络对象管理
class CSocketItemManager : public CNativeInfoManager
{
public:
	CSocketItemManager();
	virtual ~CSocketItemManager();

public:
	//初始化管理对象
	bool Init(HANDLE hCompletionPort, ISocketItemSink * pSocketItemSink, WORD wMaxItemCount);
	//释放管理对象
	bool Release();

public:
	//激活对象
	DWORD ActiveSocketItem(SOCKET hSocket);

public:
	//检测对象
	bool DetectItem();

private:
	ISocketItemSink *				m_pSocketItemSink;					//回调接口
};

//////////////////////////////////////////////////////////////////////////
