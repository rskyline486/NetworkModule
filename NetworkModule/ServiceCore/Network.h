#pragma once

//////////////////////////////////////////////////////////////////////////

#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <vector>
#include <time.h>

//////////////////////////////////////////////////////////////////////////

//协议定义
enum ProtocolSupport
{
	EnableIPv4,
	EnableIPv6,
	EnableBoth
};

//地址定义
union Address
{
	Address()
	{
		memset(&saStorage, 0, sizeof(sockaddr_storage));
		saStorage.ss_family = AF_UNSPEC;
	}

	sockaddr sa;
	sockaddr_in saIn;
	sockaddr_in6 saIn6;
	sockaddr_storage saStorage;
};

//////////////////////////////////////////////////////////////////////////

//网络对象
class CNetwork
{
public:
	static Address GetAddress(LPCTSTR pszHost, WORD wPort, ProtocolSupport Protocol, bool bBlock = true);
	static Address GetAddress(const Address& Addr, WORD wPort);
	static int GetAddressSize(const Address& Addr);
	static Address GetLocalAddress(SOCKET hSocket);
	static Address GetRemoteAddress(SOCKET hSocket);
	static bool IsValidAddress(const Address& Addr);

public:
	static SOCKET CreateSocket(const Address& Addr, ProtocolSupport Protocol, bool bUDP = false);
	static SOCKET CreateSocket(const Address& Addr, bool bUDP = false);
	static bool CloseSocket(SOCKET hSocket);
	static Address Bind(SOCKET hSocket, const Address& Addr);
	static bool Listen(SOCKET hSocket, int nBacklog);
	static SOCKET Accept(SOCKET hSocket);
	static bool Connect(SOCKET hSocket, const Address& Addr, const Address& SourceAddr);

public:
	static bool GetIP(const Address& Addr, TCHAR * pszBuffer, DWORD dwBufferLength);
	static WORD GetPort(const Address& Addr);
	static bool SetPort(Address& Addr, WORD wPort);
	static ProtocolSupport GetProtocolSupport(const Address& Addr);

public:
	static bool WouldBlock();
};

//////////////////////////////////////////////////////////////////////////
