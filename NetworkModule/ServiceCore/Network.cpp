#include "stdafx.h"
#include "Network.h"
#include <sstream>

//////////////////////////////////////////////////////////////////////////

Address CNetwork::GetAddress(LPCTSTR pszHost, WORD wPort, ProtocolSupport Protocol, bool bBlock /* = true */)
{
	Address Addr;
	memset(&Addr.saStorage, 0, sizeof(sockaddr_storage));
	if (pszHost == NULL)
	{
		if (Protocol != EnableIPv4)
		{
			Addr.saIn6.sin6_family = AF_INET6;
			Addr.saIn6.sin6_port = htons(wPort);
			Addr.saIn6.sin6_addr = in6addr_any;
		}
		else
		{
			Addr.saIn.sin_family = AF_INET;
			Addr.saIn.sin_port = htons(wPort);
			Addr.saIn.sin_addr.s_addr = htonl(INADDR_ANY);
		}
	}
	else
	{
		//设置参数
		ADDRINFOT hints = { 0 };
		if (Protocol == EnableIPv4)
		{
			hints.ai_family = PF_INET;
		}
		else if (Protocol == EnableIPv6)
		{
			hints.ai_family = PF_INET6;
		}
		else
		{
			hints.ai_family = PF_UNSPEC;
		}

		if (!bBlock)
		{
			hints.ai_flags = AI_NUMERICHOST;
		}

		//解析地址
		ADDRINFOT* info = 0;
		int retry = 5;
		int rs = 0;
		do
		{
			rs = GetAddrInfo(pszHost, 0, &hints, &info);
		} while (info == 0 && rs == EAI_AGAIN && --retry >= 0);

		if (!bBlock && (rs == EAI_NONAME || rs == EAI_NODATA))
		{
			return Addr;
		}

		if (rs != 0)
		{
			return Addr;
		}

		//查找地址
		for (ADDRINFOT *p = info; p != NULL; p = p->ai_next)
		{
			memcpy(&Addr.saStorage, p->ai_addr, p->ai_addrlen);
			if (p->ai_family == PF_INET)
			{
				Addr.saIn.sin_port = htons(wPort);
			}
			else if (p->ai_family == PF_INET6)
			{
				Addr.saIn6.sin6_port = htons(wPort);
			}
			break;
		}

		FreeAddrInfo(info);
	}

	return Addr;
}

Address CNetwork::GetAddress(const Address& Addr, WORD wPort)
{
	Address bindAddr;
	memset(&bindAddr.saStorage, 0, sizeof(sockaddr_storage));
	if (Addr.saStorage.ss_family == AF_INET)
	{
		bindAddr.saIn.sin_family = AF_INET;
		bindAddr.saIn.sin_port = htons(wPort);
		bindAddr.saIn.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else if (Addr.saStorage.ss_family == AF_INET6)
	{
		bindAddr.saIn6.sin6_family = AF_INET6;
		bindAddr.saIn6.sin6_port = htons(wPort);
		bindAddr.saIn6.sin6_addr = in6addr_any;
	}

	return bindAddr;
}

int CNetwork::GetAddressSize(const Address& Addr)
{
	if (Addr.saStorage.ss_family == AF_INET)
	{
		return sizeof(sockaddr_in);
	}

	if (Addr.saStorage.ss_family == AF_INET6)
	{
		return sizeof(sockaddr_in6);
	}

	return 0;
}

Address CNetwork::GetLocalAddress(SOCKET hSocket)
{
	Address Addr;
	memset(&Addr.saStorage, 0, sizeof(sockaddr_storage));

	socklen_t len = static_cast<socklen_t>(sizeof(sockaddr_storage));
	if (getsockname(hSocket, &Addr.sa, &len) == SOCKET_ERROR)
	{
		memset(&Addr.saStorage, 0, sizeof(sockaddr_storage));
	}

	return Addr;
}

Address CNetwork::GetRemoteAddress(SOCKET hSocket)
{
	Address Addr;
	memset(&Addr.saStorage, 0, sizeof(sockaddr_storage));

	socklen_t len = static_cast<socklen_t>(sizeof(sockaddr_storage));
	if (getpeername(hSocket, &Addr.sa, &len) == SOCKET_ERROR)
	{
		memset(&Addr.saStorage, 0, sizeof(sockaddr_storage));
	}

	return Addr;
}

bool CNetwork::IsValidAddress(const Address& Addr)
{
	return (Addr.saStorage.ss_family != AF_UNSPEC);
}

SOCKET CNetwork::CreateSocket(const Address& Addr, ProtocolSupport Protocol, bool bUDP /* = false */)
{
	SOCKET hSocket = CreateSocket(Addr, bUDP);
	if (hSocket == INVALID_SOCKET) return hSocket;
	if (Addr.saStorage.ss_family == AF_INET6 && Protocol != EnableIPv4)
	{
		int flag = (Protocol == EnableIPv6) ? 1 : 0;
		if (setsockopt(hSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&flag), int(sizeof(int))) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAENOPROTOOPT)
			{
				return hSocket;
			}
			CloseSocket(hSocket);
			hSocket = INVALID_SOCKET;
		}
	}

	return hSocket;
}

