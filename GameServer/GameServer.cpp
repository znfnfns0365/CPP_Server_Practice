#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <future>
#include "ThreadManager.h"

#include <urlmon.h>
#include <winSock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void HandleError(const char* funcName) {
	int32 errCode = ::WSAGetLastError();
	cout << "Causon Func: " << funcName << endl << "socket error : " << errCode << endl;
}

const int32 BUFFER_SIZE = 1000;

struct Session {
	WSAOVERLAPPED overlapped = {};
	SOCKET socket;
	char recvBuffer[BUFFER_SIZE] = {};
	int32 recvBytes = 0;
};

void CALLBACK RecvCallback(DWORD dwError, DWORD recvLen, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags) {
	cout << "Data Recv Len Callback = " << recvLen << endl;
	Session* session = (Session*)lpOverlapped;
}

int main() {
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == SOCKET_ERROR)
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

	while (true) {
		SOCKADDR_IN clientAddr;
		int clientAddrLen = sizeof(clientAddr);

		SOCKET clientSocket;
		// 추후에 accept도 비동기로 바꾸면 코드가 깔끔해질 예정
		while (true) {
			clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrLen);
			if (clientSocket != INVALID_SOCKET)
				break;

			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// Error
			return 0;
		}

		Session session = Session{};
		session.socket = clientSocket;
		WSAEVENT wsaEvent = ::WSACreateEvent();
		session.overlapped.hEvent = wsaEvent;

		cout << "Connected to Client!" << endl;

		while (true) {
			::WSAResetEvent(wsaEvent);
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUFFER_SIZE;

			DWORD recvBytes = 0;
			DWORD flags = 0;
			// 비동기 recv
			// 수동 초기화 안 해주면 WSARecv 호출전에 자동으로 신호 모두 꺼버림
			if (::WSARecv(clientSocket, &wsaBuf, 1, &recvBytes, &flags, &session.overlapped, RecvCallback) ==
				SOCKET_ERROR) {
				if (::WSAGetLastError() == WSA_IO_PENDING) {
					// Pending (recv 지연중)
					// 스레드를 Alertable Wait 상태로 바꿔줘야 함
					// ::WSAWaitForMultipleEvents(1, &wsaEvent, true, WSA_INFINITE, TRUE);
					::SleepEx(INFINITE, TRUE);
					// 두 함수 모두 APC queue에 들어온 콜백 함수를 기다렸다가 호출함
					// APC queue는 각 스레드마다 독립적으로 존재함
					// 모든 콜백 함수를 호출하여 APC queue를 비운 뒤, 스레드는 Alertable Wait 상태에서 빠져나옴
				} else {
					// TODO: 문제 있는 상황
					break;
				}
			} else {
				cout << "Recv Data! Len =" << recvBytes << endl;
			}
		}

		::closesocket(session.socket);
		::WSACloseEvent(wsaEvent);
	}

	// ---------------------------------------

	// 윈속 종료
	::WSACleanup();
	return 0;
}