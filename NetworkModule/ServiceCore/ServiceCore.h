#pragma once

//////////////////////////////////////////////////////////////////////////

#include "Logger.h"
#include "Thread.h"
#include "Locker.h"
#include "Buffer.h"
#include "DataQueue.h"
#include "Network.h"

//////////////////////////////////////////////////////////////////////////

//��������
enum enOperationType
{
	enOperationType_Send,			//��������
	enOperationType_Recv,			//��������
	enOperationType_Accept,			//��������
	enOperationType_Connect,		//��������
	enOperationType_Count			//��������
};

//////////////////////////////////////////////////////////////////////////

//�ص��ṹ��
class COverLapped
{
public:
	COverLapped(enOperationType OperationType, DWORD dwBufferSize);
	virtual ~COverLapped();

public:
	//����ָ��
	operator LPWSABUF();
	//�ص��ṹ
	operator LPWSAOVERLAPPED();

public:
	//�ָ�����
	virtual bool ResumeBuffer() = 0;
	//��������
	virtual void ResetData() = 0;

public:
	//�����ַ
	LPVOID GetBufferAddr();
	//�����С
	DWORD GetBufferSize();

public:
	//��������
	enOperationType GetOperationType();
	//��������
	void SetOperationType(enOperationType OperationType);

public:
	//���ñ�ʶ
	void SetHandleIng(bool bHandleIng);
	//��ȡ��ʶ
	bool GetHandleIng();

protected:
	friend class CWorkerThread;

	WSABUF							m_WSABuffer;						//����ָ��
	OVERLAPPED						m_OverLapped;						//�ص��ṹ
	enOperationType					m_OperationType;					//��������
	bool							m_bHandleIng;						//Ͷ�ݱ�ʶ
	CBuffer							m_Buffer;							//��������
};

//////////////////////////////////////////////////////////////////////////

//�����ص��ṹ
class COverLappedSend : public COverLapped
{
public:
	COverLappedSend();
	virtual ~COverLappedSend();

public:
	//�ָ�����
	virtual bool ResumeBuffer();
	//��������
	virtual void ResetData();

public:
	//��������
	bool SendData(LPVOID lpData, DWORD dwDataSize);
	//�������
	bool SendCompleted(DWORD dwSendSize);
};

//////////////////////////////////////////////////////////////////////////

//�����ص��ṹ
class COverLappedRecv : public COverLapped
{
public:
	COverLappedRecv();
	virtual ~COverLappedRecv();

public:
	//�ָ�����
	virtual bool ResumeBuffer();
	//��������
	virtual void ResetData();

public:
	//�������
	bool RecvCompleted(DWORD dwRecvSize);
	//�������
	bool DealCompleted(DWORD dwDealSize);
};

//////////////////////////////////////////////////////////////////////////

//�����ص��ṹ
class COverLappedAccept : public COverLapped
{
public:
	COverLappedAccept();
	virtual ~COverLappedAccept();

public:
	//�ָ�����
	virtual bool ResumeBuffer();
	//��������
	virtual void ResetData();

public:
	//�洢����
	bool StoreSocket(SOCKET hSocket);
	//ȡ������
	SOCKET GetOutSocket();

private:
	SOCKET							m_hSocket;							//���Ӿ��
};

//////////////////////////////////////////////////////////////////////////

//���Ӷ������
class CNativeInfo
{
public:
	CNativeInfo(WORD wIndex);
	virtual ~CNativeInfo(void);

public:
	//��������
	virtual void DealAsync(COverLapped * pOverLapped, DWORD dwThancferred) = 0;
	//���ͺ���
	virtual bool SendData(LPVOID lpData, DWORD dwDataSize, WORD wRountID) = 0;
	//�ر�����
	virtual bool CloseSocket(WORD wRountID) = 0;

public:
	//���ü��
	virtual bool SetDetect(WORD wRountID, bool bDetect);
	//��������
	virtual bool SetUserData(WORD wRountID, DWORD dwUserData);
	//���ùر�
	virtual bool ShutDownSocket(WORD wRountID);

public:
	//��ȡ����
	DWORD GetUserData();
	//���ü��
	void SetDetect();
	//ȡ�����
	void CancelDetect();
	//��ȡ���
	bool GetDetect();

public:
	//�ж�ʹ��
	bool IsUsed();
	//��ȡ����
	WORD GetIndex();
	//��ȡ����
	WORD GetRountID();
	//��ȡ��ʶ
	DWORD GetSocketID();
	//����ʱ��
	DWORD GetActiveTime();

public:
	//��ȡ��ַ
	bool GetLocalAddress(TCHAR * pszIBuffer, DWORD dwBufferLength, LPWORD pwPort);
	//��ȡ��ַ
	bool GetRemoteAddress(TCHAR * pszBuffer, DWORD dwBufferLength, LPWORD pwPort);
	//��ȡIPV4
	DWORD GetClientIPV4();

protected:
	//��������
	void InitData();
	//�����ж�
	bool SendVerdict(WORD wRountID);
	//��������
	void ResetData();

protected:
	Address							m_LocalAddress;						//���ص�ַ
	Address							m_RemoteAddress;					//�Զ˵�ַ
	DWORD							m_dwActiveTime;						//����ʱ��
	DWORD							m_dwRealIPV4;						//��ʵIP��Ϣ

protected:
	CMutex							m_Mutex;							//ͬ������
	DWORD							m_dwUserData;						//�û�����
	SOCKET							m_hSocket;							//���Ӿ��
	WORD							m_wIndex;							//��������
	WORD							m_wRountID;							//ѭ������
	bool							m_bUsed;							//ʹ�ñ�ʶ
	bool							m_bDetect;							//����ʶ
	bool							m_bShutDown;						//�رձ�־
};

