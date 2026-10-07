#pragma once
#include <SDL3/SDL.h>
#include <memory>

namespace Sexy {
class Thread
{
public:
    SDL_ThreadID mThread = 0;
    bool mValid = false;
    static Thread Create(void (*theProc)(void*), void* theArg);
    int Join();
    bool IsValid();
    static Thread* Self();
private:
    struct Worker;
    std::shared_ptr<Worker> mWorker;
};
}
namespace TM {
class Thread
{
public:
    class Mutex
    {
    public:
        SDL_Mutex* mSection;
        Mutex();
        ~Mutex();
        Mutex(const Mutex&) = delete;
        Mutex& operator=(const Mutex&) = delete;
        void Acquire();
        void Release();
        void Lock();
        void Unlock();
    };
    SDL_Thread* mHandle = nullptr;
    SDL_ThreadID mId = 0;
    bool mRunning = false;
    Thread() = default;
    ~Thread();
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;
    static int Create(Thread* theThread, void (*theProc)(void*), void* theArg);
    static int Acquire(Thread* theThread);
    static int Release(Thread* theThread);
    static void Sleep(int theMillis);
    static SDL_ThreadID GetCurrentThreadId();
    int Join();
    int End();
    bool IsValid();
    static Thread* Self();
};
}
