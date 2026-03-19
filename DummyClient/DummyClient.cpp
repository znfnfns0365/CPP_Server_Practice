#include "pch.h"
#include <iostream>

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
	SOCKET clientSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket == INVALID_SOCKET) {
		int32 errCode = ::WSAGetLastError();
		cout << "socket error: " << errCode << endl;
		return 0;
	}

	// 목적지 주소 (서버 주소) = IP주소 + Port번호
	SOCKADDR_IN serverAddr;	 // IPv4 주소 구조체
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	// serverAddr.sin_addr.s_addr = ::inet_addr("127.0.0.1"); // 너무 예전 방식
	// inet_pton: 글자 자체 주소를 IPv4 주소로 변환하여 pAddrBuf에 삽입
	::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
	// 이런 식으로 serverAddr.sin_addr에 로컬 ip 삽입
	serverAddr.sin_port = ::htons(7777);

	// htons: host(local) to network(network 방식) short
	// Endian issue를 해결하기 위해 사용(little to big)
	// low [0x78][0x56][0x34][0x12] high < little endian = intel cpu(내 컴퓨터)에서 사용
	// low [0x12][0x34][0x56][0x78] high < big endian = network에서 사용

	if (::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		int32 errCode = ::WSAGetLastError();
		cout << "connect error: " << errCode << endl;
		return 0;
	}

	// ---------------------------------------
	// 연결 성공, 데이터 송수신 가능
	// TODO

	cout << "Connected to server" << endl;

	while (true) {
		this_thread::sleep_for(1s);
	}
	// ---------------------------------------

	// 소켓 리소스 반환
	::closesocket(clientSocket);

	// 윈속 종료
	::WSACleanup();
	return 0;
}