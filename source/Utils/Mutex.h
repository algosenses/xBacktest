#ifndef UTILS_MUTEX_H
#define UTILS_MUTEX_H

#ifdef _MSC_VER
#include <windows.h>
#else
#include <thread>
#include <mutex>
#endif

namespace Utils
{

/**
Portable mutex synchronization primitive.
*/
class Mutex
{
public:

    friend class Condition;
    friend class Lock;

    Mutex()
    {
#ifdef _MSC_VER
        InitializeCriticalSection(&mCriticalSection);
#endif
    }

    ~Mutex()
    {
#ifdef _MSC_VER
        DeleteCriticalSection(&mCriticalSection);
#endif
    }

    /**
    Locks the mutex, guaranteeing exclusive access to a protected resource associated with it.
    \note This is a blocking call and should be used with care to avoid deadlocks.
    */
    void Lock()
    {
#ifdef _MSC_VER
        EnterCriticalSection(&mCriticalSection);
#else
        mMutex.lock();
#endif
    }

    /**
    Unlocks the mutex, releasing exclusive access to a protected resource associated with it.
    */
    void Unlock()
    {
#ifdef _MSC_VER
        LeaveCriticalSection(&mCriticalSection);
#else
        mMutex.unlock();
#endif
    }

private:

    Mutex(const Mutex &other);
    Mutex &operator=(const Mutex &other);

#ifdef _MSC_VER
    CRITICAL_SECTION mCriticalSection;
#else
    std::mutex mMutex;
#endif
};

} // namespace Utils

#endif // UTILS_MUTEX_H