#include "stdafx.h"
#include "AsynchronismWrapper.h"

//////////////////////////////////////////////////////////////////////////

//���캯��
CAsynchronismKernel::CAsynchronismKernel()
{
}

//��������
CAsynchronismKernel::~CAsynchronismKernel()
{
}

//�����ں�
bool CAsynchronismKernel::StartKernel()
{
	//��ʼ���첽����
	if (m_AsynchronismEngine.Init(this, 1) == false) return false;

	return true;
}

//ֹͣ�ں�
bool CAsynchronismKernel::StopKernel()
{
	//ֹͣ�첽����
	m_AsynchronismEngine.Release();

	return true;
}

//�첽��ʼ
bool CAsynchronismKernel::OnEventAsynchronismStrat()
{
	return true;
}

//�첽����
bool CAsynchronismKernel::OnEventAsynchronismStop()
{
	return true;
}

//�첽�¼�
bool CAsynchronismKernel::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	Logger_Info(TEXT("�첽�¼�=>��ʶ��Ϣ:%u, ���ݴ�С:%u"), wIdentifier, wDataSize);

	return true;
}

//////////////////////////////////////////////////////////////////////////

//���캯��
CAsynchronismInstance::CAsynchronismInstance()
{
	m_pAsynchronismEvent = NULL;
}

//��������
CAsynchronismInstance::~CAsynchronismInstance()
{
}

//��������
bool CAsynchronismInstance::StartServer(WORD wThreadCount, IAsynchronismEvent * pAsynchronismEvent)
{
	//���ýӿ�
	m_pAsynchronismEvent = pAsynchronismEvent;

	//��ʼ���첽����
	if (m_AsynchronismEngine.Init(this, wThreadCount) == false)
	{
		m_pAsynchronismEvent = NULL;
		return false;
	}

	return true;
}

//ֹͣ����
bool CAsynchronismInstance::StopServer()
{
	//ֹͣ����
	__super::StopKernel();

	//���ýӿ�
	m_pAsynchronismEvent = NULL;

	return true;
}

//��������
bool CAsynchronismInstance::PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	return m_AsynchronismEngine.PostAsynchronismData(wIdentifier, pData, wDataSize);
}

//�첽��ʼ
bool CAsynchronismInstance::OnEventAsynchronismStrat()
{
	//�ص�����
	if (m_pAsynchronismEvent)
	{
		return m_pAsynchronismEvent->OnAsynchronismStart();
	}

	return __super::OnEventAsynchronismStrat();
}

//�첽����
bool CAsynchronismInstance::OnEventAsynchronismStop()
{
	//�ص�����
	if (m_pAsynchronismEvent)
	{
		return m_pAsynchronismEvent->OnAsynchronismStop();
	}

	return __super::OnEventAsynchronismStop();
}

//�첽�¼�
bool CAsynchronismInstance::OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize)
{
	//�ص�����
	if (m_pAsynchronismEvent)
	{
		return m_pAsynchronismEvent->OnAsynchronismData(wIdentifier, pData, wDataSize);
	}

	return __super::OnEventAsynchronismData(wIdentifier, pData, wDataSize);
}

//////////////////////////////////////////////////////////////////////////
