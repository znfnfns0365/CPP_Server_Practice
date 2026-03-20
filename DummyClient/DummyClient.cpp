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
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	SOCKET clientSocket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		HandleError("socket");
		return 0;
	}

	SOCKADDR_IN serverAddr;	 // IPv4 주소 구조체
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	serverAddr.sin_port = ::htons(7777);

	// Connected UDP
	::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

	while (true) {
		char sendBuffer[100] = "Hello Server";
		for (int32 i = 0; i < 10; i++) {
			// Unconnected UDP
			// int32 sendLen =
			// 	::sendto(clientSocket, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

			// Connected UDP
			int32 sendLen = ::send(clientSocket, sendBuffer, sizeof(sendBuffer), 0);

			if (sendLen == SOCKET_ERROR) {
				HandleError("sendto");
				return 0;
			}
			cout << "Send Data! Data =" << sendBuffer << endl;
			cout << "Send Data! Len =" << sendLen << endl;
		}

		SOCKADDR_IN recvAddr;
		::memset(&recvAddr, 0, sizeof(recvAddr));
		int32 recvAddrLen = sizeof(recvAddr);
		char recvBuffer[100];
		// Unconnected UDP
		// int32 recvLen = ::recvfrom(clientSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&recvAddr,
		// &recvAddrLen);

		// Connected UDP
		int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);

		if (recvLen <= 0) {
			HandleError("recvfrom");
			return 0;
		}

		cout << "Recv Data! Data =" << recvBuffer << endl;
		cout << "Recv Data! Len =" << recvLen << endl;

		this_thread::sleep_for(1s);
	}
	// ---------------------------------------

	// 소켓 리소스 반환
	::closesocket(clientSocket);

	// 윈속 종료
	::WSACleanup();
	return 0;
}