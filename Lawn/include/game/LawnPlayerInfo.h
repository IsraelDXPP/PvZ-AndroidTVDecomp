// LawnPlayerInfo -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class LawnPlayerInfo : public Sexy::DefaultPlayerInfo`.
// The binary's RTTI base for _ZTV14LawnPlayerInfo is N4Sexy17DefaultPlayerInfoE at
// offset 0, and the table confirms it: slots 2..4 (GetId, GetProfileId, GetName)
// and 6 (SyncSummary) still hold DefaultPlayerInfo implementations while 5, 7 and
// 8 hold this class's own.
//
// Real size 0x810, and it carries NO members of its own -- the whole object is the
// PlayerInfo layout: the authoritative header PvZ\Lawn\System\PlayerInfo.h ends
// with `// 516 ints` = 0x810 and puts mGameStats at +1908 with 0x98 bytes, i.e. up
// to +2060. The allocation site agrees: LawnApp::CreatePlayerInfo @0x2B2724
// requests 0x810 bytes.
//
// The transcribed object was 0x788 (1928 bytes), which is 132 bytes SHORT of the
// real object: LawnPlayerInfo.cpp:243 calls SyncBytes(this + 1908, 0x98) and writes
// up to +2060. Removing mObject in favour of the base class fixes that overrun.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x17d1e0  LawnPlayerInfo::~LawnPlayerInfo
//   [  1] 0x17d23c  LawnPlayerInfo::~LawnPlayerInfo
//   [  5] 0x17d4a4  LawnPlayerInfo::Reset
//   [  7] 0x17d8f8  LawnPlayerInfo::SyncDetails
//   [  8] 0x17d310  LawnPlayerInfo::DeleteUserFiles
// Slots 2, 3, 4 (DefaultPlayerInfo), 6 (DefaultPlayerInfo::SyncSummary), 9, 10
// (DefaultPlayerInfo) and 11..13 (PlayerInfo) are inherited unchanged.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNPLAYERINFO_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNPLAYERINFO_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "DataSync.h"
// Defines the base class Sexy::DefaultPlayerInfo. Including "PlayerInfo.h" alone is
// not enough -- that one only declares Sexy::PlayerInfo, which DefaultPlayerInfo
// derives from, so the base class stayed incomplete (C2504).
#include "DefaultPlayerInfo.h"
#include "GameStats.h"
#include "PlayerInfo.h"
#include "SexyAppBase.h"

// This file never names LawnApp, so the include was pure cycle
// (LawnApp.h <-> LawnPlayerInfo.h) -- and a costly one, because LawnApp.h
// includes this header: the pair sat on the long chains that run
// LawnApp -> AwardScreen -> LawnPlayerInfo -> GameStats -> SeedBank -> LawnApp.
class LawnApp;

class LawnPlayerInfo : public Sexy::DefaultPlayerInfo
{
public:
    virtual ~LawnPlayerInfo();
    // Return types follow the vtable in the authoritative header, which types all
    // three as void; LawnPlayerInfo.cpp still defines Reset as `void *` and
    // SyncDetails as `int`, so those two bodies need the phase-2 rewrite.
    virtual void Reset(); // slot 5
    virtual void SyncDetails(DataSync &theSync, int theVersion); // slot 7
    virtual void DeleteUserFiles(); // slot 8

    // No members of its own: the 0x810 bytes are the Sexy::PlayerInfo layout,
    // supplied by Sexy::DefaultPlayerInfo.
    // size 0x810, but not asserted here because DefaultPlayerInfo.h still holds an
    // opaque placeholder in this tree.

    bool GetFlag(PlayerFlags theFlag);
    int SetFlag(int theFlag, int theValue);
    int SyncGameData(DataSync &theSync);
    int SyncZenGardenData(DataSync &theSync);
    int SyncGameStats(DataSync &theSync);
    int AddCoins(int theAmount);
    int UpdateHouseData();
    int GetLevel();
    int SetLevel(int theLevel);
    int ResetChallengeRecord(GameMode theMode);
    LawnPlayerInfo(Sexy::ProfileMgr *theProfileMgr);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNPLAYERINFO_H
