#ifndef UTILS_THREAD_H
#define UTILS_THREAD_H

#include <cassert>

#ifdef _MSC_VER
#include <windows.h>
#include <process.h>
#else
#include <thread>
#endif

namespace Utils
{

#ifdef _MSC_VER
/**
A process thread.
*/
class Thread
{
public:

    /**
    Defines a function that can serve as a thread entry point.
    \note Entry point functions must be static -- implying that they can't be class
    member functions.
    */
    typedef void (*EntryPoint)(void *const context);

    /**
    Default constructor
    */
    inline Thread() : mThread(0)
    {
    }

    /**
    Destructor
    */
    inline ~Thread()
    {
        assert(mThread == 0);
    }

    /**
    Starts the thread, executing the given entry point function.
    \param entryPoint The entry point function that the thread should execute.
    \param context Pointer to a context object providing the environment in which the thread runs.
    \return True, if the thread was started successfully.
    */
    inline bool Start(EntryPoint entryPoint, void *const context)
    {
        assert(mThread == 0);

        // Create a data structure to wrap the data we need to pass to the entry function.
        mThreadData.mEntryPoint = entryPoint;
        mThreadData.mContext = context;

        // It's safer to use _beginthreadex() than calling CreateThread() directly.
        // http://stackoverflow.com/questions/331536/windows-threading-beginthread-vs-beginthreadex-vs-createthread-c
        mThread = (HANDLE)_beginthreadex(
            0,                                      // default security attributes
            0,                                      // use default stack size
            ThreadStartProc,                        // thread entry point function
            reinterpret_cast<void *>(&mThreadData), // pass the real entry point and context
            0,                                      // use default creation flags
            0);                                     // returns the thread identifier (unused)

        // Check the return value for success. 
        return (mThread != 0);
    }

    /**
    Waits for the thread to finish and return.
    The semantics are that Start and Join can be called repeatedly in pairs.
    */
    inline void Join()
    {
        assert(mThread != 0);

        // Wait for the thread to terminate.
        // This treats the thread as an event, which is 'set' when it terminates.
        // Apparently this is common practice.
        WaitForSingleObject(
            mThread,                                // Event handle
            INFINITE);                              // Timeout

        // Destroy the thread object so it can be safely recreated next time.
        // For threads started with _beginthreadex(), CloseHandle() isn't called automatically.
        // This allows the caller time to synchronize with the thread ending.
        CloseHandle(mThread);
        mThread = 0;
    }

    /**
    Returns true if the thread is currently running.
    The thread is running if Start was called more recently than Join.
    */
    bool Running() const
    {
        return (mThread != 0);
    }

private:

    /**
    Struct that holds a pointer to a thread entry point function and some context data.
    */
    struct ThreadData
    {
        EntryPoint mEntryPoint;
        void *mContext;
    };

    /**
    Thread entry point adapter function.
    Wraps a call to a standard Theron-style thread entry point in a Win32-style
    static thread entry point function signature.
    \param pData A pointer to a ThreadData structure containing an entry point
    function and a context pointer.
    \return Unused dummy return value.
    */
    inline static unsigned int __stdcall ThreadStartProc(void *pData)
    {
        // Call the real entry point function, passing the provided context.
        ThreadData *threadData = reinterpret_cast<ThreadData *>(pData);
        threadData->mEntryPoint(threadData->mContext);

        // Terminating a thread with a call to _endthreadex helps to ensure proper
        // recovery of resources allocated for the thread.
        // http://msdn.microsoft.com/en-us/library/kdzttdcb(v=vs.80).aspx
        _endthreadex(0);

        return 0;
    }

    Thread(const Thread &other);
    Thread &operator=(const Thread &other);

    HANDLE mThread;             ///< Handle of the internal Win32 thread.
    ThreadData mThreadData;     ///< Wrapper around the data passed to the thread on start.
};
#else
/**
A process thread.
\note This implementation uses C++0x threads so requires C++0x compiler (e.g. GCC >= 4.6)
*/
class Thread
{
public:

    /**
    Defines a function that can serve as a thread entry point.
    \note Entry point functions must be static -- implying that they can't be
    non-static class member functions.
    */
    typedef void(*EntryPoint)(void *const context);

    /**
    Default constructor
    */
    inline Thread() : mThread(0)
    {
    }

    /**
    Destructor
    */
    inline ~Thread()
    {
        assert(mThread == 0);
    }

    /**
    Starts the thread, executing the given entry point function.
    \param entryPoint The entry point function that the thread should execute.
    \param context Pointer to a context object providing the environment in which the thread runs.
    \return True, if the thread was started successfully.
    */
    inline bool Start(EntryPoint entryPoint, void *const context)
    {
        assert(mThread == 0);

        // Construct a std::thread in the allocated memory
        // Pass it a callable object that in turn calls the entry point, passing it some context.
        ThreadStarter starter(entryPoint, context);
        mThread = new std::thread(starter);

        if (mThread == 0)
        {
            return false;
        }

        return true;
    }

    /**
    Waits for the thread to finish and return.
    The semantics are that Start and Join can be called repeatedly in pairs.
    */
    inline void Join()
    {
        assert(mThread);

        // This waits for the thread function to return.
        mThread->join();

        // We destroy the underlying thread object and recreate it next time.
        // Explicitly destruct because allocated with placement new.
        mThread->~thread();

        delete mThread;
        mThread = 0;
    }

    /**
    Returns true if the thread is currently running.
    The thread is running if Start was called more recently than Join.
    */
    bool Running() const
    {
        return (mThread != 0);
    }

private:

    /**
    A callable object implementing operator() that starts a thread.
    Basically a struct that holds a pointer to a thread entry point function and some context data.
    */
    class ThreadStarter
    {
    public:

        inline ThreadStarter(EntryPoint entryPoint, void *const context) :
            mEntryPoint(entryPoint),
            mContext(context)
        {
        }

        inline void operator()()
        {
            mEntryPoint(mContext);
        }

    private:

        EntryPoint mEntryPoint;
        void *mContext;
    };

    Thread(const Thread &other);
    Thread &operator=(const Thread &other);

    std::thread *mThread;     ///< Pointer to the owned std::thread.
};
#endif

} // namespace Utils

#endif // UTILS_THREAD_H