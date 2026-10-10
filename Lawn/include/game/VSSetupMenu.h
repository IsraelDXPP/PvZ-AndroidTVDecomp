// VSSetupMenu -- reconstructed from libGameMain.so.
//
// Real inheritance: `class VSSetupMenu : public LawnMenuWidget`
// (LawnMenuWidget : Sexy::MenuWidget : Sexy::Widget : Sexy::WidgetContainer).
// The vtable says so directly: slot 6 is MenuWidget::AddWidget, slot 30
// MenuWidget::RemovedFromManager, slot 38 MenuWidget::DrawAll and slot 127
// LawnMenuWidget::CreateMenuParser, so LawnMenuWidget sits between MenuWidget and
// this class. MenuWidget is 0x118 (its own members start at +0x104 and end at
// +0x117), and LawnMenuWidget adds no data, so VSSetupMenu's own members start at
// +0x118 -- exactly where the authoritative header PvZ\Lawn\Widget\VSSetupMenu.h
// puts its first field (word 70).
//
// Size 0x150, from `(VSSetupMenu *)operator new of 0x150 bytes` at LawnApp.cpp:1040. The
// authoritative header's longer `unkInt83[85]` tail belongs to the hook project's
// own extended version, not to this binary.
//
// vtable order -- slots 0..126 come from the bases and are NOT redeclared here.
// This class overrides these base slots:
//   [  6] 0x2af6e8  Sexy::MenuWidget::AddWidget   (inherited unchanged)
//   [ 29] 0x194ee4  VSSetupMenu::AddedToManager
//   [ 31] 0x195660  VSSetupMenu::Update
//   [ 36] 0x195130  VSSetupMenu::Draw
//   [ 54] 0x194f5c  VSSetupMenu::DrawOverlay
//   [ 61] 0x195380  VSSetupMenu::GotFocus
//   [ 67] 0x194ed8  VSSetupMenu::KeyChar
//   [ 71] 0x19516c  VSSetupMenu::KeyDown
// and appends five new ones, declared below in this order:
//   [128] 0x194ecc  VSSetupMenu::ButtonPress
//   [129] 0x1961b4  VSSetupMenu::ButtonDepress
//   [130] 0x194edc  VSSetupMenu::GameAxisMove
//   [131] 0x195978  VSSetupMenu::GameButtonDown
//   [132] 0x194ee0  VSSetupMenu::GameButtonUp

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_VSSETUPMENU_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_VSSETUPMENU_H

#include "ReconTypes.h"
// The base class, plus the classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "CutScene.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "LawnMenuWidget.h"
#include "MenuWidget.h"
#include "SeedPacket.h"
#include "WaitForSecondPlayerDialog.h"
#include "Widget.h"

// LawnApp is reached only through `LawnApp *mApp` at +0x124, and SeedChooserScreen
// is not named in this file at all, so both includes were pure cycle
// (VSSetupMenu.h <-> LawnApp.h and VSSetupMenu.h <-> SeedChooserScreen.h). This
// header derives from LawnMenuWidget, so leaving the cycle in place is what made
// it report C2504. Sexy\ReconTypes.h already declares LawnApp.
class LawnApp;
class SeedChooserScreen;

class VSSetupMenu : public LawnMenuWidget
{
public:
    enum
    {
        VSSetupMenu_Quick_Play = 9,
        VSSetupMenu_Custom_Battle = 10,
        VSSetupMenu_Random_Battle = 11,
        VSSetupMenu_Enter = 1000,
        VSSetupMenu_Back = 1001,
    };

    enum WidgetIds
    {
        BACKGROUND_FRAME,
        PLANT_SIDE,
        PLANT_SIDE_MID,
        PLANT_SIDE_FRONT,
        ZOMBIE_SIDE,
        ZOMBIE_SIDE_MID,
        ZOMBIE_SIDE_FRONT,
        CONTROLLER_0,
        CONTROLLER_1,
        QUICK_BUTTON,
        CUSTOM_BUTTON,
        RANDOM_BUTTON
    };

    enum VSSetupState
    {
        VS_SETUP_STATE_CONTROLLERS = 0, // WaitForSecondPlayerDialog
        VS_SETUP_STATE_SIDES = 1, // no controller has a side yet
        VS_SETUP_STATE_SELECT_BATTLE = 2, // both controllers have a side
        VS_SETUP_STATE_CUSTOM_BATTLE = 3, // picking seeds for the custom battle
    };

    enum VSSetupMode
    {
        VS_SETUP_MODE_QUICK_PLAY = 0,
        VS_SETUP_MODE_CUSTOM_BATTLE = 1,
        VS_SETUP_MODE_RANDOM_BATTLE = 2,
    };

    enum VSSide
    {
        VS_SIDE_NONE = -1,
        VS_SIDE_PLANT = 0,
        VS_SIDE_ZOMBIE = 1,
    };

