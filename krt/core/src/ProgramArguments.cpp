#include "StdInc.h"
#include "ProgramArguments.h"

namespace krt
{
ProgramArguments::ProgramArguments(int argc, char** argv)
{
	m_arguments.resize(argc);

	for (auto i : irange(argc))
	{
		m_arguments[i] = argv[i];
	}
}
}