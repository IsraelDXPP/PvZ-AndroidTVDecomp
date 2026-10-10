// TodHesitationBracket -- reconstructed from libGameMain.so.
//
// The bodies below are the binary's own code, rewritten in ordinary C++: the
// `this + 256` accesses are mBracketStartTime, and the hand-written
// `((TodHesitationBracket *)this)->EndBracket()` cast in the destructor is a
// plain member call.

#include "TodHesitationBracket.h"

#include "PerfTimer.h"
#include "ReconData.h"

#include <cstdarg>
#include <cstring>

static int HesitationMilliseconds()
{
    return (int)gHesitationTrace.mTimer.GetDuration();
}

// ==========================================================================
// TodHesitationBracket::TodHesitationBracket(char const*,...)
// @00207990  88 bytes
// ==========================================================================
TodHesitationBracket::TodHesitationBracket(const char *theFormat, ...)
{
    va_list aArgs;
    va_start(aArgs, theFormat);

    if (gHesitationTrace.mEnabled == false)
    {
        va_end(aArgs);
        return;
    }

    TodVsnprintf(mMessage, 256, theFormat, aArgs);
    va_end(aArgs);
    TodHesitationTrace("Enter %s", mMessage);
    gHesitationTrace.mDepth += 2;

    // The shared timer starts with the process. The bracket records the elapsed
    // time so EndBracket can subtract it again.
    mBracketStartTime = HesitationMilliseconds();
}

// ==========================================================================
// TodHesitationBracket::EndBracket()
// @002079F4  78 bytes
// ==========================================================================
int TodHesitationBracket::EndBracket()
{
    if (gHesitationTrace.mEnabled == false || mBracketStartTime == -1)
        return 0;

    gHesitationTrace.mDepth -= 2;
    int aDuration = HesitationMilliseconds();
    int aResult = TodHesitationTrace("Exit %s (%d ms)", mMessage, aDuration - mBracketStartTime);
    mBracketStartTime = -1;
    return aResult;
}

// ==========================================================================
// TodHesitationBracket::~TodHesitationBracket()
// @00207A50  12 bytes
// ==========================================================================
TodHesitationBracket::~TodHesitationBracket()
{
    EndBracket();
}
