// SexyPerf times named scopes. BeginPerf clears the table and turns sampling
// on. StartTiming and StopTiming nest by name. EndPerf closes the run and
// GetResults formats the totals. Sampling is a no-op until BeginPerf.

#include "SexyPerf.h"
#include "AndroidShims.h"

#include <cstdio>
#include <map>
#include <string>

namespace {

struct PerfStat
{
    int mCalls;
    int mDepth;
    long long mOpenStamp;
    double mTotalMs;
    double mLongestMs;

    PerfStat()
        : mCalls(0)
        , mDepth(0)
        , mOpenStamp(0)
        , mTotalMs(0.0)
        , mLongestMs(0.0)
    {
    }
};

bool gPerfOn = false;
std::map<std::string, PerfStat> gStats;
int gOpenCount = 0;
long long gFrameStart = 0;
double gDurationMs = 0.0;

long long NowMs()
{
    PlatformTimeval stamp;
    PlatformGetTimeOfDay(&stamp, nullptr);
    return (long long)stamp.tv_sec * 1000 + stamp.tv_usec / 1000;
}

} // namespace

int Sexy::SexyPerf::StopTimingFlag = 0;

int Sexy::SexyPerf::IsPerfOn()
{
    return gPerfOn ? 1 : 0;
}

int Sexy::SexyPerf::BeginPerf(bool theEnable)
{
    (void)theEnable;
    gStats.clear();
    gOpenCount = 0;
    gDurationMs = 0.0;
    gPerfOn = true;
    gFrameStart = NowMs();
    return (int)gFrameStart;
}

void Sexy::SexyPerf::StartTiming(const char* theName)
{
    if (!gPerfOn || theName == nullptr)
        return;

    PerfStat& stat = gStats[theName];
    if (stat.mDepth == 0)
        stat.mOpenStamp = NowMs();
    ++stat.mDepth;
    ++stat.mCalls;
    ++gOpenCount;
}

void Sexy::SexyPerf::StopTiming(const char* theName)
{
    if (!gPerfOn || theName == nullptr)
        return;

    std::map<std::string, PerfStat>::iterator found = gStats.find(theName);
    if (found != gStats.end() && found->second.mDepth > 0)
    {
        --found->second.mDepth;
        if (found->second.mDepth == 0)
        {
            const double elapsed = (double)(NowMs() - found->second.mOpenStamp);
            found->second.mTotalMs += elapsed;
            if (elapsed > found->second.mLongestMs)
                found->second.mLongestMs = elapsed;
        }
    }
    if (gOpenCount > 0)
        --gOpenCount;
}

int Sexy::SexyPerf::EndPerf()
{
    gDurationMs = (double)(NowMs() - gFrameStart);
    gPerfOn = false;
    gOpenCount = 0;
    return 0;
}

Sexy::String Sexy::SexyPerf::GetResults()
{
    String report;
    char line[512];
    std::sprintf(line, "Total Time: %.2f\n", gDurationMs);
    report += line;

    for (std::map<std::string, PerfStat>::iterator it = gStats.begin(); it != gStats.end(); ++it)
    {
        const PerfStat& stat = it->second;
        const double average = stat.mCalls > 0 ? stat.mTotalMs / (double)stat.mCalls : 0.0;
        const double percent = gDurationMs > 0.0 ? stat.mTotalMs / gDurationMs * 100.0 : 0.0;
        std::sprintf(line,
                      "%s (%d calls, %%%.2f time): %.2f (%.2f avg, %.2f longest)\n",
                      it->first.c_str(), stat.mCalls, percent, stat.mTotalMs, average, stat.mLongestMs);
        report += line;
    }
    return report;
}
