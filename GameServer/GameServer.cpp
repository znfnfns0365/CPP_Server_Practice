#include "CoreGlobal.h"
#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <future>
#include "ThreadManager.h"
#include "SocketUtils.h"
#include "Listener.h"

int main() {
	Listener listener;
	listener.StartAccept(NetAddress(L"127.0.0.1", 7777));

	for (int32 i = 0; i < 5; i++) {
		GThreadManager->Launch([=] {
			while (true) {
				GIocpCore.Dispatch();
			}
		});
	}

	GThreadManager->Join();
	// CoreGlobal() 호출용 (사용 안 하면 컴파일러가 생성자 호출을 스킵함)
	return 0;
}