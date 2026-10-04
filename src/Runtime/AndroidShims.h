// AndroidShims.h -- the Android/pthread surface the decompiled bodies mention.
//
// The target is an Android build, so the decompilation is full of calls that
// have no Windows equivalent: pthread mutexes, JNI, the asset manager, EGL.
// The instruction for this project is to replace those with synonyms or
// placeholders rather than reproduce them, so this header provides the
// smallest set of declarations that lets the transcribed code compile and run
// on Windows with the same observable behaviour.
//
// Each shim is a real implementation where a Windows equivalent exists (a
// mutex is a CRITICAL_SECTION) and a no-op where the concept does not apply
// (the asset manager, which this project replaces with a filesystem reader
// rooted at the executable's directory).
//
// Nothing here is on the game's logic path. The bodies that call into it are
// the ones the binary itself delegates to the platform.

#ifndef __ANDROIDSHIMS_H__
#define __ANDROIDSHIMS_H__

#include "SexyTypes.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
// winsock2.h MUST precede windows.h; windows.h otherwise pulls in the ancient
// winsock.h and the two collide. LogManager's socket/timeout code needs the
// BSD socket surface, which winsock2 provides under the same names.
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// <windows.h> reaches <minwindef.h>, which defines LOWORD/HIWORD/LOBYTE/HIBYTE
// as value expressions WITHOUT an include guard, so it overwrites whatever was
// there before. The transcribed bodies use those macros as assignment targets,
// so the lvalue forms have to be re-installed after windows.h has had its say.
// This is why IdaMacros.h carries no include guard of its own.
#include "IdaMacros.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
// _close lives here; the POSIX `close` shim below forwards to it.
#include <io.h>

// ---------------------------------------------------------------------------
// pthread, mapped onto the Win32 primitives that do the same job.
//
// The binary's CritSect wraps a pthread_mutex_t. On Windows the equivalent is a
// CRITICAL_SECTION, which is recursive like PTHREAD_MUTEX_RECURSIVE and is what
// the game's usage (Lock/Unlock around a container) requires.
// ---------------------------------------------------------------------------
typedef CRITICAL_SECTION  pthread_mutex_t;
typedef DWORD             pthread_t;

enum
{
    PTHREAD_MUTEX_INITIALIZER_ = 0,
    PTHREAD_MUTEX_RECURSIVE    = 1
};

inline int pthread_mutex_init(pthread_mutex_t* m, const void* /*attr*/)
{
    InitializeCriticalSection(m);
    return 0;
}

inline int pthread_mutex_destroy(pthread_mutex_t* m)
{
    DeleteCriticalSection(m);
    return 0;
}

inline int pthread_mutex_lock(pthread_mutex_t* m)
{
    EnterCriticalSection(m);
    return 0;
}

inline int pthread_mutex_unlock(pthread_mutex_t* m)
{
    LeaveCriticalSection(m);
    return 0;
}

inline int pthread_mutex_trylock(pthread_mutex_t* m)
{
    return TryEnterCriticalSection(m) ? 0 : 16 /* EBUSY */;
}

// pthread_cond_t: the game uses it only for the resource loader's queue, which
// this project serves synchronously. A Win32 event is the closest equivalent
// and keeps the wait/signal shape.
typedef CONDITION_VARIABLE pthread_cond_t;

inline int pthread_cond_init(pthread_cond_t* c, const void* /*attr*/)
{
    InitializeConditionVariable(c);
    return 0;
}

inline int pthread_cond_destroy(pthread_cond_t* /*c*/) { return 0; }

inline int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m)
{
    SleepConditionVariableCS(c, m, INFINITE);
    return 0;
}

inline int pthread_cond_signal(pthread_cond_t* c)
{
    WakeConditionVariable(c);
    return 0;
}

inline int pthread_cond_broadcast(pthread_cond_t* c)
{
    WakeAllConditionVariable(c);
    return 0;
}

// ---------------------------------------------------------------------------
// The Android asset manager and logging.
//
// Assets are read from the filesystem in this project (see
// Resource/ResourceManager.h), so the asset-manager calls are placeholders that
// report failure rather than silently succeeding -- a load that cannot happen
// should be visible, not silent.
// ---------------------------------------------------------------------------
struct AAssetManager;
struct AAsset;
struct AConfiguration;

inline AAssetManager* AAssetManager_fromJava(void* /*env*/, void* /*obj*/)
{
    return 0;
}

inline AAsset* AAssetManager_open(AAssetManager* /*mgr*/, const char* /*name*/,
                                  int /*mode*/)
{
    return 0;
}

inline int   AAsset_read(AAsset* /*a*/, void* /*buf*/, size_t /*n*/) { return -1; }
inline long  AAsset_getLength(AAsset* /*a*/) { return 0; }
inline long  AAsset_seek(AAsset* /*a*/, long /*off*/, int /*origin*/) { return -1; }
inline void  AAsset_close(AAsset* /*a*/) {}

