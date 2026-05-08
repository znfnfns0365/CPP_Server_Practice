#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "BufferReader.h"
#include "ClientPacketHandler.h"

char sendData[] = "Hello World";

class ServerSession : public PacketSession {
	// 실제 게임 서버 로직이 돌아갈 세션을 Session 상속받아서 작성 후 사용
public:
	~ServerSession() { cout << "~ServerSession" << endl; }

	virtual void OnConnected() override {
		//cout << "Connected To Server" << endl;

	}
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override {
		ClientPacketHandler::HandlePacket(buffer, len);
	}

	virtual void OnSend(int32 len) override { 
		//cout << "OnSend Data Len: " << len << endl; 
	}

	virtual void OnDisconnected() override { 
		//cout << "OnDisconnected" << endl; 
	}
};

int main() {
	this_thread::sleep_for(1s);

	// 5개의 세션을 생성하고, 서버에 연결
	ClientServiceRef service =
		MakeShared<ClientService>(NetAddress(L"127.0.0.1", 7777), MakeShared<IocpCore>(), MakeShared<ServerSession>, 1);
	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 2; i++) {
		GThreadManager->Launch([=] {
			while (true) {
				service->GetIocpCore()->Dispatch();
			}
		});
	}

	GThreadManager->Join();

	return 0;
}