#pragma once

//////////////////////////////////////////////////////////////////////////

#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////
#pragma pack(1)

//数据类型
#define DK_MAPPED					0x01								//映射类型
#define DK_ENCRYPT					0x02								//加密类型
#define DK_COMPRESS					0x04								//压缩类型

//网络内核
struct NT_Info
{
	BYTE							cbDataKind;							//数据类型
	BYTE							cbCheckCode;						//效验字段
	WORD							wPacketSize;						//数据大小
};

//网络命令
struct NT_Command
{
	WORD							wMainCmdID;							//主命令码
	WORD							wSubCmdID;							//子命令码
};

//网络包头
struct NT_Head
{
	NT_Info							NTInfo;								//基础结构
	NT_Command						CommandInfo;						//命令信息
};

//内核命令
#define MDM_NT_COMMAND				0									//内核命令
#define SUB_NT_DETECT_SOCKET		1									//检测命令
#define SUB_NT_VALIDATE_SOCKET		2									//验证命令

//私钥信息
const TCHAR szValidateSecretKey[] = TEXT("B40BA201-BB30-4021-91E9-E62EC6606018");

//验证结构
struct NT_Validate
{
	TCHAR							szValidateKey[64];					//验证字符
};

//数据信息
struct tagEncryptData
{
	WORD							wDataSize;							//数据大小
	VOID *							pDataBuffer;						//数据指针
};

//数据信息
struct tagDecryptData
{
	//命令信息
	NT_Command						Command;							//网络命令
	WORD							wDataSize;							//数据大小
	VOID *							pDataBuffer;						//数据指针
};

#pragma pack()
//////////////////////////////////////////////////////////////////////////

//密码结构
class CCrypto
{
public:
	CCrypto();
	~CCrypto();

public:
	//加密数据
	bool Encrypt(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, tagEncryptData& EncryptData);
	//解密数据
	bool Decrypt(VOID * pData, WORD wDataSize, tagDecryptData& DecryptData);

private:
	//映射数据
	bool MappedBuffer();
	//映射数据
	bool UnMappedBuffer();

private:
	//加密数据
	bool EncryptBuffer();
	//解密数据
	bool DecryptBuffer();

private:
	//压缩数据
	bool CompressBuffer();
	//解压数据
	bool UnCompressBuffer();

private:
	BYTE							m_cbDataBuffer[SOCKET_BUFFER];		//数据缓冲
	WORD							m_wDataSize;						//数据长度
};

//////////////////////////////////////////////////////////////////////////

//管理结构
class CCryptoManager
{
public:
	CCryptoManager();
	~CCryptoManager();

public:
	//初始化管理对象
	bool Init();
	//释放管理对象
	bool Release();

public:
	//获取对象
	CCrypto * PopCrypto();
	//添加对象
	void PushCrypto(CCrypto * pCrypto);

private:
	CMutex							m_Mutex;							//同步对象
	std::vector<CCrypto *>			m_CryptoPtrActiveList;				//活动对象
	std::vector<CCrypto *>			m_CryptoPtrStorageList;				//存储对象
};

//////////////////////////////////////////////////////////////////////////

//辅助结构
class CCryptoHelper
{
public:
	CCryptoHelper(CCryptoManager& CryptoManager);
	~CCryptoHelper(void);

public:
	//加密数据
	bool Encrypt(WORD wMainCmdID, WORD wSubCmdID, VOID * pData, WORD wDataSize, tagEncryptData& EncryptData);
	//解密数据
	bool Decrypt(VOID * pData, WORD wDataSize, tagDecryptData& DecryptData);

private:
	CCryptoHelper(const CCryptoHelper&);

private:
	CCryptoManager &				m_CryptoManager;					//管理对象
	CCrypto *						m_pCrypto;							//加密对象
};

//////////////////////////////////////////////////////////////////////////