// android/log.h
enum
{
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT
};

// Routes to OutputDebugStringA so the messages are visible under a debugger and
// in DebugView, which is the Windows equivalent of logcat.
void AndroidLogPrint(int thePriority, const char* theTag, const char* theFormat, ...);

#define __android_log_print AndroidLogPrint
#define __android_log_vprint AndroidLogPrint

// The transcription is inconsistent about the leading underscores: LogComposer
// calls `__android_log_print` while LogManager calls `_android_log_print`.
// Both spellings are the same logcat entry point, so both map here.
#define _android_log_print AndroidLogPrint
#define _android_log_vprint AndroidLogPrint


// ---------------------------------------------------------------------------
// ssize_t -- Android's libc exposes it, the MSVC CRT does not. The bodies
// declare locals of this type for read()/recv() results, so it is the signed
// pointer-sized integer POSIX defines (long on Win32, where long is 32-bit).
//
// Some third-party headers define it themselves, so the usual MSVC guard is
// honoured; if one of them already provided it, this is skipped.
// ---------------------------------------------------------------------------
#if !defined(_SSIZE_T_DEFINED) && !defined(ssize_t)
#define _SSIZE_T_DEFINED
typedef long ssize_t;
#endif

// ---------------------------------------------------------------------------
// The rest of the POSIX socket/signal surface winsock2 does not provide.
//
//   bsd_signal  -- glibc's "no restart" signal(); the plain CRT signal() is the
//                  same registration on Windows, where the SA_RESTART concept
//                  does not exist.
//   __fd_mask   -- glibc's fd_set word type; winsock's fd_set uses u_int.
// ---------------------------------------------------------------------------
typedef u_int __fd_mask;

#ifndef bsd_signal
#define bsd_signal(sig, handler) signal((sig), (handler))
#endif


// ---------------------------------------------------------------------------
// The binary's CritSect wraps a pthread mutex, but on Windows the section is
// heap allocated behind a one-word handle, so there is no in-place mutex to
// reach: `Sexy::CritSect` owns Lock/Unlock/TryLock directly (Sexy\CritSect.cpp)
// and every call site goes through those. The five `pthread_mutex_*(CritSect*)`
// overloads that used to live here forwarded to the class's first word and
// described the old in-place layout; nothing called them, so they are gone.
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// The framework's logging entry points.
//
// The Android build sends these to logcat; here they forward to the same shim
// the rest of the logging uses, so the messages are visible under a debugger
// and the call sites compile unchanged.
// ---------------------------------------------------------------------------
int TodLogMessage(int thePriority, const char* theFormat, ...);

#define TOD_ERROR(...)   TodLogMessage(ANDROID_LOG_ERROR, __VA_ARGS__)
#define TOD_WARN(...)    TodLogMessage(ANDROID_LOG_WARN, __VA_ARGS__)
#define TOD_INFO(...)    TodLogMessage(ANDROID_LOG_INFO, __VA_ARGS__)
#define TOD_DEBUG(...)   TodLogMessage(ANDROID_LOG_DEBUG, __VA_ARGS__)
#define TOD_TRACE(...)   TodLogMessage(ANDROID_LOG_VERBOSE, __VA_ARGS__)
#define TOD_ASSERT(cond, ...) \
    do { if (!(cond)) TodLogMessage(ANDROID_LOG_FATAL, __VA_ARGS__); } while (0)
#define TOD_HALT(...)    TodLogMessage(ANDROID_LOG_FATAL, __VA_ARGS__)


// ---------------------------------------------------------------------------
// The POSIX clock and time types the binary reads.
//
// The target is Android, so its bodies use gettimeofday and struct timeval.
// Windows has no such pair, so they are provided here with the same fields and
// the same meaning: `tv_sec` is seconds since the epoch and `tv_usec` the
// microseconds within the second. The implementation reads the system clock
// through GetSystemTimeAsFileTime, which is the same count.
//
// `__time_t` is the target's `time_t`; on this target it is a 32-bit seconds
// count, and the transcribed signatures name it directly.
// ---------------------------------------------------------------------------
typedef long __time_t;
typedef long __suseconds_t;

// winsock2.h (pulled in above for LogManager's socket code) already defines
// `struct timeval` with exactly this layout -- `long tv_sec; long tv_usec;` --
// so defining it again is a C2011 redefinition. The STRUCT therefore yields to
// winsock; on a non-winsock platform the branch below is the fallback.
//
// The FUNCTION must NOT carry the same guard: this header includes <winsock2.h>
// unconditionally at the top, so `!defined(_WINSOCK2API_)` is always false and
// a guarded gettimeofday would simply never be compiled. winsock2.h provides
// `struct timeval` but NOT `gettimeofday`, so the bodies that call it
// (LogManager, SexyPerf, PerfTimer) would all report C3861 without the
// definition below.
#if !defined(_WINSOCK2API_) && !defined(_WINSOCKAPI_) && !defined(_TIMEVAL_DEFINED)
#define _TIMEVAL_DEFINED
struct timeval
{
    __time_t        tv_sec;
    __suseconds_t   tv_usec;
};
#endif

