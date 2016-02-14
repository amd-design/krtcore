// Lock utilities, like faster locks of RAII wrappers.
#pragma once

namespace krt
{

template <typename lockType>
struct shared_lock_acquire
{
    inline shared_lock_acquire( lockType& theLock ) : theLock( theLock )
    {
        theLock.lock_shared();
    }

    inline ~shared_lock_acquire( void )
    {
        theLock.unlock_shared();
    }

    lockType& theLock;
};

template <typename lockType>
struct exclusive_lock_acquire
{
    inline exclusive_lock_acquire( lockType& theLock ) : theLock( theLock )
    {
        theLock.lock();
    }

    inline ~exclusive_lock_acquire( void )
    {
        theLock.unlock();
    }

    lockType& theLock;
};

};