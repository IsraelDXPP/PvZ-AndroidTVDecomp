// LawnSession -- reconstructed from libGameMain.so.
//
// Polymorphic class with no base: RTTI "11LawnSession" lists no base class and
// the vtable at 0x6862d8 has only the two destructor slots.
//
//   [  0] 0x1ebcb4  LawnSession::~LawnSession   (D1)
//   [  1] 0x1ebcd8  LawnSession::~LawnSession   (D0)
// MSVC x86 emits ONE slot for a virtual destructor where Itanium emits two.
//
// Layout, from _recon/ida/LawnSession.cpp:
//   ctor        @0x1EBD2C  +5 = 1, +4 = 0, +8 = 0, then Thread::Thread(this + 12)
//   dtor        @0x1EBCB4  destroys the Sexy::Thread at this + 12
//   HandleTask  @0x1EBCEC  reads +8 as the pending task, keeps +5 as the flag
//   StartTask   @0x1EBD88  refuses when +5 == 0 or +8 != 0
//   Init        @0x1EBD58  builds an 8-byte temporary Thread on the stack, so
//                          sizeof(Sexy::Thread) is 8 and the object ends at 0x14.
//
// The member is `Sexy::Thread`, which Sexy\Thread.h now declares next to
// TM::Thread: the transcription had merged the two classes, and TM::Thread is 12
// bytes (two words and a bool) where Sexy::Thread is 8. That one word alone made
// sizeof(LawnSession) 0x18 instead of 0x14 and pushed every LawnApp member after
// mSession four bytes late.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNSESSION_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNSESSION_H

#include "ReconTypes.h"
// Needed for the complete Sexy::Thread type of the member below.
#include "Thread.h"

class LawnSessionTask;

class LawnSession
{
public:
    virtual ~LawnSession();

    bool mShutdown; // +0x04  (the SessionThreadProc loop tests it)
    bool mTaskDone; // +0x05  (StartTask / HandleTask / IsTaskDone)
    // +0x06 ~ +0x07 padding to the next word
    LawnSessionTask *mTask; // +0x08
    Sexy::Thread mThread; // +0x0C  (the destructor tears down a Thread at this+12)
    // Size 0x14.

    LawnSession();
    int Init();
    int Shutdown();
    int StartTask(LawnSessionTask *theTask);
    int IsTaskDone();
    bool HandleTask();
    LawnSession *SessionThreadProc(void *theUnused);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNSESSION_H
