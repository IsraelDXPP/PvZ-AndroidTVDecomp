// GameStats -- reconstructed from libGameMain.so.
//
// No base class and no vtable. It is one of the few objects this tree embeds BY
// VALUE elsewhere: Sexy\PlayerInfo.h:151 declares `GameStats mGameStats;` and
// annotates it as ordinals "477 ~ 514", i.e. 38 words == 0x98 bytes. The IDA type
// export (libGameMain115.h:2059) lists exactly the members below and they add up
// to the same 0x98:
//   4 (mHouseType) + 4 (unk) + 9*4 (mMiscStats) + 49 + 47 + 3*4 = 152 = 0x98.
//
// The previous 0xA8 came from the transcriber's "largest this-relative offset"
// heuristic, which had folded in an offset belonging to a neighbouring object;
// it disagreed with PlayerInfo's own member table.

#pragma once

#include "ReconTypes.h"
class SeedBank;

class GameStats
{
public:
    HouseType mHouseType;
    int mHouseLevel;
    int mMiscStats[9];
    char mFavoritePlants[49];
    char mFavoriteZombies[47];
    float mSurvivalCompletion;
    float mMiniGameCompletion;
    float mPuzzleCompletion;

    GameStats(const GameStats& theOther);
    void Reset();
    GameStats();
    int GetFavoritePlant();
    int GetFavoriteZombie();
    GameStats* TallyPlantsUsed(SeedBank *theSeedBank);
    void SetSurvivalCompletion(float theCompletion);
    void SetMinigameCompletion(float theCompletion);
    void SetPuzzleCompletion(float theCompletion);
    void ChangeMiscStat(GameStats_MiscStat theStat, int theDelta);
    void SetMiscStat(GameStats_MiscStat theStat, int theValue);
    int GetMiscStat(GameStats_MiscStat theStat);
    Sexy::String GetStatName(GameStats_MiscStat theStat);
};
