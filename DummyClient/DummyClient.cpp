#include "pch.h"
#include <iostream>

#include <winSock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void HandleError(const char* funcName) {
	int32 errCode = ::WSAGetLastError();
	cout << "Causon Func: " << funcName << endl << "socket error : " << errCode << endl;
}

int main() {
	// this_thread::sleep_for(1s);

	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET)
		return 0;

	u_long on = 1;
	if (::ioctlsocket(clientSocket, FIONBIO, &on) == SOCKET_ERROR)
		return 0;

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = htons(7777);

	// Connect to Server
	while (true) {
		if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
			// 원래 connect 될 때까지 블록인데, 논블록 설정 중이라 넘어옴
			int WSA = ::WSAGetLastError();
			if (WSA == WSAEWOULDBLOCK || WSA == WSAEALREADY)
				continue;

			// 이미 연결되어 있음
			if (::WSAGetLastError() == WSAEISCONN)
				break;

			// Error
			cout << WSA << endl;
			// return 0;
		}
	}

	cout << "Connected to Server!" << endl;
	WSAEVENT wsaEvent = ::WSACreateEvent();
	WSAOVERLAPPED overlapped = {};
	overlapped.hEvent = wsaEvent;

	char sendBuffer[100] = "Hello Server!";
	while (true) {
		::WSAResetEvent(wsaEvent);
		WSABUF wsaBuf;
		wsaBuf.buf = sendBuffer;
		wsaBuf.len = 100;

		DWORD sendBytes = 0;
		DWORD flags = 0;
		if (::WSASend(clientSocket, &wsaBuf, 1, &sendBytes, flags, &overlapped, nullptr) == SOCKET_ERROR) {
			if (::WSAGetLastError() == WSA_IO_PENDING) {
				::WSAWaitForMultipleEvents(1, &wsaEvent, true, WSA_INFINITE, FALSE);
				::WSAGetOverlappedResult(clientSocket, &overlapped, &sendBytes, false, &flags);
			} else {
				// TODO: 문제 있는 상황
				break;
			}
		}

		cout << "Send Data! Len =" << sendBytes << endl;

		this_thread::sleep_for(1s);
	}

	// ---------------------------------------

	// 소켓 리소스 반환
	::closesocket(clientSocket);

	// 윈속 종료
	::WSACleanup();
	return 0;
}