SOCKET CNetwork::CreateSocket(const Address& Addr, bool bUDP /* = false */)
{
	if (bUDP)
	{
		return socket(Addr.saStorage.ss_family, SOCK_DGRAM, IPPROTO_UDP);
	}
	
	SOCKET hSocket = socket(Addr.saStorage.ss_family, SOCK_STREAM, IPPROTO_TCP);
	/*
	//设置属性
	if (hSocket != INVALID_SOCKET)
	{
		int flag = 1;
		if (setsockopt(hSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&flag), int(sizeof(int))) == SOCKET_ERROR)
		{
			CloseSocket(hSocket);
			hSocket = INVALID_SOCKET;
		}
	}
	//设置属性
	if (hSocket != INVALID_SOCKET)
	{
		int flag = 1;
		if (setsockopt(hSocket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&flag), int(sizeof(int))) == SOCKET_ERROR)
		{
			CloseSocket(hSocket);
			hSocket = INVALID_SOCKET;
		}
	}
	*/

	return hSocket;
}

bool CNetwork::CloseSocket(SOCKET hSocket)
{
	if (hSocket == INVALID_SOCKET) return true;
	if (closesocket(hSocket) == SOCKET_ERROR)
	{
		return false;
	}

	return true;
}

Address CNetwork::Bind(SOCKET hSocket, const Address& Addr)
{
	Address local;
	memset(&local.saStorage, 0, sizeof(sockaddr_storage));

	//获取大小
	int size = GetAddressSize(Addr);
	if (size > 0)
	{
		//绑定对象
		if (::bind(hSocket, &Addr.sa, size) != SOCKET_ERROR)
		{
			socklen_t len = static_cast<socklen_t>(sizeof(sockaddr_storage));
			if (getsockname(hSocket, &local.sa, &len) == SOCKET_ERROR)
			{
				memset(&local.saStorage, 0, sizeof(sockaddr_storage));
			}
		}
	}

	return local;
}

bool CNetwork::Listen(SOCKET hSocket, int nBacklog)
{
	do
	{
		if (::listen(hSocket, nBacklog) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEINTR) continue;
			return false;
		}

		return true;

	} while (1);

	return false;
}

SOCKET CNetwork::Accept(SOCKET hSocket)
{
	SOCKET hConnectSocket = INVALID_SOCKET;

	do
	{
		hConnectSocket = ::accept(hSocket, NULL, NULL);
		if (hConnectSocket == INVALID_SOCKET)
		{
			int error = WSAGetLastError();
			if (error == WSAEINTR || error == WSAECONNABORTED || error == WSAECONNRESET || error == WSAETIMEDOUT) continue;
			return INVALID_SOCKET;
		}
		/*
		//设置属性
		if (hConnectSocket != INVALID_SOCKET)
		{
			int flag = 1;
			if (setsockopt(hConnectSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&flag), int(sizeof(int))) == SOCKET_ERROR)
			{
				CloseSocket(hConnectSocket);
				hConnectSocket = INVALID_SOCKET;
			}
		}
		//设置属性
		if (hConnectSocket != INVALID_SOCKET)
		{
			int flag = 1;
			if (setsockopt(hConnectSocket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&flag), int(sizeof(int))) == SOCKET_ERROR)
			{
				CloseSocket(hConnectSocket);
				hConnectSocket = INVALID_SOCKET;
			}
		}
		*/

		return hConnectSocket;

	} while (1);

	return hConnectSocket;
}

bool CNetwork::Connect(SOCKET hSocket, const Address& Addr, const Address& SourceAddr)
{
	int size = GetAddressSize(Addr);
	if (size == 0)
	{
		return false;
	}

	if (IsValidAddress(SourceAddr))
	{
		Bind(hSocket, SourceAddr);
	}

	do
	{
		if (::connect(hSocket, &Addr.sa, size) == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEINTR) continue;
			return false;
		}

		return true;

	} while (1);

	return false;
}

bool CNetwork::GetIP(const Address& Addr, TCHAR * pszBuffer, DWORD dwBufferLength)
{
	int size = GetAddressSize(Addr);
	if (size == 0) return false;
	return (GetNameInfo(&Addr.sa, size, pszBuffer, dwBufferLength, 0, 0, NI_NUMERICHOST) == 0);
}

WORD CNetwork::GetPort(const Address& Addr)
{
	if (Addr.saStorage.ss_family == AF_INET)
	{
		return ntohs(Addr.saIn.sin_port);
	}

	if (Addr.saStorage.ss_family == AF_INET6)
	{
		return ntohs(Addr.saIn6.sin6_port);
	}

	return 0;
}

bool CNetwork::SetPort(Address& Addr, WORD wPort)
{
	if (Addr.saStorage.ss_family == AF_INET)
	{
		Addr.saIn.sin_port = htons(wPort);
		return true;
	}

	if (Addr.saStorage.ss_family == AF_INET6)
	{
		Addr.saIn6.sin6_port = htons(wPort);
		return true;
	}

	return false;
}

ProtocolSupport CNetwork::GetProtocolSupport(const Address& Addr)
{
	return (Addr.saStorage.ss_family == AF_INET ? EnableIPv4 : EnableIPv6);
}

bool CNetwork::WouldBlock()
{
	int error = WSAGetLastError();
	//return (error == WSAEWOULDBLOCK || error == WSA_IO_PENDING || error == ERROR_IO_PENDING);
	return (error == WSA_IO_PENDING);
}

//////////////////////////////////////////////////////////////////////////
