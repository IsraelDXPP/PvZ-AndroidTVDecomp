#include "Thread.h"

namespace {
struct ThreadStart { void (*mProc)(void*); void* mArg; };
int SDLCALL RunThread(void* argument)
{
    std::unique_ptr<ThreadStart> start(static_cast<ThreadStart*>(argument));
    start->mProc(start->mArg);
    return 0;
}
SDL_Thread* StartWorker(void (*procedure)(void*), void* argument)
{
    if (!procedure) return nullptr;
    auto start = std::make_unique<ThreadStart>();
    start->mProc = procedure; start->mArg = argument;
    SDL_Thread* thread = SDL_CreateThread(RunThread, "framework-worker", start.get());
    if (thread) start.release();
    return thread;
}
}
namespace TM {
Thread::~Thread()
{
    if (mHandle && mId == SDL_GetCurrentThreadID()) SDL_DetachThread(mHandle);
    else Join();
}
int Thread::Create(Thread* thread, void (*procedure)(void*), void* argument)
{
    if (!thread || !procedure) return 0;
    thread->Join();
    thread->mHandle = StartWorker(procedure, argument);
    thread->mRunning = thread->mHandle != nullptr;
    thread->mId = thread->mHandle ? SDL_GetThreadID(thread->mHandle) : 0;
    return thread->mRunning ? 1 : 0;
}
int Thread::Acquire(Thread* thread) { return thread ? thread->Join() : 0; }
int Thread::Release(Thread* thread) { return thread ? thread->Join() : 0; }
void Thread::Sleep(int milliseconds) { if (milliseconds > 0) SDL_Delay(static_cast<Uint32>(milliseconds)); }
int Thread::Join()
{
    if (!mHandle) return 0;
    if (mId == SDL_GetCurrentThreadID()) return 0;
    SDL_WaitThread(mHandle, nullptr);
    mHandle = nullptr; mRunning = false;
    return 1;
}
int Thread::End() { return Join(); }
bool Thread::IsValid() { return mRunning; }
Thread* Thread::Self()
{
    static thread_local Thread self;
    self.mId = SDL_GetCurrentThreadID(); self.mRunning = true;
    return &self;
}
SDL_ThreadID Thread::GetCurrentThreadId() { return SDL_GetCurrentThreadID(); }
Thread::Mutex::Mutex() : mSection(SDL_CreateMutex()) {}
Thread::Mutex::~Mutex() { SDL_DestroyMutex(mSection); }
void Thread::Mutex::Acquire() { SDL_LockMutex(mSection); }
void Thread::Mutex::Release() { SDL_UnlockMutex(mSection); }
void Thread::Mutex::Lock() { Acquire(); }
void Thread::Mutex::Unlock() { Release(); }
}
namespace Sexy {
struct Thread::Worker
{
    SDL_Thread* mHandle;
    SDL_Mutex* mJoinMutex = SDL_CreateMutex();
    explicit Worker(SDL_Thread* handle) : mHandle(handle) {}
    ~Worker()
    {
        if (mHandle && SDL_GetThreadID(mHandle) == SDL_GetCurrentThreadID()) SDL_DetachThread(mHandle);
        else Join();
        SDL_DestroyMutex(mJoinMutex);
    }
    bool Join()
    {
        SDL_LockMutex(mJoinMutex);
        bool joined = mHandle != nullptr;
        if (mHandle) { SDL_WaitThread(mHandle, nullptr); mHandle = nullptr; }
        SDL_UnlockMutex(mJoinMutex);
        return joined;
    }
    bool IsValid()
    {
        SDL_LockMutex(mJoinMutex);
        bool valid = mHandle != nullptr;
        SDL_UnlockMutex(mJoinMutex);
        return valid;
    }
};
Thread* Thread::Self()
{
    static thread_local Thread self;
    self.mThread = SDL_GetCurrentThreadID(); self.mValid = true;
    return &self;
}
bool Thread::IsValid() { return mWorker ? mWorker->IsValid() : mValid; }
int Thread::Join()
{
    if (!mWorker || mThread == SDL_GetCurrentThreadID()) return 0;
    bool joined = mWorker->Join(); mValid = false; return joined ? 1 : 0;
}
Thread Thread::Create(void (*procedure)(void*), void* argument)
{
    Thread result;
    SDL_Thread* handle = StartWorker(procedure, argument);
    if (handle)
    {
        result.mThread = SDL_GetThreadID(handle); result.mValid = true;
        result.mWorker = std::make_shared<Worker>(handle);
    }
    return result;
}
}
