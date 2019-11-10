#include "stdafx.h"
#include "DataQueue.h"
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////

//构造函数
CDataQueue::CDataQueue()
{
	//初始化临界区
	InitializeCriticalSection(&m_Mutex);

	//数据信息
	m_pBuffer = NULL;
	m_dwBufferSize = 0L;
	m_dwDataSize = 0L;
	m_dwPacketCount = 0L;

	//位置信息
	m_dwInsertPos = 0L;
	m_dwTerminalPos = 0L;
	m_dwDataQueryPos = 0L;
}

//析构函数
CDataQueue::~CDataQueue()
{
	//释放数据
	RemoveData(true);

	//释放临界区
	DeleteCriticalSection(&m_Mutex);
}

//初始大小
void CDataQueue::InitSize(DWORD dwInitBufferSize)
{
	//进入临界区
	EnterCriticalSection(&m_Mutex);

	//预分配内存(16M)
	Resize(16384 * 1024);

	//离开临界区
	LeaveCriticalSection(&m_Mutex);
}

//负荷信息
void CDataQueue::GetBurthenInfo(tagBurthenInfo & BurthenInfo)
{
	//进入临界区
	EnterCriticalSection(&m_Mutex);

	//设置变量
	BurthenInfo.dwBufferSize = m_dwBufferSize;
	BurthenInfo.dwDataSize = m_dwDataSize;
	BurthenInfo.dwPacketCount = m_dwPacketCount;

	//离开临界区
	LeaveCriticalSection(&m_Mutex);
}

//插入数据
bool CDataQueue::InsertData(WORD wIdentifier, VOID * pBuffer, WORD wDataSize)
{
	//变量定义
	tagDataHead DataHead;
	ZeroMemory(&DataHead, sizeof(DataHead));

	//设置变量
	DataHead.wDataSize = wDataSize;
	DataHead.wIdentifier = wIdentifier;

	//进入临界区
	EnterCriticalSection(&m_Mutex);

	//设置缓冲
	if (Resize(sizeof(DataHead) + DataHead.wDataSize) == false)
	{
		//离开临界区
		LeaveCriticalSection(&m_Mutex);
		return false;
	}

	//头部信息
	CopyMemory(m_pBuffer + m_dwInsertPos, &DataHead, sizeof(DataHead));
	//内容信息
	if (wDataSize > 0)
	{
		CopyMemory(m_pBuffer + m_dwInsertPos + sizeof(DataHead), pBuffer, wDataSize);
	}

	//调整数据
	m_dwPacketCount++;
	m_dwDataSize += sizeof(DataHead) + wDataSize;
	m_dwInsertPos += sizeof(DataHead) + wDataSize;
	m_dwTerminalPos = max(m_dwTerminalPos, m_dwInsertPos);

	//离开临界区
	LeaveCriticalSection(&m_Mutex);

	return true;
}

//提取数据
bool CDataQueue::DistillData(tagDataHead & DataHead, VOID * pBuffer, WORD wBufferSize)
{
	//进入临界区
	EnterCriticalSection(&m_Mutex);

	//效验变量
	if (m_dwDataSize == 0L || m_dwPacketCount == 0L)
	{
		//离开临界区
		LeaveCriticalSection(&m_Mutex);
		return false;
	}

	//调整参数
	if (m_dwDataQueryPos == m_dwTerminalPos)
	{
		m_dwDataQueryPos = 0L;
		m_dwTerminalPos = m_dwInsertPos;
	}

	//获取数据
	tagDataHead * pDataHead = (tagDataHead *)(m_pBuffer + m_dwDataQueryPos);
	WORD wPacketSize = sizeof(DataHead) + pDataHead->wDataSize;

	//拷贝数据
	DataHead = *pDataHead;
	if (DataHead.wDataSize > 0)
	{
		//判断缓冲
		if (wBufferSize >= pDataHead->wDataSize)
		{
			CopyMemory(pBuffer, pDataHead + 1, DataHead.wDataSize);
		}
		else
		{
			DataHead.wDataSize = 0;
		}
	}

	//设置变量
	m_dwPacketCount--;
	m_dwDataSize -= wPacketSize;
	m_dwDataQueryPos += wPacketSize;

	//离开临界区
	LeaveCriticalSection(&m_Mutex);

	return true;
}

//删除数据
void CDataQueue::RemoveData(bool bFreeMemroy)
{
	//进入临界区
	EnterCriticalSection(&m_Mutex);

	//释放内存
	if (bFreeMemroy)
	{
		if (m_pBuffer)
		{
			::free(m_pBuffer);
			m_pBuffer = NULL;
		}
		m_dwBufferSize = 0L;
	}

	//清空数据
	m_dwDataSize = 0L;
	m_dwPacketCount = 0L;

	//位置信息
	m_dwInsertPos = 0L;
	m_dwTerminalPos = 0L;
	m_dwDataQueryPos = 0L;

	//离开临界区
	LeaveCriticalSection(&m_Mutex);
}

//调整大小
bool CDataQueue::Resize(DWORD dwNeedSize)
{
	//如果缓冲足够
	if ((m_dwDataSize + dwNeedSize) <= m_dwBufferSize)
	{
		//如果是继续往后追加数据
		if (m_dwInsertPos == m_dwTerminalPos)
		{
			//如果剩余缓冲区域足够
			if ((m_dwInsertPos + dwNeedSize) <= m_dwBufferSize)
			{
				return true;
			}
			else
			{
				//如果前面空出来的部分足够,则调整为往前面插数据 
				if (m_dwDataQueryPos >= dwNeedSize)
				{
					m_dwInsertPos = 0L;
					return true;
				}
			}
		}
		else
		{
			//如果中间区域位置足够
			if ((m_dwInsertPos + dwNeedSize) <= m_dwDataQueryPos)
			{
				return true;
			}
		}
	}

	//计算大小
	DWORD dwNewSize = m_dwBufferSize + max(dwNeedSize, 16384);

	//申请内存
	BYTE* pNewBuffer = reinterpret_cast<BYTE*>(::malloc(dwNewSize));
	if (pNewBuffer)
	{
		//整理数据
		if (m_pBuffer)
		{
			//后面未处理的数据放在前面
			DWORD dwPartOneSize = m_dwTerminalPos - m_dwDataQueryPos;
			if (dwPartOneSize > 0L)
			{
				CopyMemory(pNewBuffer, m_pBuffer + m_dwDataQueryPos, dwPartOneSize);
			}

			//如果插入位置在前面,且有数据,则前面插入的数据接在后面(此时,理论上:(dwPartOneSize + m_dwInsertPos) == m_dwDataSize)
			if ((m_dwInsertPos != m_dwTerminalPos) && (m_dwInsertPos > 0L))
			{
				CopyMemory(pNewBuffer + dwPartOneSize, m_pBuffer, m_dwInsertPos);
			}

			//释放原内存
			::free(m_pBuffer);
			m_pBuffer = NULL;
		}

		//设置缓冲
		m_pBuffer = pNewBuffer;
		m_dwBufferSize = dwNewSize;

		//设置数据
		m_dwDataQueryPos = 0L;
		m_dwInsertPos = m_dwDataSize;
		m_dwTerminalPos = m_dwInsertPos;

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
