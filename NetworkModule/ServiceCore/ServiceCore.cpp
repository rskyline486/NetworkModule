#include "stdafx.h"
#include "ServiceCore.h"

//////////////////////////////////////////////////////////////////////////

//构造函数
COverLapped::COverLapped(enOperationType OperationType, DWORD dwBufferSize)
{
	//设置变量
	ZeroMemory(&m_WSABuffer, sizeof(m_WSABuffer));
	ZeroMemory(&m_OverLapped, sizeof(m_OverLapped));
	m_OperationType = OperationType;
	m_bHandleIng = false;
	if (!m_Buffer.Init(dwBufferSize))
	{
		Logger_Error(TEXT("初始化缓冲对象失败,dwBufferSize:%u"), dwBufferSize);
	}
}

//析构函数
COverLapped::~COverLapped()
{
	m_Buffer.Release();
}

//数据指针
COverLapped::operator LPWSABUF()
{
	return &m_WSABuffer;
}

//重叠结构
COverLapped::operator LPWSAOVERLAPPED()
{
	return &m_OverLapped;
}

//缓冲地址
LPVOID COverLapped::GetBufferAddr()
{
	return m_Buffer.GetData();
}

//缓冲大小
DWORD COverLapped::GetBufferSize()
{
	return m_Buffer.GetDataSize();
}

//操作类型
enOperationType COverLapped::GetOperationType()
{
	return m_OperationType;
}

//设置类型
void COverLapped::SetOperationType(enOperationType OperationType)
{
	m_OperationType = OperationType;
}

//设置标识
void COverLapped::SetHandleIng(bool bHandleIng)
{
	m_bHandleIng = bHandleIng;
}

//获取标识
bool COverLapped::GetHandleIng()
{
	return m_bHandleIng;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
COverLappedSend::COverLappedSend() : COverLapped(enOperationType_Send, SOCKET_BUFFER)
{
}

//析构函数
COverLappedSend::~COverLappedSend()
{
}

//恢复缓冲
bool COverLappedSend::ResumeBuffer()
{
	DWORD dwSendSize = m_Buffer.GetDataSize();
	if (dwSendSize == 0)
	{
		return false;
	}

	if (dwSendSize > SOCKET_BUFFER)
	{
		dwSendSize = SOCKET_BUFFER;
	}

	m_WSABuffer.len = dwSendSize;
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetData();

	return true;
}

//重置数据
void COverLappedSend::ResetData()
{
	m_Buffer.Init(SOCKET_BUFFER);
}

//发送数据
bool COverLappedSend::SendData(LPVOID lpData, DWORD dwDataSize)
{
	//数据校验
	if (lpData == NULL || dwDataSize == 0)
	{
		return false;
	}

	//获取缓冲数据区尾地址
	LPVOID lpBuffer = m_Buffer.GetDeliverData(dwDataSize);
	if (lpBuffer == NULL)
	{
		return false;
	}

	//拷贝数据
	memcpy(lpBuffer, lpData, dwDataSize);

	//更新长度
	return m_Buffer.RecvSize(dwDataSize);
}

//发送完毕
bool COverLappedSend::SendCompleted(DWORD dwSendSize)
{
	return m_Buffer.DealSize(dwSendSize);
}

//////////////////////////////////////////////////////////////////////////

//构造函数
COverLappedRecv::COverLappedRecv() : COverLapped(enOperationType_Recv, SOCKET_BUFFER)
{
}

//析构函数
COverLappedRecv::~COverLappedRecv()
{
}

//恢复缓冲
bool COverLappedRecv::ResumeBuffer()
{
	m_WSABuffer.len = SOCKET_BUFFER;
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetDeliverData(m_WSABuffer.len);

	return (m_WSABuffer.buf != NULL);
}

//重置数据
void COverLappedRecv::ResetData()
{
	m_Buffer.Init(SOCKET_BUFFER);
}

//接收完毕
bool COverLappedRecv::RecvCompleted(DWORD dwRecvSize)
{
	return m_Buffer.RecvSize(dwRecvSize);
}

//处理完毕
bool COverLappedRecv::DealCompleted(DWORD dwDealSize)
{
	return m_Buffer.DealSize(dwDealSize);
}

//////////////////////////////////////////////////////////////////////////

//构造函数
COverLappedAccept::COverLappedAccept() : COverLapped(enOperationType_Accept, (sizeof(sockaddr_storage) + 16) * 2)
{
	//重置数据
	m_Buffer.Init((sizeof(sockaddr_storage) + 16) * 2);
	//设置数据
	m_Buffer.RecvSize((sizeof(sockaddr_storage) + 16) * 2);
	//设置缓冲
	m_WSABuffer.len = m_Buffer.GetDataSize();
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetData();

	//重置句柄
	m_hSocket = INVALID_SOCKET;
}

//析构函数
COverLappedAccept::~COverLappedAccept()
{
}

//恢复缓冲
bool COverLappedAccept::ResumeBuffer()
{
	return true;
}

//重置数据
void COverLappedAccept::ResetData()
{
	//重置数据
	m_Buffer.Init((sizeof(sockaddr_storage) + 16) * 2);
	//设置数据
	m_Buffer.RecvSize((sizeof(sockaddr_storage) + 16) * 2);
	//设置缓冲
	m_WSABuffer.len = m_Buffer.GetDataSize();
	m_WSABuffer.buf = (CHAR*)m_Buffer.GetData();
}

//存储对象
bool COverLappedAccept::StoreSocket(SOCKET hSocket)
{
	if (m_hSocket != INVALID_SOCKET)
	{
		return false;
	}

	m_hSocket = hSocket;
	return true;
}

//取出对象
SOCKET COverLappedAccept::GetOutSocket()
{
	SOCKET hSocket = m_hSocket;
	m_hSocket = INVALID_SOCKET;
	return hSocket;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CNativeInfo::CNativeInfo(WORD wIndex)
{
	m_dwActiveTime = 0L;
	m_dwRealIPV4 = 0L;

	m_dwUserData = 0L;
	m_hSocket = INVALID_SOCKET;
	m_wIndex = wIndex;
	m_wRountID = 1;
	m_bUsed = false;
	m_bDetect = false;
	m_bShutDown = false;
}

//析构函数
CNativeInfo::~CNativeInfo(void)
{
}

//设置监测
bool CNativeInfo::SetDetect(WORD wRountID, bool bDetect)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//状态判断
	if (m_wRountID != wRountID) return false;

	if (bDetect)
	{
		//设置监测
		SetDetect();
	}
	else
	{
		//取消监测
		CancelDetect();
	}

	return true;
}

//设置数据
bool CNativeInfo::SetUserData(WORD wRountID, DWORD dwUserData)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//状态判断
	if (m_wRountID != wRountID) return false;

	m_dwUserData = dwUserData;

	return true;
}

