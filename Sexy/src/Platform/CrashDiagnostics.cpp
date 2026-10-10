#include "Platform/CrashDiagnostics.h"

#include <SDL3/SDL.h>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <limits>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <DbgHelp.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace
{
std::atomic<int> gGameMode(-1);
std::atomic<int> gLevel(-1);
std::atomic<int> gScene(-1);
std::atomic<int> gFrame(0);
std::atomic<int> gIntroTime(0);
std::atomic<int> gGraves(0);
std::atomic<int> gGravesEnabled(0);
std::atomic<const char *> gPhase("startup");
std::atomic_flag gReporting = ATOMIC_FLAG_INIT;
bool gInstalled = false;

#ifdef _WIN32
HANDLE gLogFile = INVALID_HANDLE_VALUE;
std::wstring gDumpPath;
#else
int gLogFile = -1;
#endif

#ifdef _WIN32
std::wstring Utf8ToWindowsPath(const std::string &thePath)
{
    if (thePath.empty() || thePath.size() > std::numeric_limits<int>::max())
        return {};
    const int aByteCount = static_cast<int>(thePath.size());
    const int aLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, thePath.data(), aByteCount, nullptr, 0);
    if (aLength <= 0)
        return {};
    std::wstring aResult(aLength, L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, thePath.data(), aByteCount, aResult.data(), aLength) !=
        aLength)
        return {};
    return aResult;
}
#endif

