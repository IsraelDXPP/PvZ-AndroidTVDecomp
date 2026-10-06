// LawnPlayerInfo -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members come from the class's own header and its bases.

#include "LawnPlayerInfo.h"
#include "DataSync.h"
#include "DefaultPlayerInfo.h"
#include "GameStats.h"
#include "LawnApp.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"
#include <cstring>

// ==========================================================================
// LawnPlayerInfo::DeleteUserFiles(void)
// @0017D310  126 bytes
// ==========================================================================
void LawnPlayerInfo::DeleteUserFiles()
{
    // The two indirect calls are this object's own vtable at byte 12 and byte 8,
    // i.e. the inherited GetProfileId and GetId. GetSavedGameName hands its
    // Sexy::String back through the sret register, so the destination comes
    // first, as in the other variadic framework calls.
    for ( int i = 0; i != 92; ++i )
    {
        Sexy::String aSavedGameName;
        GetSavedGameName(&aSavedGameName, (GameMode)i, GetProfileId(), GetId());
        ((SexyAppBase *)gSexyAppBase)->EraseFile(aSavedGameName);
    }
}

// ==========================================================================
// LawnPlayerInfo::Reset(void)
// @0017D4A4  224 bytes
// ==========================================================================
void LawnPlayerInfo::Reset()
{
    mLevel = 0;
    mCoins = 0;

    for ( int i = 0; i != 100; ++i )
        mChallengeRecords[i] = 0;

    // The 320 bytes from mPurchases on are the original `int mPurchases[80]`
    // array; this header splits its tail into the named VS and zombatar flags,
    // so the span is cleared through its first element, as the binary does.
    int *aPurchases = mPurchases;
    for ( int i = 0; i != 80; ++i )
        aPurchases[i] = 0;
    mZenWords[0] = 0;
    mZenWords[1] = 0;
    mLastStinkyChocolateTime = 0;
    mStinkyPosX = 0;
    mStinkyPosY = 0;
    memset(mPottedPlants, 0, sizeof(mPottedPlants));
    mNumPottedPlants = 0;
    mPacketUpgradeDeclines = 0;
    mGameStats.Reset();
    mFlags = 0;
    mSoundVolume = 0.5;
    mMusicVolume = 0.5;
    // The dump clears the two finished-adventure words as one eight-byte store.
    mProgress[2] = 0;
    mProgress[3] = 0;
    for ( int i = 0; i != 6; ++i )
        mHelpTextSeen[i] = false;
    // The dump clears the first progress word and, as one four-byte store,
    // the two flag bytes plus the two bools that follow them.
    mProgress[0] = 0;
    memset(mProgressFlags, 0, sizeof(mProgressFlags));
    mVSPlantWon = false;
    mHasFavoritePlant = false;
    mWinStreak = 0;
    mEndlessHighScore = false;
    mExtraAchievement = false;
    mPassedShopSeedTutorial = false;
    memset(mMailMessageRead, 0, sizeof(mMailMessageRead));
    memset(mMailMessageSeen, 0, sizeof(mMailMessageSeen));
}

// ==========================================================================
// LawnPlayerInfo::LawnPlayerInfo(Sexy::ProfileMgr *)
// @0017D70C  44 bytes
// ==========================================================================
LawnPlayerInfo::LawnPlayerInfo(Sexy::ProfileMgr *theProfileMgr)
    : Sexy::DefaultPlayerInfo(theProfileMgr)
{
    mDetailFileCount = 3;
    Reset();
}

// ==========================================================================
// LawnPlayerInfo::GetFlag(PlayerFlags)
// @0017D74C  12 bytes
// ==========================================================================
bool LawnPlayerInfo::GetFlag(PlayerFlags theFlag)
{
    return (theFlag & mFlags) != 0;
}

// ==========================================================================
// LawnPlayerInfo::SetFlag(PlayerFlags,bool)
// @0017D758  18 bytes
// ==========================================================================
int LawnPlayerInfo::SetFlag(int theFlag, int theValue)
{
    if (theValue != 0)
        mFlags |= theFlag;
    else
        mFlags &= ~theFlag;
    return mFlags;
}

