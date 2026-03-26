#include "CoreGlobal.h"
#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <future>
#include "ThreadManager.h"

#include <handleapi.h>
#include <ioapiset.h>
#include <urlmon.h>
#include <winSock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#include <winnt.h>

#include "Memory.h"

#pragma comment(lib, "ws2_32.lib")

void HandleError(const char* funcName) {
	int32 errCode = ::WSAGetLastError();
	cout << "Causon Func: " << funcName << endl << "socket error : " << errCode << endl;
}

const int32 BUFFER_SIZE = 1000;

struct Session {
	SOCKET socket;
	char recvBuffer[BUFFER_SIZE] = {};
	int32 recvBytes = 0;
};

enum IO_TYPE {
	READ,
	WRITE,
	ACCEPT,
	CONNECT,
};

struct OverlappedEx {
	WSAOVERLAPPED overlapped = {};
	IO_TYPE type = IO_TYPE::READ;  // 0
};

void WorkerThreadMain(HANDLE iocpHandle) {
	while (true) {
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;
		bool ret = ::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (ULONG_PTR*)&session,
											   (LPOVERLAPPED*)&overlappedEx, INFINITE);

		if (ret == false || bytesTransferred == 0) {
			// TODO: 연결 끊김
			continue;
		}

		ASSERT_CRASH(overlappedEx->type == IO_TYPE::READ);	// 현재 recv만 작업중

		cout << "Recv Data! Len =" << bytesTransferred << endl;

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFFER_SIZE;

		DWORD recvBytes = 0;
		DWORD flags = 0;
		// Recv가 아니라면 overlappedEx->type을 변경해서 넣어줘야 함
		::WSARecv(session->socket, &wsaBuf, 1, &recvBytes, &flags, &overlappedEx->overlapped, NULL);
	}
}

int main() {
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(7777);

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	cout << "Accept" << endl;

	// 모든 세션(Client)을 관리해주는 session 관리자가 있다고 가정
	vector<Session*> sessionManager;

	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Worker Thread 생성
	for (int32 i = 0; i < 5; i++) {
		GThreadManager->Launch([=]() { WorkerThreadMain(iocpHandle); });
	}

	// Main Thread = Accept 담당하므로 논블로킹이 필요없음
	// 한 클라이언트에 대한 역할 >> Accept 하고 첫 번재 WSARecv 호출하고 끝
	while (true) {
		SOCKADDR_IN clientAddr;
		int clientAddrLen = sizeof(clientAddr);

		// 추후에 accept도 비동기로 바꾸면 코드가 깔끔해질 예정
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = new Session();
		//Session* session = xnew<Session>();
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Connected to Client!" << endl;

		::CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, (ULONG_PTR)session, 0);

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUFFER_SIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvBytes = 0;
		DWORD flags = 0;
		::WSARecv(clientSocket, &wsaBuf, 1, &recvBytes, &flags, &overlappedEx->overlapped, NULL);

		// Problem) 유저가 접속 종료함
		/*Session* s = sessionManager.back();
		sessionManager.pop_back();
		xdelete(s);*/
		// 유저가 종료 전에 send 했다면 WSARecv 한 번 더 할 때 오류 발생
		// CP에 넣은 session, overlapped은 IO 작업이 걸려있는 상황에선 삭제가 불가능하게 만들어줘야 함

		// ::closesocket(session.socket);
		// ::WSACloseEvent(wsaEvent);
		// ---------------------------------------
	}
	GThreadManager->Join();

	// 윈속 종료
	::WSACleanup();
	return 0;
}