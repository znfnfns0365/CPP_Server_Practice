#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <future>
#include "ThreadManager.h"

#include <winSock2.h>
#include <mswsock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

void HandleError(const char* funcName) {
	int32 errCode = ::WSAGetLastError();
	cout << "Causon Func: " << funcName << endl << "socket error : " << errCode << endl;
}

int main() {
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET serverSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		HandleError("socket");
		return 0;
	}

	SOCKADDR_IN serverAddr;	 // IPv4 주소 구조체
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	serverAddr.sin_port = ::htons(7777);

	if (::bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		HandleError("bind");
		return 0;
	}

	while (true) {
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 clientAddrLen = sizeof(clientAddr);
		char recvBuffer[100];

		this_thread::sleep_for(1s);
		int32 recvLen =
			::recvfrom(serverSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&clientAddr, &clientAddrLen);

		if (recvLen == SOCKET_ERROR) {
			HandleError("recvfrom");
			return 0;  // 원래는 끝내는게 아니라 해당 클라이언트를 제외하고 다시 받는 작업을 해야함
		}

		cout << "Recv Data! Data=" << recvBuffer << endl;
		cout << "Recv Data Length=" << recvLen << endl;

		int32 errorCode =
			::sendto(serverSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&clientAddr, clientAddrLen);
		if (errorCode == SOCKET_ERROR) {
			HandleError("sendto");
			return 0;
		}
	}

	// ---------------------------------------

	// 윈속 종료
	::WSACleanup();
	return 0;
}