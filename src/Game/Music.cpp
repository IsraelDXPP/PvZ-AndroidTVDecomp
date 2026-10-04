// Music -- reconstructed from libGameMain.so.
//
// The game's music director. One Music object owns the tracks of the tune that
// is playing: a main track plus the layers the "music burst" mixes in while the
// level is busy -- the hihats of the five level tunes, the drums of the night
// tune. It is a SyncObject, and the tune/file state from +0x18 to +0x5F is
// registered as one synchronised block by the constructor.
//
// MusicTune is a one-int wrapper and MusicFile is `typedef int` in the
// reconstructed headers, so the tune and file ids below are the numbers the
// authoritative PvZ\Lawn\System\Music.h names, spelled out in comments.

#include "Music.h"
#include "MusicInterface.h"
#include "Board.h"
#include "LawnApp.h"
#include "ReconTypes.h"
#include <cstring>

// Sexy::MusicInterface is declared in Sexy\MusicInterface.h. Music is the one
// class that calls it, and every call is a virtual slot; the header carries the
// slot table taken from _ZTVN4Sexy14MusicInterfaceE @0x68D640.

// TodAnimateCurveFloat is declared as the C-style variadic
// `int TodAnimateCurveFloat(...)` in Sexy\ReconFreeFunctions.h, so the float
// bounds travel as their bit patterns and the answer comes back in r0 as one.
// Every call in this file ramps linearly between 1.0f and 0.0f, so one wrapper
// covers them all.
static float AnimateVolumeRamp(int theStartTime, int theCurrentTime, bool theRising)
{
    const int kOneBits = 0x3F800000; // 1.0f
    int aResult = TodAnimateCurveFloat(theStartTime, 0, theCurrentTime,
                                       theRising ? 0 : kOneBits,
                                       theRising ? kOneBits : 0,
                                       CURVE_LINEAR);
    float aVolume;
    std::memcpy(&aVolume, &aResult, sizeof(aVolume));
    return aVolume;
}

// ==========================================================================
// Music::~Music()
// @001B7F18  36 bytes
// ==========================================================================
Music::~Music()
{
  // Music owns no heap object of its own: the only pointer the binary releases
  // here is SyncObject's own mSyncBlocks vector, which the base destructor
  // already handles. The dump's vptr store and its `operator delete` on
  // `*((void**)this + 1)` are the base's work, not Music's.
}

