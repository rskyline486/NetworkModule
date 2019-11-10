#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//监听对象回调接口
class IAcceptItemSink
{
public:
	//开始监听
	virtual VOID OnEventListenStart(CNativeInfo* pNativeInfo) = 0;
	//接收事件
	virtual VOID OnEventListenAccept(CNativeInfo* pNativeInfo, SOCKET hSocket) = 0;
	//结束监听
	virtual VOID OnEventListenStop(CNativeInfo* pNativeInfo) = 0;
};

//////////////////////////////////////////////////////////////////////////

//监听对象
class CAcceptItem : public CNativeInfo
{
public:
	CAcceptItem(WORD wIndex, IAcceptItemSink * pAcceptItemSink);
	virtual ~CAcceptItem(void);

public:
	//监听对象
	DWORD Listen(HANDLE hCompletionPort, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch);
	//恢复监听
	DWORD ResumeListen(HANDLE hCompletionPort);

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
	void PostAccept();

private:
	IAcceptItemSink *				m_pAcceptItemSink;					//回调接口
	WORD							m_wListenPort;						//监听端口
	ProtocolSupport					m_Protocol;							//协议类型
	LPFN_ACCEPTEX					m_fnAcceptEx;						//接口函数
	COverLappedAccept				m_OverLappedAccept;					//重叠结构
};

//////////////////////////////////////////////////////////////////////////

//监听对象管理
class CAcceptItemManager : public CNativeInfoManager
{
public:
	CAcceptItemManager();
	virtual ~CAcceptItemManager();

public:
	//初始化管理对象
	bool Init(HANDLE hCompletionPort, IAcceptItemSink * pAcceptItemSink, WORD wMaxItemCount);
	//释放管理对象
	bool Release();

public:
	//激活对象
	DWORD ActiveAcceptItem(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//检测对象
	bool DetectItem();

private:
	IAcceptItemSink *				m_pAcceptItemSink;					//回调接口
};

//////////////////////////////////////////////////////////////////////////
