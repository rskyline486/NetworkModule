#pragma once

//////////////////////////////////////////////////////////////////////////

//����ṹ
class CBuffer
{
public:
	CBuffer();
	~CBuffer();

public:
	//��ʼ��
	bool Init(DWORD dwBufferSize);
	//�ͷ�
	void Release();

public:
	//���մ�С
	bool RecvSize(DWORD dwRecvSize);
	//�����С
	bool DealSize(DWORD dwDealSize);

public:
	//����ָ��
	BYTE* GetData();
	//���ݴ�С
	DWORD GetDataSize();

public:
	//Ͷ��ָ��
	BYTE* GetDeliverData(DWORD dwDeliverSize);

private:
	//������С
	bool Resize(DWORD dwNeedSize);

private:
	BYTE *							m_pBuffer;							//����ָ��
	DWORD							m_dwBufferSize;						//���峤��
	DWORD							m_dwDataSize;						//���ݴ�С
};

//////////////////////////////////////////////////////////////////////////
