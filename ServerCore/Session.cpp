#include "pch.h"
#include "Session.h"
#include <winSock2.h>
#include "SocketUtils.h"
#include "Service.h"

/*--------------------
	Session
--------------------*/

Session::Session() {
	_socket = SocketUtils::CreateSocket();
}

Session::~Session() {
	SocketUtils::Close(_socket);
}

void Session::Disconnect(const WCHAR* cause) {
	// 이미 연결 끊어져 있으면 종료
	if (_connected.exchange(false) == false)
		return;

	// TEMP
	wcout << "Disconnect: " << cause << endl;

	OnDisconnected();  // 컨텐츠 코드에서 오버로딩할 OnDisconnected 호출
	SocketUtils::Close(_socket);
	GetService()->ReleaseSession(GetSessionRef());
}

HANDLE Session::GetHandle() {
	return reinterpret_cast<HANDLE>(_socket);
}

void Session::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes) {
	switch (iocpEvent->eventType) {
		case EventType::Connect:
			ProcessConnect();
			break;
		case EventType::Recv:
			ProcessRecv(numOfBytes);
			break;
		case EventType::Send:
			ProcessSend(numOfBytes);
			break;
		default:
			ASSERT_CRASH("Invalid event type");
			break;
	}
}

void Session::RegisterConnect() {}

void Session::RegisterRecv() {
	if (IsConnected() == false)
		return;

	_recvEvent.Init();
	_recvEvent.owner = shared_from_this();
	// ADD_REF해서 WSARecv에 등록한 뒤로 절대 삭제되지 않게 함

	WSABUF wsaBuf;
	wsaBuf.buf = reinterpret_cast<char*>(_recvBuffer);
	wsaBuf.len = len32(_recvBuffer);

	DWORD numOfBytes = 0;
	DWORD flags = 0;
	if (SOCKET_ERROR == ::WSARecv(_socket, &wsaBuf, 1, OUT & numOfBytes, &flags, &_recvEvent, nullptr)) {
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			HandleError(errorCode);
			_recvEvent.owner = nullptr;	 // RELEASE_REF
			return;
		}
	}
}

void Session::RegisterSend() {}

void Session::ProcessConnect() {
	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 오버로딩할 OnConnected 호출
	OnConnected();

	// 수신 등록 (낚시대 던지기)
	RegisterRecv();

	// 송신 등록록
}

void Session::ProcessRecv(int32 numOfBytes) {
	_recvEvent.owner = nullptr;	 // RELEASE_REF

	if (numOfBytes == 0) {	// 연결 끊김
		Disconnect(L"Recv 0 byte");
		return;
	}

	// TODO
	cout << "Recv Data Len: " << numOfBytes << endl;

	// 다시 수신 등록 (낚시대 다시 던지기)
	RegisterRecv();
}

void Session::ProcessSend(int32 numOfBytes) {}

void Session::HandleError(int32 errorCode) {
	switch (errorCode) {
		case WSAECONNRESET:
		case WSAECONNABORTED:
			Disconnect(L"HandleError");
			break;
		default:
			// TODO: LOG
			cout << "Handle Error: " << errorCode << endl;
			break;
	}
}
