#include "pch.h"
#include <iostream>
#include "CorePch.h"
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <future>
#include "ThreadManager.h"

#include "RefCounting.h"
#include "Memory.h"
#include "Allocator.h"

using KnightRef = TSharedPtr<class Knight>;
using InventoryRef = TSharedPtr<class Inventory>;

class Player {
public:
	Player() { cout << "Player()" << endl; }
	virtual ~Player() { cout << "~Player()" << endl; }
};

class Knight : public Player {
public:
	Knight() { cout << "Knight()" << endl; }
	Knight(int32 hp) : _hp(hp) { cout << "Knight(int32 hp): " << hp << endl; }
	~Knight() { cout << "~Knight()" << endl; }

	int32 _hp = 100;
	int32 _mp = 100;
};

int main() {
	Vector<Knight> vec(100);

	return 0;
}