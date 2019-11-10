#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//���Ӷ���ص��ӿ�
class IConnectItemSink
{
public:
	//�����¼�
	virtual VOID OnEventConnectLink(CNativeInfo* pNativeInfo) = 0;
	//��ȡ�¼�
	virtual DWORD OnEventConnectRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize) = 0;
	//�ر��¼�
	virtual VOID OnEventConnectShut(CNativeInfo* pNativeInfo) = 0;
};

//////////////////////////////////////////////////////////////////////////

//�������
class CConnectItem : public CNativeInfo
{
public:
	CConnectItem(WORD wIndex, IConnectItemSink * pConnectItemSink);
	virtual ~CConnectItem(void);

public:
	//���Ӷ���
	DWORD Connect(HANDLE hCompletionPort, LPCTSTR pszConnectAddress, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch);
	//�ָ�����
	DWORD ResumeConnect(HANDLE hCompletionPort);

public:
	//��������
	virtual void DealAsync(COverLapped * pOverLapped, DWORD dwThancferred);
	//���ͺ���
	virtual bool SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID);
	//�ر�����
	virtual bool CloseSocket(WORD wRountID);

private:
	//�ر�����
	void CloseSocket();
	//�ָ�����
	void ResumeData();
	//Ͷ������
	void PostRecv();
	//Ͷ������
	void PostSend();

private:
	IConnectItemSink *				m_pConnectItemSink;					//�ص��ӿ�
	TCHAR							m_szConnectAddress[33];				//���ӵ�ַ
	WORD							m_wConnectPort;						//���Ӷ˿�
	ProtocolSupport					m_Protocol;							//Э������
	COverLappedRecv					m_OverLappedRecv;					//�ص��ṹ
	COverLappedSend					m_OverLappedSend;					//�ص��ṹ
};

//////////////////////////////////////////////////////////////////////////

//����������
class CConnectItemManager : public CNativeInfoManager
{
public:
	CConnectItemManager();
	virtual ~CConnectItemManager();

public:
	//��ʼ���������
	bool Init(HANDLE hCompletionPort, IConnectItemSink * pConnectItemSink, WORD wMaxItemCount);
	//�ͷŹ������
	bool Release();

public:
	//�������
	DWORD ActiveConnectItem(LPCTSTR pszConnectAddress, WORD wConnectPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//������
	bool DetectItem();

private:
	IConnectItemSink *				m_pConnectItemSink;					//�ص��ӿ�
};

//////////////////////////////////////////////////////////////////////////
