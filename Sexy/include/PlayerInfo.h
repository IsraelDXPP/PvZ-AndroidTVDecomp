// Player profile record. The class is abstract: the nine profile operations
// stay pure, and IsSaving / IsLoaded / IsLoading have bodies. The derived
// record is what the game reads and writes.

#pragma once

#include "ReconTypes.h"
#include "PottedPlant.h"

class DataSync;

namespace Sexy {

class PlayerInfo
{
public:
    virtual ~PlayerInfo();

    virtual int GetId() = 0;
    virtual int GetProfileId() = 0;
    virtual Sexy::String GetName() = 0;
    virtual void Reset() = 0;
    virtual void SyncSummary(DataSync & theSync) = 0;
    virtual void SyncDetails(DataSync & theSync, int theVersion) = 0;
    virtual void DeleteUserFiles() = 0;
    virtual void LoadDetails() = 0;
    // Writes every userdata file and reports whether all of them landed.
    virtual bool SaveDetails() = 0;

    virtual bool IsSaving();
    virtual bool IsLoaded();
    virtual bool IsLoading();

    // Profile list ownership count. The last release deletes the record.
    int mRefCount;
    Sexy::ProfileMgr * mProFileMgr;
    // How many userdata files this profile keeps.
    int mDetailFileCount;
    Sexy::String mName;
    int mUseSeq;
    int mId;
    int mProfileId;
    int mFlags;
    int mLevel;
    int mCoins;
    int mChallengeRecords[100];
    int mPurchases[36];
    bool mAchievements[12];
    unsigned short mVSRoomPort;
    bool mVSExtraPacketMode;
    bool mVSExtendedSeedsMode;
    bool mVSBanMode;
    bool mVSBalancePatchMode;
    bool mVSResultsSendPlayerName;
    bool mVSResultsAutoSaveReplay;
    bool mHostAllowSpectate;
    char mRecentServerAddr[3][22];
    bool mRenamed;
    bool mVSPlantAIMode;
    bool mVSZombieAIMode;
    bool mVSAIEnhancementMode;
    bool mVSAIDraftDisabledMode;
    bool mVSAITemplateDeckDisabledMode;
    bool mVSTimedDraftMode;
    bool mPurchasePad[63];
    bool mHapticFeedbackEnabled;
    bool mZombatarEnabled;
    unsigned char mZombatarHat;
    unsigned char mZombatarHatColor;
    unsigned char mZombatarHair;
    unsigned char mZombatarHairColor;
    unsigned char mZombatarFacialHair;
    unsigned char mZombatarFacialHairColor;
    unsigned char mZombatarAccessory;
    unsigned char mZombatarAccessoryColor;
    unsigned char mZombatarEyeWear;
    unsigned char mZombatarEyeWearColor;
    unsigned char mZombatarTidBit;
    unsigned char mZombatarTidBitColor;
    bool mIs3DAcceleratedClosed;
    int mUsedCoins;
    int mZenWords[2];
    int mLastStinkyChocolateTime;
    int mStinkyPosX;
    int mStinkyPosY;
    int mNumPottedPlants;
    // Times the player declined the seed-packet upgrade offer.
    int mPacketUpgradeDeclines;
    PottedPlant mPottedPlants[50];
    double mMusicVolume;
    double mSoundVolume;
    // Progress words at the start of the flag block. Word 0 holds the
    // gargantuar-kill byte. Words 2 and 3 are the finished-adventure bitset.
    int mProgress[4];
    // Byte 0 is the favourite-plant mark. Byte 1 is the no-catapult flag.
    unsigned char mProgressFlags[2];
    bool mVSPlantWon;
    bool mHasFavoritePlant;
    bool mHelpTextSeen[7];
    int mWinStreak;
    // Set when endless stage 3 is cleared with forty or more flags.
    bool mEndlessHighScore;
    bool mExtraAchievement;
    bool mPassedShopSeedTutorial;
    // 32 bytes, 8 messages each. Message 0 is the low bit of byte 0.
    unsigned char mMailMessageRead[32];
    unsigned char mMailMessageSeen[32];
    int mGameStats[38];
    int mTail;

    // GameStats::mFavoritePlants sits eleven words into the stats block.
    char *FavoritePlants() { return (char *)&mGameStats[11]; }

    PlayerInfo(Sexy::ProfileMgr * theMgr);
};

} // namespace Sexy

using Sexy::PlayerInfo;