// ==========================================================================
// Music::SetupMusicFileForTune(MusicFile,MusicTune)
// @001B7C54  424 bytes
// ==========================================================================
// An xm module keeps every instrument on a channel of its own, so a tune is
// really a channel map: the channels the tune plays through stay at full volume
// and the rest are muted, which is how the game strips the parts it wants the
// interface to mix in separately (the drums and hihats tracks).
void Music::SetupMusicFileForTune(MusicFile theMusicFile, MusicTune theMusicTune)
{
    // Channels theFirstChannel..theLastChannel play, plus theExtraChannel on its
    // own; the walk below runs over channels 0..theNumChannels.
    int theFirstChannel = -1;
    int theLastChannel = -1;
    int theExtraChannel = -1;
    int theNumChannels = 0;
    bool theChannelMapIsKnown = false;

    switch (theMusicTune)
    {
    case 1: // MUSIC_TUNE_DAY_GRASSWALK
        if (theMusicFile == 1) // MUSIC_FILE_MAIN_MUSIC
        {
            theFirstChannel = 0;
            theLastChannel = 23;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 4) // MUSIC_FILE_HIHATS
        {
            theFirstChannel = 27;
            theLastChannel = 27;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 2) // MUSIC_FILE_DRUMS
        {
            theFirstChannel = 24;
            theLastChannel = 26;
            theChannelMapIsKnown = true;
        }
        break;

    case 3: // MUSIC_TUNE_POOL_WATERYGRAVES
        if (theMusicFile == 1)
        {
            theFirstChannel = 0;
            theLastChannel = 17;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 4)
        {
            theFirstChannel = 18;
            theLastChannel = 24;
            theExtraChannel = 29;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 2)
        {
            theFirstChannel = 18;
            theLastChannel = 28;
            theChannelMapIsKnown = true;
        }
        break;

    case 4: // MUSIC_TUNE_FOG_RIGORMORMIST
        if (theMusicFile == 1)
        {
            theFirstChannel = 0;
            theLastChannel = 15;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 4)
        {
            theFirstChannel = 23;
            theLastChannel = 23;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 2)
        {
            theFirstChannel = 16;
            theLastChannel = 22;
            theChannelMapIsKnown = true;
        }
        break;

    case 5: // MUSIC_TUNE_ROOF_GRAZETHEROOF
        if (theMusicFile == 1)
        {
            theFirstChannel = 0;
            theLastChannel = 17;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 4)
        {
            theFirstChannel = 21;
            theLastChannel = 21;
            theChannelMapIsKnown = true;
        }
        else if (theMusicFile == 2)
        {
            theFirstChannel = 18;
            theLastChannel = 20;
            theChannelMapIsKnown = true;
        }
        break;

    default:
        break;
    }

    if (theChannelMapIsKnown)
    {
        theNumChannels = 29;
    }
    else if (theMusicFile >= 1 && theMusicFile <= 3)
    {
        // A tune with no map of its own (the night tune) or a file the map does
        // not name (the night drums) keeps every channel.
        theFirstChannel = 0;
        theLastChannel = 29;
        theNumChannels = 29;
    }

    for (int aChannel = 0; aChannel <= theNumChannels; ++aChannel)
    {
        bool isUsed = aChannel >= theFirstChannel && aChannel <= theLastChannel;
        if (aChannel == theExtraChannel)
            isUsed = true;
        mMusicInterface->SetChannelVolume(theMusicFile, aChannel, isUsed ? 1.0f : 0.0f);
    }
}

// ==========================================================================
// Music::LoadSong(MusicFile,Sexy::String const&)
// @001B7E00  26 bytes
// ==========================================================================
// A song the interface cannot load disables music for the rest of the session.
void Music::LoadSong(MusicFile theMusicFile, Sexy::String const& theFileName)
{
    if (!mMusicInterface->LoadMusic(theMusicFile, theFileName))
        mMusicDisabled = true;
}

// ==========================================================================
// Music::GetNumLoadingTasks()
// @001B7E1C  6 bytes
// ==========================================================================
int Music::GetNumLoadingTasks()
{
    return 7000;
}

// ==========================================================================
// Music::StopAllMusic()
// @001B7E24  96 bytes
// ==========================================================================
void Music::StopAllMusic()
{
    if (mMusicInterface != nullptr)
    {
        if (mCurMusicFileMain != -1)    // MUSIC_FILE_NONE
            mMusicInterface->StopMusic(mCurMusicFileMain);
        if (mCurMusicFileDrums != -1)
            mMusicInterface->StopMusic(mCurMusicFileDrums);
        if (mCurMusicFileHihats != -1)
            mMusicInterface->StopMusic(mCurMusicFileHihats);
    }

    // Whatever was playing becomes the previous tune, and every layer is back
    // to none.
    const MusicTune aPreviousTune = mCurMusicTune;
    mNormalVolume = 0.0f;
    mCurMusicTune = -1;     // MUSIC_TUNE_NONE
    mCurMusicFileMain = -1; // MUSIC_FILE_NONE
    mLastMusicTune = aPreviousTune;
    mCurMusicFileDrums = -1;
    mCurMusicFileHihats = -1;
    mQueuedDrumTrackPackedOrder = -1;
    mMusicBurstState = MUSIC_BURST_OFF;
    mPauseOffset = 0;
    mPauseOffsetDrums = 0;
    mPaused = false;
    mFadeOutCounter = 0;
}

