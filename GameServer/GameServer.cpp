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
	SOCKET socket;
	char recvBuffer[BUFFER_SIZE] = {};
	int32 recvBytes = 0;
	int32 sendBytes = 0;
};

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

	// WSAEVENT, Session 벡터를 만들어 각 소켓과 연동된 이벤트 객체를 관리
	vector<WSAEVENT> wsaEvents;
	vector<Session> sessions;
	sessions.reserve(100);	// 불필요한 복사를 막기 위해 미리 공간을 할당

	WSAEVENT listenEvent = WSACreateEvent();
	wsaEvents.push_back(listenEvent);
	sessions.push_back(Session{listenSocket});
	if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
		return 0;

	while (true) {
		int32 index = WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);
		if (index == WSA_WAIT_FAILED)
			continue;

		// WSAWaitForMultipleEvents의 return value에 WSA_WAIT_EVENT_0가 더해져서 오기 때문에 빼줌
		index -= WSA_WAIT_EVENT_0;

		// ::WSAResetEvent(wsaEvents[index]);
		// WSAEnumNetworkEvents의 2번째 인자에 wasEvents[index]를 넘겨주면, 자동으로로 이벤트 객체를 non-signaled 상태로
		// 리셋하기 때문에 안 해도 됨

		WSANETWORKEVENTS networkEvents;
		if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
			continue;

		// Listener Socket 체크
		if (networkEvents.lNetworkEvents & FD_ACCEPT) {
			// Error-Check
			if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
				continue;

			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET) {
				cout << "Client Connected" << endl;

				WSAEVENT clientEvent = WSACreateEvent();
				wsaEvents.push_back(clientEvent);
				sessions.push_back(Session{clientSocket});
				if (::WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
					return 0;
			}
		}

		// Client Session 소켓 체크
		if (networkEvents.lNetworkEvents & FD_READ || networkEvents.lNetworkEvents & FD_WRITE) {
			// Error-Check
			if ((networkEvents.lNetworkEvents & FD_READ) && (networkEvents.iErrorCode[FD_READ_BIT] != 0))
				continue;

			// Error-Check
			if ((networkEvents.lNetworkEvents & FD_WRITE) && (networkEvents.iErrorCode[FD_WRITE_BIT] != 0))
				continue;

			Session& s = sessions[index];

			// Read
			if (s.recvBytes == 0) {
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUFFER_SIZE, 0);

				// 드물게 WSAEWOULDBLOCK 오류가 뜰 수 있음
				// Socket Err 발생했을 때, GetLastError가 WOULDBLOCK이 아니면 에러 처리
				if (recvLen == SOCKET_ERROR) {
					if (::WSAGetLastError() != WSAEWOULDBLOCK)
						// TODO: Remove Session
						return 0;
					continue;
				}

				s.recvBytes = recvLen;
				cout << "Recv Data! Len =" << recvLen << endl;
			}
			if (s.recvBytes > s.sendBytes) {
				int32 sendLen = ::send(s.socket, &s.recvBuffer[s.sendBytes], s.recvBytes - s.sendBytes, 0);
				if (sendLen == SOCKET_ERROR) {
					if (::WSAGetLastError() != WSAEWOULDBLOCK) {
						// TODO: Remove Session
						return 0;
					}
					continue;
				}

				s.sendBytes += sendLen;
				if (s.recvBytes == s.sendBytes) {
					s.sendBytes = 0;
					s.recvBytes = 0;
				}

				cout << "Send Data! Len =" << sendLen << endl;
			}
		}

		// FD_CLOSE 처리
		if (networkEvents.lNetworkEvents & FD_CLOSE) {
			// TODO: Remove Session
			continue;
		}
	}

	// ---------------------------------------

	// 윈속 종료
	::WSACleanup();
	return 0;
}