//设置关闭
bool CNativeInfo::ShutDownSocket(WORD wRountID)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//状态判断
	if (m_wRountID != wRountID) return false;

	//设置标识
	m_bShutDown = true;

	return true;
}

//获取数据
DWORD CNativeInfo::GetUserData()
{
	//同步控制
	//此处无须加锁,程序确保了此处的调用操作为回调线程内部
	//回调线程内部已经加锁处理了,此处再次锁定,已无必要
	//严格上来说,应该加锁(同线程内多次加锁,只是增加引用计数,对应释放即可)
	//CLocker Locker(m_Mutex);

	return m_dwUserData;
}

//设置监测
void CNativeInfo::SetDetect()
{
	m_bDetect = true;
}

//取消监测
void CNativeInfo::CancelDetect()
{
	m_bDetect = false;
}

//获取监测
bool CNativeInfo::GetDetect()
{
	return m_bDetect;
}

//判断使用
bool CNativeInfo::IsUsed()
{
	//bool变量为原子变量,不存在中间值
	//只要确保赋值的时机,即可不必加锁

	return m_bUsed;
}

//获取索引
WORD CNativeInfo::GetIndex()
{
	return m_wIndex;
}

//获取计数
WORD CNativeInfo::GetRountID()
{
	return m_wRountID;
}

//获取标识
DWORD CNativeInfo::GetSocketID()
{
	return MAKELONG(m_wIndex, m_wRountID);
}

//激活时间
DWORD CNativeInfo::GetActiveTime()
{
	return m_dwActiveTime;
}

//获取地址
bool CNativeInfo::GetLocalAddress(TCHAR * pszBuffer, DWORD dwBufferLength, LPWORD pwPort)
{
	*pwPort = CNetwork::GetPort(m_LocalAddress);
	return CNetwork::GetIP(m_LocalAddress, pszBuffer, dwBufferLength);
}

//获取地址
bool CNativeInfo::GetRemoteAddress(TCHAR * pszBuffer, DWORD dwBufferLength, LPWORD pwPort)
{
	*pwPort = CNetwork::GetPort(m_RemoteAddress);
	return CNetwork::GetIP(m_RemoteAddress, pszBuffer, dwBufferLength);
}