// ==========================================================================
// Music::PlayFromOffset(MusicFile,int,double)
// @001B7E84  54 bytes
// ==========================================================================
void Music::PlayFromOffset(MusicFile theMusicFile, int theOffset, double theVolume)
{
    mMusicInterface->StopMusic(theMusicFile);
    SetupMusicFileForTune(theMusicFile, mCurMusicTune);
    // The credits song is the only one that loops; the volume is set later, by
    // MusicUpdate.
    mMusicInterface->PlayMusic(theMusicFile, theOffset,
                               theMusicFile == 5); // MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN
}

// ==========================================================================
// Music::MusicResyncChannel(MusicFile,MusicFile)
// @001B7EBC  2 bytes
// ==========================================================================
void Music::MusicResyncChannel(MusicFile theMusicFile1, MusicFile theMusicFile2)
{
    // The PC version re-syncs one channel of the second file against the first;
    // this build does not.
    ;
}

// ==========================================================================
// Music::MusicResync()
// @001B7EC0  40 bytes
// ==========================================================================
void Music::MusicResync()
{
    if (mCurMusicFileMain == -1) // MUSIC_FILE_NONE
        return;

    if (mCurMusicFileDrums != -1)
        MusicResyncChannel(mCurMusicFileMain, mCurMusicFileDrums);
    if (mCurMusicFileHihats != -1)
        MusicResyncChannel(mCurMusicFileMain, mCurMusicFileHihats);
}

// ==========================================================================
// Music::StartBurst()
// @001B7EE8  16 bytes
// ==========================================================================
void Music::StartBurst()
{
    if (mMusicBurstState == MUSIC_BURST_OFF)
    {
        mMusicBurstState = MUSIC_BURST_STARTING;
        mBurstStateCounter = 400;
    }
}

// ==========================================================================
// Music::GameMusicPause(bool)
// @001B7EF8  2 bytes
// ==========================================================================
void Music::GameMusicPause(bool thePaused)
{
    // Pausing is done by the game mode, not by the music object.
    ;
}

// ==========================================================================
// Music::FadeOut(int)
// @001B7EFC  12 bytes
// ==========================================================================
void Music::FadeOut(int theFadeOutDuration)
{
    if (mCurMusicTune != -1) // MUSIC_TUNE_NONE
    {
        mFadeOutCounter = theFadeOutDuration;
        mFadeOutDuration = theFadeOutDuration;
    }
}

// ==========================================================================
// Music::GetMusicOrder(MusicFile)
// @001B7F08  16 bytes
// ==========================================================================
int Music::GetMusicOrder(MusicFile theMusicFile)
{
    return mMusicInterface->GetOrder(theMusicFile);
}

// ==========================================================================
// Music::MusicDispose()
// @001B7F68  42 bytes
// ==========================================================================
void Music::MusicDispose()
{
    StopAllMusic();

    for (int aSlot = 0; aSlot < 6; ++aSlot)
    {
        if (gMusicFileData[aSlot] == nullptr)
            continue;
        operator delete[](gMusicFileData[aSlot]);
        gMusicFileData[aSlot] = nullptr;
    }
}

