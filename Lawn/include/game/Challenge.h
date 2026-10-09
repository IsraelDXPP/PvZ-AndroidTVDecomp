// Challenge -- reconstructed from libGameMain.so.
//
// Inheritance: `class Challenge : public SyncObject`, exactly as the
// authoritative header PvZ\Lawn\Board\Challenge.h:67 declares it. The binary
// confirms it twice over in ~Challenge @0x17DCCC: the destructor re-stores the
// BASE table last (`*(_DWORD *)this = &off_67CDC8` right after the Challenge
// table `&off_6816B0`) and then frees `*((void **)this + 1)`, which is
// SyncObject's std::vector -- the epilogue of ~SyncObject. The base occupies
// +0x00..+0x0F, so mApp starts at +0x10 = word 4, the origin of the "// 4"
// ordinals in the authoritative member comments.
//
// Members: names, types and order are copied from that authoritative header. Its
// comments mix absolute word ordinals with byte offsets; the offset at the end of
// every line below is the byte offset, and the constructor @0x1892E0 corroborates
// each one: word 4 = mApp (and `*((_DWORD *)this + 4)` is what the destructor
// dereferences), word 5 = mBoard, word 6 = mHelpBar, byte 28 = mBeghouledMouseCapture,
// bytes 40..93 = mBeghouledEated, bytes 94..96 = mBeghouledPurcasedUpgrade,
// word 25 = mBeghouledMatchesThisMove, words 26/28/29/30 = mChallengeState/
// mConveyorBeltCounter1/mConveyorBeltCounter2/mChallengeScore, byte 124 =
// mShowBowlingLine, words 77..79 = the three mReanimHeavyWeaponID fields.
//
// Size 0x140 (80 words). The old 0x958-byte placeholder was not this class's
// size: the constructor computes the end of the object as this + 320 and then
// zeroes everything from this + 28 up to it, so the object ends at +0x140, and
// the authoritative header's own tail comment says "80 integers".
// The table below adds up to exactly that -- the last member sits at +0x13C and
// ends at +0x140 -- and every `+ 0xNN` anchor the .cpp bodies use lands on a
// member of it: 132 = mSurvivalStage (read by Board), 140 = mReanimChallenge,
// 212 = mBeghouledMouseDown, 232 = mPauseStartTick, 236 = mIsMPSuddenDeathNow,
// 268 = mHeavyWeaponX.
//
// vtable order -- the export _ZTV9ChallengeE holds nothing but the two Itanium
// destructor slots, so a single virtual destructor reproduces it (MSVC x86
// collapses the D2/D0 pair into one slot):
//   [  0] 0x17dccc  Challenge::~Challenge   (D2)
//   [  1] 0x17dd50  Challenge::~Challenge   (D0)
// No other method of this class is virtual. The 176 declarations at the end keep
// the order the transcription produced, so phase 2 can retype them one at a time.

#pragma once

#include "ReconTypes.h"
#include "SyncObject.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "BaseGamepadControls.h"
#include "Coin.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "GridItem.h"
#include "HelpBarWidget.h"
#include "LawnMower.h"
#include "MTRand.h"
#include "MessageWidget.h"
#include "Projectile.h"
#include "Reanimation.h"
#include "SeedPacket.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "TodParticleSystem.h"
#include "ToolTipWidget.h"
#include "Zombie.h"

// These four were half of an include cycle each (Challenge.h <-> Board.h,
// CutScene.h, ZenGarden.h and LawnApp.h) and none of them is needed here: Board
// is stored as `Board *mBoard`, LawnApp as `LawnApp *mApp`, and CutScene and
// ZenGarden are not named in this file at all. A cycle is what lets
// `#pragma once` skip a class definition and turn a derived class into C2504.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
class CutScene;
class ZenGarden;
class LawnApp;

// The two helper types the authoritative header defines in this file. The
// transcribed signatures still spell their parameters `_DWORD *`, so nothing
// below uses them yet; they are declared here so phase 2 can retype those
// parameters without inventing the types.
enum BeghouledUpgrade
{
    BEGHOULED_UPGRADE_REPEATER,
    BEGHOULED_UPGRADE_FUMESHROOM,
    BEGHOULED_UPGRADE_TALLNUT,
    NUM_BEGHOULED_UPGRADES = 3,
};

