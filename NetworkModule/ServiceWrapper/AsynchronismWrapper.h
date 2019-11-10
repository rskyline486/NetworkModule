#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceKernel.h"
#include "NetworkModule.h"

//////////////////////////////////////////////////////////////////////////

//�������
class CAsynchronismKernel : protected IAsynchronismEventSink
{
public:
	CAsynchronismKernel();
	virtual ~CAsynchronismKernel();

public:
	//�����ں�
	bool StartKernel();
	//ֹͣ�ں�
	bool StopKernel();

protected:
	//�첽��ʼ
	virtual bool OnEventAsynchronismStrat() = 0;
	//�첽����
	virtual bool OnEventAsynchronismStop() = 0;
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	CAsynchronismEngine				m_AsynchronismEngine;				//�첽����
};

//////////////////////////////////////////////////////////////////////////

//����ʵ��
class CAsynchronismInstance : protected CAsynchronismKernel, public IAsynchronismService
{
public:
	CAsynchronismInstance();
	virtual ~CAsynchronismInstance();

public:
	//��������
	virtual bool StartServer(WORD wThreadCount, IAsynchronismEvent * pAsynchronismEvent);
	//ֹͣ����
	virtual bool StopServer();

public:
	//��������
	virtual bool PostAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

protected:
	//�첽��ʼ
	virtual bool OnEventAsynchronismStrat();
	//�첽����
	virtual bool OnEventAsynchronismStop();
	//�첽�¼�
	virtual bool OnEventAsynchronismData(WORD wIdentifier, VOID * pData, WORD wDataSize);

private:
	IAsynchronismEvent *			m_pAsynchronismEvent;				//�¼��ӿ�
};

//////////////////////////////////////////////////////////////////////////
