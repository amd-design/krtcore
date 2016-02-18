#pragma once

// Native performance locks because C++ objects tend to overuse OS resources.
// Please port these locks to all kind of systems we want to support.
// Do not import this header in the global namespace.

#include <Windows.h>

namespace krt
{

// Lock based on the "SRWLOCK_VIRTUAL" definition.
struct NativeSRW_Exclusive
{
    inline NativeSRW_Exclusive( SRWLOCK_VIRTUAL& lockHandle ) : lockHandle( lockHandle )
    {
        AcquireSRWLockExclusive( (SRWLOCK*)&lockHandle );
    }

    inline ~NativeSRW_Exclusive( void )
    {
        ReleaseSRWLockExclusive( (SRWLOCK*)&lockHandle );
    }

private:
    SRWLOCK_VIRTUAL& lockHandle;
};

struct NativeSRW_Shared
{
    inline NativeSRW_Shared( SRWLOCK_VIRTUAL& lockHandle ) : lockHandle( lockHandle )
    {
        AcquireSRWLockShared( (SRWLOCK*)&lockHandle );
    }

    inline ~NativeSRW_Shared( void )
    {
        ReleaseSRWLockShared( (SRWLOCK*)&lockHandle );
    }

private:
    SRWLOCK_VIRTUAL& lockHandle;
};

}