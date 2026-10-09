// CutScene -- reconstructed from libGameMain.so.
//
// No base class and no vtable: the binary has neither a _ZTV nor a _ZTI symbol
// for CutScene, and the transcribed bodies are plain member functions.
//
// Size 0x78 (30 ints). The authoritative header PvZ\Lawn\Board\CutScene.h:35-62
// lists every member and ends with "size = 30 ints"; the last named member
// (mMPWonPosY, byte 92) plus its `int unk[6]` tail lands exactly on 0x78.
// The previous 0x29E0 came from the transcriber's "largest this-relative offset"
// heuristic, which had folded in offsets that belong to other objects (the dump
// passes several sret buffers through `result + N`, e.g. `result + 0xa74`).

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Challenge.h"
#include "ChallengeScreen.h"
#include "Coin.h"
#include "GameObject.h"
#include "GameOverDialog.h"
#include "Graphics.h"
#include "GridItem.h"
#include "PerfTimer.h"
#include "Reanimation.h"
#include "TodFoley.h"
#include "TodParticleSystem.h"
#include "WaitForSecondPlayerDialog.h"
#include "Zombie.h"
#include <string>

// Board and LawnApp are stored only as `Board *mBoard` / `LawnApp *mApp` (plus
// `LawnApp **a1` in one signature), and LawnMower, Projectile, SeedBank,
// StoreScreen and LawnPlayerInfo are not named in this file at all, so all seven
// includes were pure cycle. CutScene.h was one of the busiest members of the
// cluster, so cutting all seven here removes a whole family.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
struct BoardRenderItem;
class LawnMower;
class Projectile;
class SeedBank;
class StoreScreen;
class LawnPlayerInfo;
class LawnApp;
#include <vector>
#include <map>
#include <set>

class CutScene
{
public:
    LawnApp *mApp;                           // +0x00
    Board *mBoard;                           // +0x04 (CutScene.cpp reads this + 4)
    int mCutsceneTime;                       // +0x08
    int mSodTime;                            // +0x0C
    int mGraveStoneTime;                     // +0x10
    int mReadySetPlantTime;                  // +0x14
    int mFogTime;                            // +0x18
    int mBossTime;                           // +0x1C
    int mCrazyDaveTime;                      // +0x20
    int mLawnMowerTime;                      // +0x24
    int mCrazyDaveDialogStart;               // +0x28
    bool mSeedChoosing;                      // +0x2C (byte 44 in the authoritative table)
    ReanimationID mZombiesWonReanimID;       // +0x30
    bool mPreloaded;                         // +0x34 (byte 52)
    bool mPlacedZombies;                     // +0x35 (byte 53)
    bool mPlacedLawnItems;                   // +0x36 (byte 54)
    int mCrazyDaveCountDown;                 // +0x38 (upsell countdown)
    int mCrazyDaveLastTalkIndex;             // +0x3C (upsell line index)
    bool mUpsellHideBoard;                   // +0x40 (byte 64)
    ChallengeScreen *mUpsellChallengeScreen; // +0x44
    bool mPreUpdatingBoard;                  // +0x48 (byte 72)
    ReanimationID mPlantsWonReanimID;        // +0x4C (byte 76)
    Zombie *mZombieWonZombie;                // +0x50 (byte 80)
    int mMPZombieWonPosition;                // +0x54 (byte 84)
    float mMPWonPosX;                        // +0x58 (byte 88)
    float mMPWonPosY;                        // +0x5C (byte 92)
    std::set<Sexy::String> mLoadedResources;

    CutScene();
    bool Is2x2Zombie(ZombieType theZombieType);
    void PlaceAZombie(ZombieType theZombieType, int theGridX, int theGridY);
    bool CanZombieGoInGridSpot(ZombieType theZombieType, int theGridX, int theGridY, bool (*theZombieGrid)[5]);
    void FindPlaceForStreetZombies(ZombieType theZombieType, bool (*theZombieGrid)[5], int &theGridX, int &theGridY);
    void FindAndPlaceZombie(ZombieType theZombieType, bool (*theZombieGrid)[5]);
    void PlaceStreetZombies();
    bool IsSurvivalRepick();
    bool IsNonScrollingCutscene();
    bool IsScrolledLeftAtStart();
    bool CanGetPacketUpgrade();
    bool CanGetSecondPacketUpgrade();
    void StartSeedChooser();
    void EndSeedChooser();
    bool IsAfterSeedChooser();
    bool IsAfterScrollLeft();
    bool IsBeforePreloading();
    int CalcPosition(int theStartTime, int theEndTime, int theStartPos, int theEndPos);
    void AddGraveStoneParticles();
    void AddFlowerPots();
    void PlaceLawnItems();
    void AnimateBoard();
    void ShowShovel();
    bool IsInShovelTutorial();
    void StartZombiesWon();
    void StartZombiesWonMP(Zombie * theZombie);
    void StartPlantsWon(Reanimation * theReanim);
    bool ShowZombieWalking();
    void UpdateZombiesWon();
    void UpdateZombiesWonMP();
    void UpdatePlantsWon();
    bool IsIntroCutSceneOver();
    bool IsEndCutSceneOver();
    void ZombieWonClick();
    void AdvanceCrazyDaveDialog(bool theFromButton);
    bool IsShowingCrazyDave();
    // The mangled name is
    // _ZN8CutScene14GameButtonDownEN4Sexy13GamepadButtonEij, i.e. three
    // parameters. The old one-argument spelling made Board.cpp's call a C2660.
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    int ParseDelayTimeFromMessage();
    int ParseTalkTimeFromMessage();
    void AddUpsellZombie(ZombieType theZombieType, int thePosX, int theRow);
    bool ShouldRunUpsellBoard();
    void DrawIntro(Sexy::Graphics * theGraphics);
    void DrawMPFade(Sexy::Graphics * theGraphics, const SexyVector2 & thePos);
    void DrawPlantsWonMP(Sexy::Graphics * theGraphics);
    void DrawZombiesWonMP(Sexy::Graphics * theGraphics);
    void DrawGameObjectsHook(BoardRenderItem *aList, int &aCount);
    ~CutScene();
    int LoadResources(Sexy::String const& theGroup);
    int PreloadResources();
    int CancelIntro();
    int MouseDown(int theX, int theY);
    int OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags);
    void KeyDown(Sexy::KeyCode theKey);
    void StartLevelIntro();
    void ClearUpsellBoard();
    int LoadIntroBoard();
    void UpdateIntro();
    void LoadUpsellBoardPool();
    void LoadUpsellBoardFog();
    void LoadUpsellChallengeScreen();
    void LoadUpsellBoardRoof();
    void UpdateUpsell();
    void Update();
    void DrawUpsell(Sexy::Graphics * theGraphics);
};
