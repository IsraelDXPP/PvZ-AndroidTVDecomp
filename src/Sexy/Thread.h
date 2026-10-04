// Thread.h holds two thread wrappers.
//
// Sexy::Thread is the session thread: a thread id and a validity flag.
// TM::Thread is the worker wrapper: a Win32 handle, an id, a running flag,
// and a nested mutex that owns one critical section.

#pragma once

#include "SexyTypes.h"
// windows.h pulls the old winsock.h unless the newer socket header comes first.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>

namespace Sexy {

class Thread
{
public:
    unsigned int mThread;       // +0x00
    bool mValid;                // +0x04
    bool mPadding[3];

    Thread() : mThread(0), mValid(false)
    {
        mPadding[0] = false;
        mPadding[1] = false;
        mPadding[2] = false;
    }

    ~Thread() {}

    static Thread Create(void (*theProc)(void*), void* theArg);
    int  Join();
    bool IsValid();
    static Thread* Self();
};

} // namespace Sexy

namespace TM {

class Thread
{
public:
    class Mutex
    {
    public:
        CRITICAL_SECTION* mSection;

        Mutex();
        ~Mutex();

        void Acquire();
        void Release();
        void Lock();
        void Unlock();
    };

    void* mHandle;
    unsigned int mId;
    bool mRunning;

    Thread();
    ~Thread();

    Thread& operator=(const Thread& theOther);

    static int Create(Thread* theThread, void (*theProc)(void*), void* theArg);
    static int Acquire(Thread* theThread);
    static int Release(Thread* theThread);
    static void Sleep(int theMillis);
    static int  GetCurrentThreadId();

    int  Join();
    int  End();
    bool IsValid();
    static Thread* Self();
};

} // namespace TM
