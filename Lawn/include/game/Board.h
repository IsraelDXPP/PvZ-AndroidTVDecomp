// Board -- reconstructed from libGameMain.so.
//
// Inheritance chain (from the binary's RTTI / ZTV5Board @0x680058):
//   class Board : public Sexy::Widget, public Sexy::ButtonListener, public SyncObject
// The three sub-objects sit at fixed offsets, which the ZTV's
// offset-to-top words confirm:
//   +0x000  Sexy::Widget            (0x100 bytes; primary base, holds the vptr)
//   +0x100  Sexy::ButtonListener    (0x004 bytes; secondary vtable, offset-to-top = -0x100)
//   +0x104  SyncObject              (0x010 bytes; secondary vtable, offset-to-top = -0x104)
//   +0x114  Board's own members
// The 0x114 figure is independently confirmed by the transcribed bodies, which
// reach the LawnApp* through `*(LawnApp**)this + 276` (Board.cpp:130,
// 226, 240, ...). 276 == 0x114.
//
// Size 0x591C. Evidence: the authoritative member table in
// PvZ\Lawn\Board\Board.h ends with `bool mJacksonDanceMode` at +0x5918; a bool
// there rounds the object up to 0x591C. The previous 0x5918 came from the
// transcriber's "largest this-relative offset" heuristic, which stops one byte
// short of the tail -- every store to +0x5918 was an out-of-bounds write.
//
// vtable order exactly as the binary holds it (125 slots; slot numbers are the
// real byte offset / 4). `virtual` is declared below in this order, and only for
// the slots whose target is a Board:: implementation -- the other slots still
// point at the base-class implementation, so re-declaring them here would
// redirect them to the wrong function.
//
//   [  0] 0x164360  Board::~Board                        [  1] 0x16470c  Board::~Board
//   [  2] 0x3aced8  Sexy::WidgetContainer::GetRect        [  3] 0x3ad324  Sexy::WidgetContainer::Intersects
//   [  4] 0x3ae4c0  Sexy::WidgetContainer::SortWidgets    [  5] 0x3ae52c  Sexy::WidgetContainer::SortWidgets
//   [  6] 0x3ad7e8  Sexy::WidgetContainer::AddWidget     [  7] 0x3adbb8  Sexy::WidgetContainer::RemoveWidget
//   [  8] 0x3acef0  Sexy::WidgetContainer::HasWidget     [  9] 0x3acf0c  Sexy::WidgetContainer::FindWidget
//   [ 10] 0x3ad31c  Sexy::WidgetContainer::DisableWidget [ 11] 0x3ace68  Sexy::WidgetContainer::RemoveAllWidgets
//   [ 12] 0x3ad320  Sexy::WidgetContainer::SetFocus      [ 13] 0x3ad750  Sexy::WidgetContainer::IsBelow
//   [ 14] 0x3acf3c  Sexy::WidgetContainer::MarkAllDirty  [ 15] 0x3ad854  Sexy::WidgetContainer::BringToFront
//   [ 16] 0x3ad8b0  Sexy::WidgetContainer::BringToBack   [ 17] 0x3ad910  Sexy::WidgetContainer::PutBehind
//   [ 18] 0x3ad994  Sexy::WidgetContainer::PutInfront    [ 19] 0x3acf68  Sexy::WidgetContainer::GetAbsPos
//   [ 20] 0x3acfa8  WidgetContainer::GetAbsPosInManager  [ 21] 0x3acfb8  WidgetContainer::GetAbsPosInScreen
//   [ 22] 0x3acff0  Sexy::WidgetContainer::GetCenter     [ 23] 0x3ad014  Sexy::WidgetContainer::GetAbsCenter
//   [ 24] 0x3ad048  WidgetContainer::GetAbsCenterInScreen[ 25] 0x3ad10c  Sexy::WidgetContainer::MarkDirty
//   [ 26] 0x3ad128  Sexy::WidgetContainer::MarkDirtyFull [ 27] 0x3ad3f8  Sexy::WidgetContainer::MarkDirtyFull
//   [ 28] 0x3ad374  Sexy::WidgetContainer::MarkDirty     [ 29] 0x3ad07c  Sexy::WidgetContainer::AddedToManager
//   [ 30] 0x164a38  Board::RemovedFromManager            [ 31] 0x164a88  Board::Update
//   [ 32] 0x3ad14c  Sexy::WidgetContainer::UpdateAll     [ 33] 0x3ae7d4  Sexy::Widget::UpdateF
//   [ 34] 0x3ad22c  Sexy::WidgetContainer::UpdateFAll    [ 35] 0x3af470  Sexy::Widget::DrawPre
//   [ 36] 0x165ccc  Board::Draw                           [ 37] 0x3ae73c  Sexy::Widget::DrawOther
//   [ 38] 0x3ae5a8  Sexy::WidgetContainer::DrawAll       [ 39] 0x3ad2e8  WidgetContainer::SysColorChangedAll
//   [ 40] 0x3ad2e4  WidgetContainer::SysColorChanged     [ 41] 0x3ae730  Sexy::Widget::OrderInManagerChanged
//   [ 42] 0x3aec98  Sexy::Widget::SetVisible             [ 43] 0x3aec50  Sexy::Widget::SetColors
//   [ 44] 0x3aed24  Sexy::Widget::SetColors              [ 45] 0x3b0cd0  Sexy::Widget::SetColor
//   [ 46] 0x3aed78  Sexy::Widget::GetColor               [ 47] 0x3af374  Sexy::Widget::GetColor
//   [ 48] 0x3aede0  Sexy::Widget::SetDisabled            [ 49] 0x3aee40  Sexy::Widget::ShowFinger
//   [ 50] 0x3aecc8  Sexy::Widget::Resize                  [ 51] 0x3ae750  Sexy::Widget::Resize
//   [ 52] 0x151874  Board::Move                           [ 53] 0x3ae77c  Sexy::Widget::WantsFocus
//   [ 54] 0x15181c  Board::DrawOverlay                    [ 55] 0x3ae744  Sexy::Widget::DrawOverlay
//   [ 56] 0x3af414  Sexy::Widget::SetDefaultFocus         [ 57] 0x3b00d0  Sexy::Widget::SetFocus
//   [ 58] 0x3ae7d8  Sexy::Widget::SetFocusLink            [ 59] 0x3ae7e4  Sexy::Widget::SetFocusLinks
//   [ 60] 0x3ae800  Sexy::Widget::FocusDirectionHint      [ 61] 0x1664c0  Board::GotFocus
//   [ 62] 0x3af23c  Sexy::Widget::LostFocus               [ 63] 0x3ae784  Sexy::Widget::HasWindowFocus
//   [ 64] 0x3af28c  Sexy::Widget::SetWindowFocus          [ 65] 0x3ae78c  Sexy::Widget::GotWindowFocus
//   [ 66] 0x3ae7b0  Sexy::Widget::LostWindowFocus         [ 67] 0x16694c  Board::KeyChar
//   [ 68] 0x3af98c  Sexy::Widget::KeyUnicode             [ 69] 0x3ae804  Sexy::Widget::KeyDown
//   [ 70] 0x3ae820  Sexy::Widget::KeyUp                  [ 71] 0x15df24  Board::KeyDown
//   [ 72] 0x15d4d4  Board::KeyUp                         [ 73] 0x3ae83c  Sexy::Widget::MouseEnter
//   [ 74] 0x3ae840  Sexy::Widget::MouseLeave             [ 75] 0x151878  Board::MouseMove
//   [ 76] 0x151870  Board::MouseDown                     [ 77] 0x3ae848  Sexy::Widget::MouseDown
//   [ 78] 0x3ae84c  Sexy::Widget::MouseUp                [ 79] 0x1518b0  Board::MouseUp
//   [ 80] 0x3ae850  Sexy::Widget::MouseUp                [ 81] 0x151894  Board::MouseDrag
//   [ 82] 0x3ae858  Sexy::Widget::MouseWheel            [ 83] 0x1382f0  Sexy::Widget::MouseWheel
//   [ 84] 0x3ae85c  Sexy::Widget::TouchEnter             [ 85] 0x3ae860  Sexy::Widget::TouchLeave
//   [ 86] 0x3ae864  Sexy::Widget::TouchDown              [ 87] 0x3af348  Sexy::Widget::TouchMove
//   [ 88] 0x3ae878  Sexy::Widget::TouchUp                [ 89] 0x3ae88c  Sexy::Widget::TouchCancel
//   [ 90] 0x3ae890  Sexy::Widget::TouchEvents            [ 91] 0x3aee54  Sexy::Widget::TouchDown
//   [ 92] 0x3aee94  Sexy::Widget::TouchMove              [ 93] 0x3aeed4  Sexy::Widget::TouchUp
//   [ 94] 0x3aef14  Sexy::Widget::TouchCancel            [ 95] 0x3af39c  Sexy::Widget::TouchBegan
//   [ 96] 0x3ae8fc  Sexy::Widget::TouchMoved             [ 97] 0x3ae9d8  Sexy::Widget::TouchEnded
//   [ 98] 0x3aeab8  Sexy::Widget::TouchCanceled          [ 99] 0x3ae8d4  Sexy::Widget::TouchBegan
//   [100] 0x3ae9b0  Sexy::Widget::TouchMoved            [101] 0x3aea90  Sexy::Widget::TouchEnded
//   [102] 0x3aeb7c  Sexy::Widget::TouchCanceled         [103] 0x3aeb78  Sexy::Widget::TouchesCanceled
//   [104] 0x3af7c8  Sexy::Widget::AxisMoved             [105] 0x3aeba4  Sexy::Widget::UserEvent
//   [106] 0x3ae734  Sexy::Widget::IsPointVisible        [107] 0x3afb28  Sexy::Widget::IsOnScreen
//   [108] 0x3aef54  Sexy::Widget::WriteCenteredLine     [109] 0x3aefb0  Sexy::Widget::WriteCenteredLine
//   [110] 0x3af08c  Sexy::Widget::WriteString           [111] 0x3af0e4  Sexy::Widget::WriteWordWrapped
//   [112] 0x3af134  Sexy::Widget::GetWordWrappedHeight  [113] 0x3aeba8  Sexy::Widget::GetNumDigits
//   [114] 0x3af14c  Sexy::Widget::WriteNumberFromStrip  [115] 0x3aebc4  Sexy::Widget::Contains
//   [116] 0x3aebf4  Sexy::Widget::GetInsetRect         [117] 0x3b0300  Sexy::Widget::FindFocusableWidget
//   [118] 0x3af810  Sexy::Widget::FindClosest           [119] 0x3afbac  Sexy::Widget::OnArrowKeys
//   [120] 0x3af9dc  Sexy::Widget::OnKeyReturn           [121] 0x3afa38  Sexy::Widget::OnKeyEscape
//   [122] 0x15183c  Board::ButtonMouseEnter             [123] 0x151848  Board::ButtonMouseLeave
//   [124] 0x151854  Board::ButtonPress
//
// MSVC x86 has ONE virtual-destructor slot where the Itanium ABI used by ARM has
// two (D1 complete + D0 deleting), so a faithful MSVC vtable holds 124 entries.
// Only the declaration ORDER matters; the slot numbers above are the ARM ones.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "BaseGamepadControls.h"
#include "ButtonListener.h"
#include "Challenge.h"
#include "Coin.h"
#include "CursorObject.h"
#include "CursorPreview.h"
#include "CutScene.h"
#include "DaveHelp.h"
#include "EffectSystem.h"
#include "Font.h"
#include "GameObject.h"
#include "GameOverDialog.h"
#include "GamepadApp.h"
// GamepadControls is only ever held here as a pointer (+0x230), so it is
// forward-declared instead of included. Including it closed a cycle
//     GamepadControls.h -> Board.h -> ZenGardenControls.h / TreeOfWisdomControls.h
//         -> GamepadControls.h
// in which the two derived classes were declared while GamepadControls was still
// incomplete; that raised C2504 and collapsed their size assertions to 1. Board.cpp
// includes Board\GamepadControls.h itself, so nothing is lost.
class GamepadControls;
#include "Graphics.h"
#include "GridItem.h"
#include "HelpBarWidget.h"
#include "HyperlinkWidget.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnMower.h"
#include "LawnPlayerInfo.h"
#include "Level.h"
#include "LevelStats.h"
#include "MTRand.h"
#include "MemoryImage.h"
#include "MessageWidget.h"
#include "PerfTimer.h"
#include "PoolEffect.h"
#include "PottedPlant.h"
#include "Projectile.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "ReanimatorDefinition.h"
#include "ResourceManager.h"
#include "SeedBank.h"
#include "SeedPacket.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "SharedImageRef.h"
#include "ShopSeedPacket.h"
#include "StoreScreen.h"
#include "TodFoley.h"
#include "TodHesitationBracket.h"
#include "TodParticleDefinition.h"
#include "TodParticleSystem.h"
#include "ToolTipWidget.h"
#include "Trail.h"
#include "TrailDefinition.h"
#include "TrailHolder.h"
#include "TypingCheck.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "XMLWriter.h"
#include "ZenGarden.h"
#include "Zombie.h"
#include "DataArray.h"
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>

