// VSResultsMenu -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class VSResultsMenu : public LawnMenuWidget`.
// The binary's RTTI base for _ZTV13VSResultsMenu is `14LawnMenuWidget`, and the
// table's AddWidget [6], RemovedFromManager [30] and DrawAll [38] entries are
// Sexy::MenuWidget implementations, which is the chain LawnMenuWidget inherits.
//
// Object size 0x1A0. LawnMenuWidget is Sexy::MenuWidget (0x118) plus its own
// `int data118[3]`, so 0x124, and this class's members run 0x124..0x19F.
// The transcribed object was 0x3A0, driven by an access at +0x390 that belongs to
// a different receiver (the literal 912 also appears in AlmanacDialog, CheatDialog,
// GameButton, LawnDialog, MainMenu, TitleScreen and others).
//
// The reference tree's member ordinals 70..72 are the base's data118[3], not this
// class's; every ordinal from 73 on maps to absolute = ordinal * 4, which the
// bodies confirm: VSResultsMenu.cpp:169 reads (this + 300) as mResultsButtonId and
// :246-248 clear the three base words, which puts mSparkleCounter at +0x124.
//
// vtable order -- the slots this class occupies, in the binary's order:
//   [  0] 0x190c9c  VSResultsMenu::~VSResultsMenu
//   [  1] 0x190cd0  VSResultsMenu::~VSResultsMenu
//   [ 29] 0x190c68  VSResultsMenu::AddedToManager
//   [ 31] 0x190d34  VSResultsMenu::Update
//   [ 36] 0x190e8c  VSResultsMenu::Draw
//   [ 54] 0x19169c  VSResultsMenu::DrawOverlay
//   [ 61] 0x190cec  VSResultsMenu::GotFocus
//   [ 71] 0x190c90  VSResultsMenu::KeyDown
//   [126] 0x190e28  VSResultsMenu::OnExit
//   [128] 0x190e6c  VSResultsMenu::ButtonDepress
// Slots 54 and 126 are absent from the community restoration; they are kept here
// because the binary's table holds this class's own addresses at those indices.
//
// MSVC x86 gives a virtual destructor one slot where Itanium and ARM give it two,
// so the absolute slot numbers run one lower here; only the order is copied.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "CheckboxListener.h"
#include "Font.h"
#include "Graphics.h"
#include "LawnMenuWidget.h"
#include "MenuWidget.h"
#include "TodParticleSystem.h"
#include "Widget.h"

// LawnApp is not named in this file, so the include was pure cycle
// (LawnApp.h <-> VSResultsMenu.h). Sexy\ReconTypes.h already declares it.
class LawnApp;

class GameButton;
class VSResultsCheckboxController;
namespace Sexy { class Checkbox; }

class VSResultsMenu : public LawnMenuWidget
{
public:
    enum {
        VSResultsMenu_Play_Again = 0,
        VSResultsMenu_Quit_VS = 1,
        VSResultsMenu_Info_Box_P1 = 2,
        VSResultsMenu_Info_Box_P2 = 3,
        VSResultsMenu_Plant_Side = 4,
        VSResultsMenu_Plant_Side_Front = 5,
        VSResultsMenu_Zombie_Side = 6,
        VSResultsMenu_Zombie_Side_Front = 7,
        VSResultsMenu_Win_Image = 8,
        VSResultsMenu_Back = 9,
        VSResultsMenu_Save_Replay = 1200,
    };

    enum WinSide {
        INVALID_WIN_SIDE = -1,
        WIN_SIDE_PLANT = 0,
        WIN_SIDE_ZOMBIE = 1,
    };

    // [record slot] -> [winning side, plant wins, zombie wins, win streak, playerIndex2]
    // The binary keeps the whole table at _ZN13VSResultsMenu15msPlayerRecordsE
    // (0x69E8DC, 40 bytes) and every reader indexes it by slot, so it is the 2x5
    // array the authoritative header declares. It is a static, so the shape does
    // not affect the object layout; the definition is in Sexy\ReconStatics.cpp.
    static int msPlayerRecords[2][5];