inline int gettimeofday(struct timeval* theTime, void* /*theTimezone*/)
{
    if (theTime == nullptr)
        return -1;

    // 100-nanosecond intervals since 1601-01-01, which is the Windows epoch.
    FILETIME aFileTime;
    GetSystemTimeAsFileTime(&aFileTime);

    unsigned __int64 aCount =
        ((unsigned __int64)aFileTime.dwHighDateTime << 32) |
        (unsigned __int64)aFileTime.dwLowDateTime;

    // To the Unix epoch, then to seconds and microseconds.
    aCount -= 116444736000000000ULL;

    theTime->tv_sec  = (__time_t)(aCount / 10000000ULL);
    theTime->tv_usec = (__suseconds_t)((aCount % 10000000ULL) / 10ULL);
    return 0;
}

// winsock2.h #defines TRY_AGAIN (WSATRY_AGAIN), which collides with the
// framework's own `extern _DWORD TRY_AGAIN;` string constant -- the macro turns
// that declaration into `extern _DWORD (WSABASEERR+1002);` and it will not
// parse. Undefining it here lets the identifier mean the constant again; the
// socket code in this project never uses the errno spelling.
#ifdef TRY_AGAIN
#undef TRY_AGAIN
#endif

// The PLT-prefixed spelling the decompiler emitted for the same call.
#define j_gettimeofday gettimeofday

// ---------------------------------------------------------------------------
// The other libc entry points the bodies call by name.
//
// These are Android's own libc, which the binary was linked against, so the
// decompiler printed them verbatim. MSVC has no equivalent, and the three
// below have no header declaration anywhere in this project.
// ---------------------------------------------------------------------------

// lrand48 is the 48-bit linear congruential generator Android's libc provides.
// The bodies use it as a plain random source, so rand() is the same contract.
inline long lrand48(void)
{
    return (long)(((unsigned int)rand() << 15) ^ (unsigned int)rand());
}

inline long mrand48(void)
{
    return lrand48();
}

// ---------------------------------------------------------------------------
// The POSIX socket/timeout surface logcat's sender uses. LogManager had to
// carry all three locally; they belong here.
// ---------------------------------------------------------------------------

// glibc spells the fd_set bit array `__fds_bits`; winsock's fd_set is
// `{ u_int fd_count; SOCKET fd_array[64]; }`. The two are NOT the same layout --
// glibc is a flat 1024-bit mask, winsock counts entries -- so this alias only
// keeps the transcribed indexing readable. Any code that actually has to SELECT
// on the set has to translate the masks instead.
// TODO(recon): translate rather than alias if the socket path is ever enabled.
#define __fds_bits fd_array

// ioctl(FIONBIO) is winsock's ioctlsocket, and the request constant differs
// (glibc 0x5421 vs winsock 0x8004667E), so the command is translated.
#ifndef FIONBIO
#define FIONBIO 0x5421
#endif

inline int ioctl(int theSocket, unsigned long theRequest, void* theArg)
{
    // Only the non-blocking flag is ever requested by this code.
    if (theRequest == FIONBIO)
        return ioctlsocket((SOCKET)theSocket, (long)0x8004667E, (u_long*)theArg);
    return -1;
}

// libc close(); the CRT calls it _close. It is inline rather than a macro so
// `::close(fd)` resolves to it even where a member named `close` is in scope
// (LogManager has one).
inline int close(int theFile)
{
    return _close(theFile);
}

// localtime_r is the reentrant localtime. Windows spells it localtime_s, which
// takes the arguments the other way round.
inline struct tm* localtime_r(const time_t* theTime, struct tm* theResult)
{
    if (theTime == nullptr || theResult == nullptr)
        return nullptr;
    return (localtime_s(theResult, theTime) == 0) ? theResult : nullptr;
}

// setenv(name, value, overwrite) -- POSIX. The CRT has _putenv_s, which always
// overwrites; a zero `overwrite` with the variable already set is the one case
// the two disagree on, so it is checked explicitly.
inline int setenv(const char* theName, const char* theValue, int theOverwrite)
{
    if (theName == nullptr || theValue == nullptr)
        return -1;
    if (theOverwrite == 0 && ::getenv(theName) != nullptr)
        return 0;
    return _putenv_s(theName, theValue);
}

// The framework's own millisecond sleep, which the bodies call by name.
int MSleep(int theMillis);

// The PLT-prefixed spellings of the file calls the decompiler emitted.
#define j_fwrite fwrite
#define j_fread  fread
#define j_fopen  fopen
#define j_fclose fclose
#define j_fseek  fseek

#endif // __ANDROIDSHIMS_H__
