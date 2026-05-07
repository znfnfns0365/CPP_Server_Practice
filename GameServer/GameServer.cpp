#include "pch.h"
#include <iostream>
#include "ThreadManager.h"
#include "Service.h"
#include "Session.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "BufferWriter.h"

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

	char sendData[] = "Hello World";

	while (true)
	{
		SendBufferRef sendBuffer = MakeShared<SendBuffer>(4096);

		BufferWriter bw(sendBuffer->Buffer(), 4096);

		// Reserve로 header가 들어갈 공간 미리 확보
		PacketHeader* header = bw.Reserve<PacketHeader>();

		// id(uint64), 체력(uint32), 공격력(uint16)
		bw << (uint64)1001 << (uint32)100 << (uint16)10;
		bw.Write(sendData, sizeof(sendData));
		sendBuffer->SetWriteSize(bw.WriteSize());
		
		header->size = bw.WriteSize();
		header->id = 1;  // 1: Test Msg

		GSessionManager.Broadcast(sendBuffer);
		this_thread::sleep_for(250ms);
	}

	GThreadManager->Join();
	// CoreGlobal() 호출용 (사용 안 하면 컴파일러가 생성자 호출을 스킵함)
	return 0;
}