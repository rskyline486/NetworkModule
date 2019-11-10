#include "stdafx.h"
#include "Buffer.h"
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////

//���캯��
CBuffer::CBuffer()
{
	m_pBuffer = NULL;
	m_dwBufferSize = 0;
	m_dwDataSize = 0;
}

//��������
CBuffer::~CBuffer()
{
	Release();
}

//��ʼ��
bool CBuffer::Init(DWORD dwBufferSize)
{
	//����У��
	if (dwBufferSize == 0) return false;

	//�жϴ���
	if (m_pBuffer)
	{
		//�������
		m_dwDataSize = 0;
		//���û���
		if (Resize(dwBufferSize))
		{
			return true;
		}

		//��������
		Release();
	}

	//�����ڴ�
	BYTE* pNewBuffer = reinterpret_cast<BYTE*>(::malloc(dwBufferSize));
	if (pNewBuffer)
	{
		m_pBuffer = pNewBuffer;
		m_dwBufferSize = dwBufferSize;
		return true;
	}

	return false;
}

//�ͷ�
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

//���մ�С
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

//�����С
bool CBuffer::DealSize(DWORD dwDealSize)
{
	DWORD dwLeftSize = m_dwDataSize - dwDealSize;
	if (dwLeftSize > m_dwDataSize)
	{
		return false;
	}

	//�ƶ�����
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

//����ָ��
BYTE* CBuffer::GetData()
{
	return m_pBuffer;
}

//���ݴ�С
DWORD CBuffer::GetDataSize()
{
	return m_dwDataSize;
}

//Ͷ��ָ��
BYTE* CBuffer::GetDeliverData(DWORD dwDeliverSize)
{
	if (Resize(dwDeliverSize))
	{
		return &m_pBuffer[m_dwDataSize];
	}

	return NULL;
}

//������С
bool CBuffer::Resize(DWORD dwNeedSize)
{
	DWORD dwNewSize = m_dwDataSize + dwNeedSize;
	if (dwNewSize <= m_dwBufferSize)
	{
		return true;
	}

	//����������
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