// ==========================================================================
// Music::UpdateMusicBurst()
// @001B7FA0  932 bytes
// ==========================================================================
// The burst is the layer the tune adds while the level is busy: the hihats
// track of the five level tunes, or the drums track of the night tune. Two
// counters drive it -- mBurstStateCounter for the burst itself and
// mDrumsStateCounter for the drums fade -- and the interface's order value is
// used to keep the drums in step with the main track.
void Music::UpdateMusicBurst()
{
    if (mApp->mBoard == nullptr || mApp->mGameMode == GAMEMODE_INTRO)
        return;

    // MUSIC_TUNE_NIGHT_MOONGRAINS == 2; 1, 3, 4 and 5 are the five level tunes.
    const bool isNightMoonGrains = mCurMusicTune == 2;
    if (!isNightMoonGrains && (mCurMusicTune < 1 || mCurMusicTune > 5))
        return;

    const int theMusicOrder = GetMusicOrder(mCurMusicFileMain);

    if (mBurstStateCounter > 0)
        --mBurstStateCounter;
    if (mDrumsStateCounter > 0)
        --mDrumsStateCounter;

    // aMainTrackVolume is the hihats layer, aDrumsJumpOrder the main track while
    // the night tune ramps its drums in, and aFadeTrackVolume always the drums.
    float aMainTrackVolume = 0.0f;
    float aDrumsJumpOrder = 1.0f;
    float aFadeTrackVolume = 0.0f;
    // The order the drums track should jump to, or -1 for "leave it playing".
    int aPackedOrderMain = -1;
    MusicDrumsState theDrumsState = mMusicDrumsState;

    switch (mMusicBurstState)
    {
    case MUSIC_BURST_OFF:
        // Ten zombies on screen -- or an explicit override -- start the burst.
        if (mApp->mBoard->CountZombiesOnScreen() >= 10 || mBurstOverride == 1)
            StartBurst();
        aMainTrackVolume = 0.0f;
        aDrumsJumpOrder = 1.0f;
        break;

    case MUSIC_BURST_STARTING:
        if (isNightMoonGrains)
        {
            if (theDrumsState == MUSIC_DRUMS_OFF)
            {
                // Queue the drums and let the main track settle for a moment.
                mMusicDrumsState = MUSIC_DRUMS_ON_QUEUED;
                mBurstStateCounter = 400;
                mQueuedDrumTrackPackedOrder = theMusicOrder;
                theDrumsState = MUSIC_DRUMS_ON_QUEUED;
                aDrumsJumpOrder = 1.0f;
            }
            else if (theDrumsState == MUSIC_DRUMS_ON_QUEUED)
            {
                mBurstStateCounter = 400;
                aDrumsJumpOrder = 1.0f;
            }
            else
            {
                // The drums are already in: ramp the jump order down and hand
                // over to the "on" state when it reaches zero.
                aDrumsJumpOrder = AnimateVolumeRamp(400, mBurstStateCounter, false);
                if (mBurstStateCounter == 0)
                {
                    mMusicBurstState = MUSIC_BURST_ON;
                    mBurstStateCounter = 800;
                }
            }
        }
        else
        {
            // A level tune opens with the hihats fading in.
            aDrumsJumpOrder = 1.0f;
            aMainTrackVolume = AnimateVolumeRamp(400, mBurstStateCounter, true);
            if (mBurstStateCounter == 100)
            {
                mMusicDrumsState = MUSIC_DRUMS_ON_QUEUED;
                mQueuedDrumTrackPackedOrder = theMusicOrder;
                theDrumsState = MUSIC_DRUMS_ON_QUEUED;
            }
            else if (mBurstStateCounter == 0)
            {
                mMusicBurstState = MUSIC_BURST_ON;
                mBurstStateCounter = 800;
            }
        }
        break;

    case MUSIC_BURST_ON:
        aMainTrackVolume = 1.0f;
        // The night tune keeps the main track going and drops the jump order,
        // which is what makes its drums fall back to the main track's own.
        aDrumsJumpOrder = isNightMoonGrains ? 0.0f : 1.0f;
        if (mBurstStateCounter != 0
            || ((mApp->mBoard->CountZombiesOnScreen() > 3 || mBurstOverride != -1)
                && mBurstOverride != 2))
            break;

        // Fewer than four zombies and no override: wind the burst down.
        if (isNightMoonGrains)
        {
            mMusicBurstState = MUSIC_BURST_FINISHING;
            mMusicDrumsState = MUSIC_DRUMS_FADING;
            theDrumsState = MUSIC_DRUMS_FADING;
            mBurstStateCounter = 1100;
            mDrumsStateCounter = 800;
        }
        else
        {
            mMusicBurstState = MUSIC_BURST_FINISHING;
            mMusicDrumsState = MUSIC_DRUMS_OFF_QUEUED;
            theDrumsState = MUSIC_DRUMS_OFF_QUEUED;
            mQueuedDrumTrackPackedOrder = theMusicOrder;
            mBurstStateCounter = 800;
        }
        break;

    case MUSIC_BURST_FINISHING:
        if (isNightMoonGrains)
            aDrumsJumpOrder = AnimateVolumeRamp(400, mBurstStateCounter, true);
        else
            aMainTrackVolume = AnimateVolumeRamp(800, mBurstStateCounter, false);
        if (mBurstStateCounter == 0 && mMusicDrumsState == MUSIC_DRUMS_OFF)
            mMusicBurstState = MUSIC_BURST_OFF;
        break;
    }

    // The order value the interface reports is packed: it counts the position
    // inside a track in one word and the block in the other, and the two tunes
    // read a different word. A block boundary carried into the index is what the
    // 252 test picks up.
    const int theQueuedPackedOrder = mQueuedDrumTrackPackedOrder;
    int theMainOrderIndex;
    int theQueuedOrderIndex;
    if (isNightMoonGrains)
    {
        theMainOrderIndex = theMusicOrder & 0xFFFF;
        theQueuedOrderIndex = theQueuedPackedOrder & 0xFFFF;
        if ((theMusicOrder >> 16) > 252)
            ++theMainOrderIndex;
        if ((theQueuedPackedOrder >> 16) > 252)
            ++theQueuedOrderIndex;
    }
    else
    {
        theMainOrderIndex = (theMusicOrder >> 16) / 128;
        theQueuedOrderIndex = (theQueuedPackedOrder >> 16) / 128;
    }

    switch (theDrumsState)
    {
    case MUSIC_DRUMS_ON_QUEUED:
        // The drums come in once the main track reaches the order that was
        // queued for them; the night tune then jumps them to the odd order.
        if (theMainOrderIndex == theQueuedOrderIndex)
        {
            aFadeTrackVolume = 0.0f;
        }
        else
        {
            mMusicDrumsState = MUSIC_DRUMS_ON;
            aFadeTrackVolume = 1.0f;
            if (isNightMoonGrains)
                aPackedOrderMain = theMainOrderIndex & 1;
        }
        break;

    case MUSIC_DRUMS_ON:
        aFadeTrackVolume = 1.0f;
        break;

    case MUSIC_DRUMS_OFF_QUEUED:
        aFadeTrackVolume = 1.0f;
        // A level tune fades the drums out over 50 ticks; the night tune leaves
        // them in step with the main track instead.
        if (theMainOrderIndex != theQueuedOrderIndex && !isNightMoonGrains)
        {
            mMusicDrumsState = MUSIC_DRUMS_FADING;
            mDrumsStateCounter = 50;
        }
        break;

    case MUSIC_DRUMS_FADING:
        aFadeTrackVolume = AnimateVolumeRamp(isNightMoonGrains ? 800 : 50,
                                             mDrumsStateCounter, false);
        if (mDrumsStateCounter == 0)
            mMusicDrumsState = MUSIC_DRUMS_OFF;
        break;

    default: // MUSIC_DRUMS_OFF
        aFadeTrackVolume = 0.0f;
        break;
    }

    if (isNightMoonGrains)
    {
        mMusicInterface->SetSongVolume(mCurMusicFileMain, aDrumsJumpOrder * mPauseVolume);
        mMusicInterface->SetSongVolume(mCurMusicFileDrums, aFadeTrackVolume * mPauseVolume);
        if (aPackedOrderMain != -1)
            mMusicInterface->PlayMusic(mCurMusicFileDrums, aPackedOrderMain, false);
    }
    else
    {
        mMusicInterface->SetSongVolume(mCurMusicFileHihats, aMainTrackVolume * mPauseVolume);
        mMusicInterface->SetSongVolume(mCurMusicFileDrums, aFadeTrackVolume * mPauseVolume);
    }
}

