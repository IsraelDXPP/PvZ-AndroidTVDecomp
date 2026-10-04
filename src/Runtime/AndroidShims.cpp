// AndroidShims.cpp -- the parts of the Android surface that need a definition.
//
// See AndroidShims.h for what these are and why they exist. Only the logging
// shim needs out-of-line code; everything else is inline in the header.

#include "AndroidShims.h"

#include <cstdarg>
#include <cstdio>
#include <windows.h>

void AndroidLogPrint(int thePriority, const char* theTag, const char* theFormat, ...)
{
    // Only warnings and above are forwarded. The binary logs at VERBOSE from
    // several hot paths, and mirroring that to the debugger would drown out
    // anything useful.
    if (thePriority < ANDROID_LOG_WARN)
        return;

    static const char* const kNames[] = {
        "?", "?", "V", "D", "I", "W", "E", "F", "S"
    };
    const char* aName = (thePriority >= 0 && thePriority <= 8)
                            ? kNames[thePriority] : "?";

    char aBody[1024];
    va_list aArgs;
    va_start(aArgs, theFormat);
    _vsnprintf_s(aBody, sizeof(aBody), _TRUNCATE, theFormat, aArgs);
    va_end(aArgs);

    char aLine[1200];
    _snprintf_s(aLine, sizeof(aLine), _TRUNCATE, "[%s] %s: %s\n",
                aName, theTag ? theTag : "?", aBody);
    OutputDebugStringA(aLine);
}
