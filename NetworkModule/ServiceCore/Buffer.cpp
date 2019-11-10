#include "stdafx.h"
#include "Buffer.h"
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////

//构造函数
CBuffer::CBuffer()
{
	m_pBuffer = NULL;
	m_dwBufferSize = 0;
	m_dwDataSize = 0;
}

//析构函数
CBuffer::~CBuffer()
{
	Release();
}

//初始化
bool CBuffer::Init(DWORD dwBufferSize)
{
	//数据校验
	if (dwBufferSize == 0) return false;

	//判断存在
	if (m_pBuffer)
	{
		//清空数据
		m_dwDataSize = 0;
		//设置缓冲
		if (Resize(dwBufferSize))
		{
			return true;
		}

		//清理数据
		Release();
	}

	//分配内存
	BYTE* pNewBuffer = reinterpret_cast<BYTE*>(::malloc(dwBufferSize));
	if (pNewBuffer)
	{
		m_pBuffer = pNewBuffer;
		m_dwBufferSize = dwBufferSize;
		return true;
	}

	return false;
}

//释放
void CBuffer::Release()
{
	if (m_pBuffer)
	{
		::free(m_pBuffer);
		m_pBuffer = NULL;
	}

	m_dwBufferSize = 0;
	m_dwDataSize = 0;
}

//接收大小
bool CBuffer::RecvSize(DWORD dwRecvSize)
{
	DWORD dwTotalSize = m_dwDataSize + dwRecvSize;
	if (dwTotalSize > m_dwBufferSize)
	{
		return false;
	}

	m_dwDataSize = dwTotalSize;
	return true;
}

//处理大小
bool CBuffer::DealSize(DWORD dwDealSize)
{
	DWORD dwLeftSize = m_dwDataSize - dwDealSize;
	if (dwLeftSize > m_dwDataSize)
	{
		return false;
	}

	//移动数据
	if (dwLeftSize < m_dwDataSize)
	{
		if (dwLeftSize > 0)
		{
			memmove(m_pBuffer, &m_pBuffer[dwDealSize], dwLeftSize);
		}
		m_dwDataSize = dwLeftSize;
	}

	return true;
}

//数据指针
BYTE* CBuffer::GetData()
{
	return m_pBuffer;
}

//数据大小
DWORD CBuffer::GetDataSize()
{
	return m_dwDataSize;
}

//投递指针
BYTE* CBuffer::GetDeliverData(DWORD dwDeliverSize)
{
	if (Resize(dwDeliverSize))
	{
		return &m_pBuffer[m_dwDataSize];
	}

	return NULL;
}

//调整大小
bool CBuffer::Resize(DWORD dwNeedSize)
{
	DWORD dwNewSize = m_dwDataSize + dwNeedSize;
	if (dwNewSize <= m_dwBufferSize)
	{
		return true;
	}

	//如果缓冲存在
	if (m_pBuffer)
	{
		BYTE* pResizeBuffer = reinterpret_cast<BYTE*>(::realloc(m_pBuffer, dwNewSize));
		if (pResizeBuffer)
		{
			m_pBuffer = pResizeBuffer;
			m_dwBufferSize = dwNewSize;
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
