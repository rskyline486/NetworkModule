#include "stdafx.h"
#include "DataQueue.h"
#include <malloc.h>

//////////////////////////////////////////////////////////////////////////

//���캯��
CDataQueue::CDataQueue()
{
	//��ʼ���ٽ���
	InitializeCriticalSection(&m_Mutex);

	//������Ϣ
	m_pBuffer = NULL;
	m_dwBufferSize = 0L;
	m_dwDataSize = 0L;
	m_dwPacketCount = 0L;

	//λ����Ϣ
	m_dwInsertPos = 0L;
	m_dwTerminalPos = 0L;
	m_dwDataQueryPos = 0L;
}

//��������
CDataQueue::~CDataQueue()
{
	//�ͷ�����
	RemoveData(true);

	//�ͷ��ٽ���
	DeleteCriticalSection(&m_Mutex);
}

//��ʼ��С
void CDataQueue::InitSize(DWORD dwInitBufferSize)
{
	//�����ٽ���
	EnterCriticalSection(&m_Mutex);

	//Ԥ�����ڴ�(16M)
	Resize(16384 * 1024);

	//�뿪�ٽ���
	LeaveCriticalSection(&m_Mutex);
}

//������Ϣ
void CDataQueue::GetBurthenInfo(tagBurthenInfo & BurthenInfo)
{
	//�����ٽ���
	EnterCriticalSection(&m_Mutex);

	//���ñ���
	BurthenInfo.dwBufferSize = m_dwBufferSize;
	BurthenInfo.dwDataSize = m_dwDataSize;
	BurthenInfo.dwPacketCount = m_dwPacketCount;

	//�뿪�ٽ���
	LeaveCriticalSection(&m_Mutex);
}

//��������
bool CDataQueue::InsertData(WORD wIdentifier, VOID * pBuffer, WORD wDataSize)
{
	//��������
	tagDataHead DataHead;
	ZeroMemory(&DataHead, sizeof(DataHead));

	//���ñ���
	DataHead.wDataSize = wDataSize;
	DataHead.wIdentifier = wIdentifier;

	//�����ٽ���
	EnterCriticalSection(&m_Mutex);

	//���û���
	if (Resize(sizeof(DataHead) + DataHead.wDataSize) == false)
	{
		//�뿪�ٽ���
		LeaveCriticalSection(&m_Mutex);
		return false;
	}

	//ͷ����Ϣ
	CopyMemory(m_pBuffer + m_dwInsertPos, &DataHead, sizeof(DataHead));
	//������Ϣ
	if (wDataSize > 0)
	{
		CopyMemory(m_pBuffer + m_dwInsertPos + sizeof(DataHead), pBuffer, wDataSize);
	}

	//��������
	m_dwPacketCount++;
	m_dwDataSize += sizeof(DataHead) + wDataSize;
	m_dwInsertPos += sizeof(DataHead) + wDataSize;
	m_dwTerminalPos = max(m_dwTerminalPos, m_dwInsertPos);

	//�뿪�ٽ���
	LeaveCriticalSection(&m_Mutex);

	return true;
}

//��ȡ����
bool CDataQueue::DistillData(tagDataHead & DataHead, VOID * pBuffer, WORD wBufferSize)
{
	//�����ٽ���
	EnterCriticalSection(&m_Mutex);

	//Ч�����
	if (m_dwDataSize == 0L || m_dwPacketCount == 0L)
	{
		//�뿪�ٽ���
		LeaveCriticalSection(&m_Mutex);
		return false;
	}

	//��������
	if (m_dwDataQueryPos == m_dwTerminalPos)
	{
		m_dwDataQueryPos = 0L;
		m_dwTerminalPos = m_dwInsertPos;
	}

	//��ȡ����
	tagDataHead * pDataHead = (tagDataHead *)(m_pBuffer + m_dwDataQueryPos);
	WORD wPacketSize = sizeof(DataHead) + pDataHead->wDataSize;

	//��������
	DataHead = *pDataHead;
	if (DataHead.wDataSize > 0)
	{
		//�жϻ���
		if (wBufferSize >= pDataHead->wDataSize)
		{
			CopyMemory(pBuffer, pDataHead + 1, DataHead.wDataSize);
		}
		else
		{
			DataHead.wDataSize = 0;
		}
	}

	//���ñ���
	m_dwPacketCount--;
	m_dwDataSize -= wPacketSize;
	m_dwDataQueryPos += wPacketSize;

	//�뿪�ٽ���
	LeaveCriticalSection(&m_Mutex);

	return true;
}

//ɾ������
void CDataQueue::RemoveData(bool bFreeMemroy)
{
	//�����ٽ���
	EnterCriticalSection(&m_Mutex);

	//�ͷ��ڴ�
	if (bFreeMemroy)
	{
		if (m_pBuffer)
		{
			::free(m_pBuffer);
			m_pBuffer = NULL;
		}
		m_dwBufferSize = 0L;
	}

	//�������
	m_dwDataSize = 0L;
	m_dwPacketCount = 0L;

	//λ����Ϣ
	m_dwInsertPos = 0L;
	m_dwTerminalPos = 0L;
	m_dwDataQueryPos = 0L;

	//�뿪�ٽ���
	LeaveCriticalSection(&m_Mutex);
}

//������С
bool CDataQueue::Resize(DWORD dwNeedSize)
{
	//��������㹻
	if ((m_dwDataSize + dwNeedSize) <= m_dwBufferSize)
	{
		//����Ǽ�������׷������
		if (m_dwInsertPos == m_dwTerminalPos)
		{
			//���ʣ�໺�������㹻
			if ((m_dwInsertPos + dwNeedSize) <= m_dwBufferSize)
			{
				return true;
			}
			else
			{
				//���ǰ��ճ����Ĳ����㹻,�����Ϊ��ǰ������� 
				if (m_dwDataQueryPos >= dwNeedSize)
				{
					m_dwInsertPos = 0L;
					return true;
				}
			}
		}
		else
		{
			//����м�����λ���㹻
			if ((m_dwInsertPos + dwNeedSize) <= m_dwDataQueryPos)
			{
				return true;
			}
		}
	}

	//�����С
	DWORD dwNewSize = m_dwBufferSize + max(dwNeedSize, 16384);

	//�����ڴ�
	BYTE* pNewBuffer = reinterpret_cast<BYTE*>(::malloc(dwNewSize));
	if (pNewBuffer)
	{
		//��������
		if (m_pBuffer)
		{
			//����δ��������ݷ���ǰ��
			DWORD dwPartOneSize = m_dwTerminalPos - m_dwDataQueryPos;
			if (dwPartOneSize > 0L)
			{
				CopyMemory(pNewBuffer, m_pBuffer + m_dwDataQueryPos, dwPartOneSize);
			}

			//�������λ����ǰ��,��������,��ǰ���������ݽ��ں���(��ʱ,������:(dwPartOneSize + m_dwInsertPos) == m_dwDataSize)
			if ((m_dwInsertPos != m_dwTerminalPos) && (m_dwInsertPos > 0L))
			{
				CopyMemory(pNewBuffer + dwPartOneSize, m_pBuffer, m_dwInsertPos);
			}

			//�ͷ�ԭ�ڴ�
			::free(m_pBuffer);
			m_pBuffer = NULL;
		}

		//���û���
		m_pBuffer = pNewBuffer;
		m_dwBufferSize = dwNewSize;

		//��������
		m_dwDataQueryPos = 0L;
		m_dwInsertPos = m_dwDataSize;
		m_dwTerminalPos = m_dwInsertPos;

		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
