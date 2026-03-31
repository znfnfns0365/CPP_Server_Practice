#pragma once
#include "IocpCore.h"
#include "IocpEvent.h"
#include "NetAddress.h"

/*--------------------
	Session
--------------------*/

class Session : public IocpObject {
public:
	Session();
	virtual ~Session();

public:
	/* 정보 관련 */
	// 클라이언트 주소 및 소켓
	void SetNetAddress(NetAddress address) { _netAddress = address; }
	NetAddress getNetAdress() { return _netAddress; }
	SOCKET GetSocket() { return _socket; }

public:
	/* 인터페이스 구현 */
	virtual HANDLE GetHandle() override;
	virtual void Dispatch(class IocpEvent* iocpEvent, int32 numOfBytes = 0) override;

public:
	// TEMP(임시)
	char _recvBuffer[1000];

private:
	SOCKET _socket = INVALID_SOCKET;
	NetAddress _netAddress = {};
	Atomic<bool> _connected = false;
};