// Board's file also owns these helper types (PvZ\Lawn\Board\Board.h declares them
// in the same header). They are declared here because the transcribed bodies need
// the complete types.
inline constexpr int MAX_GRID_SIZE_X = 9;
inline constexpr int MAX_GRID_SIZE_Y = 6;
inline constexpr int MAX_ZOMBIES_IN_WAVE = 50;
inline constexpr int MAX_ZOMBIE_WAVES = 100;
inline constexpr int MAX_GRAVE_STONES = MAX_GRID_SIZE_X * MAX_GRID_SIZE_Y;
inline constexpr int MAX_POOL_GRID_SIZE = 10;
inline constexpr int MAX_RENDER_ITEMS = 2048;
inline constexpr int PROGRESS_METER_COUNTER = 150;

// HitResult -- what MouseHitTest fills in (authoritative Board.h:68). It now
// lives in Game\HitResult.h, because BaseGamepadControls::GetHitResult returns
// one by value and this header includes BaseGamepadControls.h.
#include "HitResult.h"

// 12-byte render-queue record used by Board::DrawGameObjects. The type word is
// a RenderObjectType. The third word is an object pointer or, for ice and boss
// parts, an int.
struct BoardRenderItem
{
    int mType;
    int mOrder;
    union
    {
        GameObject *mObject;
        LawnMower *mMower;
        GridItem *mGridItem;
        Reanimation *mReanimation;
        TodParticleSystem *mParticle;
        int mValue;
    };
};

