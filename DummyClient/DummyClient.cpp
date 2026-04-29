#include "pch.h"
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"

char sendBuffer[] = "Hello World";

class ServerSession : public Session {
	// 실제 게임 서버 로직이 돌아갈 세션을 Session 상속받아서 작성 후 사용
public:
	~ServerSession() { 
		cout << "~ServerSession" << endl;
	}

	virtual void OnConnected() override {
		cout << "Connected To Server" << endl;
		Send((BYTE*)sendBuffer, sizeof(sendBuffer));
	}
	virtual int32 OnRecv(BYTE* buffer, int32 len) override {
		// Echo
		cout << "OnRecv Data Len: " << len << endl;

		this_thread::sleep_for(1s);

		Send((BYTE*)sendBuffer, sizeof(sendBuffer));
		return len;	 // len 반환 이유는 나중에 설명
	}

	virtual void OnSend(int32 len) override { cout << "OnSend Data Len: " << len << endl; }

	virtual void OnDisconnected() override { cout << "OnDisconnected" << endl; }
};

int main() {
	this_thread::sleep_for(1s);

	ClientServiceRef service = MakeShared<ClientService>(NetAddress(L"127.0.0.1", 7777), MakeShared<IocpCore>(),
														 MakeShared<ServerSession>, 1);
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