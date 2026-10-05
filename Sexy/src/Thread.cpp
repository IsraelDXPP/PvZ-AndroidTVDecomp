// TM::Thread starts a Win32 thread and waits on its handle.
// Sexy::Thread reports the calling thread's id.

#include "Thread.h"
#include "ReconHelpers.h"
#include <windows.h>
#include <process.h>

namespace TM {

Thread::Thread()
    : mHandle(nullptr), mId(0), mRunning(false)
{
}

Thread::~Thread()
{
    if (mRunning && mHandle != nullptr)
    {
        WaitForSingleObject((HANDLE)mHandle, INFINITE);
        CloseHandle((HANDLE)mHandle);
        mHandle = nullptr;
        mRunning = false;
    }
}

Thread& Thread::operator=(const Thread& theOther)
{
    if (this != &theOther)
    {
        mHandle = theOther.mHandle;
        mId = theOther.mId;
        mRunning = theOther.mRunning;
    }
    return *this;
}

namespace {

struct ThreadStart
{
    void (*mProc)(void*);
    void*  mArg;
};

unsigned __stdcall ThreadTrampoline(void* theParam)
{
    ThreadStart* aStart = static_cast<ThreadStart*>(theParam);
    void (*aProc)(void*) = aStart->mProc;
    void* aArg = aStart->mArg;
    delete aStart;

    if (aProc != nullptr)
        aProc(aArg);
    return 0;
}

} // namespace

int Thread::Create(Thread* theThread, void (*theProc)(void*), void* theArg)
{
    if (theThread == nullptr || theProc == nullptr)
        return 0;

    ThreadStart* aStart = new ThreadStart;
    aStart->mProc = theProc;
    aStart->mArg = theArg;

    unsigned int aId = 0;
    HANDLE aHandle = (HANDLE)_beginthreadex(nullptr, 0, ThreadTrampoline, aStart, 0, &aId);
    if (aHandle == nullptr)
    {
        delete aStart;
        return 0;
    }

    theThread->mHandle = aHandle;
    theThread->mId = aId;
    theThread->mRunning = true;
    return 1;
}

int Thread::Acquire(Thread* theThread)
{
    if (theThread == nullptr || !theThread->mRunning)
        return 0;
    WaitForSingleObject((HANDLE)theThread->mHandle, INFINITE);
    return 1;
}

int Thread::Release(Thread* theThread)
{
    if (theThread == nullptr || theThread->mHandle == nullptr)
        return 0;
    CloseHandle((HANDLE)theThread->mHandle);
    theThread->mHandle = nullptr;
    theThread->mRunning = false;
    return 1;
}

void Thread::Sleep(int theMillis)
{
    ::Sleep((DWORD)theMillis);
}

int Thread::Join()
{
    if (mHandle == nullptr)
        return 0;
    WaitForSingleObject((HANDLE)mHandle, INFINITE);
    mRunning = false;
    return 1;
}

int Thread::End()
{
    mHandle = nullptr;
    mRunning = false;
    return 1;
}

bool Thread::IsValid()
{
    return mHandle != nullptr;
}

Thread* Thread::Self()
{
    static Thread aSelf;
    aSelf.mHandle = (void*)(unsigned int)::GetCurrentThreadId();
    aSelf.mId = (unsigned int)aSelf.mHandle;
    aSelf.mRunning = true;
    return &aSelf;
}

int Thread::GetCurrentThreadId()
{
    return (int)::GetCurrentThreadId();
}

Thread::Mutex::Mutex()
    : mSection(nullptr)
{
    mSection = new CRITICAL_SECTION;
    ::InitializeCriticalSection(mSection);
}

Thread::Mutex::~Mutex()
{
    if (mSection != nullptr)
    {
        ::DeleteCriticalSection(mSection);
        delete mSection;
        mSection = nullptr;
    }
}

void Thread::Mutex::Acquire()
{
    if (mSection == nullptr)
        return;
    ::EnterCriticalSection(mSection);
}

void Thread::Mutex::Release()
{
    if (mSection == nullptr)
        return;
    ::LeaveCriticalSection(mSection);
}

void Thread::Mutex::Lock()
{
    Acquire();
}

void Thread::Mutex::Unlock()
{
    Release();
}

} // namespace TM

namespace Sexy {

Thread* Thread::Self()
{
    static thread_local Thread aSelf;
    aSelf.mThread = (unsigned int)::GetCurrentThreadId();
    aSelf.mValid = true;
    return &aSelf;
}

bool Thread::IsValid()
{
    return mValid;
}

int Thread::Join()
{
    return 0;
}

Thread Thread::Create(void (*theProc)(void*), void* theArg)
{
    (void)theProc;
    (void)theArg;
    return Thread();
}

} // namespace Sexy
