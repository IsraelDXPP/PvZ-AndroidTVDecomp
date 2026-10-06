// PerfTimer records a start time and a duration. There is no vtable.
// Start stores the clock and raises the running flag. Stop clears the flag
// and stores the measured duration. GetDuration returns that duration, or
// the time since Start while the timer is still running.
//
// The three trailing words keep the object the size the double's alignment
// already forces. Nothing reads them.

#pragma once

#include "AndroidShims.h"

namespace Sexy {

class PerfTimer
{
public:
    int mStartSec;
    int mStartUsec;
    double mDuration;
    int mRunning;
    int mPad14;
    int mPad18;
    int mPad1C;

    PerfTimer();
    __time_t CalcDuration();
    int Start();
    PerfTimer * Stop();
    double GetDuration();
};

} // namespace Sexy

using Sexy::PerfTimer;
