#include <StdInc.h>
#include <utils/CallStack.h>

#include <windows.h>

// dbghelp.h causes this warning
#pragma warning(disable: 4091)
#include <dbghelp.h>

#pragma comment(lib, "dbghelp.lib")

namespace krt
{
std::unique_ptr<CallStack> CallStack::Build(int maxSteps, int skipDepth, void* startingPoint)
{
	void* traceList[128];
	int numFrames = RtlCaptureStackBackTrace(1, _countof(traceList), traceList, nullptr);

	// make a call stack for returning
	std::unique_ptr<CallStack> retval = std::make_unique<CallStack>();

	for (int i : irange(numFrames))
	{

	}

	return retval;
}
}