#include "pch.h"
#include <iostream>

// 클라
// 1) 소켓 생성 (socket)
// 2) 서버에 연결 요청 (connect)
// 3) 통신

int main()
{
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777);

	if(::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	cout << "Connected To Server!" << endl;

	while (true)
	{
		// send는 서버에서 recv를 안해도 블로킹되지 않음
		// 클라의 커널 레벨 sendBuffer에 복사 해둔 뒤 서버의 커널 레벨 recvBuffer에 전송
		char sendBuffer[100] = "Hello I am Client!";
		int32 resultCode = ::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
		if (resultCode == SOCKET_ERROR)
			return 0;

		char recvBuffer[100];
		int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		if (recvLen <= 0)
			return 0;

		cout << "Echo Data : " << recvBuffer << endl;

		this_thread::sleep_for(1s);
	}

	::closesocket(clientSocket);
	::WSACleanup();
}