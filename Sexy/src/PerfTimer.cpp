// PerfTimer.cpp -- the framework's millisecond timer.
//
// Transcribed from libGameMain.so. The target reads the clock with gettimeofday,
// which AndroidShims.h provides.

#include "PerfTimer.h"
#include "AndroidShims.h"

// ---------------------------------------------------------------------------
// Sexy::PerfTimer::PerfTimer()
// @00328FAC  24 bytes
//
// The constructor's four stores, all zero:
//   +0x00  the start time's tv_sec
//   +0x04  its tv_usec
//   +0x08  the measured duration
//   +0x10  the running flag, written as a single byte
//
// The padding at +0x14 is cleared here as well, so that no member is ever
// indeterminate; the binary leaves those bytes alone.
// ---------------------------------------------------------------------------
Sexy::PerfTimer::PerfTimer()
    : mStartSec(0)
    , mStartUsec(0)
    , mDuration(0.0)
    , mRunning(0)
    , mPad14(0)
    , mPad18(0)
    , mPad1C(0)
{
}

// ---------------------------------------------------------------------------
// Sexy::PerfTimer::CalcDuration()
// @00328FC4  80 bytes
//
// The elapsed time in milliseconds, from the two words Start stored. Both
// differences and the two scalings are integer arithmetic, exactly as the
// binary computes them, and only the two halves are added as doubles.
// ---------------------------------------------------------------------------
__time_t Sexy::PerfTimer::CalcDuration()
{
    struct timeval aNow;

    gettimeofday(&aNow, nullptr);
    mDuration =
        (double)((aNow.tv_usec - mStartUsec) / 1000)
        + (double)(1000 * (aNow.tv_sec - mStartSec));
    // The binary's r0 is the raw clock reading, not the duration it just
    // computed, so the return value is tv_sec.
    return aNow.tv_sec;
}

// ---------------------------------------------------------------------------
// Sexy::PerfTimer::Start()
// @00329014  10 bytes
//
// The start time is stored directly at the object's first two words. The
// running flag is set first and the return value is gettimeofday's own 0/-1.
// ---------------------------------------------------------------------------
int Sexy::PerfTimer::Start()
{
    struct timeval aNow;
    int aResult;

    mRunning = 1;
    aResult = gettimeofday(&aNow, nullptr);
    mStartSec = (int)aNow.tv_sec;
    mStartUsec = (int)aNow.tv_usec;
    return aResult;
}

// ---------------------------------------------------------------------------
// Sexy::PerfTimer::Stop()
// @00329020  18 bytes
//
// Only a running timer measures; either way the object is handed back.
// ---------------------------------------------------------------------------
Sexy::PerfTimer* Sexy::PerfTimer::Stop()
{
    if ( mRunning != 0 )
    {
        CalcDuration();
        mRunning = 0;
    }
    return this;
}

// ---------------------------------------------------------------------------
// Sexy::PerfTimer::GetDuration()
// @00329034  18 bytes
//
// A running timer is measured first. ARM returns the stored double in r0/r1.
// ---------------------------------------------------------------------------
double Sexy::PerfTimer::GetDuration()
{
    if ( mRunning != 0 )
        CalcDuration();
    return mDuration;
}
