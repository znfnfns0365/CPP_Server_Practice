#include "CoreGlobal.h"
#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <future>
#include "ThreadManager.h"

#include "Service.h"
#include "Session.h"

class GameSession : public Session {
	// 실제 게임 서버 로직이 돌아갈 세션을 Session 상속받아서 작성 후 사용
public:
	virtual int32 OnRecv(BYTE* buffer, int32 len) override {
		// Echo
		cout << "OnRecv Data Len: " << len << endl;
		Send(buffer, len);
		return len;	 // len 반환 이유는 나중에 설명
	}

	virtual void OnSend(int32 len) override { cout << "OnSend Data Len: " << len << endl; }
};

int main() {
	ServerServiceRef service =
		MakeShared<ServerService>(NetAddress(L"127.0.0.1", 7777), MakeShared<IocpCore>(), MakeShared<GameSession>, 100);

	ASSERT_CRASH(service->Start());

	for (int32 i = 0; i < 5; i++) {
		GThreadManager->Launch([=] {
			while (true) {
				service->GetIocpCore()->Dispatch();
			}
		});
	}

	GThreadManager->Join();
	// CoreGlobal() 호출용 (사용 안 하면 컴파일러가 생성자 호출을 스킵함)
	return 0;
}