// ZombiePicker -- the per-wave zombie draw state (authoritative Board.h:94).
// Both counters are indexed by ZombieType, and the binary's own
// Board::PutZombieInWave reaches mAllWavesZombieTypeCount at +0x90, i.e. word 36
// == 2 + NUM_ZOMBIE_TYPES, so the arrays hold exactly NUM_ZOMBIE_TYPES entries.
struct ZombiePicker
{
    int mZombieCount;
    int mZombiePoints;
    int mZombieTypeCount[NUM_ZOMBIE_TYPES];
    int mAllWavesZombieTypeCount[NUM_ZOMBIE_TYPES];
};

// PlantsOnLawn -- filled by GetPlantsOnLawn (authoritative Board.h:104).
struct PlantsOnLawn
{
    Plant *mUnderPlant;
    Plant *mPumpkinPlant;
    Plant *mFlyingPlant;
    Plant *mNormalPlant;
};

// BungeeDropGrid -- filled by SetupBungeeDrop, consumed by BungeeDropZombie.
// 54 weighted cells (0x30C bytes) plus the count at +0x288, which is exactly the
// layout the binary uses: Board::SpawnZombiesFromSky reads the count from its
// local buffer at +648, and the mangled name of SetupBungeeDrop is
// _ZN5Board15SetupBungeeDropEP14BungeeDropGrid.
struct BungeeDropGrid
{
    TodWeightedGridArray mGridArray[MAX_GRID_SIZE_X * MAX_GRID_SIZE_Y];
    int mGridArrayCount;
};

// TouchState -- the touch-routing state machine (authoritative Board.h:120).
enum TouchState
{
    TOUCHSTATE_NONE = 0,
    TOUCHSTATE_SEED_BANK = 1,
    TOUCHSTATE_SHOVEL_RECT = 2,
    TOUCHSTATE_BUTTER_RECT = 3,
    TOUCHSTATE_BOARD = 4,
    TOUCHSTATE_BOARD_MOVED_FROM_SEED_BANK = 5,
    TOUCHSTATE_BOARD_MOVED_FROM_SHOVEL_RECT = 6,
    TOUCHSTATE_BOARD_MOVED_FROM_BUTTER_RECT = 7,
    TOUCHSTATE_VALID_COBCONON = 8,
    TOUCHSTATE_USEFUL_SEED_PACKET = 9,
    TOUCHSTATE_UNUSED = 10,
    TOUCHSTATE_HEAVY_WEAPON = 11,
    TOUCHSTATE_PICKING_SOMETHING = 12,
    TOUCHSTATE_ZEN_GARDEN_TOOLS = 13,
    TOUCHSTATE_BOARD_MOVED_FROM_ZEN_GARDEN_TOOLS = 14,
    TOUCHSTATE_VALID_COBCONON_SECOND = 15,
};

class Board : public Sexy::Widget, public Sexy::ButtonListener, public SyncObject
{
public:
    // ------------------------------------------------------------------
    // The virtual slots this class actually implements, in vtable order.
    // ------------------------------------------------------------------
    virtual ~Board(); // [0],[1] 0x000/0x004

    // The return types below are the ones Sexy::Widget / Sexy::WidgetContainer /
    // Sexy::ButtonListener declare; a derived declaration that matches a base
    // virtual's name and parameter list IS an override whether or not the
    // `virtual` keyword is repeated, so a different return type would be C2555.
    // TODO(recon): the transcribed bodies were written against `int`/`_DWORD*`
    // returns for several of these, so Game\Board.cpp must be retyped in the
    // same pass.
    virtual void RemovedFromManager(Sexy::WidgetManager * theManager); // [ 30] 0x078
    virtual void Update();                                    // [ 31] 0x07C
    virtual void Draw(Sexy::Graphics * g);                    // [ 36] 0x090
    virtual void Move(int theX, int theY);                    // [ 52] 0x0D0
    virtual void DrawOverlay(Sexy::Graphics * g);             // [ 54] 0x0D8
    virtual void GotFocus();                                  // [ 61] 0x0F4
    virtual int KeyChar(char theChar);                        // [ 67] 0x10C
    virtual int KeyDown(Sexy::KeyCode theKey);                // [ 71] 0x11C
    virtual int KeyUp(Sexy::KeyCode theKey);                  // [ 72] 0x120
    virtual void MouseMove(int theX, int theY);               // [ 75] 0x12C
    virtual void MouseDown(int theX, int theY, int theClickCount); // [ 76] 0x130
    virtual void MouseUp(int theX, int theY, int theClickCount);   // [ 79] 0x13C
    virtual void MouseDrag(int theX, int theY);               // [ 81] 0x144
    // Slots [122]/[123]/[124] are the Sexy::ButtonListener overrides. The
    // binary's order is MouseEnter, MouseLeave, Press -- keep it.
    virtual void ButtonMouseEnter(int theId); // [122] 0x1E8
    virtual void ButtonMouseLeave(int theId); // [123] 0x1EC
    virtual void ButtonPress(int theId);      // [124] 0x1F0

