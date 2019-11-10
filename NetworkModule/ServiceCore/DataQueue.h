#pragma once

//////////////////////////////////////////////////////////////////////////

//数据列头
struct tagDataHead
{
	WORD							wDataSize;							//数据大小
	WORD							wIdentifier;						//类型标识
};

//负荷信息
struct tagBurthenInfo
{
	DWORD							dwBufferSize;						//缓冲长度
	DWORD							dwDataSize;							//数据大小
	DWORD							dwPacketCount;						//数据包数
};

//////////////////////////////////////////////////////////////////////////

//缓冲结构
class CDataQueue
{
public:
	CDataQueue();
	~CDataQueue();

public:
	//初始大小
	void InitSize(DWORD dwInitBufferSize);
	//负荷信息
	void GetBurthenInfo(tagBurthenInfo & BurthenInfo);

public:
	//插入数据
	bool InsertData(WORD wIdentifier, VOID * pBuffer, WORD wDataSize);
	//提取数据
	bool DistillData(tagDataHead & DataHead, VOID * pBuffer, WORD wBufferSize);
	//删除数据
	void RemoveData(bool bFreeMemroy);

private:
	//调整大小
	bool Resize(DWORD dwNeedSize);

private:
	CRITICAL_SECTION				m_Mutex;							//临界对象
	BYTE *							m_pBuffer;							//缓冲指针
	DWORD							m_dwBufferSize;						//缓冲长度
	DWORD							m_dwDataSize;						//数据大小
	DWORD							m_dwPacketCount;					//数据包数

private:
	DWORD							m_dwInsertPos;						//插入位置
	DWORD							m_dwTerminalPos;					//结束位置
	DWORD							m_dwDataQueryPos;					//查询位置
};

//////////////////////////////////////////////////////////////////////////