    // The state parameter is spelled with the global model in Sexy\ReconTypes.h
    // because that is what the transcribed bodies use; the authoritative header
    // declares the same enum nested as VSSetupState.

    // Statics the binary keeps in this class's own storage; the transcribed
    // bodies name them through the qualified spelling.
    static int msNextFirstPick;

    // VSSetupMenu::msQuickPlayDecks -- a class-scope table in the binary
    // (_ZN11VSSetupMenu16msQuickPlayDecksE @0x5a1ca0). GotFocus walks it as two
    // rows of 24 bytes, each row read as six SeedTypes at a stride of 4 (the
    // ARM advances the row cursor by 0x18 and the entry cursor by 4), so it is
    // a [2][6] table: one quick-play deck per player.
    static const SeedType msQuickPlayDecks[2][6];

    // VSSetupMenu::GAMEPAD_X_POSITIONS -- a class-scope table in the binary
    // (_ZN11VSSetupMenu19GAMEPAD_X_POSITIONSE @0x5a1b74). The dump indexes it with
    // a running player index, so it is an array of ints; the 16 entries are the
    // binary's own data.
    static const int GAMEPAD_X_POSITIONS[16];

    // Nine rows of eight seed ids. Rows 0..5 are plants, rows 6..8 are zombies.
    // A row ends at the first -1. The table is the binary's msRandomPools.
    static const SeedType msRandomPools[9][8];

    virtual ~VSSetupMenu();

    // Overrides of base slots.
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager); // [ 29] 0x194ee4
    virtual void Update(); // [ 31] 0x195660
    virtual void Draw(Sexy::Graphics *g); // [ 36] 0x195130
    virtual void DrawOverlay(Sexy::Graphics *g); // [ 54] 0x194f5c
    virtual void GotFocus(); // [ 61] 0x195380
    virtual int KeyChar(char theChar); // [ 67] 0x194ed8
    virtual int KeyDown(Sexy::KeyCode theKey); // [ 71] 0x19516c
    void MouseDown(int x, int y, int theClickCount) override;
    void MouseDrag(int x, int y) override;
    void MouseUp(int x, int y, int theClickCount) override;

    // The five entries this class appends.
    virtual void ButtonPress(int theId); // [128] 0x194ecc
    virtual void ButtonDepress(int theId); // [129] 0x1961b4
    virtual void GameAxisMove(Sexy::GamepadAxis theAxis, int theValue, int thePlayerIndex); // [130] 0x194edc
    virtual void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex,
        unsigned int theModifierFlag); // [131] 0x195978
    virtual void GameButtonUp(Sexy::GamepadButton theButton, int thePlayerIndex,
        unsigned int theModifierFlag); // [132] 0x194ee0

    // Not vtable slots; the signatures are the ones the transcribed bodies define.
    VSSetupMenu();
    void CloseVSSetup(bool theTearDown);
    int EndSeedChooser();
    int EndZombieChooser();
    int OnStateExit(VSSetupMenu_VSSetupState theState, VSSetupMenu_VSSetupState theNext);
    int SetSecondPlayerIndex(int theIndex);
    int OnStateEnter(VSSetupMenu_VSSetupState theState, VSSetupMenu_VSSetupState thePrevious);
    int GoToState(int theState);
    int OnPlayerPickedSeed(int theSeed);
    void PickRandomZombies(std::vector<SeedType> &theOut);
    void PickRandomPlants(std::vector<SeedType> &theOut, std::vector<SeedType> const &thePool);

    // -----------------------------------------------------------------------
    // Members +0x124..+0x14F. Names and order from the authoritative header
    // PvZ\Lawn\Widget\VSSetupMenu.h.
    //
    // +0x118..+0x123 deliberately does NOT appear here: the base class
    // LawnMenuWidget already declares mBackdropReanimID, mCameraOffsetX and
    // mCameraOffsetY at exactly those offsets. This header used to repeat them,
    // which pushed every member below 12 bytes late and made sizeof(VSSetupMenu)
    // 0x15C against the 0x150 allocation at LawnApp.cpp:1040.
    // -----------------------------------------------------------------------
    LawnApp *mApp; // +0x124
    VSSetupMenu_VSSetupState mState; // +0x128
    int mControllerIndex[2]; // +0x12C, +0x130  (0: pad 1, 1: pad 2)
    VSSide mSides[2]; // +0x134, +0x138
    bool mSideLocked[2]; // +0x13C, +0x13D
    VSSide mSeedPickTurn; // +0x140
    int mChooserAnimateUpdateCnt; // +0x144
    VSSetupMode mSetupMode; // +0x148
    // Set by the selector's quick-play cheat. GotFocus then fills both banks
    // from msQuickPlayDecks and closes the setup.
    bool mQuickPlay; // +0x14C
    int mMouseController = -1;
    int mMouseLastX = 0;
    // size 0x150, matching `operator new(0x150u)` at LawnApp.cpp:1040.
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_VSSETUPMENU_H
