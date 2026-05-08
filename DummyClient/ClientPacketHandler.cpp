#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"

/*------------------------
	ClientPacketHandler
------------------------*/

void ClientPacketHandler::HandlePacket(BYTE* buffer, int32 len) {
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	// switch case보다 효율적인 방법이 있음. Map?
	switch (header.id)
	{ 
		case S_TEST:
			Handle_S_TEST(buffer, len);
			break;
	}

	
}

// 패킷 설계 TEMP
struct BuffData
{
	uint64 buffId;
	float remainTime;
};

struct S_TEST
{
	uint64 id;
	uint32 hp;
	uint16 attack;
	// 가변 데이터
	// 1) 문자열 (ex. name)
	// 2) 그냥 바이트 배열 (ex. 길드 img)
	// 3) 일반적인 리스트
	vector<BuffData> buffs;
};

void ClientPacketHandler::Handle_S_TEST(BYTE* buffer, int32 len) {
	BufferReader br(buffer, len);

	PacketHeader header;
	br >> header;

	uint64 id;
	uint32 hp;
	uint16 attack;
	br >> id >> hp >> attack;

	cout << "ID: " << id << " HP: " << hp << " ATT: " << attack << endl;

	vector<BuffData> buffs;
	uint16 buffSize;
	br >> buffSize;

	buffs.resize(buffSize);
	for (int i = 0; i < buffSize; i++) {
		br >> buffs[i].buffId >> buffs[i].remainTime;
	}

	cout << "BufSize: " << buffSize << endl;
	for (int i = 0; i < buffSize; i++) {
		cout << "BuffInfo: " << buffs[i].buffId << " " << buffs[i].remainTime << endl;
	}
}