// ==========================================================================
// Music::MusicUpdate()
// @001B8348  166 bytes
// ==========================================================================
void Music::MusicUpdate()
{
    if (mFadeOutCounter <= 0)
    {
        // Keep the interface's volume in step with the pause volume; there is
        // nothing else to do while no fade is running.
        if (mPauseVolume != mNormalVolume)
        {
            mNormalVolume = mPauseVolume;
            mMusicInterface->SetSongVolume(mCurMusicFileMain, mPauseVolume);
        }
    }
    else
    {
        --mFadeOutCounter;
        if (mFadeOutCounter > 0)
        {
            const float aVolume = AnimateVolumeRamp(mFadeOutDuration, mFadeOutCounter, false);
            mMusicInterface->SetSongVolume(mCurMusicFileMain, aVolume * mPauseVolume);
        }
        else
        {
            StopAllMusic();
        }
    }

    UpdateMusicBurst();

    const Board *aBoard = mApp->mBoard;
    if ((aBoard == nullptr || !aBoard->mPaused) && mCurMusicFileMain != -1)
        MusicResync();
}

// ==========================================================================
// Music::StartGameMusic(bool)
// @001B83F0  314 bytes
// ==========================================================================
// Picks the tune for the level that is about to start. theStartPaused simply
// forgets the current tune, so the first MusicUpdate starts one.
void Music::StartGameMusic(bool theStartPaused)
{
    if (theStartPaused)
        mCurMusicTune = -1; // MUSIC_TUNE_NONE

    // The Zen garden and the tree of wisdom share a tune.
    if (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN
        || mApp->mGameMode == GAMEMODE_TREE_OF_WISDOM)
        return MakeSureMusicIsPlaying(8); // MUSIC_TUNE_ZEN_GARDEN

    if (mApp->IsFinalBossLevel())
        return MakeSureMusicIsPlaying(12); // MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC
    if (mApp->IsWallnutBowlingLevel())
        return MakeSureMusicIsPlaying(10); // MUSIC_TUNE_MINIGAME_LOONBOON
    if (mApp->IsWhackAZombieLevel())
        return MakeSureMusicIsPlaying(10);
    if (mApp->IsLittleTroubleLevel())
        return MakeSureMusicIsPlaying(10);
    if (mApp->IsBungeeBlitzLevel())
        return MakeSureMusicIsPlaying(10);
    if (mApp->mGameMode == GAMEMODE_CHALLENGE_SPEED)
        return MakeSureMusicIsPlaying(10);

    // Levels 10, 20 and 30 of the adventure are the conveyor levels.
    if (mApp->IsAdventureMode())
    {
        const int aLevel = mApp->mPlayerInfo->mLevel;
        if (aLevel == 10 || aLevel == 20 || aLevel == 30)
            return MakeSureMusicIsPlaying(11); // MUSIC_TUNE_CONVEYER
    }
    if (mApp->mGameMode == GAMEMODE_CHALLENGE_COLUMN)
        return MakeSureMusicIsPlaying(11);

    if (mApp->IsStormyNightLevel())
        return StopAllMusic();

    if (mApp->IsScaryPotterLevel() || mApp->IsIZombieLevel())
        return MakeSureMusicIsPlaying(9); // MUSIC_TUNE_PUZZLE_CEREBRAWL

    Board *aBoard = mApp->mBoard;
    if (aBoard->mBackground == BACKGROUND_4_FOG)
        return MakeSureMusicIsPlaying(4); // MUSIC_TUNE_FOG_RIGORMORMIST
    if (aBoard->StageIsNight())
        return MakeSureMusicIsPlaying(2); // MUSIC_TUNE_NIGHT_MOONGRAINS
    if (aBoard->StageHasPool())
        return MakeSureMusicIsPlaying(3); // MUSIC_TUNE_POOL_WATERYGRAVES
    if (aBoard->StageHasRoof())
        return MakeSureMusicIsPlaying(5); // MUSIC_TUNE_ROOF_GRAZETHEROOF
    return MakeSureMusicIsPlaying(1);     // MUSIC_TUNE_DAY_GRASSWALK
}

