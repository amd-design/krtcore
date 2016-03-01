#include "StdInc.h"
#include "fonts/FontRendererAllocator.h"

#include <windows.h>

static void* g_seqArena = VirtualAlloc(nullptr, 3 * 8 * 1024 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
static int g_seqPage;
static LONG g_seqStart;
static HANDLE g_swapEvent = CreateEvent(nullptr, TRUE, TRUE, nullptr);

void* FrpUseSequentialAllocator::operator new[] (size_t size)
{
	char* arenaPtr = reinterpret_cast<char*>(g_seqArena) + (g_seqPage * (8 * 1024 * 1024));

	LONG offset = InterlockedAdd(&g_seqStart, static_cast<LONG>(size)) - static_cast<LONG>(size);
	void* retPtr = &arenaPtr[offset];

	return retPtr;
}

void FrpUseSequentialAllocator::operator delete[] (void* ptr)
{
	// no-op
}

void FrpSeqAllocatorWaitForSwap()
{
	WaitForSingleObject(g_swapEvent, INFINITE);
}

void FrpSeqAllocatorUnlockSwap()
{
	SetEvent(g_swapEvent);
}

void FrpSeqAllocatorSwapPage()
{
	g_seqPage = (g_seqPage + 1) % 3;
	g_seqStart = 0;
}