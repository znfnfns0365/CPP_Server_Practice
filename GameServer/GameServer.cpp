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

int main() {
	// 원속 초기화(ws2_32 lib 초기화)
	// 관련 정보가 wsaData에 저장됨
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;

	// ad: Address Family (AF_INET: IPv4, AF_INET6: IPv6)
	// type: Socket Type (SOCK_STREAM: TCP, SOCK_DGRAM: UDP)
	// protocol:0
	// return: descriptor
	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		int32 errCode = ::WSAGetLastError();
		cout << "socket error: " << errCode << endl;
		return 0;
	}

	// 나의 주소 (서버 주소) = IP주소 + Port번호
	SOCKADDR_IN serverAddr;	 // IPv4 주소 구조체
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);  // 주소 알아서 골라줘
	serverAddr.sin_port = ::htons(7777);

	// bind: 소켓에 주소 바인딩
	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		int32 errCode = ::WSAGetLastError();
		cout << "bind error: " << errCode << endl;
		return 0;
	}

	// listen: 소켓을 수신 대기 상태로 전환
	if (::listen(listenSocket, 10) == SOCKET_ERROR) {  // backlog: 최대 대기 수
		int32 errCode = ::WSAGetLastError();
		cout << "listen error: " << errCode << endl;
		return 0;
	}

	// ---------------------------------------
	// 클라이언트 연결 대기

	while (true) {
		SOCKADDR_IN clientAddr;	 // 클라이언트 주소 구조체
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 clientAddrSize = sizeof(clientAddr);
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
		if (clientSocket == INVALID_SOCKET) {
			int32 errCode = ::WSAGetLastError();
			cout << "accept error: " << errCode << endl;
			continue;
		}

		// 클라이언트 연결 성공
		char ipAddress[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ipAddress, sizeof(ipAddress));

		cout << "Client Connected, IP: " << ipAddress << endl;

		// TODO
	}
	// ---------------------------------------

	// 윈속 종료
	::WSACleanup();
	return 0;
}