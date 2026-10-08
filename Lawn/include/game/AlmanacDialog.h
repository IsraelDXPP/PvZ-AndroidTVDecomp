// AlmanacDialog -- reconstructed from libGameMain.so.
//
// Inheritance:
//     class AlmanacDialog : public LawnDialog, public Sexy::ScrollListener
// The ScrollListener base is proven by the THIRD vtable group inside
// _ZTV13AlmanacDialog @0x67CB30: a secondary vtable whose header word is
// 0xFFFFFD04 = -764 = -0x2FC, i.e. the sub-object starts at +0x2FC -- exactly
// where LawnDialog ends -- and whose only entry is
// `Sexy::ScrollListener::ScrollPosition(int, double)`. The constructor writes
// that sub-object's vptr (`&off_67CD84` at +764) and AlmanacDialog.cpp:732 hands
// `(char *)this + 764` to `new CustomScrollbarWidget(...)` as the ScrollListener.
// The authoritative header types the same word as `int *mScrollListener`, which
// is the placeholder for this base.
//
// SIZE 0x3B0 (the v111 layout, 236 ints) -- NOT the transcriber's 0x3A8.
//   The authoritative header's closing comment is "115: 234, 111: 236", this
//   binary's member table is the v111 one (the v115 `mIsZombie`/`unk2` fields are
//   absent, see build/map-lawn.md), and LawnApp.cpp:1436 allocates
//   `operator new(0x3A8u)` -- the v115 size, i.e. the allocation site is 8 bytes
//   short of the layout this .so actually uses. 0x3B0 is what the members add up
//   to, so 0x3B0 is declared.
//
// ---------------------------------------------------------------------------
// VTABLE @0x67CB30, 134 primary slots. Own entries:
//   [  0] 0x138428  ~AlmanacDialog            override
//   [  1]           (ARM D0; MSVC x86 keeps ONE dtor slot)
//   [ 29] 0x13bb74  AddedToManager            override
//   [ 30] 0x13bbac  RemovedFromManager        override
//   [ 31] 0x13bbf0  Update                    override
//   [ 36] 0x13bb10  Draw                      override
//   [ 61] 0x13bea0  GotFocus                  override
//   [ 67] 0x1382dc  KeyChar                   override
//   [ 71] 0x13bdb0  KeyDown(KeyCode)          override
//   [ 76] 0x13a63c  MouseDown(x,y,clicks)     override
//   [ 79] 0x138a10  MouseUp(x,y,clicks)       override
//   [ 82] 0x13b9a8  MouseWheel(delta)         override
//   [104] 0x13b6d0  AxisMoved                 override
//   [128] 0x1385c0  ButtonPress               override
//   [129] 0x13861c  ButtonDepress             override
//   --- slot 133 is new in AlmanacDialog ---
//   [133] 0x138a4c  GameButtonDown
// Its slot [50] Resize still holds LawnDialog's implementation, so it is not
// redeclared.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "CustomScrollbarWidget.h"
#include "GameButton.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "InputManager.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "PoolEffect.h"
#include "ReanimatorCache.h"
#include "ScrollListener.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "Zombie.h"

class Plant;

// AlmanacDialog stores only `LawnApp * mApp` at +0x300 and takes one as the
// constructor's pointer parameter, so the include was half of the
// LawnApp.h <-> AlmanacDialog.h cycle. Sexy\ReconTypes.h already declares it, but
// it is spelled here as well so this header says out loud what it depends on.
class LawnApp;

