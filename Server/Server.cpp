#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
using namespace std;
#include <atomic>
#include <mutex>
#include <windows.h>
#include "ThreadManager.h"

// IOCP는 항상 네트워크에서만 쓸 수 있는건 아님
// 네트워크 IO 뿐만 아니라 게임 내 컨텐츠 용도로도 섞어서 쓸 수도 있다

const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT
};

struct OverlappedEx
{
	WSAOVERLAPPED overlapped = {};
	int32 type = 0;
	// TODO;
};

void WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		//GQCS : Thread Safe한 함수
		// (주의) 소켓이 끊긴 채로 해당 소켓의 session, overlapped에 접근하면 크래시가 발생하므로 나중에 레퍼런스 카운트 등의 처리를 해줘야함
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;
		bool ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);

		if (ret == false || bytesTransferred == 0)
			continue;

		cout << "Recv Data Len = " << bytesTransferred << endl;
		cout << "Recv Data IOCP = " << session->recvBuffer << endl;

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUF_SIZE;

		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags, /*중요*/&overlappedEx->overlapped, NULL);
	}
}

int main()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	// IOCP에 등록하면 자동으로 논 블로킹 소켓이 됨

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;

	vector<Session*> sessionManager;

	// IOCP큐(CP : CompletionPort) 생성 -> WSA 함수의 결과를 모으는 큐, 쓰레드간 공유 됨
	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// WorkerThraeds -> CP에 들어온 일감들을 처리하는 쓰레드
	for (int32 i = 0; i < 5; ++i)
		GThreadManager->Launch([=]() { WorkerThreadMain(iocpHandle); });

	// Accept -> 메인 쓰레드는 Accept만 담당
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		// 일단 지금은 블로킹 함수로 구현
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = new Session();
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Client Connected" << endl;

		// CP에 소켓을 등록하는데 식별 키로 session의 주소값을 이용한다.
		::CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, /*key*/(ULONG_PTR)session, 0);

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUF_SIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->type = IO_TYPE::READ;

		// 처리 했으면 다시 낚싯대 던지기
		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, /*중요*/&overlappedEx->overlapped, NULL);
	}

	SocketUtils::Close(listenSocket);
}