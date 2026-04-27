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

void Session::Send(BYTE* buffer, int32 len) {
	// 생각할 문제
	// 1) 버퍼 관리
	// 2) sendEvent 관리? 단일 or 여러 개(WSASend) 중첩?

	// TEMP
	SendEvent* sendEvent = xnew<SendEvent>();
	sendEvent->owner = shared_from_this();	// ADD_REF
	sendEvent->buffer.resize(len);
	::memcpy(sendEvent->buffer.data(), buffer, len);

	WRITE_LOCK;
	RegisterSend(sendEvent);
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
			ProcessSend(static_cast<SendEvent*>(iocpEvent), numOfBytes);
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

void Session::RegisterSend(SendEvent* sendEvent) {
	if (IsConnected() == false)
		return;

	WSABUF wsaBuf;
	wsaBuf.buf = (char*)sendEvent->buffer.data();
	wsaBuf.len = (ULONG)sendEvent->buffer.size();
	// 복사에 cost가 들어감

	DWORD numOfBytes = 0;
	// WSASend는 thread safe하지 않음
	// 따라서 락을 걸어서 동시성 문제를 해결해야 함
	// (Session::Send 함수에서 RegisterSend 함수 호출 전에 락을 걸어서 해결)
	if (SOCKET_ERROR == ::WSASend(_socket, &wsaBuf, 1, OUT & numOfBytes, 0, sendEvent, nullptr)) {
		// 매번 1개씩 보내기 때문에 성능이 떨어짐
		// 스캐터 개더 패턴을 활용해서 해결해야 함
		int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			HandleError(errorCode);
			sendEvent->owner = nullptr;	 // RELEASE_REF
			xdelete(sendEvent);
			return;
		}
	}
}

void Session::ProcessConnect() {
	_connected.store(true);

	// 세션 등록
	GetService()->AddSession(GetSessionRef());

	// 컨텐츠 코드에서 오버로딩할 OnConnected 호출
	OnConnected();

	// 수신 등록 (낚시대 던지기)
	RegisterRecv();

	// 송신 등록
}

void Session::ProcessRecv(int32 numOfBytes) {
	_recvEvent.owner = nullptr;	 // RELEASE_REF

	if (numOfBytes == 0) {	// 연결 끊김
		Disconnect(L"Recv 0 byte");
		return;
	}

	// 컨텐츠 코드에서 오버로딩할 OnRecv 호출
	OnRecv(_recvBuffer, numOfBytes);

	// 다시 수신 등록 (낚시대 다시 던지기)
	RegisterRecv();
}

void Session::ProcessSend(SendEvent* sendEvent, int32 numOfBytes) {
	// Completion Port(Queue)까진 순서대로 끝나지만 IocpCore::Dispatch에서
	// ProcessSend를 호출하는 부분에서는 순서가 보장되지 않음
	sendEvent->owner = nullptr;	 // RELEASE_REF
	xdelete(sendEvent);

	if (numOfBytes == 0) {
		Disconnect(L"Send 0 byte");
		return;
	}

	// 컨텐츠 코드에서 오버로딩할 OnSend 호출
	OnSend(numOfBytes);
}

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
