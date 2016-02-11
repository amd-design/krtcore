#pragma once

// assertion handlers
namespace krt
{
enum AssertionType
{
	ASSERT_DEBUG,
	ASSERT_CHECK
};

void Assert(AssertionType type, const char* assertionString, int line, const char* file, const char* function, int* ignoreCounter);
}

// assertion macros

#define _DO_ASSERT(type, expression, counter) \
	{ \
		static int __assert_counter_##counter; \
		!!(expression) || (::krt::Assert(::krt::type, #expression, __LINE__, __FILE__, __FUNCTION__, &__assert_counter_##counter), 1); \
	}

#ifdef _DEBUG
#define assert(n) \
	_DO_ASSERT(ASSERT_DEBUG, n, __COUNTER__)
#else
#define assert(n) (void)0
#endif

#define check(n) \
	_DO_ASSERT(ASSERT_CHECK, n, __COUNTER__)