//////////////////////////////////////////////////////////////////////////

//��������
#define MAX_ADDRSTRLEN				INET6_ADDRSTRLEN					//��ַ����
#define INVALID_SOCKETID			0L									//��Ч���
#define MAX_SOCKET					512									//�������
#define SOCKET_BUFFER				16384								//�����С
#define ASYNCHRONISM_BUFFER			(SOCKET_BUFFER+24)					//�첽����(��չ24�ֽڵ�ͷ����Ϣ)

//��ʱ������
#define TIMES_INFINITY				DWORD(-1)							//���޴���

//��������
#define INDEX_SOCKET				(WORD)(0x0000)						//��������
#define INDEX_CONNECT				(WORD)(0x4000)						//��������
#define INDEX_LISTEN				(WORD)(0x8000)						//��������

//��������
#define SOCKET_INDEX(dwSocketID)	LOWORD(dwSocketID)					//λ������
#define SOCKET_ROUNTID(dwSocketID)	HIWORD(dwSocketID)					//ѭ������

//������
#define CHECK_SOCKETID(dwSocketID)	(HIWORD(dwSocketID) > 0)			//�����

//�������ݰ�����
struct tagHeartbeatPacket
{
	WORD							wDataSize;							//���ݴ�С
	BYTE							cbSendBuffer[SOCKET_BUFFER];		//���ͻ���
};

//////////////////////////////////////////////////////////////////////////

//���Ӷ��������
class CNativeInfoManager
{
public:
	CNativeInfoManager();
	virtual ~CNativeInfoManager();

public:
	//��ȡ��С
	WORD GetTotalNativeCount();
	//��ȡ��С
	WORD GetActiveNativeCount();

public:
	//������������
	bool SetHeartbeatData(VOID * pData, WORD wDataSize);

public:
	//��������
	bool SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize);
	//Ⱥ������
	bool SendDataBatch(VOID * pData, WORD wDataSize);
	//�ر�����
	bool CloseSocket(DWORD dwSocketID);
	//���ü��
	bool SetDetect(DWORD dwSocketID, bool bDetect);
	//��������
	bool SetUserData(DWORD dwSocketID, DWORD dwUserData);
	//���ùر�
	bool ShutDownSocket(DWORD dwSocketID);
	//��ȡ��ַ
	DWORD GetClientIP(DWORD dwSocketID);

protected:
	//��ȡ����
	CNativeInfo * GetFreeNativeInfo();
	//��ȡ����
	WORD GetIndex(DWORD dwSocketID);
	//��ȡ����
	WORD GetRountID(DWORD dwSocketID);

protected:
	//���ò���
	bool SetParameter(HANDLE hCompletionPort, WORD wStartIndex, WORD wMaxNativeItemCount);
	//�ͷ�����
	bool ReleaseNativeInfo(bool bFreeMemroy);

protected:
	CMutex							m_Mutex;							//ͬ������
	HANDLE							m_hCompletionPort;					//��ɶ˿�
	WORD							m_wStartIndex;						//��ʼ����
	std::vector<CNativeInfo *>		m_NativeInfoPtrList;				//��������
	WORD							m_wMaxNativeItemCount;				//�������
	tagHeartbeatPacket				m_HeartbeatPacket;					//��������
};

//////////////////////////////////////////////////////////////////////////

//����ά��
#define CountArray(Array) (sizeof(Array)/sizeof(Array[0]))
//�洢����
#define CountStringBuffer(String) ((UINT)((lstrlen(String)+1)*sizeof(TCHAR)))

//////////////////////////////////////////////////////////////////////////

//�������
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
