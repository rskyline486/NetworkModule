#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//��������ص��ӿ�
class IAcceptItemSink
{
public:
	//��ʼ����
	virtual VOID OnEventListenStart(CNativeInfo* pNativeInfo) = 0;
	//�����¼�
	virtual VOID OnEventListenAccept(CNativeInfo* pNativeInfo, SOCKET hSocket) = 0;
	//��������
	virtual VOID OnEventListenStop(CNativeInfo* pNativeInfo) = 0;
};

//////////////////////////////////////////////////////////////////////////

//��������
class CAcceptItem : public CNativeInfo
{
public:
	CAcceptItem(WORD wIndex, IAcceptItemSink * pAcceptItemSink);
	virtual ~CAcceptItem(void);

public:
	//��������
	DWORD Listen(HANDLE hCompletionPort, WORD wPort, DWORD dwUserData, ProtocolSupport Protocol, bool bWatch);
	//�ָ�����
	DWORD ResumeListen(HANDLE hCompletionPort);

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
	void PostAccept();

private:
	IAcceptItemSink *				m_pAcceptItemSink;					//�ص��ӿ�
	WORD							m_wListenPort;						//�����˿�
	ProtocolSupport					m_Protocol;							//Э������
	LPFN_ACCEPTEX					m_fnAcceptEx;						//�ӿں���
	COverLappedAccept				m_OverLappedAccept;					//�ص��ṹ
};

//////////////////////////////////////////////////////////////////////////

//�����������
class CAcceptItemManager : public CNativeInfoManager
{
public:
	CAcceptItemManager();
	virtual ~CAcceptItemManager();

public:
	//��ʼ���������
	bool Init(HANDLE hCompletionPort, IAcceptItemSink * pAcceptItemSink, WORD wMaxItemCount);
	//�ͷŹ������
	bool Release();

public:
	//�������
	DWORD ActiveAcceptItem(WORD wListenPort, DWORD dwUserData, WORD wProtocol, bool bWatch);

public:
	//������
	bool DetectItem();

private:
	IAcceptItemSink *				m_pAcceptItemSink;					//�ص��ӿ�
};

//////////////////////////////////////////////////////////////////////////
