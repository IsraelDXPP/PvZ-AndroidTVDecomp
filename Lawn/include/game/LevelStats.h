// LevelStats -- reconstructed from libGameMain.so.
//
// Plain (non-polymorphic) class, no base class and no virtual function:
// the binary's only bodies are `LevelStats::LevelStats()` at 0x145708 and
// `LevelStats::Reset()` at 0x145700, and neither writes a vptr.
//
//     LevelStats::Reset()   { *this = 0; return this; }   // one word only
//     LevelStats::LevelStats() { LevelStats::Reset(this); }
//
// Size 0x8, taken from the allocation sites the transcription recorded.
// The transcribed bodies only ever touch +0x00, so +0x04 keeps its slot and is
// documented as unknown rather than invented.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_LEVELSTATS_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_LEVELSTATS_H

#include "ReconTypes.h"

class LevelStats
{
public:
    // Reset clears this word. Board stores the untriggered-mower count here
    // at the end of a level, matching the PC field mUnusedLawnMowers.
    int mUnusedLawnMowers;

    LevelStats *Reset();
    LevelStats();
};

// Layout check: two four-byte words.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_LEVELSTATS_H