//获取IPV4
DWORD CNativeInfo::GetClientIPV4()
{
	if (m_dwRealIPV4 > 0L) return m_dwRealIPV4;
	return m_RemoteAddress.saIn.sin_addr.S_un.S_addr;
}

//设置数据
void CNativeInfo::InitData()
{
	//设置使用
	m_bUsed = true;
	//重置标识
	m_bShutDown = false;
	//记录时间(从1970年1月1日0时0分0秒到现在的秒数)(若干年后数据有可能超出范围)
	m_dwActiveTime = (DWORD)time(NULL);
	//获取地址
	m_LocalAddress = CNetwork::GetLocalAddress(m_hSocket);
	m_RemoteAddress = CNetwork::GetRemoteAddress(m_hSocket);
	m_dwRealIPV4 = 0L;
}

//发送判断
bool CNativeInfo::SendVerdict(WORD wRountID)
{
	//状态判断
	if ((m_wRountID != wRountID) || (m_bShutDown == true)) return false;
	if (m_hSocket == INVALID_SOCKET) return false;

	return true;
}

//重置数据
void CNativeInfo::ResetData()
{
	m_dwActiveTime = 0L;
	//重置地址
	memset(&m_LocalAddress.saStorage, 0, sizeof(sockaddr_storage));
	memset(&m_RemoteAddress.saStorage, 0, sizeof(sockaddr_storage));
	m_dwRealIPV4 = 0L;
	//重置用户数据
	if (m_bDetect == false) m_dwUserData = 0L;
	//重置句柄
	m_hSocket = INVALID_SOCKET;
	//恢复计数(这种写法有bug,当循环回来之后,还是会变为0)
	//m_wRountID = max(1, m_wRountID + 1);
	m_wRountID = m_wRountID + 1;
	if (m_wRountID == 0) m_wRountID = 1;
	//重置标识
	m_bShutDown = false;
	//设置空闲
	m_bUsed = false;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CNativeInfoManager::CNativeInfoManager()
{
	m_hCompletionPort = NULL;
	m_wStartIndex = 0;
	m_wMaxNativeItemCount = MAX_SOCKET;
	ZeroMemory(&m_HeartbeatPacket, sizeof(m_HeartbeatPacket));
}

//析构函数
CNativeInfoManager::~CNativeInfoManager()
{
}

//获取大小
WORD CNativeInfoManager::GetTotalNativeCount()
{
	//同步控制
	CLocker Locker(m_Mutex);
	return (WORD)m_NativeInfoPtrList.size();
}

//获取大小
WORD CNativeInfoManager::GetActiveNativeCount()
{
	//活动数量
	WORD wActiveCount = 0;

	//遍历对象
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		if (m_NativeInfoPtrList[i]->IsUsed() == true)
		{
			wActiveCount++;
		}
	}

	return wActiveCount;
}

//设置心跳数据
bool CNativeInfoManager::SetHeartbeatData(VOID * pData, WORD wDataSize)
{
	//同步控制
	CLocker Locker(m_Mutex);

	//如果已经在运行,则不能修改数据
	if (m_NativeInfoPtrList.size() > 0)
	{
		return false;
	}

	//数据校验
	if (pData == NULL || wDataSize == 0)
	{
		ZeroMemory(&m_HeartbeatPacket, sizeof(m_HeartbeatPacket));
		m_HeartbeatPacket.wDataSize = 0;
		return true;
	}

	//大小校验
	if (wDataSize > sizeof(m_HeartbeatPacket.cbSendBuffer))
	{
		return false;
	}

	//设置心跳
	CopyMemory(m_HeartbeatPacket.cbSendBuffer, pData, wDataSize);
	m_HeartbeatPacket.wDataSize = wDataSize;

	return true;
}

//发送数据
bool CNativeInfoManager::SendData(DWORD dwSocketID, VOID * pData, WORD wDataSize)
{
	//获取索引
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//范围校验
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->SendData(pData, wDataSize, wRountID);
	}

	return false;
}

//群发数据
bool CNativeInfoManager::SendDataBatch(VOID * pData, WORD wDataSize)
{
	//遍历对象
	WORD wNativeInfoCount = GetTotalNativeCount();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		if (m_NativeInfoPtrList[i]->IsUsed() == false) continue;
		m_NativeInfoPtrList[i]->SendData(pData, wDataSize, m_NativeInfoPtrList[i]->GetRountID());
	}

	return true;
}

//设置监测
bool CNativeInfoManager::SetDetect(DWORD dwSocketID, bool bDetect)
{
	//获取索引
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//范围校验
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->SetDetect(wRountID, bDetect);
	}

	return false;
}