// ==========================================================================
// Music::MusicTitleScreenInit()
// @001B852C  90 bytes
// ==========================================================================
void Music::MusicTitleScreenInit()
{
    LoadSong(1, "sounds/mainmusic.xm"); // MUSIC_FILE_MAIN_MUSIC
}

// ==========================================================================
// Music::MusicLoadCreditsSong()
// @001B85A0  106 bytes
// ==========================================================================
void Music::MusicLoadCreditsSong()
{
    if (!mMusicInterface->IsLoaded(5)) // MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN
        LoadSong(5, "sounds/ZombiesOnYourLawn.ogg");
}

// ==========================================================================
// Music::MusicInit()
// @001B8624  290 bytes
// ==========================================================================
void Music::MusicInit()
{
    // The binary opens with a discarded call through slot 18; GetNumLoadingTasks
    // is a constant in this class, so the call is kept only because a derived
    // override could have a side effect.
    GetNumLoadingTasks();

    // The three xm files are the tracks of the five level tunes -- the ids are
    // the ones the binary passes, which do not line up with the MusicFile enum.
    LoadSong(2, "sounds/mainmusic.xm");
    mApp->mCompletedLoadingThreadTasks += 3500;
    LoadSong(3, "sounds/mainmusic_drums.xm");
    mApp->mCompletedLoadingThreadTasks += 3500;
    LoadSong(4, "sounds/mainmusic_hihats.xm");
    mApp->mCompletedLoadingThreadTasks += 3500;
}

