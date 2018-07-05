#ifndef UTILS_LOCK_H
#define UTILS_LOCK_H

#ifndef _MSC_VER
#include <thread>
#endif
#include "Mutex.h"

namespace Utils
{

/**
Portable lock synchronization primitive.
This class is a helper which allows a mutex to be locked and automatically locked within a scope.
*/
class Lock
{
public:

    friend class Condition;

    /**
    Constructor. Locks the given mutex object.
    */
    explicit Lock(Mutex &mutex) :
#ifdef _MSC_VER
        mMutex(mutex)
#else
        mLock(mutex.mMutex)
#endif
    {
#ifdef _MSC_VER
        mMutex.Lock();
#else
        // The wrapped std::unique_lock locks the mutex itself.
#endif
    }

    /**
    Destructor. Automatically unlocks the locked mutex that was locked on construction.
    */
    ~Lock()
    {
#ifdef _MSC_VER
        mMutex.Unlock();
#else
        // The wrapped std::unique_lock unlocks the mutex itself.
#endif
    }

    /**
    Explicitly and temporarily unlocks the locked mutex.
    \note The caller should always call \ref Relock after this call and before destruction.
    */
    void Unlock()
    {
#ifdef _MSC_VER
        mMutex.Unlock();
#else
        assert(mLock.owns_lock() == true);
        mLock.unlock();
#endif
    }

    /**
    Re-locks the associated mutex, which must have been previously unlocked with \ref Unlock.
    \note Relock can only be called after a preceding call to \ref Unlock.
    */
    void Relock()
    {
#ifdef _MSC_VER
        mMutex.Lock();
#else
        assert(mLock.owns_lock() == false);
        mLock.lock();
#endif
    }

private:

    Lock(const Lock &other);
    Lock &operator=(const Lock &other);

#ifdef _MSC_VER
    Mutex &mMutex;
#else
    std::unique_lock<std::mutex> mLock;
#endif
};

} // namespace Utils

#endif // UTILS_LOCK_H