    virtual ~VSResultsMenu();
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);  // slot 29
    virtual void Update();                                  // slot 31
    virtual void Draw(Sexy::Graphics *theGraphics);                 // slot 36
    virtual void DrawOverlay(Sexy::Graphics *theGraphics);          // slot 54
    virtual void GotFocus();                                // slot 61
    virtual int KeyDown(Sexy::KeyCode theKey);                 // slot 71
    // TODO(recon): the authoritative header spells this `void OnExit() const`.
    // Declared non-const because the framework's MenuWidget declares it without
    // const; whichever way LawnMenuWidget ends up, both the declaration and
    // VSResultsMenu.cpp's `int VSResultsMenu::OnExit()` have to agree.
    virtual void OnExit();                                  // slot 126
    virtual void ButtonDepress(int theId);                     // slot 128

    // +0x124 .. +0x19F. The first 0x124 bytes are LawnMenuWidget
    // (Sexy::MenuWidget is 0x118, its `int data118[3]` is +0x118..+0x123).
    int mSparkleCounter;                 // +0x124
    int mSmokeCounter;                   // +0x128
    int mResultsButtonId;                // +0x12C
    int mPlayerIndices[2];               // +0x130, +0x134, playerIndex2 of P1 / P2
    WinSide mSides[2];                   // +0x138, +0x13C
    float mTrophyPosX;                   // +0x140, trophy centre X
    float mTrophyPosY;                   // +0x144, trophy centre Y
    ParticleSystemID mSparkleParticleID; // +0x148
    ParticleSystemID mSmokeParticleID;   // +0x14C
    int mUpdateCounter;                  // +0x150
    int mBoardMainCounter;               // +0x154
    BackgroundType mBoardBackground;     // +0x158
    SeedType mPlantSeeds[6];             // +0x15C .. +0x173
    SeedType mZombieSeeds[6];            // +0x174 .. +0x18B
    bool mIsReplaySession;               // +0x18C
    bool mIsOnlineSession;               // +0x18D
    int mVSResultsCounter;               // +0x190
    VSResultsCheckboxController *mCheckboxController; // +0x194
    GameButton *mBackButton;             // +0x198
    GameButton *mSaveReplayButton;       // +0x19C
    // size 0x1A0

    int * GetPlayerRecord(unsigned int theSlot);
    void DrawInfoBox(Sexy::Graphics *theGraphics, int thePlayerIndex);
    void InitFromBoard(Board *theBoard);
    // ClearPlayerRecords touches nothing but the static record table and the
    // one call site (LawnApp::ShowMainMenuScreen @0x2B9DC6) passes no receiver,
    // so it is the `static void` the authoritative header declares.
    static void ClearPlayerRecords();
    VSResultsMenu();
};

// VSResultsCheckboxController was missing from this tree entirely. The
// authoritative header declares it in this file, and the binary's RTTI base for
// the class is N4Sexy16CheckboxListenerE.
// TODO(recon): no definition exists yet, so this is a declaration only; the
// bodies live in the phase-2 rewrite. Sexy::Checkbox does not exist in this tree
// either, hence the forward declaration above.
class VSResultsCheckboxController final : public Sexy::CheckboxListener
{
public:
    enum {
        VSResultsMenu_Send_Player_Name = 100,
        VSResultsMenu_Auto_Save_Replay = 101,
    };

    Sexy::Checkbox *mSendPlayerNameCheckbox;
    Sexy::Checkbox *mAutoSaveReplayCheckbox;

    VSResultsCheckboxController();
    ~VSResultsCheckboxController();

    void CheckboxChecked(int theId, bool checked);
    void InitCheckboxWidget(VSResultsMenu *parentMenu);
    void DrawCheckboxLabel(Sexy::Graphics *g) const;
    void HideCheckboxWidget();
    void SetSendPlayerNameCheckboxVisible(bool visible);
    void SetAutoSaveReplayCheckboxVisible(bool visible);
    void DestroyCheckboxWidget();

private:
    VSResultsMenu *mParentMenu;
};