// ==========================================================================
// Music::PlayMusic(MusicTune,int,int)
// @001B876C  462 bytes
// ==========================================================================
// Starts a tune: every layer is forgotten, the tune's files are chosen, and the
// interface is told to play them at the offset the caller asks for (or at the
// offset the tune is known to start at). Returns the binary's r0, which no
// caller reads.
void Music::PlayMusic(MusicTune theMusicTune, int theOffset, int theDrumsOffset)
{
    if (mMusicDisabled)
        return;

    mLastMusicTune = theMusicTune;
    mCurMusicTune = theMusicTune;
    mCurMusicFileMain = -1;  // MUSIC_FILE_NONE
    mCurMusicFileDrums = -1;
    mCurMusicFileHihats = -1;

    // The offset each tune starts at when the caller does not name one; the
    // night tune also has a starting offset for its drums.
    if (theOffset == -1)
    {
        switch (theMusicTune)
        {
        case 1:  theOffset = 0;                     break; // MUSIC_TUNE_DAY_GRASSWALK
        case 2:  theDrumsOffset = 0; theOffset = 48; break; // MUSIC_TUNE_NIGHT_MOONGRAINS
        case 3:  theOffset = 94;                    break; // MUSIC_TUNE_POOL_WATERYGRAVES
        case 4:  theOffset = 125;                   break; // MUSIC_TUNE_FOG_RIGORMORMIST
        case 5:  theOffset = 184;                   break; // MUSIC_TUNE_ROOF_GRAZETHEROOF
        case 6:  theOffset = 122;                   break; // MUSIC_TUNE_CHOOSE_YOUR_SEEDS
        case 7:  theOffset = 152;                   break; // MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME
        case 8:  theOffset = 221;                   break; // MUSIC_TUNE_ZEN_GARDEN
        case 9:  theOffset = 177;                   break; // MUSIC_TUNE_PUZZLE_CEREBRAWL
        case 10: theOffset = 166;                   break; // MUSIC_TUNE_MINIGAME_LOONBOON
        case 11: theOffset = 212;                   break; // MUSIC_TUNE_CONVEYER
        case 12: theOffset = 158;                   break; // MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC
        case 13: theOffset = 0;                     break; // MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN
        default: break;
        }
    }

    switch (theMusicTune)
    {
    case 1: // MUSIC_TUNE_DAY_GRASSWALK
    case 3: // MUSIC_TUNE_POOL_WATERYGRAVES
    case 4: // MUSIC_TUNE_FOG_RIGORMORMIST
    case 5: // MUSIC_TUNE_ROOF_GRAZETHEROOF
        // The five level tunes play the main track with both extra layers; only
        // the main track is audible until the burst mixes the others in.
        mCurMusicFileMain = 1;   // MUSIC_FILE_MAIN_MUSIC
        mCurMusicFileDrums = 2;  // MUSIC_FILE_DRUMS
        mCurMusicFileHihats = 4; // MUSIC_FILE_HIHATS
        PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
        PlayFromOffset(mCurMusicFileDrums, theOffset, 0.0);
        PlayFromOffset(mCurMusicFileHihats, theOffset, 0.0);
        break;

    case 2: // MUSIC_TUNE_NIGHT_MOONGRAINS
        mCurMusicFileMain = 1;
        mCurMusicFileDrums = 3; // MUSIC_FILE_DRUMS_NIGHTMOONGRAINS
        PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
        PlayFromOffset(mCurMusicFileDrums, theDrumsOffset, 0.0);
        break;

    case 6:  // MUSIC_TUNE_CHOOSE_YOUR_SEEDS
    case 7:  // MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME
    case 8:  // MUSIC_TUNE_ZEN_GARDEN
    case 9:  // MUSIC_TUNE_PUZZLE_CEREBRAWL
    case 10: // MUSIC_TUNE_MINIGAME_LOONBOON
    case 11: // MUSIC_TUNE_CONVEYER
    case 12: // MUSIC_TUNE_FINAL_BOSS_BRAINIAC_MANIAC
        // The menu and puzzle tunes have the main track only.
        mCurMusicFileMain = 1;
        PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
        break;

    case 13: // MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN
        mCurMusicFileMain = 5; // MUSIC_FILE_CREDITS_ZOMBIES_ON_YOUR_LAWN
        PlayFromOffset(mCurMusicFileMain, theOffset, 1.0);
        break;

    default:
        return;
    }

    return;
}