    // ------------------------------------------------------------------
    // Member area, +0x114 .. +0x591B.
    // `DataArray<T, Slot>` is seven words. The block pointer is the slot type
    // (object plus the id word), so a reader indexes mBlock with no cast.
    // The slot structs themselves are completed at the end of each item header.
    // ------------------------------------------------------------------

    // Plant indexes and loaded resource names are standard containers. The TV
    // binary stored each as a 24-byte libstdc++ tree; MSVC's set is larger, so
    // members after mPumpkinTree no longer sit at the old board offsets.
    LawnApp *mApp;                       // +0x114
    DataArray<Zombie, ZombieSlot> mZombies;         // +0x118 (block 0x118, count 0x11C, cap 0x120)
    DataArray<Plant, PlantSlot> mPlants;           // +0x134
    DataArray<Projectile, ProjectileSlot> mProjectiles; // +0x150
    DataArray<Coin, CoinSlot> mCoins;             // +0x16C
    DataArray<LawnMower, LawnMowerSlot> mLawnMowers;   // +0x188 (stride 88; see Board.cpp:3309)
    DataArray<GridItem, GridItemSlot> mGridItems;     // +0x1A4 (stride 0x130; Board.cpp:13722)
    std::set<Plant *> mTangleKelpTree;
    std::set<Plant *> mFlowerPotTree;
    std::set<Plant *> mPumpkinTree;
    MessageWidget *mAdvice;
    SeedBank *mSeedBank[2];
    // Set while a mouse-up must be swallowed. The next MouseUp clears it and returns.
    int mSwallowMouseUp;
    // Cleared at the start of every level. Nothing else reads it.
    std::map<Sexy::String, int> mLevelStringMap;
    GamepadControls *mGamepadControls[2]; // +0x230
    CursorObject *mCursorObject[2];      // +0x238
    CursorPreview *mCursorPreview[2];    // +0x240
    ToolTipWidget *mToolTip;             // +0x248
    Sexy::Font *mDebugFont;              // +0x24C
    CutScene *mCutScene;                 // +0x250 (Board.cpp:1750 reads this + 0)
    Challenge *mChallenge;               // +0x254 (Board.cpp:1418 reads this + 0x84)
    // TryToExit sets this. Update calls FinishExit once saving and the exit dialog are done.
    bool mExitRequested;                 // +0x258
    bool mPaused;                        // +0x259 (Board.cpp:1226/1749/18779/...)
    GridSquareType mGridSquareType[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y]; // +0x25C
    int mGridCelLook[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y];               // +0x334
    int mGridCelOffset[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y][2];          // +0x40C
    int mGridCelFog[MAX_GRID_SIZE_X][MAX_GRID_SIZE_Y + 1];            // +0x5BC
    bool mEnableGraveStones;             // +0x6B8 (Board.cpp:9816/17064/18217)
    int mSpecialGraveStoneX;             // +0x6BC
    int mSpecialGraveStoneY;             // +0x6C0
    float mFogOffset;                    // +0x6C4
    int mOffsetMoved;                    // +0x6C8
    ReanimationID mCoverLayerAnimIDs[7]; // +0x6CC
    int mFogBlownCountDown;              // +0x6E8
    PlantRowType mPlantRow[MAX_GRID_SIZE_Y];      // +0x6EC
    int mWaveRowGotLawnMowered[MAX_GRID_SIZE_Y];  // +0x704
    int mBonusLawnMowersRemaining;       // +0x71C
    int mIceMinX[MAX_GRID_SIZE_Y];       // +0x720
    int mIceTimer[MAX_GRID_SIZE_Y];      // +0x738
    ParticleSystemID mIceParticleID[MAX_GRID_SIZE_Y]; // +0x750
    // Six smooth-pick rows. Each record is 16 bytes, so the table is still
    // the 24 words at +0x768.
    TodSmoothArray mRowPickingArray[MAX_GRID_SIZE_Y]; // +0x768
    ZombieType mZombiesInWave[MAX_ZOMBIE_WAVES][MAX_ZOMBIES_IN_WAVE]; // +0x7C8 (20000 bytes)
    bool mZombieAllowed[100];            // +0x55E8 (Challenge.cpp:3597 reads board + 21992)
    int mSunCountDown;                   // +0x564C
    int mNumSunsFallen;                  // +0x5650
    int mShakeCounter;                   // +0x5654
    int mShakeAmountX;                   // +0x5658
    int mShakeAmountY;                   // +0x565C
    BackgroundType mBackground;          // +0x5660 (Board.cpp:3532 compares with 7)
    int mLevel;                          // +0x5664 (Board.cpp:524/959 compares with 1)
    int mSodPosition;                    // +0x5668
    int mPrevMouseX;                     // +0x566C
    int mPrevMouseY;                     // +0x5670
    int mSunMoney1;                      // +0x5674
    int mSunMoney2;                      // +0x5678
    int mDeathMoney;                     // +0x567C
    int mNumWaves;                       // +0x5680
    int mMainCounter;                    // +0x5684
    int mEffectCounter;                  // +0x5688
    int mDrawCount;                      // +0x568C
    int mRiseFromGraveCounter;           // +0x5690
    int mOutOfMoneyCounter;              // +0x5694
    int mCurrentWave;                    // +0x5698
    int mTotalSpawnedWaves;              // +0x569C
    TutorialState mTutorialState;        // +0x56A0
    ParticleSystemID mTutorialParticleID; // +0x56A4 (passed to ParticleTryToGet/ParticleGetID)
    int mTutorialTimer;                  // +0x56A8
    int mLastBungeeWave;                 // +0x56AC
    int mZombieHealthToNextWave;         // +0x56B0
    int mZombieHealthWaveStart;          // +0x56B4
    int mZombieCountDown;                // +0x56B8
    int mZombieCountDownStart;           // +0x56BC
    int mHugeWaveCountDown;              // +0x56C0
    bool mHelpDisplayed[72];             // +0x56C4
    AdviceType mHelpIndex;               // +0x570C
    bool mFinalBossKilled;               // +0x5710
    bool mShowShovel;                    // +0x5711
    bool mShowButter;                    // +0x5712
    bool mShowHammer;                    // +0x5713
    int mCoinBankFadeCount;              // +0x5714
    DebugTextMode mDebugTextMode;        // +0x5718
    bool mLevelComplete;                 // +0x571C
    bool mNewWallNutAndSunFlowerAndChomperOnly; // +0x571D
    char mNewPeaShooterCount;            // +0x571E
    int mBoardFadeOutCounter;            // +0x5720
    int mNextSurvivalStageCounter;       // +0x5724 (Board.cpp:2479)
    int mScoreNextMowerCounter;          // +0x5728
    bool mLevelAwardSpawned;             // +0x572C (Board.cpp:1440/12538/12672/18194)
    int mProgressMeterWidth;             // +0x5730
    int mFlagRaiseCounter;               // +0x5734
    int mIceTrapCounter;                 // +0x5738
    int mBoardRandSeed;                  // +0x573C
    ParticleSystemID mPoolSparklyParticleID; // +0x5740
    ReanimationID mFwooshID[MAX_GRID_SIZE_Y][12]; // +0x5744
    int mFwooshCountDown;                // +0x5864
    int mTimeStopCounter;                // +0x5868
    bool mDroppedFirstCoin;              // +0x586C (Board.cpp:8228/8363/18215)
    int mFinalWaveSoundCounter;          // +0x5870
    int mCobCannonCursorDelayCounter;    // +0x5874
    int mCobCannonMouseX;                // +0x5878
    int mCobCannonMouseY;                // +0x587C
    bool mKilledYeti;                    // +0x5880
    bool mMustacheMode;                  // +0x5881
    bool mSuperMowerMode;                // +0x5882
    bool mFutureMode;                    // +0x5883
    bool mPinataMode;                    // +0x5884
    bool mDanceMode;                     // +0x5885
    bool mDaisyMode;                     // +0x5886
    bool mSukhbirMode;                   // +0x5887
    int mPrevBoardResult;                // +0x5888
    int mTriggeredLawnMowers;            // +0x588C
    int mPlayTimeActiveLevel;            // +0x5890
    int mPlayTimeInactiveLevel;          // +0x5894
    int mMaxSunPlants;                   // +0x5898
    int mStartDrawTime;                  // +0x589C
    int mIntervalDrawTime;               // +0x58A0
    int mIntervalDrawCountStart;         // +0x58A4
    float mMinFPS;                       // +0x58A8
    int mPreloadTime;                    // +0x58AC
    int mGameID;                         // +0x58B0
    int mGravesCleared;                  // +0x58B4
    int mPlantsEaten;                    // +0x58B8
    int mPlantsShoveled;                 // +0x58BC
    int mCoinsCollected;                 // +0x58C0
    int mDiamondsCollected;              // +0x58C4
    int mPottedPlantsCollected;          // +0x58C8
    int mChocolateCollected;             // +0x58CC
    bool mPeaShooterUsed;                // +0x58D0
    bool mCatapultPlantsUsed;            // +0x58D1
    int mCollectedCoinStreak;            // +0x58D4 (Board.cpp:18884)
    // The "2P joined" banner. Update counts the flashes still left, times the
    // current flash, and flips which half of the blink is showing.
    int mSecondPlayerBannerCount;        // +0x58D8
    bool mSecondPlayerBannerVisible;     // +0x58DC
    float mSecondPlayerBannerTimer;      // +0x58E0
    // homura::Storage<pvzstl::string>: a libstdc++ COW string is one pointer, so
    // the type is size-neutral here -- but it IS a string (Board::InitLevel
    // assigns "[P2_JOIN]" into it), not an int.
    Sexy::String mStringSecondPlayer;    // +0x58E4
    bool mHelpTextDirty;                 // +0x58E8  a new tool in hand redraws the help text
    // PlantsWon and ZombiesWon set this. TryToSaveGame then skips the save.
    bool mLevelFinished = false;         // +0x58E9
    unsigned char mPadding58EA[2];       // +0x58EA
    // Groups LoadResources has already loaded. A second request finds the name.
    std::set<Sexy::String> mLoadedResourceGroups;
    int mBoardInitCounter;               // +0x5904  InitLevel and the constructor clear it
    GameButton *mBoardMenuButton = nullptr;        // +0x5908
    GameButton *mBoardStoreButton = nullptr;       // +0x590C
    void *mShovelWidget = nullptr;                 // +0x5910  TODO(recon): ShovelRedirectWidget*, not reconstructed yet
    void *mReplayControlsWidget = nullptr;         // +0x5914  TODO(recon): ReplayControlsWidget*, not reconstructed yet
    bool mJacksonDanceMode = false;                // +0x5918
    // 0x591C (the tail bool rounds the object up to a 4-byte boundary).

