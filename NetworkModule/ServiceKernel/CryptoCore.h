#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////
#pragma pack(1)

//��������
#define DK_MAPPED					0x01								//ӳ������
#define DK_ENCRYPT					0x02								//��������
#define DK_COMPRESS					0x04								//ѹ������

//�����ں�
struct NT_Info
{
	BYTE							cbDataKind;							//��������
	BYTE							cbCheckCode;						//Ч���ֶ�
	WORD							wPacketSize;						//���ݴ�С
};

//��������
struct NT_Command
{
	WORD							wMainCmdID;							//��������
	WORD							wSubCmdID;							//��������
};

//�����ͷ
struct NT_Head
{
	NT_Info							NTInfo;								//�����ṹ
	NT_Command						CommandInfo;						//������Ϣ
};

//�ں�����
#define MDM_NT_COMMAND				0									//�ں�����
#define SUB_NT_DETECT_SOCKET		1									//�������
#define SUB_NT_VALIDATE_SOCKET		2									//��֤����

//˽Կ��Ϣ
const TCHAR szValidateSecretKey[] = TEXT("B40BA201-BB30-4021-91E9-E62EC6606018");

//��֤�ṹ
struct NT_Validate
{
	TCHAR							szValidateKey[64];					//��֤�ַ�
};

//������Ϣ
struct tagEncryptData
{
	WORD							wDataSize;							//���ݴ�С
	VOID *							pDataBuffer;						//����ָ��
};

//������Ϣ
struct tagDecryptData
{
	//������Ϣ
	NT_Command						Command;							//��������
	WORD							wDataSize;							//���ݴ�С
	VOID *							pDataBuffer;						//����ָ��
};

#pragma pack()
//////////////////////////////////////////////////////////////////////////

//����ṹ
class CCrypto
{
public:
	CCrypto();
	~CCrypto();

public:
	//��������
	bool Encrypt(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, tagEncryptData& EncryptData);
	//��������
	bool Decrypt(VOID * pData, WORD wDataSize, tagDecryptData& DecryptData);

private:
	//ӳ������
	bool MappedBuffer();
	//ӳ������
	bool UnMappedBuffer();

private:
	//��������
	bool EncryptBuffer();
	//��������
	bool DecryptBuffer();

private:
	//ѹ������
	bool CompressBuffer();
	//��ѹ����
	bool UnCompressBuffer();

private:
	BYTE							m_cbDataBuffer[SOCKET_BUFFER];		//���ݻ���
	WORD							m_wDataSize;						//���ݳ���
};

//////////////////////////////////////////////////////////////////////////

//����ṹ
class CCryptoManager
{
public:
	CCryptoManager();
	~CCryptoManager();

public:
	//��ʼ���������
	bool Init();
	//�ͷŹ������
	bool Release();

public:
	//��ȡ����
	CCrypto * PopCrypto();
	//��Ӷ���
	void PushCrypto(CCrypto * pCrypto);

private:
	CMutex							m_Mutex;							//ͬ������
	std::vector<CCrypto *>			m_CryptoPtrActiveList;				//�����
	std::vector<CCrypto *>			m_CryptoPtrStorageList;				//�洢����
};

//////////////////////////////////////////////////////////////////////////

//�����ṹ
class CCryptoHelper
{
public:
	CCryptoHelper(CCryptoManager& CryptoManager);
	~CCryptoHelper(void);

public:
	//��������
	bool Encrypt(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, tagEncryptData& EncryptData);
	//��������
	bool Decrypt(VOID * pData, WORD wDataSize, tagDecryptData& DecryptData);

private:
	CCryptoHelper(const CCryptoHelper&);

private:
	CCryptoManager &				m_CryptoManager;					//�������
	CCrypto *						m_pCrypto;							//���ܶ���
};

//////////////////////////////////////////////////////////////////////////
