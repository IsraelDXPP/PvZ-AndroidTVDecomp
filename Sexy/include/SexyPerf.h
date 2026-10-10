// SexyPerf is a profiler with no instance state.
//
// The records live in file-scope tables and in the class statics, which is
// why StartTiming and StopTiming take only a name. GetResults returns the
// report string by value.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SEXYPERF_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SEXYPERF_H

#include "ReconTypes.h"
#include "SexyString.h"

namespace Sexy
{

class SexyPerf
{
    static int StopTimingFlag;

public:
    static void StartTiming(const char *theName);
    static void StopTiming(const char *theName);

    int IsPerfOn();
    int EndPerf();
    String GetResults();
    int BeginPerf(bool theEnable);
};

} // namespace Sexy

using Sexy::SexyPerf;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SEXYPERF_H