// ==========================================================================
// Music::MakeSureMusicIsPlaying(MusicTune)
// @001B893C  38 bytes
// ==========================================================================
void Music::MakeSureMusicIsPlaying(MusicTune theMusicTune)
{
    if (mCurMusicTune == theMusicTune)
        return;

    StopAllMusic();
    PlayMusic(theMusicTune, -1, -1);
}

// ==========================================================================
// Music::Music()
// @001B8964  146 bytes
// ==========================================================================
Music::Music()
{
    // Everything from the tune through the fade pair is one synchronised block:
    // mCurMusicTune (+0x18) up to mPauseVolume (+0x60), 72 bytes. Registering
    // it and then clearing it is how the binary initialises the tail of the
    // object, so every field below is written after the clear.
    const int aSyncBlockSize = 72;
    SyncBlockInfo aBlock;
    aBlock.mData = &mCurMusicTune;
    aBlock.mSize = aSyncBlockSize;
    mSyncBlocks.push_back(aBlock);
    memset(&mCurMusicTune, 0, aSyncBlockSize);

    mPauseVolume = 1.0f;
    mNormalVolume = 1.0f;

    mApp = (LawnApp *)gSexyAppBase;
    mMusicInterface = mApp->mMusicInterface;

    mLastMusicTune = -1;    // MUSIC_TUNE_NONE
    mCurMusicTune = -1;
    mCurMusicFileMain = -1; // MUSIC_FILE_NONE
    mCurMusicFileDrums = -1;
    mCurMusicFileHihats = -1;
    mBurstOverride = -1;
    mQueuedDrumTrackPackedOrder = -1;
    mMusicBurstState = MUSIC_BURST_OFF;
    mMusicDrumsState = MUSIC_DRUMS_OFF;
    mDrumsStateCounter = 0;
    mPauseOffset = 0;
    mPauseOffsetDrums = 0;
    mPaused = false;
    mMusicDisabled = false;
    mFadeOutCounter = 0;
    mFadeOutDuration = 0;
    mBaseBPM = 155;
    mBaseModSpeed = 3;
}
