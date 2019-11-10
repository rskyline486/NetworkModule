#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//连接对象回调接口
class IConnectItemSink
{
public:
	//连接事件
	virtual VOID OnEventConnectLink(CNativeInfo* pNativeInfo) = 0;
	//读取事件
	virtual DWORD OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize) = 0;
	//关闭事件
	virtual VOID OnEventConnectShut(CNativeInfo* pNativeInfo) = 0;
};

//////////////////////////////////////////////////////////////////////////

//网络对象
class CConnectItem : public CNativeInfo
{
public:
	CConnectItem(WORD wIndex, IConnectItemSink * pConnectItemSink);
	virtual ~CConnectItem(void);

public:
	//连接对象
	DWORD Connect(HANDLE hCompletionPort, LPCTSTR pszConnectAddress, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch);
	//恢复连接
	DWORD ResumeConnect(HANDLE hCompletionPort);

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
	IConnectItemSink *				m_pConnectItemSink;					//回调接口
	TCHAR							m_szConnectAddress[33];				//连接地址
	WORD							m_wConnectPort;						//连接端口
	ProtocolSupport					m_Protocol;							//协议类型
	COverLappedRecv					m_OverLappedRecv;					//重叠结构
	COverLappedSend					m_OverLappedSend;					//重叠结构
};

//////////////////////////////////////////////////////////////////////////

//网络对象管理
class CConnectItemManager : public CNativeInfoManager
{
public:
	CConnectItemManager();
	virtual ~CConnectItemManager();

public:
	//初始化管理对象
	bool Init(HANDLE hCompletionPort, IConnectItemSink * pConnectItemSink, WORD wMaxItemCount);
	//释放管理对象
	bool Release();

public:
	//激活对象
	DWORD ActiveConnectItem(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//检测对象
	bool DetectItem();

private:
	IConnectItemSink *				m_pConnectItemSink;					//回调接口
};

//////////////////////////////////////////////////////////////////////////