bool OpenDiagnosticsDirectory(const std::string &theDirectory)
{
    const std::string aLogPath = theDirectory + "LawnTV-diagnostics.log";
#ifdef _WIN32
    // Fatal handlers cannot call SDL's I/O/locking code. Unicode native opens
    // also preserve append-only writes and allow a debugger to read the log.
    const std::wstring aWindowsLogPath = Utf8ToWindowsPath(aLogPath);
    if (aWindowsLogPath.empty())
        return false;
    gLogFile = CreateFileW(aWindowsLogPath.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (gLogFile == INVALID_HANDLE_VALUE)
        return false;
    // DbgHelp requires a native file handle; cache UTF-16 during startup,
    // rather than converting strings after an exception has damaged the heap.
    gDumpPath = Utf8ToWindowsPath(theDirectory + "LawnTV-crash.dmp");
    return true;
#else
    // POSIX paths are UTF-8 bytes; retain an async-signal-safe write target.
    gLogFile = open(aLogPath.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0600);
    return gLogFile >= 0;
#endif
}

// Fixed storage and direct OS writes keep reporting independent of a damaged
// heap, the renderer and the application's logging locks.
struct ReportBuffer
{
    char mBytes[1024];
    size_t mLength = 0;

    void Append(const char *theText)
    {
        while (*theText != '\0' && mLength < sizeof(mBytes))
            mBytes[mLength++] = *theText++;
    }

    void AppendNumber(unsigned long long theNumber)
    {
        char aDigits[20];
        size_t aCount = 0;
        do
        {
            aDigits[aCount++] = '0' + static_cast<char>(theNumber % 10);
            theNumber /= 10;
        } while (theNumber != 0);
        while (aCount > 0 && mLength < sizeof(mBytes))
            mBytes[mLength++] = aDigits[--aCount];
    }

    void AppendField(const char *theName, int theValue)
    {
        Append(theName);
        if (theValue < 0)
        {
            Append("-");
            AppendNumber(static_cast<unsigned int>(-(theValue + 1)) + 1u);
        }
        else
            AppendNumber(static_cast<unsigned int>(theValue));
        Append(" ");
    }
};

void WriteReport(const char *theEvent, unsigned long long theCode)
{
    ReportBuffer aReport;
    aReport.Append(theEvent);
    aReport.Append(" code=");
    aReport.AppendNumber(theCode);
    aReport.Append(" phase=");
    aReport.Append(gPhase.load(std::memory_order_relaxed));
    aReport.Append(" ");
    aReport.AppendField("mode=", gGameMode.load(std::memory_order_relaxed));
    aReport.AppendField("level=", gLevel.load(std::memory_order_relaxed));
    aReport.AppendField("scene=", gScene.load(std::memory_order_relaxed));
    aReport.AppendField("frame=", gFrame.load(std::memory_order_relaxed));
    aReport.AppendField("intro=", gIntroTime.load(std::memory_order_relaxed));
    aReport.AppendField("graves=", gGraves.load(std::memory_order_relaxed));
    aReport.AppendField("graves_enabled=", gGravesEnabled.load(std::memory_order_relaxed));
    aReport.Append("\n");
#ifdef _WIN32
    if (gLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD aWritten = 0;
        WriteFile(gLogFile, aReport.mBytes, static_cast<DWORD>(aReport.mLength), &aWritten, nullptr);
        FlushFileBuffers(gLogFile);
    }
#else
    if (gLogFile >= 0)
        write(gLogFile, aReport.mBytes, aReport.mLength);
#endif
}

#ifdef _WIN32
void WriteDump(EXCEPTION_POINTERS *theException)
{
    if (gDumpPath.empty())
        return;
    HANDLE aDumpFile = CreateFileW(
        gDumpPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (aDumpFile == INVALID_HANDLE_VALUE)
        return;
    MINIDUMP_EXCEPTION_INFORMATION anInfo{};
    anInfo.ThreadId = GetCurrentThreadId();
    anInfo.ExceptionPointers = theException;
    anInfo.ClientPointers = FALSE;
    MiniDumpWriteDump(GetCurrentProcess(),
        GetCurrentProcessId(),
        aDumpFile,
        MiniDumpNormal,
        theException != nullptr ? &anInfo : nullptr,
        nullptr,
        nullptr);
    FlushFileBuffers(aDumpFile);
    CloseHandle(aDumpFile);
}

LONG WINAPI ReportWindowsException(EXCEPTION_POINTERS *theException)
{
    if (!gReporting.test_and_set())
    {
        WriteReport("fatal_exception", theException->ExceptionRecord->ExceptionCode);
        WriteDump(theException);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void ReportSignal(int theSignal)
{
    if (!gReporting.test_and_set())
    {
        WriteReport("fatal_signal", static_cast<unsigned int>(theSignal));
#ifdef _WIN32
        WriteDump(nullptr);
#endif
    }
    std::signal(theSignal, SIG_DFL);
#ifndef _WIN32
    sigset_t aSignalSet;
    sigemptyset(&aSignalSet);
    sigaddset(&aSignalSet, theSignal);
    sigprocmask(SIG_UNBLOCK, &aSignalSet, nullptr);
#endif
    std::raise(theSignal);
    std::_Exit(128 + theSignal);
}

void ReportTerminate()
{
    if (!gReporting.test_and_set())
    {
        WriteReport("terminate", 0);
#ifdef _WIN32
        WriteDump(nullptr);
#endif
    }
    std::abort();
}
}

void Sexy::InstallCrashDiagnostics()
{
    if (gInstalled)
        return;
    gInstalled = true;
    const char *aBasePath = SDL_GetBasePath();
    const std::string aDirectory = aBasePath != nullptr ? aBasePath : "";
    if (!OpenDiagnosticsDirectory(aDirectory))
    {
        char *aUserPath = SDL_GetPrefPath("LawnProjectTV", "LawnTV");
        if (aUserPath != nullptr)
        {
            OpenDiagnosticsDirectory(aUserPath);
            SDL_free(aUserPath);
        }
    }
#ifdef _WIN32
    SetUnhandledExceptionFilter(ReportWindowsException);
#endif
    std::set_terminate(ReportTerminate);
    std::signal(SIGABRT, ReportSignal);
    std::signal(SIGSEGV, ReportSignal);
    std::signal(SIGILL, ReportSignal);
    std::signal(SIGFPE, ReportSignal);
#ifdef SIGBUS
    std::signal(SIGBUS, ReportSignal);
#endif
    WriteReport("session_start", 0);
}

void Sexy::SetCrashContext(
    int theGameMode, int theLevel, int theScene, int theFrame, int theIntroTime, int theGraves, bool theGravesEnabled)
{
    gGameMode.store(theGameMode, std::memory_order_relaxed);
    gLevel.store(theLevel, std::memory_order_relaxed);
    gScene.store(theScene, std::memory_order_relaxed);
    gFrame.store(theFrame, std::memory_order_relaxed);
    gIntroTime.store(theIntroTime, std::memory_order_relaxed);
    gGraves.store(theGraves, std::memory_order_relaxed);
    gGravesEnabled.store(theGravesEnabled, std::memory_order_relaxed);
}

void Sexy::SetCrashPhase(const char *thePhase)
{
    // Callers pass string literals whose lifetime spans every crash handler.
    gPhase.store(thePhase, std::memory_order_relaxed);
}

void Sexy::RecordCrashContext(const char *theEvent)
{
    WriteReport(theEvent, 0);
}