class AlmanacDialog : public LawnDialog, public Sexy::ScrollListener
{
public:
    // ---- members, absolute offsets (authoritative ordinals 192..233 * 4) ----
    // +0x2FC is the ScrollListener base sub-object, so the member list starts at
    // +0x300 (ordinal 192), one slot later than the authoritative ordinals.
    LawnApp * mApp;                                     // +0x300
    GameButton * mPlantButton;                          // +0x304
    GameButton * mZombieButton;                         // +0x308
    Sexy::CustomScrollbarWidget * mScrollTextView;      // +0x30C
    AlmanacPage mOpenPage;                              // +0x310
    SeedType mSelectedSeed;                             // +0x314
    ZombieType mSelectedZombie;                         // +0x318
    Plant * mPlant;                                     // +0x31C
    Zombie * mZombie;                                   // +0x320
    // The card's text panel. Plants use 485,309,258x230. Zombies use 485,365,263x230.
    Sexy::Rect mCardTextRect;
    Sexy::Rect mDescriptionRect;                        // +0x334 .. +0x343
    Sexy::Rect mCostRect;                               // +0x344 .. +0x353
    Sexy::Rect mWaitTimeRect;                           // +0x354 .. +0x363
    // [0] is a float in the bodies (DrawPlants stores -v22 in it): the vertical
    // offset the description was last drawn at. [1] is the wrapped description's
    // measured height, which is what sizes the scrollbar.
    float mDescriptionScrollY;                          // +0x364
    int mDescriptionTextHeight;                         // +0x368
    Sexy::String mNameString;                           // +0x36C
    Sexy::String mDescriptionHeaderString;              // +0x370
    Sexy::String mDescriptionStringTmp;                 // +0x374
    Sexy::String mDescriptionString;                    // +0x378
    Sexy::String mCostString;                           // +0x37C
    Sexy::String mWaitTimeString;                       // +0x380
    // TODO(recon): the authoritative header types both as DrawStringJustification,
    // which Sexy\ReconTypes.h does not define yet; the bodies only pass them
    // straight to TodDrawStringWrapped.
    int mWaitTimeJustification;                         // +0x384
    int mJustification;                                 // +0x388
    bool mSetupFinished;
    HelpBarWidget * mHelpBarWidget;
    // The authoritative header writes `int unk4[11]` here, which cannot fit in
    // the 5 slots the class has left; the bodies only touch these three.
    // mAxisDeviceIndex is stored and compared as a FLOAT (AxisMoved writes
    // `(float)(int)theEvent.lateModifiers` and compares it the same way).
    float mAxisDeviceIndex;
    // Set while an axis-driven scroll is in progress.
    bool mAxisScrollActive;
    bool mMouseDraggingDescription = false;
    bool mMouseScrollActive = false;
    int mMouseLastY = 0;

    // ---- the LawnDialog/Dialog slots this class overrides, in binary order ----
    virtual ~AlmanacDialog();                                                  // [  0]
    virtual void AddedToManager(Sexy::WidgetManager * theWidgetManager);       // [ 29]
    virtual void RemovedFromManager(Sexy::WidgetManager * theWidgetManager);   // [ 30]
    virtual void Update();                                                    // [ 31]
    virtual void Draw(Sexy::Graphics * g);                                    // [ 36]
    virtual void GotFocus();                                                  // [ 61]
    virtual int KeyChar(char theChar);                                       // [ 67]
    virtual int KeyDown(Sexy::KeyCode theKey);                               // [ 71]
    virtual void MouseDown(int x, int y, int theClickCount);                  // [ 76]
    virtual void MouseUp(int x, int y, int theClickCount);                    // [ 79]
    void MouseDrag(int x, int y) override;
    virtual void MouseWheel(int theDelta);                                    // [ 82]
    virtual void AxisMoved(const Sexy::Event & theEvent);                     // [104]
    virtual void ButtonPress(int theId);                                      // [128]
    virtual void ButtonDepress(int theId);                                    // [129]

    // ---- slot 133: new in AlmanacDialog ----
    virtual void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags); // [133]

    // ---- methods the binary has but the authoritative header does not list ----
    // The return types are the real ones, not the register the ARM code happens
    // to leave the result in: the reference source declares every one of these
    // void (or the enum) and the callers discard the value.
    void SetupHelpbar(bool theIsZombie);
    void SetupPlant();
    void SetupZombie();
    void SetPage(AlmanacPage thePage);
    void DrawIndex(Sexy::Graphics * g);
    void GetSeedPosition(SeedType theSeedType, int & x, int & y);
    void DrawPlants(Sexy::Graphics * g);
    void GetZombiePosition(ZombieType theZombieType, int & x, int & y);
    ZombieType GetZombieType(int theIndex);
    SeedType SeedHitTest(int x, int y);
    SeedType NextSeedInDir(AlmanacDialog_SeedDir theDir);
    bool ZombieHasSilhouette(ZombieType theZombieType) const;
    bool ZombieIsShown(ZombieType theZombieType);
    void DrawZombies(Sexy::Graphics * g);
    bool ZombieHasDescription(ZombieType theZombieType);
    ZombieType ZombieHitTest(int x, int y);
    ZombieType NextZombieInDir(AlmanacDialog_SeedDir theDir);
    void ShowZombie(ZombieType theZombieType);
    void ShowPlant(SeedType theSeedType);
    void SetupLayoutPlants(Sexy::Graphics * g);
    void SetupLayoutZombies(Sexy::Graphics * g);
    void ClearPlantsAndZombies();
    void GameAxisMove(Sexy::GamepadAxis theAxis, int theValue, int thePlayerIndex);
    AlmanacDialog(LawnApp * theApp);
};
