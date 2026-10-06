// StoreScreen -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class StoreScreen : public Sexy::Dialog`. The binary's RTTI
// base for _ZTV11StoreScreen is N4Sexy6DialogE, and the table's Dialog entries
// (OrderInManagerChanged [41], SetColor [45], Resize [50], KeyUp [72]) confirm it.
//
// Object size 0x3CC, from the allocation site at LawnApp.cpp:1399 which requests
// 0x3CC bytes; the member table below ends at 0x3C8 with one word to spare, which
// agrees exactly. The transcribed object was 0x820 because it counted an access at
// +0x810 that belongs to a different receiver: StoreScreen.cpp:3160/3236/3247 all
// read (this + 2064) as a function pointer, and the same literal 2064 appears in
// BuyMoneyDialog, ContinueDialog and NewOptionsDialog, so it is a mis-typed base
// access rather than a StoreScreen member.
//
// The first 0x2E0 bytes are Sexy::Dialog; every member below is at its absolute
// offset. Offsets 736..809 were verified one by one against the raw this-relative
// accesses in StoreScreen.cpp and match the authoritative header
// PvZ\Lawn\Widget\StoreScreen.h.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x190034  StoreScreen::~StoreScreen
//   [  1] 0x190160  StoreScreen::~StoreScreen
//   [ 29] 0x190a1c  StoreScreen::AddedToManager
//   [ 30] 0x18d860  StoreScreen::RemovedFromManager
//   [ 31] 0x1907c8  StoreScreen::Update
//   [ 36] 0x19017c  StoreScreen::Draw
//   [ 54] 0x18d804  StoreScreen::DrawOverlay
//   [ 61] 0x18d8bc  StoreScreen::GotFocus
//   [ 67] 0x18f9b0  StoreScreen::KeyChar
//   [ 71] 0x190a50  StoreScreen::KeyDown
//   [ 76] 0x18d7b8  StoreScreen::MouseDown
//   [128] 0x18d888  StoreScreen::ButtonPress
//   [129] 0x18f6f0  StoreScreen::ButtonDepress
// Slots 54/61/67/71/76 and 128 have no counterpart in the community restoration,
// which lists only AddedToManager/RemovedFromManager/Update/Draw/MouseDown/
// ButtonDepress; they are kept here because the binary's table holds StoreScreen's
// own addresses at those indices.
//
// MouseUp is NOT a virtual here: the binary's slot [79] still holds
// Sexy::Dialog::MouseUp and no _ZN11StoreScreen7MouseUpEiii symbol exists.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Dialog.h"
#include "Font.h"
#include "GameStats.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "HouseChooserDialog.h"
#include "LawnPlayerInfo.h"
#include "PottedPlant.h"
#include "Reanimation.h"
#include "SeedBank.h"
#include "SeedChooserScreen.h"
#include "SexyAppBase.h"
#include "TodFoley.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "DataArray.h"

// Board, Coin, LawnApp, LawnMower and ZenGarden are each stored or passed only as
// pointers here (`LawnApp *mApp`, `StoreScreen(LawnApp *)`), and none of them is
// needed complete, so all five includes were pure cycle. StoreScreen.h sits on
// the longest chains in the tree -- LawnApp -> AwardScreen -> StoreScreen -> ...
// -- so cutting them here removes a whole family at once. LawnMower.h and
// ZenGarden.h are cut on their own side as well.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
class Coin;
class LawnMower;
class ZenGarden;
class LawnApp;

