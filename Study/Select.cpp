#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
using namespace std;
#include <windows.h>

// Select 모델 = (select 함수가 핵심이 되는)
// 소켓 함수 호출이 성공할 시점을 미리 알 수 있다!
// 문제 상황)
// 수신버퍼에 데이터가 없는데, read 한다거나!
// 송신버퍼가 꽉 찼는데, write 한다거나!
// - 블로킹 소켓 : 조건이 만족되지 않아서 블로킹되는 상황 예방
// - 논블로킹 소켓 : 조건이 만족되지 않아서 불필요하게 반복 체크하는 상황을 예방

// socket set
// 1) 읽기[ ] 쓰기[ ] 예외(OOB)[ ] 관찰 대상 등록
// OutOfBand는 send() 마지막 인자 MSG_OOB로 보내는 특별한 데이터
// 받는 쪽에서도 recv OOB 세팅을 해야 읽을 수 있음
// 2) select(readSet, writeSet, exceptSet); -> 관찰 시작
// 3) 적어도 하나의 소켓이 준비되면 리턴 -> 낙오자는 알아서 제거됨
// 4) 남은 소켓 체크해서 진행

// fd_set set;
// FD_ZERO : 비운다
// ex) FD_ZERO(set);
// FD_SET : 소켓 s를 넣는다
// ex) FD_SET(s, &set);
// FD_CLR : 소켓 s를 제거
// ex) FD_CLR(s, &set);
// FD_ISSET : 소켓 s가 set에 들어있으면 0이 아닌 값을 리턴한다

const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};

int Select()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	// 논블로킹
	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;

	vector<Session> sessions;
	sessions.reserve(100);

	// 64개 한도
	fd_set reads;

	while (true)
	{
		// 소켓 셋 초기화
		FD_ZERO(&reads);

		// listenSocket 등록
		FD_SET(listenSocket, &reads);

		// 소켓 등록
		for (Session& s : sessions)
			FD_SET(s.socket, &reads);

		// [옵션] 마지막 timeout 인자 설정 가능
		int32 retVal = ::select(0, &reads, nullptr, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
			break;

		if (FD_ISSET(listenSocket, &reads))
		{
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			if (clientSocket != INVALID_SOCKET)
			{
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
					continue;

				cout << "Client Connected!" << endl;
				sessions.push_back(Session{clientSocket});
			}
		}

		for (Session& s : sessions)
		{
			if (FD_ISSET(s.socket, &reads))
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUF_SIZE, 0);
				if (recvLen <= 0)
				{
					// TODO : sessions 제거
					continue;
				}

				cout << "Recv Data = " << s.recvBuffer << endl;
				s.recvBytes = recvLen;
				cout << "RecvLen = " << recvLen << endl;
			}
		}
	}

	SocketUtils::Clear();

	return 0;
}