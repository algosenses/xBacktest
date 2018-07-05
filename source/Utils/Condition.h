#ifndef UTILS_CONDITION_H
#define UTILS_CONDITION_H

#ifdef _MSC_VER
#include <windows.h>
#else
#include <thread>
#include <condition_variable>
#endif

#include "Mutex.h"
#include "Lock.h"

namespace Utils
{

/**
Portable condition variable synchronization primitive, sometimes also called a monitor.
*/
class Condition
{
public:

    /**
    Constructor.
    */
    inline Condition()
    {
#ifdef _MSC_VER
        InitializeConditionVariable(&mCondition);
#endif
    }

    /**
    Destructor.
    \note If the condition is destroyed while threads are still waiting on it, the result is undefined.
    */
    inline ~Condition()
    {
    }

    /**
    Returns a reference to the Mutex owned by the monitor.
    */
    Mutex &GetMutex()
    {
        return mMutex;
    }

    /**
    Suspends the calling thread until it is woken by another thread calling \ref Pulse or \ref PulseAll.
    \note The calling thread must hold a lock on the mutex associated with the condition.
    The lock owned by the caller is released, and automatically regained when the thread is woken.
    */
    void Wait(Lock &lock)
    {
#ifdef _MSC_VER
        (void) lock;
        SleepConditionVariableCS(&mCondition, &lock.mMutex.mCriticalSection, INFINITE);
#else
        // The lock must be locked before calling Wait().
        assert(lock.mLock.owns_lock());
        mCondition.wait(lock.mLock);
#endif
    }

    /**
    Wakes a single thread that is suspended after having called \ref Wait.
    */
    void Pulse()
    {
#ifdef _MSC_VER
        WakeConditionVariable(&mCondition);
#else
        mCondition.notify_one();
#endif
    }

    /**
    Wakes all threads that are suspended after having called \ref Wait.
    */
    void PulseAll()
    {
#ifdef _MSC_VER
         WakeAllConditionVariable(&mCondition);
#else
        mCondition.notify_all();
#endif
    }

private:

    Condition(const Condition &other);
    Condition &operator=(const Condition &other);

    Mutex mMutex;

#ifdef _MSC_VER
    CONDITION_VARIABLE mCondition;
#else
    std::condition_variable mCondition;
#endif
};

} // namespace Utils

#endif // UTILS_CONDITION_H