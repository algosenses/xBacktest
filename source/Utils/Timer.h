#ifndef UTILS_TIMER_H
#define UTILS_TIMER_H

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS 1
#endif

#ifdef WINDOWS
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace Utils
{
// Simple timer class.
class Timer
{
public:

    Timer() : mSupported(false)
    {
#if WINDOWS
        // Read the counter frequency (in Hz) and an initial counter value.
        if (QueryPerformanceFrequency(&mTicksPerSecond) && QueryPerformanceCounter(&mCounterStartValue))
        {
            mSupported = true;
        }
#elif __GNUC__
        mSupported = true;
#endif
    }

    void Start()
    {
#if WINDOWS
        QueryPerformanceCounter(&mCounterStartValue);
#elif __GNUC__
        gettimeofday(&t1, NULL);
#endif
    }

    void Stop()
    {
#if WINDOWS
        QueryPerformanceCounter(&mCounterEndValue);
#elif __GNUC__
        gettimeofday(&t2, NULL);
#endif
    }

    bool Supported() const
    {
        return mSupported;
    }

    float Seconds() const
    {
#if WINDOWS
        const float elapsedTicks(static_cast<float>(mCounterEndValue.QuadPart - mCounterStartValue.QuadPart));
        const float ticksPerSecond(static_cast<float>(mTicksPerSecond.QuadPart));
        return (elapsedTicks / ticksPerSecond);
#elif __GNUC__
        return (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) * 1e-6f;
#else
        return 0.0f;
#endif
    }

private:

    Timer(const Timer &other);
    Timer &operator=(const Timer &other);

    bool mSupported;

#if WINDOWS
    LARGE_INTEGER mTicksPerSecond;
    LARGE_INTEGER mCounterStartValue;
    LARGE_INTEGER mCounterEndValue;
#elif __GNUC__
    timeval t1, t2;
#endif

};

}   // namespace Utils

#endif // UTILS_TIMER_H
