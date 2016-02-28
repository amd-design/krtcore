#pragma once

class FrpUseSequentialAllocator
{
public:
	void* operator new[](size_t size);

	void operator delete[](void* ptr);
};

void FrpSeqAllocatorWaitForSwap();

void FrpSeqAllocatorUnlockSwap();

void FrpSeqAllocatorSwapPage();