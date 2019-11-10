#pragma once

//////////////////////////////////////////////////////////////////////////

//������ͷ
struct tagDataHead
{
	WORD							wDataSize;							//���ݴ�С
	WORD							wIdentifier;						//���ͱ�ʶ
};

//������Ϣ
struct tagBurthenInfo
{
	DWORD							dwBufferSize;						//���峤��
	DWORD							dwDataSize;							//���ݴ�С
	DWORD							dwPacketCount;						//���ݰ���
};

//////////////////////////////////////////////////////////////////////////

//����ṹ
class CDataQueue
{
public:
	CDataQueue();
	~CDataQueue();

public:
	//��ʼ��С
	void InitSize(DWORD dwInitBufferSize);
	//������Ϣ
	void GetBurthenInfo(tagBurthenInfo & BurthenInfo);

public:
	//��������
	bool InsertData(WORD wIdentifier, VOID * pBuffer, WORD wDataSize);
	//��ȡ����
	bool DistillData(tagDataHead & DataHead, VOID * pBuffer, WORD wBufferSize);
	//ɾ������
	void RemoveData(bool bFreeMemroy);

private:
	//������С
	bool Resize(DWORD dwNeedSize);

private:
	CRITICAL_SECTION				m_Mutex;							//�ٽ����
	BYTE *							m_pBuffer;							//����ָ��
	DWORD							m_dwBufferSize;						//���峤��
	DWORD							m_dwDataSize;						//���ݴ�С
	DWORD							m_dwPacketCount;					//���ݰ���

private:
	DWORD							m_dwInsertPos;						//����λ��
	DWORD							m_dwTerminalPos;					//����λ��
	DWORD							m_dwDataQueryPos;					//��ѯλ��
};

//////////////////////////////////////////////////////////////////////////