struct BeghouledBoardState
{
    SeedType mSeedType[9][6];
};

class Challenge : public SyncObject
{
public:
    // Statics the binary keeps in this class's own storage; the transcribed
    // bodies name them through the qualified spelling. They are public because
    // other classes read them directly -- `Challenge::gVSWinMode` in
    // CutScene.cpp, for instance. Challenge.cpp defines their TV defaults.
    static int gVSResourceDropCount;
    static int gVSResourseDropMode;
    static int gVSWinMode;
    static int gVSMowerCount;
    // The other two VS-mode statics, read by GamepadControls.cpp and Board.cpp
    // as `Challenge::gVSSuddenDeathMode` / `Challenge::gVSZombieSpawnMode`.
    static int gVSSuddenDeathMode;
    static int gVSZombieSpawnMode;
    // The authoritative header also declares the shuffle-mode flag
    // (`static inline bool msVSShuffleMode = false;`). No transcribed body
    // references it yet, but the PvZ-src restoration does, so it is declared.
    static inline bool msVSShuffleMode = false;

    // --- members, in binary order ------------------------------------------
    // The SyncObject base supplies +0x00 (vptr) and +0x04..+0x0F (mSyncBlocks).
    LawnApp       *mApp;                          // +0x10
    Board         *mBoard;                        // +0x14
    HelpBarWidget *mHelpBar;                      // +0x18
    bool           mBeghouledMouseCapture;        // +0x1C
    int            mBeghouledMouseDownX;          // +0x20
    int            mBeghouledMouseDownY;          // +0x24
    bool           mBeghouledEated[9][6];         // +0x28 (54 bytes)
    bool           mBeghouledPurcasedUpgrade[NUM_BEGHOULED_UPGRADES]; // +0x5E
    // Three bytes of padding follow, so this int lands on +0x64 (word 25).
    int            mBeghouledMatchesThisMove;     // +0x64
    ChallengeState mChallengeState;               // +0x68
    int            mChallengeStateCounter;        // +0x6C
    int            mConveyorBeltCounter1;         // +0x70
    int            mConveyorBeltCounter2;         // +0x74
    int            mChallengeScore;               // +0x78
    bool           mShowBowlingLine;              // +0x7C
    SeedType       mLastConveyorSeedType;         // +0x80
    int            mSurvivalStage;                // +0x84
    int            mSlotMachineRollCount;         // +0x88
    ReanimationID  mReanimChallenge;              // +0x8C
    ReanimationID  mReanimCloud[6];               // +0x90
    int            mCloudCounter[6];              // +0xA8
    int            mChallengeGridX;               // +0xC0
    int            mChallengeGridY;               // +0xC4
    int            mScaryPotterPots;              // +0xC8
    int            mRainCounter;                  // +0xCC
    int            mTreeOfWisdomTalkIndex;        // +0xD0
    // The authoritative header types this `int`, but every access in the binary
    // is byte-wide -- the constructor and the beghouled bodies store bytes
    // (`*((_BYTE *)this + 212) = 1/0`) and UpdateBeghouled compares and copies
    // it as `*((unsigned __int8 *)this + 212)` -- so it is a bool here. The next
    // member stays word-aligned at +0xD8 either way, so the size is unchanged.
    bool           mBeghouledMouseDown;           // +0xD4
    int            mBeghouledMouseCaptureCountdown; // +0xD8
    int            mMPZombieConveyorTimer;        // +0xDC
    int            mBobSledMPCounter;             // +0xE0 (the ctor writes 3000)
    int            mSuddenDeathStartTick;         // +0xE4, unused (-1)
    int            mPauseStartTick;               // +0xE8, unused (-1)
    bool           mIsMPSuddenDeathNow;           // +0xEC
    SeedType       mSuddenDeathDisableSeeds1[3];  // +0xF0
    SeedType       mSuddenDeathDisableSeeds2[3];  // +0xFC
    int            mSuddenDeathBoomCount;         // +0x108
    float          mHeavyWeaponX;                 // +0x10C
    float          mHeavyWeaponY;                 // +0x110
    float          mHeavyWeaponAngle;             // +0x114
    // Counts down each HeavyWeaponUpdate tick. At 0 the update draws a new
    // idle delay from 1000 to 3000.
    int            mHeavyWeaponIdleTimer;         // +0x118
    int            mHeavyWeaponCatTailAttackCounter; // +0x11C
    int            mHeavyWeaponCatTailLevel;      // +0x120
    int            mHeavyWeaponPeaLevel;          // +0x124
    int            mHeavyWeaponSnowPeaCounter;    // +0x128
    int            mHeavyWeaponTorchWoodCounter;  // +0x12C
    // +0x130 was written 10 at level start and 0 in the constructor. Nothing reads it.
    ReanimationID  mReanimHeavyWeaponID1;         // +0x134
    ReanimationID  mReanimHeavyWeaponID2;         // +0x138
    ReanimationID  mReanimHeavyWeaponID3;         // +0x13C
    // size 0x140

