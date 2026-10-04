// TodHesitationBracket -- reconstructed from libGameMain.so.
//
// Plain (non-polymorphic) scope guard: no vptr and no base class. There is no
// `_ZTV20TodHesitationBracketE` in libGameMain115_rtti.c, and the constructor
// @0x207990 writes no vtable pointer. The destructor @0x207A50 is `_ZN20Tod
// HesitationBracketD2Ev`, a plain member that tail-calls EndBracket.
//
// SIZE 0x104, NOT 0x110.
//   Two independent sources agree:
//   * the stack frame of a caller. TrailLoadADef @0x201040 is
//         SUB SP, SP, #0x110
//         ADD.W R8, SP, #0x110+var_104   ; R8 = SP + 0xC, the bracket
//         ADD   R5, SP, #0x110+var_108   ; R5 = SP + 0x8, a std::string
//     so the bracket occupies SP + 0xC through the end of the 0x110-byte frame,
//     i.e. 0x110 - 0xC == 0x104 bytes, with nothing between it and the frame
//     pointer;
//   * the class's own bodies. The constructor formats into a 256-byte buffer and
//     then stores the start time:
//         TodVsnprintf(this, 256, a2, args);        ; a 256-byte buffer at +0x00
//         *((_DWORD *)this + 64) = GetDuration();   ; word 64, byte +0x100
//     and EndBracket tests that word against -1 and writes -1 back into it
//     (`*(_DWORD *)(this + 256)`). A four-byte field at +0x100 closes the object
//     at +0x104.
//   The transcriber's 0x110 was the 0x104 extent rounded up to a 16-byte
//   boundary.
//
// The PC reference declares the same two members
// (LawnProject\src\TodLib\TodDebug.h:8):
//     class TodHesitationBracket { char mMessage[256]; int mBracketStartTime; ... };
// and its EndBracket prints `Exit %s (%d ms)` with `duration - mBracketStartTime`,
// which is exactly the subtraction this binary's EndBracket performs on the word
// at +0x100, so that word is the start time and not something else.
//
// The object is always a local, never a heap allocation and never an array
// element: the constructor's nine call sites all pass a stack address.

#pragma once

#include "ReconTypes.h"
// The method signatures name these, so the includes stay as the transcriber left them.
#include "PerfTimer.h"

class TodHesitationBracket
{
public:
    char mMessage[256];     // +0x00, the TodVsnprintf destination
    int mBracketStartTime;  // +0x100, -1 while no bracket is open
    // size 0x104

    TodHesitationBracket(const char *theFormat, ...);
    int EndBracket();
    ~TodHesitationBracket();
};

// Layout check: the 256-byte message buffer plus the start time.
