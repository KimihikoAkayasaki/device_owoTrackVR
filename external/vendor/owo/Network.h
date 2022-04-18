// from https://stackoverflow.com/questions/14665543/how-do-i-receive-udp-packets-with-winsock-in-c

#pragma once

#define  _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <system_error>
#include <string>
#include <iostream>
#include <glog/logging.h>

#pragma comment(lib, "Ws2_32.lib")

class WSASession
{
public:
	WSASession()
	{
		int ret = WSAStartup(MAKEWORD(2, 2), &data);
		if (ret != 0)
			throw std::system_error(WSAGetLastError(), std::system_category(), "WSAStartup Failed");
	}

	~WSASession()
	{
		WSACleanup();
	}

private:
	WSAData data;
};

class UDPSocket
{
public:
	UDPSocket()
	{
		sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		unsigned long ul = 1;
		ioctlsocket(sock, FIONBIO, (unsigned long*)&ul);

		if (sock == INVALID_SOCKET)
			throw std::system_error(WSAGetLastError(), std::system_category(), "Error opening socket");
	}

	~UDPSocket()
	{
		closesocket(sock);
	}

	void SendTo(const std::string& address, unsigned short port, const char* buffer, int len, int flags = 0)
	{
		sockaddr_in add;
		add.sin_family = AF_INET;
		add.sin_addr.s_addr = inet_addr(address.c_str());
		add.sin_port = htons(port);
		int ret = sendto(sock, buffer, len, flags, reinterpret_cast<SOCKADDR*>(&add), sizeof(add));
		if (ret < 0)
			throw std::system_error(WSAGetLastError(), std::system_category(), "sendto failed");
	}

	void SendTo(sockaddr_in& address, const char* buffer, int len, int flags = 0)
	{
		int ret = sendto(sock, buffer, len, flags, reinterpret_cast<SOCKADDR*>(&address), sizeof(address));
		if (ret < 0)
			throw std::system_error(WSAGetLastError(), std::system_category(), "sendto failed");
	}

	bool RecvFrom(char* buffer, int len, SOCKADDR* from, int flags = 0)
	{
		int size = sizeof(sockaddr_in); // reinterpret_cast<SOCKADDR*>(&from)
		int ret = recvfrom(sock, buffer, len, flags, from, &size);
		if (ret == WSAEWOULDBLOCK)
			return false;
		else if (ret < 0)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				return false;
			}
			throw std::system_error(err, std::system_category(), "recvfrom failed");
		}

		// make the buffer zero terminated
		buffer[ret] = 0;
		return true;
	}

	void Bind(uint32_t* port)
	{
		sockaddr_in add;
		add.sin_family = AF_INET;
		add.sin_addr.s_addr = htonl(INADDR_ANY);

		for (uint32_t _port = *port; _port <= *port + 30; _port++)
		{
			add.sin_port = htons(_port);
			int ret = bind(sock, reinterpret_cast<SOCKADDR*>(&add), sizeof(add));

			if (ret < 0)
			{
				LOG(WARNING) << "OWO Device: Port " << *port << " is already taken, retrying with " << _port + 1;
			}
			else
			{
				LOG(INFO) << "Port bind successful at " << _port << ". Overriding this session defaults...";
				*port = _port;
				return; // Don't retry, we're good
			}
		}
		LOG(ERROR) << "OWO Device Error: Ports in range " << *port << "-" << *port + 30 <<
			" are all already taken. Couldn't bind.";
		throw std::system_error(WSAGetLastError(), std::system_category(), "Bind failed");
	}

	void Bind(unsigned short port)
	{
		sockaddr_in add;
		add.sin_family = AF_INET;
		add.sin_addr.s_addr = htonl(INADDR_ANY);
		add.sin_port = htons(port);

		int ret = bind(sock, reinterpret_cast<SOCKADDR*>(&add), sizeof(add));
		if (ret < 0)
			throw std::system_error(WSAGetLastError(), std::system_category(), "Bind failed");
	}

	//private:
	SOCKET sock;
};
