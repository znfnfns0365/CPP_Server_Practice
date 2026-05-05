#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"

 void GameSession::OnConnected() {
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
 }

void GameSession::OnDisconnected() {
	 GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
 }

int32 GameSession::OnRecv(BYTE* buffer, int32 len) {
	// Echo
	cout << "OnRecv Data Len: " << len << endl;

	SendBufferRef sendBuffer = MakeShared<SendBuffer>(4096);
	sendBuffer->CopyData(buffer, len);
	
	for (int32 i=0; i<5; i++)
		GSessionManager.Broadcast(sendBuffer);

	return len;	 // len 반환 이유는 나중에 설명
}

 void GameSession::OnSend(int32 len)  {
	cout << "OnSend Data Len: " << len << endl;
}