//设置数据
bool CNativeInfoManager::SetUserData(DWORD dwSocketID, DWORD dwUserData)
{
	//获取索引
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//范围校验
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->SetUserData(wRountID, dwUserData);
	}

	return false;
}

//设置关闭
bool CNativeInfoManager::ShutDownSocket(DWORD dwSocketID)
{
	//获取索引
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//范围校验
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->ShutDownSocket(wRountID);
	}

	return false;
}

//获取地址
DWORD CNativeInfoManager::GetClientIP(DWORD dwSocketID)
{
	//获取索引
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//范围校验
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->GetClientIPV4();
	}

	return 0L;
}

//关闭连接
bool CNativeInfoManager::CloseSocket(DWORD dwSocketID)
{
	//获取索引
	WORD wIndex = GetIndex(dwSocketID);
	WORD wRountID = GetRountID(dwSocketID);

	//范围校验
	if (wIndex < GetTotalNativeCount())
	{
		CNativeInfo * pNativeInfo = m_NativeInfoPtrList[wIndex];
		return pNativeInfo->CloseSocket(wRountID);
	}

	return false;
}

//获取空闲
CNativeInfo * CNativeInfoManager::GetFreeNativeInfo()
{
	//遍历对象
	WORD wNativeInfoCount = (WORD)m_NativeInfoPtrList.size();
	for (WORD i = 0; i < wNativeInfoCount; i++)
	{
		//如果对象空闲,且不需要被监测
		if (m_NativeInfoPtrList[i]->IsUsed() == false && m_NativeInfoPtrList[i]->GetDetect() == false)
		{
			return m_NativeInfoPtrList[i];
		}
	}

	return NULL;
}

//获取索引
WORD CNativeInfoManager::GetIndex(DWORD dwSocketID)
{
	return LOWORD(dwSocketID) - m_wStartIndex;
}

//获取计数
WORD CNativeInfoManager::GetRountID(DWORD dwSocketID)
{
	return HIWORD(dwSocketID);
}

//设置参数
bool CNativeInfoManager::SetParameter(HANDLE hCompletionPort, WORD wStartIndex, WORD wMaxNativeItemCount)
{
	m_hCompletionPort = hCompletionPort;
	m_wStartIndex = wStartIndex;
	m_wMaxNativeItemCount = wMaxNativeItemCount;
	
	//心跳数据
	ZeroMemory(&m_HeartbeatPacket, sizeof(m_HeartbeatPacket));
	m_HeartbeatPacket.wDataSize = 0;

	return true;
}

//释放连接
bool CNativeInfoManager::ReleaseNativeInfo(bool bFreeMemroy)
{
	if (bFreeMemroy == true)
	{
		//释放连接
		WORD wNativeInfoCount = (WORD)m_NativeInfoPtrList.size();
		for (WORD i = 0; i < wNativeInfoCount; i++)
		{
			m_NativeInfoPtrList[i]->CloseSocket(m_NativeInfoPtrList[i]->GetRountID());
			delete m_NativeInfoPtrList[i];
			m_NativeInfoPtrList[i] = NULL;
		}
		m_NativeInfoPtrList.clear();
	}
	else
	{
		//关闭连接
		WORD wNativeInfoCount = (WORD)m_NativeInfoPtrList.size();
		for (WORD i = 0; i < wNativeInfoCount; i++)
		{
			m_NativeInfoPtrList[i]->CloseSocket(m_NativeInfoPtrList[i]->GetRountID());
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

//构造函数
CServiceCore::CServiceCore()
{
}

//析构函数
CServiceCore::~CServiceCore()
{
}

bool CServiceCore::W2A(CHAR * pszBuffer, DWORD dwBufferLength, LPCWSTR pszSource)
{
	_snprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, "%ws", pszSource);
	return true;
}

bool CServiceCore::A2W(WCHAR * pszBuffer, DWORD dwBufferLength, LPCSTR pszSource)
{
	_snwprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, L"%hs", pszSource);
	return true;
}

bool CServiceCore::T2A(CHAR * pszBuffer, DWORD dwBufferLength, LPCTSTR pszSource)
{
#ifdef _UNICODE
	_snprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, "%ws", pszSource);
#else
	_snprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, "%s", pszSource);
#endif // _UNICODE
	return true;
}

bool CServiceCore::A2T(TCHAR * pszBuffer, DWORD dwBufferLength, LPCSTR pszSource)
{
	_sntprintf_s(pszBuffer, dwBufferLength, dwBufferLength - 1, TEXT("%hs"), pszSource);
	return true;
}

//////////////////////////////////////////////////////////////////////////