// ==========================================================================
// LawnPlayerInfo::SyncGameData(DataSync &)
// @0017D76C  310 bytes
// ==========================================================================
int LawnPlayerInfo::SyncGameData(DataSync &theSync)
{
    // A load resets the record first.
    if (theSync.mReader != nullptr)
        Reset();

    int aVersion = gUserVersion;
    theSync.SyncLong(aVersion);
    theSync.mVersion = aVersion;
    if (aVersion != (int)gUserVersion)
        OutputDebug("Profile version (%d) doesn't match game version (%d)\n", aVersion, (int)gUserVersion);

    theSync.SyncLong(mLevel);
    theSync.SyncLong(mCoins);
    theSync.SyncLong(mFlags);
    for (int i = 0; i != 100; ++i)
        theSync.SyncLong(mChallengeRecords[i]);

    // The 320 bytes from mPurchases on, exactly as Reset clears them.
    for (int i = 0; i != 80; ++i)
        theSync.SyncLong(mPurchases[i]);

    theSync.SyncLong(mZenWords[0]);
    theSync.SyncLong(mZenWords[1]);
    theSync.SyncLong(mLastStinkyChocolateTime);
    theSync.SyncLong(mStinkyPosX);
    theSync.SyncLong(mStinkyPosY);
    theSync.SyncLong(mPacketUpgradeDeclines);
    theSync.SyncLong(mNumPottedPlants);
    theSync.SyncDouble(mMusicVolume);
    theSync.SyncDouble(mSoundVolume);
    theSync.SyncBool(mPassedShopSeedTutorial);
    for (int i = 0; i != 6; ++i)
        theSync.SyncBool(mHelpTextSeen[i]);
    if (gUserVersion > gUserVersionTilJune2010)
    {
        theSync.SyncBytes(mMailMessageRead, sizeof(mMailMessageRead));
        theSync.SyncBytes(mMailMessageSeen, sizeof(mMailMessageSeen));
    }
    return 1;
}

// ==========================================================================
// LawnPlayerInfo::SyncZenGardenData(DataSync &)
// @0017D8B4  32 bytes
// ==========================================================================
int LawnPlayerInfo::SyncZenGardenData(DataSync &theSync)
{
    for (int i = 0; i != 50; ++i)
        theSync.SyncBytes(&mPottedPlants[i], sizeof(mPottedPlants[i]));
    return 1;
}

// ==========================================================================
// LawnPlayerInfo::SyncGameStats(DataSync &)
// @0017D8D4  34 bytes
// ==========================================================================
int LawnPlayerInfo::SyncGameStats(DataSync &theSync)
{
    // The whole stats block, then the finished-adventure bitset.
    theSync.SyncBytes(&mGameStats, sizeof(mGameStats));
    theSync.SyncBytes(&mProgress[2], 8);
    return 1;
}

// ==========================================================================
// LawnPlayerInfo::SyncDetails(DataSync &,int)
// @0017D8F8  78 bytes
// ==========================================================================
void LawnPlayerInfo::SyncDetails(DataSync &theSync, int theVersion)
{
    if (!((LawnApp *)gLawnApp)->IsFullVersion())
        return;
    if (theVersion == 1)
        SyncZenGardenData(theSync);
    else if (theVersion == 2)
        SyncGameStats(theSync);
    else if (theVersion == 0)
        SyncGameData(theSync);
}

// ==========================================================================
// LawnPlayerInfo::AddCoins(int)
// @0017D950  30 bytes
// ==========================================================================
int LawnPlayerInfo::AddCoins(int theAmount)
{
    int aTotal = mCoins + theAmount;
    if (aTotal > 99999)
        aTotal = 99999;
    if (aTotal < 0)
        aTotal = 0;
    mCoins = aTotal;
    return mCoins;
}

// ==========================================================================
// LawnPlayerInfo::UpdateHouseData(void)
// @0017D970  44 bytes
// ==========================================================================
int LawnPlayerInfo::UpdateHouseData()
{
    // The dump reads words 23 and 60 of the record, i.e. the challenge records
    // at index 12 and index 49.
    mGameStats.SetMiscStat(STAT_MONEY, mCoins);
    mGameStats.SetMiscStat(STAT_TREE_HEIGHT, mChallengeRecords[49]);
    mGameStats.SetMiscStat(STAT_ENDLESS_FLAGS, mChallengeRecords[12]);
    return 0;
}

// ==========================================================================
// LawnPlayerInfo::GetLevel(void)
// @0017D99C  4 bytes
// ==========================================================================
int LawnPlayerInfo::GetLevel()
{
    return mLevel;
}

// ==========================================================================
// LawnPlayerInfo::SetLevel(int)
// @0017D9A0  4 bytes
// ==========================================================================
int LawnPlayerInfo::SetLevel(int theLevel)
{
    mLevel = theLevel;
    return mLevel;
}

// ==========================================================================
// LawnPlayerInfo::ResetChallengeRecord(GameMode)
// @0017D9A4  12 bytes
// ==========================================================================
int LawnPlayerInfo::ResetChallengeRecord(GameMode theMode)
{
    // The record array is indexed by GameMode minus the first challenge mode,
    // which is also what LawnApp::GetCurrentChallengeIndex returns.
    mChallengeRecords[theMode - 2] = 0;
    return 0;
}

// ==========================================================================
// LawnPlayerInfo::~LawnPlayerInfo()
// @0017D1E0  92 bytes
// ==========================================================================
LawnPlayerInfo::~LawnPlayerInfo()
{
}
