// Original Android imports that are still named by reconstructed code.
// Host operations are implemented by SDL3; unused Android services remain absent.
#pragma once
#include "SexyTypes.h"
#include <SDL3/SDL.h>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>

struct AAssetManager;
struct AAsset;
struct AConfiguration;
inline AAssetManager* AAssetManager_fromJava(void*, void*) { return nullptr; }
inline AAsset* AAssetManager_open(AAssetManager*, const char*, int) { return nullptr; }
inline int AAsset_read(AAsset*, void*, size_t) { return -1; }
inline long AAsset_getLength(AAsset*) { return 0; }
inline long AAsset_seek(AAsset*, long, int) { return -1; }
inline void AAsset_close(AAsset*) {}

enum {
    ANDROID_LOG_UNKNOWN, ANDROID_LOG_DEFAULT, ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR, ANDROID_LOG_FATAL, ANDROID_LOG_SILENT
};
void AndroidLogPrint(int thePriority, const char* theTag, const char* theFormat, ...);
#define __android_log_print AndroidLogPrint
#define __android_log_vprint AndroidLogPrint
#define _android_log_print AndroidLogPrint
#define _android_log_vprint AndroidLogPrint
int TodLogMessage(int thePriority, const char* theFormat, ...);
#define TOD_ERROR(...) TodLogMessage(ANDROID_LOG_ERROR, __VA_ARGS__)
#define TOD_WARN(...) TodLogMessage(ANDROID_LOG_WARN, __VA_ARGS__)
#define TOD_INFO(...) TodLogMessage(ANDROID_LOG_INFO, __VA_ARGS__)
#define TOD_DEBUG(...) TodLogMessage(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define TOD_TRACE(...) TodLogMessage(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define TOD_ASSERT(condition, ...) do { if (!(condition)) TodLogMessage(ANDROID_LOG_FATAL, __VA_ARGS__); } while (0)
#define TOD_HALT(...) TodLogMessage(ANDROID_LOG_FATAL, __VA_ARGS__)

struct PlatformTimeval { int64_t tv_sec; int64_t tv_usec; };
inline int PlatformGetTimeOfDay(PlatformTimeval* time, void*)
{
    if (!time) return -1;
    SDL_Time now;
    if (!SDL_GetCurrentTime(&now)) return -1;
    time->tv_sec = now / 1000000000;
    time->tv_usec = (now % 1000000000) / 1000;
    return 0;
}
inline bool PlatformLocalTime(const time_t* seconds, std::tm* result)
{
    if (!seconds || !result) return false;
    *result = {};
    SDL_DateTime date{};
    if (!SDL_TimeToDateTime(static_cast<SDL_Time>(*seconds) * 1000000000, &date, true)) return false;
    result->tm_year = date.year - 1900;
    result->tm_mon = date.month - 1;
    result->tm_mday = date.day;
    result->tm_hour = date.hour;
    result->tm_min = date.minute;
    result->tm_sec = date.second;
    result->tm_wday = date.day_of_week;
    result->tm_yday = SDL_GetDayOfYear(date.year, date.month, date.day);
    result->tm_isdst = -1;
    return true;
}
inline long PlatformRandomLong() { return static_cast<long>((static_cast<unsigned int>(std::rand()) << 15) ^ std::rand()); }
int MSleep(int theMillis);
