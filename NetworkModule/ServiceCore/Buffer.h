#pragma once

//////////////////////////////////////////////////////////////////////////

//缓冲结构
class CBuffer
{
public:
	CBuffer();
	~CBuffer();

public:
	//初始化
	bool Init(DWORD dwBufferSize);
	//释放
	void Release();

public:
	//接收大小
	bool RecvSize(DWORD dwRecvSize);
	//处理大小
	bool DealSize(DWORD dwDealSize);

public:
	//数据指针
	BYTE* GetData();
	//数据大小
	DWORD GetDataSize();

public:
	//投递指针
	BYTE* GetDeliverData(DWORD dwDeliverSize);

private:
	//调整大小
	bool Resize(DWORD dwNeedSize);

private:
	BYTE *							m_pBuffer;							//缓冲指针
	DWORD							m_dwBufferSize;						//缓冲长度
	DWORD							m_dwDataSize;						//数据大小
};

//////////////////////////////////////////////////////////////////////////
