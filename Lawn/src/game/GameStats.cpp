#include "GameStats.h"

#include "SeedBank.h"
#include <cstring>

GameStats::GameStats(const GameStats& theOther)
{
  mHouseType = theOther.mHouseType;
  mHouseLevel = theOther.mHouseLevel;
  for (int aStat = 0; aStat < 9; ++aStat)
    mMiscStats[aStat] = theOther.mMiscStats[aStat];
  std::memcpy(mFavoritePlants, theOther.mFavoritePlants, sizeof(mFavoritePlants));
  std::memcpy(mFavoriteZombies, theOther.mFavoriteZombies, sizeof(mFavoriteZombies));
  mSurvivalCompletion = theOther.mSurvivalCompletion;
  mMiniGameCompletion = theOther.mMiniGameCompletion;
  mPuzzleCompletion = theOther.mPuzzleCompletion;
}

void GameStats::Reset()
{
  mHouseType = BLUEPRINT_BLING;
  mHouseLevel = 0;
  for (int aStat = 0; aStat < 9; ++aStat)
    mMiscStats[aStat] = 0;
  std::memset(mFavoritePlants, 0, sizeof(mFavoritePlants));
  std::memset(mFavoriteZombies, 0, sizeof(mFavoriteZombies));
  mSurvivalCompletion = 0.0f;
  mMiniGameCompletion = 0.0f;
  mPuzzleCompletion = 0.0f;
}

GameStats::GameStats()
{
  Reset();
}

int GameStats::GetFavoritePlant()
{
  int aBest = -1;
  int aBestCount = 0;
  for (int aSeed = 0; aSeed < 49; ++aSeed)
  {
    unsigned char aCount = (unsigned char)mFavoritePlants[aSeed];
    if (aCount >= aBestCount)
    {
      if (aCount > aBestCount)
        aBest = aSeed;
      aBestCount = aCount;
    }
  }
  return aBest == -1 ? 1 : aBest;
}

int GameStats::GetFavoriteZombie()
{
  int aBest = -1;
  int aBestCount = 0;
  for (int aZombie = 0; aZombie < 47; ++aZombie)
  {
    unsigned char aCount = (unsigned char)mFavoriteZombies[aZombie];
    if (aCount >= aBestCount)
    {
      if (aCount > aBestCount)
        aBest = aZombie;
      aBestCount = aCount;
    }
  }
  return aBest == -1 ? 1 : aBest;
}

GameStats* GameStats::TallyPlantsUsed(SeedBank *theSeedBank)
{
  for (int aPacket = 0; aPacket < theSeedBank->mNumPackets; ++aPacket)
  {
    int aTimesUsed = theSeedBank->mSeedPackets[aPacket].mTimesUsed;
    if (aTimesUsed > 0)
    {
      unsigned char &aCount =
          (unsigned char &)mFavoritePlants[theSeedBank->mSeedPackets[aPacket].mPacketType];
      aCount = (unsigned char)(aCount + aTimesUsed);
    }
  }
  return this;
}

void GameStats::SetSurvivalCompletion(float theCompletion)
{
  mSurvivalCompletion = theCompletion;
}

void GameStats::SetMinigameCompletion(float theCompletion)
{
  mMiniGameCompletion = theCompletion;
}

void GameStats::SetPuzzleCompletion(float theCompletion)
{
  mPuzzleCompletion = theCompletion;
}

void GameStats::ChangeMiscStat(GameStats_MiscStat theStat, int theDelta)
{
  mMiscStats[theStat] += theDelta;
}

void GameStats::SetMiscStat(GameStats_MiscStat theStat, int theValue)
{
  mMiscStats[theStat] = theValue;
}

int GameStats::GetMiscStat(GameStats_MiscStat theStat)
{
  return mMiscStats[theStat];
}

Sexy::String GameStats::GetStatName(GameStats_MiscStat theStat)
{
  switch (theStat)
  {
    case 0: return Sexy::String("[ZOMBIES KILLED]");
    case 1: return Sexy::String("[PLANTS_KILLED]");
    case 2: return Sexy::String("[MONEY]");
    case 4: return Sexy::String("[TREE_HEIGHT]");
    case 5: return Sexy::String("[ENDLESS_FLAGS]");
    case 6: return Sexy::String("[MOWED_ZOMBIES]");
    case 7: return Sexy::String("[STINKY_CHOCOLATES]");
    default: return Sexy::String();
  }
}
