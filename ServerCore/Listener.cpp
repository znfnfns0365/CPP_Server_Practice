#include "pch.h"
#include "Listener.h"
#include "SocketUtils.h"
#include "IocpEvent.h"
#include "Session.h"

/*---------------
	Listener
---------------*/

Listener::~Listener() {
	SocketUtils::Close(_socket);
	for (AcceptEvent* acceptEvent : _acceptEvents) {
		xdelete(acceptEvent);
	}
}

bool Listener::StartAccept(NetAddress netAddr) {
	_socket = SocketUtils::CreateSocket();
	if (_socket == INVALID_SOCKET)
		return false;

	// 이때 Listener 객체를 전달해서 key로 IocpObject(Listener) 전달
	// 후에 IocpCore::Dispatch에서 이 key를 통해 IocpObject(Listener)를 찾아서 처리
	if (GIocpCore.Register(this) == false)
		return false;

	if (SocketUtils::SetReuseAddress(_socket, true) == false)
		return false;

	if (SocketUtils::SetLinger(_socket, 1, 0) == false)
		return false;

	if (SocketUtils::Bind(_socket, netAddr) == false)
		return false;

	if (SocketUtils::Listen(_socket) == false)
		return false;

	const int32 acceptCount = 1;
	for (int32 i = 0; i < acceptCount; i++) {
		AcceptEvent* acceptEvent = xnew<AcceptEvent>();
		acceptEvent->owner = shared_from_this();
		_acceptEvents.push_back(acceptEvent);
		RegisterAccept(acceptEvent);
	}

	return true;
}

void Listener::CloseSocket() {
	SocketUtils::Close(_socket);
}

HANDLE Listener::GetHandle() {
	return reinterpret_cast<HANDLE>(_socket);
}

void Listener::RegisterAccept(AcceptEvent* acceptEvent) {
	SessionRef session = MakeShared<Session>();
	acceptEvent->Init();
	acceptEvent->session = session;

	DWORD bytesRecived = 0;
	// Client  socket은 미리 만들어서 CP에 올라간 상태
	// AcceptEx 이후 어떤 클라가 접속 시도 하면 IocpCore::Dispatch에서 반응 -> IocpObject::Dispatch(Listener::Dispatch)
	if (false == SocketUtils::AcceptEx(_socket, session->GetSocket(), session->_recvBuffer, 0, sizeof(SOCKADDR_IN) + 16,
									   sizeof(SOCKADDR_IN) + 16, OUT & bytesRecived,
									   static_cast<LPOVERLAPPED>(acceptEvent))) {
		const int32 errorCode = ::WSAGetLastError();
		if (errorCode != WSA_IO_PENDING) {
			// RegisterAccept는 끊기면 안 됨 (낚시대가 미끄러진 상황. 낚시대를 다시 던져야 함)
			RegisterAccept(acceptEvent);
			return;
		}
	}
}

void Listener::Dispatch(IocpEvent* iocpEvent, int32 numOfBytes) {
	ASSERT_CRASH(iocpEvent->eventType == EventType::Accept);

	AcceptEvent* acceptEvent = static_cast<AcceptEvent*>(iocpEvent);

	ProcessAccept(acceptEvent);
}

void Listener::ProcessAccept(AcceptEvent* acceptEvent) {
	SessionRef session = acceptEvent->session;

	// Client socket은 원래 listenSocket의 속성을 그대로 받아야 하는데 AcceptEx에선 이 작업을 해주지 않음
	// SetUpdateAcceptSocket 함수를 통해 이 작업을 해줌
	if (false == SocketUtils::SetUpdateAcceptSocket(session->GetSocket(), _socket)) {
		RegisterAccept(acceptEvent);
		return;
	}

	SOCKADDR_IN sockAddress;
	int32 sizeOfSockAddress = sizeof(sockAddress);
	if (SOCKET_ERROR ==
		::getpeername(session->GetSocket(), reinterpret_cast<SOCKADDR*>(&sockAddress), &sizeOfSockAddress)) {
		RegisterAccept(acceptEvent);
		return;
	}

	session->SetNetAddress(NetAddress(sockAddress));

	cout << "Client Connected!!" << endl;

	// TODO

	RegisterAccept(acceptEvent);
}
