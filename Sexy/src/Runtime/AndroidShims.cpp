#include "AndroidShims.h"
#include <cstdarg>
#include <cstdio>

void AndroidLogPrint(int priority, const char* tag, const char* format, ...)
{
    if (priority < ANDROID_LOG_WARN) return;
    char message[1024];
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    SDL_LogPriority level = priority >= ANDROID_LOG_FATAL ? SDL_LOG_PRIORITY_CRITICAL
        : priority >= ANDROID_LOG_ERROR ? SDL_LOG_PRIORITY_ERROR : SDL_LOG_PRIORITY_WARN;
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, level, "%s: %s", tag ? tag : "game", message);
}