    // --- methods (declaration order preserved from the transcription) -------
    //
    // Return types converged against the authoritative PvZ\Lawn\Board\Challenge.h:
    // the bytes of each body were re-read (the `// @00XXXXXX` banners give the
    // address) and every function that only leaves its result in r0 now returns
    // void, every function whose body returns a stored pointer returns that
    // pointer's type, and every function whose body returns a PlantingReason /
    // SeedType / ZombieType returns the enum. The authoritative header also marks
    // a number of these `const`; that pass is NOT done here, because a const
    // member function cannot have a body that writes members and several of these
    // bodies do.
    int LoadBeghouledBoardState(BeghouledBoardState *theState);
    int BeghouledTwistValidMove(int theGridX, int theGridY, BeghouledBoardState *theState);
    int BeghouledTwistSquareFromMouse(int theX, int theY, int & theGridX, int & theGridY);
    int BeghouledDragStart(int theX, int theY);
    int BeghouledDragCancel();
    int BeghouledGetPlantAt(int theGridX, int theGridY, BeghouledBoardState *theState);
    int BeghouledRemoveHorizontalMatch(int theGridX, int theGridY, BeghouledBoardState *theState);
    int BeghouledRemoveVerticalMatch(int theGridX, int theGridY, BeghouledBoardState *theState);
    int BeghouledHorizontalMatchLength(int theGridX, int theGridY, BeghouledBoardState *theState);
    int BeghouledVerticalMatchLength(int theGridX, int theGridY, BeghouledBoardState *theState);
    int BeghouledBoardHasMatch(BeghouledBoardState *theState);
    int BeghouledTwistMoveCausesMatch(int theGridX, int theGridY, BeghouledBoardState *theState);
    int BeghouledIsValidMove(int theFromX, int theFromY, int theToX, int theToY, BeghouledBoardState *theState);
    int BeghouledPickSeed(int theGridX, int theGridY, BeghouledBoardState *theState, bool theAllowMatches);
    int BeghouledFillHoles(BeghouledBoardState *theState, bool theAllowMatches);
    int BeghouledCheckForPossibleMoves(BeghouledBoardState *theState);
    void MouseDownWhackAZombie(int theX, int theY, int theClickCount);
    // Returns a Rect by value: the binary's `this` here is the ARM sret
    // destination (the body writes four ints at offsets 0/4/8/12 of it), not the
    // Challenge object. The old `Challenge *` return type was the transcription
    // merging the two.
    Sexy::Rect ShotMachineGetHandleRect();
    int UpdateToolTip(int theX, int theY);
    int MouseUp(int theX, int theY);
    int OnKeyUp(Sexy::KeyCode theKey, unsigned int theFlags);
    int GameButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags);
    int GameAxisMove(int theAxis, int theButton, int thePlayerIndex);
    int ClearCursor(int thePlayerIndex);
    int UpdateBeghouledPlant(Plant * thePlant);
    Plant * BeghouledFlashPlant(int theFlashX, int theFlashY, int theFromX, int theFromY, int theToX, int theToY);
    int BeghouledTwistFlashMatch(BeghouledBoardState *theState, int theGridX, int theGridY);
    int BeghouledFlashFromBoardState(BeghouledBoardState *theState, int theFromX, int theFromY, int theToX, int theToY);
    int BeghouledCancelMatchFlashing();
    int BeghouledStartFalling(ChallengeState theState);
    int BeghouledTwistMouseDown(int theX, int theY);
    int OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags);
    void BeghouledDragUpdate(int theX, int theY);
    int MouseMove(int theX, int theY);
    bool BeghouledFallIntoSquare(unsigned int theGridX, int theGridY, BeghouledBoardState *theState);
    bool BeghouledMakePlantsFall(BeghouledBoardState *theState);
    void BeghouledCreatePlants(const BeghouledBoardState *theOldState, const BeghouledBoardState *theNewState);
    void BeghouledMakeStartBoard();
    void BeghouledPopulateBoard();
    void BeghouledFlashAMatch();
    void UpdateConveyorBelt(int thePlayerIndex);
    void UpdateMPGraveStones();
    void UpdateMPZombieBank();
    void UpdateRainingSeeds();
    void UpdateStormyNight();
    void BeghouledCheckStuckState();
    static bool IsMPResourceProducer(SeedType theSeedType);
    bool IsMPSuddenDeath();
    int GetSuddenDeathCount();
    bool ISMPSeedSuddenDeathDisabled(int thePlayerIndex, SeedType theSeedType);
    SeedType GetArtChallengeSeed(int theGridX, int theGridY);
    void SpawnLevelAward(int theGridX, int theGridY);
    void UpdateSlotMachine();
    void CheckForCompleteArtChallenge(int theGridX, int theGridY);
    void DrawArtChallenge(Sexy::Graphics * g);
    int DrawBeghouled(Sexy::Graphics * g);
    void PlantAdded(Plant * thePlant);
    int InitZombieWavesSurvival();
    void InitZombieWavesFromList(const ZombieType *theZombieList, int theListLength);
    void InitZombieWaves();
    int WhackAZombiePlaceGraves(int theGraveCount);
    void WhackAZombieSpawning();
    bool UpdateZombieSpawning();
    void GraveDangerSpawnGraveAt(int theGridX, int theGridY);
    int GraveDangerSpawnRandomGrave();
    void SpawnZombieWave();
    int DrawStormFlash(Sexy::Graphics * g, int theTime, int theMaxAmount);
    void DrawRain(Sexy::Graphics * g);
    void DrawStormNight(Sexy::Graphics * g);
    void DrawWeather(Sexy::Graphics * g);
    Zombie * PlayBossEnter();
    GridItem * GetOtherPortal(GridItem * thePortal);
    int UpdatePortal(GridItem * thePortal);
    GridItem * GetPortalAt(int theGridX, int theGridY);
    GridItem * GetPortalToLeft(int theGridX, int theGridY);
    int GetPortalDistanceToMower(int theGridY);
    float PortalCombatRowSpawnWeight(int theGridY);
    GridItem * GetPortalToRight(int theGridX, int theGridY);
    int CanTargetZombieWithPortals(Plant * thePlant, Zombie * theZombie);
    int BeghouledShuffle();
    int BeghouledCanClearCrater();
    Challenge * BeghouledUpdateCraters();
    Challenge * BeghouledClearCrater(int theCount);
    void ZombieAtePlant(Zombie * theZombie, Plant * thePlant);
    int BeghouledPacketClicked(SeedPacket * thePacket);
    void BeghouledScore(int theGridX, int theGridY, int theNumPlants, bool theIsHorizontal);
    void BeghouledRemoveMatches(BeghouledBoardState *theState);
    void UpdateBeghouled();
    Zombie * ZombiquariumSpawnSnorkel();
    int ZombiquariumPacketClicked(SeedPacket * thePacket);
    void ZombiquariumUpdate();
    Plant * ShovelAddWallnuts();
    void ScaryPotterChangePotType(GridItemState thePotType, int theCount);
    void ScaryPotterDontPlaceInCol(int theCol, TodWeightedGridArray *theGrid, int theGridCount);
    void ScaryPotterFillColumnWithPlant(int theCol, SeedType theSeedType, TodWeightedGridArray *theGrid, int theGridCount);
    void ScaryPotterStart();
    int ScaryPotterIsCompleted();
    void ScaryPotterMalletPot(GridItem * thePot);
    int ScaryPotterCountPots();
    static int ScaryPotterCountSunInPot(GridItem * thePot);
    bool PuzzleIsAwardStage();
    void PuzzlePhaseComplete(int theGridX, int theGridY);
    void ScaryPotterOpenPot(GridItem * thePot);
    void ScaryPotterUpdate();
    void ScaryPotterJackExplode(int thePosX, int thePosY);
    void PuzzleNextStageClear();
    // _ZN9Challenge27IZombieSeedTypeToZombieTypeE8SeedType @0x18380c -- the body
    // opens with `CMP R0,#0x3E` on the SeedType value itself, so there is no
    // receiver and the function is STATIC.
    static ZombieType IZombieSeedTypeToZombieType(SeedType theSeedType);
    void IZombiePlaceZombie(ZombieType theZombieType, int theGridX, int theGridY);
    void IZombieSetupPlant(Plant * thePlant);
    void IZombiePlacePlantInSquare(SeedType theSeedType, int theGridX, int theGridY);
    void IZombiePlacePlants(SeedType theSeedType, int theCount, int theGridY);
    void IZombieStart();
    void IZombieUpdate();
    static int IsZombieSeedType(SeedType theSeedType);
    PlantingReason CanPlantAt(int theGridX, int theGridY, SeedType theSeedType);
    void IZombieMouseDownWithZombie(int theX, int theY, int theClickCount, int thePlayerIndex);
    static int IsMPSeedType(SeedType theSeedType);
    void IZombieSetPlantFilterEffect(Plant * thePlant, FilterEffect theFilterEffect);
    void IZombieDrawPlant(Sexy::Graphics * g, Plant * thePlant);
    GridItem * IZombieGetBrainTarget(Zombie * theZombie);
    void IZombieScoreBrain(GridItem * theBrain);
    bool IZombieEatBrain(Zombie * theZombie);
    void IZombiePlantDropRemainingSun(Plant * thePlant);
    void IZombieSquishBrain(GridItem * theBrain);
    int SquirrelCountUncaught();
    void SquirrelChew(GridItem * theSquirrel);
    void SquirrelPeek(GridItem * theSquirrel);
    void SquirrelFound(GridItem * theSquirrel);
    void SquirrelUpdateOne(GridItem * theSquirrel);
    void SquirrelUpdate();
    void UpdateRain();
    void LastStandUpate();
    void LastStandCompletedStage();
    void WhackAZombieUpdate();
    int TreeOfWisdomGetSize();
    void TreeOfWisdomDraw(Sexy::Graphics * g);
    void TreeOfWisdomRebuildHelpbar(bool theFromDialog);
    void TreeOfWisdomInit();
    void TreeOfWisdomGrow();
    void TreeOfWisdomToolUpdate(GridItem * theZenTool);
    int TreeOfWisdomBabble();
    int TreeOfWisdomGiveWisdom();
    int TreeOfWisdomSayRepeat();
    void TreeOfWisdomUpdate();
    Reanimation * TreeOfWisdomLeave();
    void TreeOfWisdomNextGarden();
    void TreeOfWisdomOpenStore();
    int TreeOfWisdomHitTest(int theX, int theY, HitResult *theHitResult);
    int TreeOfWisdomCanFeed();
    int HeavyWeaponStart();
    void HeavyWeaponPacketClicked(SeedPacket * thePacket);
    int HeavyWeaponButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags);
    int GameButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags);
    void DrawHeavyWeapon(Sexy::Graphics * g);
    Reanimation * DrawTank(Sexy::Graphics * g);
    void HeavyWeaponFire(float theDirX, float theDirY);
    void HeavyWeaponUpdate();
    void PortalStart();
    TodParticleSystem * MoveAPortal();
    void UpdatePortalCombat();
    void Update();
    int ZombiquariumDropBrain(int theX, int theY);
    int ZombiquariumMouseDown(int theX, int theY);
    int ScaryPotterPlacePot(int thePotType, int theZombieType, int theSeedType, int theCount, TodWeightedGridArray *theGrid, int theGridCount);
    int ScaryPotterPopulate();
    void AdvanceCrazyDaveDialog();
    bool MouseDown(int theX, int theY, int theClickCount, HitResult *theHitResult, int thePlayerIndex);
    void IZombieInitLevel();
    void InitLevel();
    int SquirrelStart();
    void StartLevel();
    void TreeOfWisdomFertilize();
    Challenge * TreeOfWisdomTool(int theMouseX, int theMouseY);
    Challenge();
    void DrawSlotMachine(Sexy::Graphics * g);
    void DrawBackdrop(Sexy::Graphics * g);
};

