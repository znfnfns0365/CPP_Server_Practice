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
	this_thread::sleep_for(1s);

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

	char sendBuffer[100] = "Hello Server!";
	while (true) {
		int sendLen = ::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);
		if (sendLen == SOCKET_ERROR) {
			// 원래 send 될 때까지 블록인데, 논블록 설정 중이라 넘어옴
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			// Error
			break;
		}

		cout << "Send Data! Len =" << sizeof(sendBuffer) << endl;

		while (true) {
			char recvBuffer[1000];
			int recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen == SOCKET_ERROR) {
				// 원래 recv 될 때까지 블록인데, 논블록 설정 중이라 넘어옴
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
					continue;

				// Error
				break;
			} else if (recvLen == 0) {
				// 서버가 연결을 끊음
				cout << "Client Disconnected!" << endl;
				break;
			}

			cout << "Recv Data! Len =" << recvLen << endl;
			break;
		}

		this_thread::sleep_for(1s);
	}

	// ---------------------------------------

	// 소켓 리소스 반환
	::closesocket(clientSocket);

	// 윈속 종료
	::WSACleanup();
	return 0;
}