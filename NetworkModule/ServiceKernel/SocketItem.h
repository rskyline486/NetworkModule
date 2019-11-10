#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//�������ص��ӿ�
class ISocketItemSink
{
public:
	//���¼�
	virtual VOID OnEventSocketBind(CNativeInfo* pNativeInfo) = 0;
	//��ȡ�¼�
	virtual DWORD OnEventSocketRead(CNativeInfo* pNativeInfo, VOID * pData, DWORD dwDataSize) = 0;
	//�ر��¼�
	virtual VOID OnEventSocketShut(CNativeInfo* pNativeInfo) = 0;
};

//////////////////////////////////////////////////////////////////////////

//�������
class CSocketItem : public CNativeInfo
{
public:
	CSocketItem(WORD wIndex, ISocketItemSink * pSocketItemSink);
	virtual ~CSocketItem(void);

public:
	//�󶨶���
	DWORD Attach(HANDLE hCompletionPort, SOCKET hSocket);

public:
	//��ȡ����
	DWORD GetRecvDataCount();
	//��ȡ����
	DWORD GetSendDataCount();

public:
	//�������
	bool Heartbeat(DWORD dwNowTime);

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
	//������ַ
	DWORD PraseClientIP(CHAR * pszBuffer, DWORD dwBufferSize);

private:
	ISocketItemSink *				m_pSocketItemSink;					//�ص��ӿ�
	COverLappedRecv					m_OverLappedRecv;					//�ص��ṹ
	COverLappedSend					m_OverLappedSend;					//�ص��ṹ
	DWORD							m_dwRecvDataCount;					//��������
	DWORD							m_dwSendDataCount;					//��������
	WORD							m_wSurvivalTime;					//����ʱ��
};

//////////////////////////////////////////////////////////////////////////

//����������
class CSocketItemManager : public CNativeInfoManager
{
public:
	CSocketItemManager();
	virtual ~CSocketItemManager();

public:
	//��ʼ���������
	bool Init(HANDLE hCompletionPort, ISocketItemSink * pSocketItemSink, WORD wMaxItemCount);
	//�ͷŹ������
	bool Release();

public:
	//�������
	DWORD ActiveSocketItem(SOCKET hSocket);

public:
	//������
	bool DetectItem();

private:
	ISocketItemSink *				m_pSocketItemSink;					//�ص��ӿ�
};

//////////////////////////////////////////////////////////////////////////
