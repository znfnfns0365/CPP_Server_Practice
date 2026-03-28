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

int main() {
	SOCKET socket = SocketUtils::CreateSocket();

	SocketUtils::BindAnyAddress(socket, 7777);

	SocketUtils::Listen(socket);

	SOCKET clientSocket = ::accept(socket, nullptr, nullptr);

	cout << "Client Connected!" << endl;

	while (1)
	{

	}

	GThreadManager->Join();
	// CoreGlobal() 호출용 (사용 안 하면 컴파일러가 생성자 호출을 스킵함)
	return 0;
}