class StoreScreen : public Sexy::Dialog
{
public:
    virtual ~StoreScreen();
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);  // slot 29
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // slot 30
    virtual void Update();                                  // slot 31
    virtual void Draw(Sexy::Graphics *g);                   // slot 36
    virtual void DrawOverlay(Sexy::Graphics *g);            // slot 54
    virtual void GotFocus();                                // slot 61
    virtual int KeyChar(char theChar);                      // slot 67
    virtual int KeyDown(Sexy::KeyCode theKey);              // slot 71
    virtual void MouseDown(int x, int y, int theClickCount); // slot 76
    virtual void ButtonPress(int theId);                    // slot 128
    virtual void ButtonDepress(int theId);                  // slot 129

    // +0x2E0 .. +0x3CB (the first 0x2E0 bytes are Sexy::Dialog).
    LawnApp *mApp;                    // +0x2E0
    Sexy::Widget *mOverlayWidget;     // +0x2E4
    int mStoreTime;                   // +0x2E8
    Sexy::String mBubbleText;         // +0x2EC, one-word string
    int mBubbleCountDown;             // +0x2F0
    bool mBubbleClickToContinue;      // +0x2F4
    int mAmbientSpeechCountDown;      // +0x2F8
    int mPreviousAmbientSpeechIndex;  // +0x2FC
    StorePages mPage;                 // +0x300
    // Shelf slot 0..7 that the cursor is on.
    int mSelectedSlot;                // +0x304
    StoreItem mSelectedStoreItemType; // +0x308
    int mHatchTimer;                  // +0x30C
    bool mHatchOpen;                  // +0x310
    // 0 none, 1 previous-page highlight, 2 next-page highlight while the hatch closes.
    int mHatchDirection;              // +0x314
    int mShakeX;                      // +0x318
    int mShakeY;                      // +0x31C
    int mStartDialog;                 // +0x320
    int mInputGamepadIndex;           // +0x324
    bool mEasyBuyingCheat;            // +0x328
    bool mWaitForDialog;              // +0x329
    // +0x32A and +0x32B are the last two bytes of mPottedPlantSpecs. The draw
    // variation is bits 12..15 of that plant, which the purchase path writes
    // through mPottedPlantSpecs.mDrawVariation.
    PottedPlant mPottedPlantSpecs;    // +0x32C, 0x14 bytes (see PottedPlant.h)
    // The coin pool. CoinSlot is the coin plus the id word, so the stride is
    // sizeof(Coin) + 4. The reference tree puts this member four bytes earlier;
    // that lands inside mPottedPlantSpecs.
    DataArray<Coin, CoinSlot> mCoins; // +0x340 .. +0x35B
    bool mDrawnOnce;                  // +0x35C
    bool mGoToTreeNow;                // +0x35D
    bool mPurchasedFullVersion;       // +0x35E
    bool mTrialLockedWhenStoreOpened; // +0x35F
    // GotFocus stores mApp->mHelpBarWidget here. Update brings it to the front.
    HelpBarWidget *mHelpBarWidget;    // +0x360
    // One rendered pot thumbnail per shelf slot. SetupPage rebuilds them.
    Sexy::Image *mPottedPlantImage[8]; // +0x364 .. +0x383
    // Highlight pulse control points. The constructor fills eight floats and
    // nothing in this build reads them back.
    float mHighlightCurve[8];         // +0x384 .. +0x3A3
    // Written 0 in the constructor. No reader in this build uses it.
    int mIntroStep;                   // +0x3A4
    // The selected item's highlight pulse. Update adds 0.028 each frame and
    // DrawItem clamps it into 0.3..0.5.
    float mHighlightPhase;            // +0x3C4
    int mCarKeyImageIndex;            // +0x3C8, 0..11
    // size 0x3CC

    // The binary returns r0 unchanged from the frame, which is the address of
    // the `theX` out-parameter; no caller reads the result.
    void GetStoreItemInfo(int theSlot, int theItem, Sexy::Image **theImage, int &theDrawX, int &theDrawY, int &thePurchaseCount);
    int GetStoreItemType(int theSlot);
    bool IsPottedPlant(StoreItem theItem);
    int IsFullVersionOnly(StoreItem theItem);
    bool IsComingSoon(StoreItem theItem);
    int IsItemSoldOut(StoreItem theItem);
    int IsItemUnavailable(StoreItem theItem);
    unsigned int NextSlotInDir(StoreScreen_ItemDir theDir);
    void DrawItemHighlight();
    int SetBubbleText(int theCrazyDaveMessage, int theTime, bool theClickToContinue);
    int CanInteractWithButtons();
    int UpdateSelection();
    int StorePreLoad();
    int IsPageShown(StorePage thePage);
    static int GetItemCost(StoreItem theItem);
    int DrawItem(Sexy::Graphics *g, int theSlot, StoreItem theItem);
    bool CanAffordItem(StoreItem theItem);
    int PurchaseItem(StoreItem theItem);
    int EnableButtons(bool theEnabled);
    int AdvanceCrazyDaveDialog();
    int SetupForIntro(int theDialog);
    int SetSelectedSlot(int theSlot);
    int GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    StoreScreen(LawnApp *theApp);
    int SetupPage();
};