    // ------------------------------------------------------------------
    // The remaining methods. None of these owns a vtable slot.
    // ------------------------------------------------------------------
    // ButtonDepress is NOT declared here. It was, and the note that used to sit
    // in its place already recorded why that was wrong: `void ButtonDepress(int)`
    // matches Sexy::ButtonListener::ButtonDepress, so a same-signature
    // declaration is an implicit override even without the keyword -- which made
    // MSVC emit `?ButtonDepress@Board@@UAEXH@Z` into this class's vtable and then
    // want a definition for it. The binary has no such symbol: the only
    // ButtonDepress in the image is `_ZN4Sexy5Board13ButtonDepressEi` @0x1F8898
    // (plus its `_ZThn256_` thunk), and Game::Board's secondary vtable points
    // that slot at Sexy::ButtonListener's own body. Nothing called the
    // declaration either.
    int DisposeBoard();
    bool NeedSaveGame();
    bool SaveGame(const Sexy::String &theFileName);
    int GetNumWavesPerFlag();
    int IsFlagWave(int theWaveIndex);
    int PutZombieInWave(ZombieType theZombieType, int theWaveIndex, ZombiePicker *thePicker);
    int GetLevelRandSeed();
    int DisplayAdvice(Sexy::String const& theAdvice, MessageStyle theStyle, AdviceType theType);
    int DisplayAdviceAgain(Sexy::String const& theAdvice, MessageStyle theStyle, AdviceType theType);
    int ClearAdvice(AdviceType theType);
    int ClearAdviceImmediately();
    Coin * AddCoin(int theX, int theY, CoinType theType, CoinMotion theMotion);
    Coin * AddCoinHeavyWeapon(int theX, int theY, CoinType theType);
    bool IsPlantInCursor(int thePlayerIndex);
    SeedType GetSeedTypeInCursor(int thePlayerIndex);
    bool IsPoolSquare(int theGridX, int theGridY);
    Plant * NewPlant(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex);
    Plant * GetPumpkinAt(int theGridX, int theGridY);
    Plant * GetFlowerPotAt(int theGridX, int theGridY);
    Projectile * AddProjectile(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theType);
    bool CanZombieSpawnOnLevel(ZombieType theZombieType, int theLevel);
    int PutInMissingZombies(int theWaveIndex, ZombiePicker *thePicker);
    int IsZombieWaveDistributionOk();
    int InitZombieWavesForLevel(int theLevel);
    int GetIntroducedZombieType();
    int PickGraveRisingZombieTypeMP(int theLevel);
    bool IsZombieTypePoolOnly(ZombieType theZombieType);
    int CanAddBobSled();
    int CanAddBobSledMP();
    void UpdateCursor();
    void UpdateGamepad(float theDelta);
    void UpdateToolTip();
    void MouseDownCobcannonFire(int theX, int theY, int thePlayerIndex);
    // TutorialArrowRemove / TutorialArrowShow / ShowCoinBank / Pause /
    // SetupBungeeDrop are all void in the binary: the value the transcription
    // returned was the leftover in r0 (a particle pointer, a loop counter), and
    // no caller reads it.
    void TutorialArrowRemove();
    void TutorialArrowShow(int theX, int theY);
    bool CanInteractWithBoardButtons();
    void ShowCoinBank();
    void Pause(bool thePause);
    void SetupBungeeDrop(BungeeDropGrid *theDrop);
    int NumberZombiesInWave(int theWaveIndex);
    int PuzzleSaveStreak();
    int IsFinalScaryPotterStage();
    bool IsLastStandFinalStage();
    bool IsLastStandStageWithRepick();
    int HasLevelAwardDropped();
    void SetTutorialState(TutorialState theState);
    int PickUpTool(GameObjectType theObjectType, int thePlayerIndex);
    void ClearCursor(int thePlayerIndex);
    int UpdateLayers();
    bool RowCanHaveZombies(int theRow);
    int GetIceZPos(int theRow);
    int HasProgressMeter();
    bool ProgressMeterHasFlags();
    // Both are void in the binary: the value the transcription returned was the
    // leftover in r0 from Graphics::DrawImage, and no caller reads it.
    void DrawHouseDoorBottom(Sexy::Graphics * g);
    void DrawHouseDoorTop(Sexy::Graphics * g);
    int DrawTopRightUI(Sexy::Graphics * g);
    void DrawUICoinBank(Sexy::Graphics * g);
    void DrawFog(Sexy::Graphics * g);
    Reanimation * UpdateCoverLayer();
    void DrawCoverLayer(Sexy::Graphics * g, int theRow);
    int IsScaryPotterDaveTalking();
    void SetPinataMode(bool theEnable);
    int SetDaisyMode(bool theEnable);
    int SetSukhbirMode(bool theEnable);
    Board * AddSunMoney(int theAmount, int thePlayerIndex);
    void AddDeathMoney(int theAmount);
    int TakeDeathMoney(int theAmount);
    int HasConveyorBeltSeedBank(bool thePlayer2);
    bool ChooseSeedsOnCurrentLevel();
    int GetNumSeedsInBank(bool thePlayer2);
    unsigned int StageIsNight();
    void UpdateSunSpawning();
    int StageHasGraveStones();
    int PickGraveRisingZombieType(int theLevel);
    bool StageHasPool();
    void DrawBackdrop(Sexy::Graphics * g);
    bool StageHasRoof();
    bool StageHas6Rows();
    void InitLawnMowers();
    bool StageHasZombieWalkInFromRight();
    int GetSeedPacketPositionX(int theIndex, int thePlayerIndex, bool theVersus);
    int GetSeedBankExtraWidth();
    // The three button-rect angle helpers all RETURN A TRect BY VALUE, so the
    // sret buffer occupies r0 and is therefore the receiver; the FIRST PARAMETER
    // is the real Board object. Disassembly of 0x15463c:
    //     MOV R4,R0        ; r0 = the result buffer  (IDA calls it `this`)
    //     MOV R0,R1        ; r1 = the real Board     (IDA calls it `a2`)
    //     MOV R5,R1
    //     BL  Board::GetSeedBankExtraWidth
    //     STM.W R4,{R0,R3} ; store into the result buffer
    //     LDR.W R0,[R5,#0x114]   ; the Board's LawnApp* lives at +0x114
    // IDA types that first parameter `LawnApp **` because of the +0x114 access,
    // but it is the Board itself. The mangled names carry no explicit parameter
    // for the first two, and one GameObjectType for the third:
    //     _ZN5Board19GetShovelButtonRectEv            @0x15463c
    //     _ZN5Board19GetButterButtonRectEv            @0x15468c
    //     _ZN5Board16GetZenButtonRectE14GameObjectType @0x15c500
    // Shovel/Butter are fixed: they return Sexy::Rect by value and take no
    // arguments. GetZenButtonRect below still carries the old sret spelling and
    // needs the same treatment when its body is converted.
    Sexy::Rect GetShovelButtonRect();
    Sexy::Rect GetButterButtonRect();
    int OffsetYForPlanting(int & theY, SeedType theSeedType);
    int PixelToGridX(int theX, int theY);
    int PlantingPixelToGridX(int theX, int theY, SeedType theSeedType);
    int PixelToGridXKeepOnBoard(int theX, int theY);
    bool IsIceAt(int theGridX, int theGridY);
    int PixelToGridY(int theX, int theY);
    int PixelToGridYKeepOnBoard(int theX, int theY);
    int GridToPixelX(int theGridX, int theGridY);
    int GridToPixelY(int theGridX, int theGridY);
    void DoPlantingEffects(int theGridX, int theGridY, Plant * thePlant);
    void UpdateIce();
    float GetPosYBasedOnRow(float thePosX, int theRow);
    int DrawIce(Sexy::Graphics * g, int theRow);
    int GridCellWidth(int theGridX, int theGridY);
    int GridCellHeight(int theGridX, int theGridY);
    ZombieID ZombieGetID(Zombie * theZombie);
    Zombie * ZombieGet(ZombieID theId);
    Zombie * ZombieTryToGet(ZombieID theId);
    void ShakeBoard(int theShakeAmountX, int theShakeAmountY);
    int IterateZombies(Zombie *& theZombie);
    int AreEnemyZombiesOnScreen();
    int CountZombiesOnScreen();
    int RemoveAllZombies();
    int RemoveZombiesForRepick();
    int RemoveCutsceneZombies();
    Zombie * ZombieHitTest(int theX, int theY, bool theInRow);
    int TotalZombiesHealthInWave(int theWaveIndex);
    int StopAllZombieSounds();
    int SetMustacheMode(bool theEnable);
    int SetFutureMode(bool theEnable);
    int SetDanceMode(bool theEnable);
    int IteratePlants(Plant *& thePlant);
    int GetPlantsOnLawn(unsigned int theGridX, unsigned int theGridY, PlantsOnLawn *thePlants);
    Plant * GetTopPlantAt(int theGridX, int theGridY, PlantPriority thePriority);
    bool IsPlantInGoldWateringCanRange(int theGridX, int theGridY, Plant * thePlant);
    int PlantingPixelToGridY(int theX, int theY, SeedType theSeedType);
    void GridCellBounds(int theGridX, int theGridY, SexyVector2 & theTopLeft, SexyVector2 & theTopRight, SexyVector2 & theBottomLeft, SexyVector2 & theBottomRight, SexyVector2 & theCenter, SexyVector2 & theTop, SexyVector2 & theBottom, SexyVector2 & theSide);
    int CountSunFlowers();
    int CountPlantByType(SeedType theSeedType);
    void UpdateTutorial();
    int CountEmptyPotsOrLilies(SeedType theSeedType);
    int RemoveAllPlants();
    int UpdateMousePosition();
    // Returns the plant under the point, or null. The binary leaves the plant
    // pointer in r0; a miss leaves the iterator's 0.
    Plant * SpecialPlantHitTest(int theX, int theY);
    int MouseHitTestPlant(int theX, int theY, HitResult *theHitResult);
    int IterateProjectiles(Projectile *& theProjectile);
    int RemoveAllProjectiles();
    int IterateCoins(Coin *& theCoin);
    int RemoveAllCoins();
    int CountSunBeingCollected(int thePlayerIndex);
    bool TakeSunMoney(int theAmount, int thePlayerIndex);
    bool CanTakeSunMoney(int theAmount, int thePlayerIndex);
    int CountDeathBeingCollected();
    bool CanTakeDeathMoney(int theAmount);
    int CountCoinsBeingCollected();
    int IterateLawnMowers(LawnMower *& theMower);
    LawnMower * FindLawnMowerInRow(int theRow);
    int CountUntriggerLawnMowers();
    LawnMower * GetBottomLawnMower();
    int RemoveAllMowers();
    int UpdateGameObjects();
    void DrawDebugObjectRects(Sexy::Graphics * g);
    int SetSuperMowerMode(bool theEnable);
    void DoTypingCheck(char *theKey);
    bool IterateGridItems(GridItem *& theGridItem);
    GridItem * GetGridItemAt(GridItemType theType, int theGridX, int theGridY);
    GridItem * GetGraveStoneAt(int theGridX, int theGridY);
    GridItem * GetCraterAt(int theGridX, int theGridY);
    GridItem * GetScaryPotAt(int theGridX, int theGridY);
    // @0x157444 returns a PlantingReason in r0. Zero is PLANTING_OK.
    PlantingReason CanPlantAt(int theGridX, int theGridY, SeedType theSeedType);
    int IsValidCobCannonSpotHelper(int theGridX, int theGridY);
    bool IsValidCobCannonSpot(int theGridX, int theGridY);
    int HasValidCobCannonSpot();
    GridItem * GetSquirrelAt(int theGridX, int theGridY);
    GridItem * GetZenToolAt(int theGridX, int theGridY);
    GridItem * GetLadderAt(int theGridX, int theGridY);
    GridItem * GetMoundAt(int theGridX, int theGridY);
    GridItem * GetRake();
    int CanAddGraveStoneAt(int theGridX, int theGridY);
    int RemoveAllGridItems();
    int GetGraveStoneCount();
    int GetMPTargetCount();
    LawnApp * DrawProgressMeter(Sexy::Graphics * g, int theX, int theY);
    GridItem * PickSpecialGraveStone();
    int IterateParticles(TodParticleSystem *& theParticle);
    int IterateReanimations(Reanimation *& theReanimation);
    int FreezeEffectsForCutscene(bool theFreeze);
    int PlantsWon(GridItem * theBrain);
    void StartLevel();
    int KillAllPlantsInRadius(int theX, int theY, int theRadius);
    // MakeRenderOrder(RenderLayer,int,int) -- STATIC. The dump body is
    // `int __fastcall Board::MakeRenderOrder(int a1, int a2, int a3)` with no
    // receiver: the disassembly is `ADD R2,R0 / MOVW R0,#0x2710 / MLA R0,R0,R1,R2`,
    // i.e. all three registers are real parameters. The decompiler renders the
    // call sites as `((Board*)<a1>)->MakeRenderOrder(a1, a2, a3)`.
    static int MakeRenderOrder(RenderLayer theLayer, int theRow, int theGridX);
    int AddBossRenderItem(BoardRenderItem *aRecords, int &aCount, Zombie *aZombie);
    int InitCoverLayer();
    int CountCoinByType(CoinType theType);
    Coin * DropLootPiece(int theX, int theY, int thePiece);
    int CanDropLoot();
    int CompleteEndLevelSequenceForSaving();
    // False means the level file could not be written; callers must keep the board.
    bool TryToSaveGame();
    int BungeeIsTargetingCell(int theGridX, int theGridY);
    Zombie * GetBossZombie();
    int UpdateProgressMeter();
    void DrawDebugText(Sexy::Graphics * g);
    Plant * FindUmbrellaPlant(int theGridX, int theGridY);
    ReanimationID DoFwoosh(int theRow);
    void UpdateFwoosh();
    int UpdateGridItems();
    bool PlantingRequirementsMet(SeedType theSeedType);
    int KillAllZombiesInRadius(int theX, int theY, int theRadius, int theRow, int theFlags, bool theBurn, int theDamage);
    int KillAllGravestonesInRadius(int theX, int theY, int theRadius);
    int GetNumWavesPerSurvivalStage();
    bool RowCanHaveZombieType(int theRow, ZombieType theZombieType);
    int PickRowForNewZombie(ZombieType theZombieType);
    int GetSurvivalFlagsCompleted();
    int SurvivalSaveScore();
    int PickZombieType(int theWaveIndex, int thePointCount, ZombiePicker *thePicker);
    void ZombiesWon(Zombie * theZombie);
    void DrawLevel(Sexy::Graphics * g);
    int IsFinalSurvivalStage();
    int IsSurvivalStageWithRepick();
    int NextWaveComing();
    int DrawFadeOut(Sexy::Graphics * g);
    void UpdateLevelEndSequence();
    int RemoveParticleByType(ParticleEffect theEffect);
    Zombie *GetWinningZombie();
    int ResetFPSStats();
    int PlantUsesAcceleratedPricing(SeedType theSeedType);
    int GetCurrentPlantCost(SeedType theSeedType, SeedType theImitaterType);
    int CanUseGameObject(GameObjectType theObjectType);
    // Same sret shape as GetShovelButtonRect above: the dump's `a1` is the
    // result buffer and its `a2` is the receiver, so this is really a one-argument
    // member returning the rect by value
    // (_ZN5Board16GetZenButtonRectE14GameObjectType @0x15c500).
    Sexy::Rect GetZenButtonRect(GameObjectType theObjectType);
    int DrawZenWheelBarrowButton(Sexy::Graphics * g, int thePlayerIndex);
    int MouseHitTest(int theX, int theY, HitResult *theHitResult, int thePlayerIndex);
    Plant * ToolHitTest(int theX, int theY);
    Plant * HighlightPlantsForMouse(int theX, int theY);
    int DrawZenButtons(Sexy::Graphics * g);
    int DrawShovel(Sexy::Graphics * g);
    void GameAxisMove(Sexy::GamepadAxis theAxis, int theValue, int thePlayerIndex);
    void AddSecondPlayer(int theGamepadIndex);
    void GameButtonUp(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    GamepadControls * GetGamepadControlsByPlayerIndex(int thePlayerIndex);
    void RefreshSeedPacketFromCursor(int thePlayerIndex);
    void FadeOutLevel();
    GamepadControls * GetGamepadControlsByGamepadIndex(int theGamepadIndex);
    void TryToExit();
    int FinishExit();
    bool OkToPause();
    void RebuildHelpBar();
    int GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    void OnFullVersionLicenseChange();
    std::set<Plant *> * GetTangleKelpList();
    void StartHelp(int theHelpIndex);
    bool IsLevelDataLoaded();
    int StageHasFog();
    int SeedNotRecommendedForLevel(SeedType theSeedType);
    void DrawUIBottom(Sexy::Graphics * g);
    int LeftFogColumn();
    void ClearFogAroundPlant(Plant * thePlant, int theRadius);
    void UpdateFog();
    int GetPointsInWave(int theWaveIndex);
    void dumpLevelDesignStatToLog();
    void AddALadder(int theGridX, int theGridY);
    GridItem * AddACrater(int theGridX, int theGridY);
    GridItem * AddAGraveStone(int theGridX, int theGridY);
    void AddGraveStones(int theCount, int theMinX, MTRand & theRand);
    void AddAMound(int theGridX, int theGridY, int theCount);
    LawnApp * PlaceRake();
    void AddMPTarget(int theGridX, int theGridY);
    Zombie * AddZombieInRow(ZombieType theZombieType, int theRow, int theFromWave, bool theIsVisible);
    Zombie * SpawnZombiesFromPool();
    Zombie * AddZombie(ZombieType theZombieType, int theFromWave, bool theIsVisible);
    // _ZN5Board16BungeeDropZombieEP14BungeeDropGrid10ZombieType -- the first
    // parameter is the filled grid, not an int.
    void BungeeDropZombie(BungeeDropGrid *theDrop, ZombieType theZombieType);
    void SpawnZombiesFromSky();
    void SpawnZombieWave();
    Zombie * SpawnZombiesFromGraves();
    void UpdateZombieSpawning();
    int UpdateGame();
    int LoadResources(Sexy::String const& theGroup);
    void LoadBackgroundImages();
    int PostLoadGame();
    int PickBackground();
    Plant * AddPlant(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex, bool theFromSave);
    void MouseDownWithPlant(int theX, int theY, int theClickCount, int thePlayerIndex);
    void MouseDownWithTool(int theX, int theY, int theClickCount, CursorType theCursorType, int thePlayerIndex);
    int PickZombieWaves();
    int InitZombieWaves();
    int InitSurvivalStage();
    int InitLevel();
    // Both return a String BY VALUE. ARM passes the destination in r0, so the
    // dump's `this` is that destination, not the Board; the transcription read
    // it as the receiver and declared a Board* result. dumpLevelConfiguration
    // cannot be finished until these two are rewritten to match.
    Sexy::String dumpAllowedZombiesInLevel();
    Sexy::String dumpZombiesInWave(int theWaveIndex);
    // @0x163828. r0 is the LawnMower FindLawnMowerInRow left there, or null.
    LawnMower * ResetLawnMowers();
    Board(LawnApp * theApp);
    int ProcessDeleteQueue();
    // Leaves r0 alone. The one caller discards the result.
    void DrawUITop(Sexy::Graphics * g);
    int DrawGameObjects(Sexy::Graphics * g);
    void dumpLevelConfiguration();
    // Everything from AddedToManager through MakeBuddyImage used to be declared
    // here. None of it is ::Board's: those bodies are all `_ZN4Sexy5Board*`
    // symbols -- Sexy::Board, a SexyAppFramework class with an unrelated layout
    // -- and they now live in Sexy\Board.h / Sexy\Board.cpp. In particular
    // Sexy::Board's `EffectSystem *mEffectSystem` sits at +0x448, which inside
    // this class falls in the middle of mGridCelOffset.

    // Board::PLANT_WIN_TIME -- a class-scope global in the binary
    // (_ZN5Board14PLANT_WIN_TIMEE @0x695d00, initial value 300000). The dump
    // both reads it and assigns to it through the class name, so it is a static.
    static int PLANT_WIN_TIME;
};

// sizeof(Board) can only be asserted once Sexy::Widget / Sexy::ButtonListener
// and SyncObject carry their real layouts: Widget is still opaque storage of
// 0x7A0 bytes here, where the binary's is 0x100.
// TODO(recon): enable `static_assert(sizeof(Board) == 0x591C, "Board must be
// 0x591C bytes");` as soon as Sexy\Widget.h is reconstructed.
