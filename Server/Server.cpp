#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
using namespace std;
#include <windows.h>

// ����
// 1) ���ο� ���� ���� (socket)
// 2) ���Ͽ� �ּ�/��Ʈ ��ȣ ���� (bind)
// 3) ���� �� ��Ű�� (listen)
// 4) �մ� ���� (accept)
// 5) Ŭ��� ���

int main()
{
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	//1) ���� ����
	//int32 errorCode = ::WSAGetLastError();
	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	// 2) �ּ�/��Ʈ ��ȣ ����
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	serverAddr.sin_port = ::htons(7777);

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	// 3) ���� ����
	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	// 4)
	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int addrLen = sizeof(clientAddr);

		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		char ip[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
		cout << "Client Connected! IP = " << ip << endl;

		while (true)
		{
			// 5)
			char recvBuffer[100];
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen <= 0)
				return 0;

			cout << "Recv Data : " << recvBuffer << endl;
			cout << "Recv Data Len : " << recvLen << endl;

			int32 resultCode = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (resultCode == SOCKET_ERROR)
				return 0;
		}
	}

	::closesocket(listenSocket);
	::WSACleanup();
}