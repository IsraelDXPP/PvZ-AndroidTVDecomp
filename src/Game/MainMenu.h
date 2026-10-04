// MainMenu -- reconstructed from libGameMain.so.
//
// Real inheritance: `class MainMenu : public LawnMenuWidget`
// (LawnMenuWidget : Sexy::MenuWidget : Sexy::Widget : Sexy::WidgetContainer).
// The vtable shows it: slot 6 is MenuWidget::AddWidget, slots 30/38 are MenuWidget
// overrides and slot 127 is LawnMenuWidget::CreateMenuParser. MenuWidget is 0x118
// and LawnMenuWidget adds no data, so MainMenu's own members start at +0x118 --
// which is where the authoritative header PvZ\Lawn\Widget\MainMenu.h puts its
// first field (word 70), and where MainMenu.cpp's raw accesses start.
//
// Size 0x1B0, from `(MainMenu *)operator new of 0x1B0 bytes` at LawnApp.cpp:7128. That is
// 108 ints, exactly the authoritative header's "size = 108 ints".
//
// vtable order -- slots 0..126 come from the bases and are NOT redeclared here.
// This class overrides these base slots:
//   [ 29] 0x19bc60  MainMenu::AddedToManager
//   [ 30] 0x199784  MainMenu::RemovedFromManager
//   [ 31] 0x19ad88  MainMenu::Update
//   [ 36] 0x19a9fc  MainMenu::Draw
//   [ 54] 0x19d2a8  MainMenu::DrawOverlay
//   [ 61] 0x19a0a0  MainMenu::GotFocus
//   [ 62] 0x19c828  MainMenu::LostFocus
//   [ 67] 0x19c02c  MainMenu::KeyChar
//   [ 71] 0x19c920  MainMenu::KeyDown
// and appends five new ones, declared below in this order:
//   [128] 0x199d78  MainMenu::UpdateCrow
//   [129] 0x19b250  MainMenu::DrawSpeechBubble
//   [130] 0x19a530  MainMenu::ButtonPress
//   [131] 0x19c414  MainMenu::ButtonDepress
//   [132] 0x19c898  MainMenu::GameButtonDown

#pragma once

#include "ReconTypes.h"
// The base class, plus the classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "EffectSystem.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnMenuWidget.h"
#include "LawnPlayerInfo.h"
#include "MailScreen.h"
#include "Mailbox.h"
#include "MenuWidget.h"
#include "Reanimation.h"
#include "ReanimatorTransform.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "StoreScreen.h"
#include "Widget.h"

// LawnApp is stored as `LawnApp *mApp` and taken as the constructor's pointer
// parameter, and UnlockDialog is not named in this file at all, so both includes
// were pure cycle (MainMenu.h <-> LawnApp.h and MainMenu.h <-> UnlockDialog.h).
// Sexy\ReconTypes.h already declares LawnApp.
class LawnApp;
class UnlockDialog;
#include "ZenGarden.h"

class MainMenu : public LawnMenuWidget
{
public:
    enum MainMenuButtonId
    {
        START_ADVENTURE_BUTTON = 0,
        ADVENTURE_BUTTON = 1,
        MORE_WAYS_BUTTON = 2,
        HOUSE_BUTTON = 3,
        ACHIEVEMENTS_BUTTON = 4,
        HELP_AND_OPTIONS_BUTTON = 5,
        UNLOCK_BUTTON = 6,
        RETURN_TO_ARCADE_BUTTON = 7,
        MORE_BUTTON = 8,
        BACK_STONE_BUTTON = 9,
        VS_BUTTON = 10,
        VS_COOP_BUTTON = 11,
        MINI_GAMES_BUTTON = 12,
        SURVIVAL_BUTTON = 13,
        PUZZLE_BUTTON = 14,
        BACK_POT_BUTTON = 15,
        STORE_BUTTON = 16,
        ZEN_BUTTON = 17,
        ALMANAC_BUTTON = 18,
        MAIL_BUTTON = 19,
        HELP_BAR = 20,
        ACHIEVEMENTS_BACK_BUTTON = 21
    };

    // MENUSCENE_MORE_WAYS = 0, MENUSCENE_MAIN = 1, MENUSCENE_EXTRA = 2. The
    // authoritative header (PvZ\Lawn\Widget\MainMenu.h:59) declares these as a
    // nested enum; the transcribed bodies spelled them through the global
    // one-int shim MainMenu_MenuScene, which carries no enumerators. The enum
    // below gives the three values their names back, and `MenuScene` is the
    // spelling the bodies and this header both use for the type.
    enum MenuScene
    {
        MENUSCENE_MORE_WAYS = 0,
        MENUSCENE_MAIN      = 1,
        MENUSCENE_EXTRA     = 2
    };

    // Statics the binary keeps in this class's own storage; the transcribed
    // bodies name them through the qualified spelling.
    // The table the camera slides along: one X position per menu scene, indexed
    // by mMenuScene/mTargetMenuScene. MainMenu.cpp reads a float out of it, so it
    // is a float array and not the single int the transcription declared.
    static float msMenuOffsetX[4];

    virtual ~MainMenu();

    // Overrides of base slots.
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);   // [ 29] 0x19bc60
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [ 30] 0x199784
    virtual void Update();                                               // [ 31] 0x19ad88
    virtual void Draw(Sexy::Graphics *g);                                // [ 36] 0x19a9fc
    virtual void DrawOverlay(Sexy::Graphics *g);                         // [ 54] 0x19d2a8
    virtual void GotFocus();                                             // [ 61] 0x19a0a0
    virtual void LostFocus();                                            // [ 62] 0x19c828
    virtual int KeyChar(char theChar);                                  // [ 67] 0x19c02c
    virtual int KeyDown(Sexy::KeyCode theKey);                          // [ 71] 0x19c920
    virtual bool UpdateExit();                                           // [122] 0x199b9c
    virtual void Enter();                                                // [123] 0x1997a0
    // The binary's Exit returns the object itself in r0 (the transcribed body
    // writes `return (int)(uintptr_t)this` and `return (int)this`), which is the
    // chained `this` of `mApp->mGameSelector->Exit()`; the base declares `void`,
    // so a differing return type here is a C2556.
    virtual void Exit();                                                 // [124] 0x19ca34
    virtual bool RetainWidgetsOnExit();                                  // [125] 0x19977c
    virtual void OnExit();                                               // [126] 0x199898

    // The five entries this class appends.
    virtual Reanimation *UpdateCrow(Reanimation *theCrow);                    // [128] 0x199d78
    virtual void DrawSpeechBubble(Sexy::Graphics *g);                    // [129] 0x19b250
    virtual void ButtonPress(int theId);                                 // [130] 0x19a530
    virtual void ButtonDepress(int theId);                               // [131] 0x19c414
    virtual void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex,
                                unsigned int theModifierFlag);           // [132] 0x19c898

    // Not vtable slots; the signatures are the ones the transcribed bodies define.
    MainMenu(LawnApp *theApp);
    bool FinishedTransition(char const *theTrackName);
    Reanimation *OnScene(MainMenu_MenuScene theScene);
    bool InTransition();
    float UpdateCameraPosition();
    // The binary returns whatever the last call produced (the object itself on
    // the early exits, PlaySample's result on the crow path). Every call site in
    // the tree discards it, so the declaration is `void`.
    void SetScene(unsigned int theScene);
    int GetScene();
    void StartAdventureMode();
    void StartCoopAdventureMode();
    int DoUnlockDialog(int theLevel, int theItemId);
    int SyncButtons();
    int SyncProfile(bool theReload);
    int HideCrowBubble();
    int CancelCrowTalk();
    int OnFullVersionLicenseChange();
    int DrawFade(Sexy::Graphics *theGraphics);

    // -----------------------------------------------------------------------
    // Members +0x118..+0x1AF. Names from the authoritative header
    // PvZ\Lawn\Widget\MainMenu.h where it names them; the offsets are the ones
    // the bodies in MainMenu.cpp actually reach, verified one by one against
    // the raw `this`-relative accesses in that file. Where the authoritative
    // header's ordering and the binary disagreed, the binary won and the field
    // carries its authoritative name only if the use matched.
    // -----------------------------------------------------------------------
    ReanimationID mMainMenuReanimID;      // +0x118
    float mCameraPositionX;               // +0x11C
    float mCameraPositionY;               // +0x120
    LawnApp *mApp;                        // +0x124
    int mHasCameraTrack;                  // +0x128  the menu reanim has a "camera" track
    // The scene on screen and the one being slid towards. Both hold a MenuScene
    // and the constructor seeds both with -1.
    int mMenuScene;                       // +0x12C
    int mTargetMenuScene;                 // +0x130
    // The name of the looping menu track, and of the track the exit fades
    // through. The binary stores a Sexy::String here: the constructor writes the
    // empty-string sentinel word and the bodies assign string literals.
    // A plain `const char *`, not the refcounted Sexy::String the neighbouring
    // mExitTrackName is: SetScene stores the address of a track-name literal
    // straight into it (STR to [R4,#0x134], no assignment call) and the
    // constructor seeds it with a bare 0 rather than the shared empty rep, so
    // nothing ever takes a reference on it.
    const char *mPlayingTrackName;        // +0x134
    int mTransitionCounter;               // +0x138
    ReanimationID mSkyReanimID;           // +0x13C
    ReanimationID mSky2ReanimID;          // +0x140
    ReanimationID mHouseReanimID;         // +0x144
    ReanimationID mZombieHandReanimID;    // +0x148
    ReanimationID mCrowReanimID;          // +0x14C
    ReanimationID mFallingLeavesReanimID; // +0x150
    ReanimationID mButterflyReanimID;     // +0x154
    ReanimationID mOverlayReanimID;       // +0x158  non-zero hides the butterfly on the fade overlay
    int mPressedButtonId;                 // +0x15C
    Sexy::String mExitTrackName;          // +0x160
    int mEnterReanimationCounter;         // +0x164
    int mExitCounter;                     // +0x168
    bool mKeepDrawing;                    // +0x16C  store, almanac, mail, and the profile reload keep drawing
    bool mFirstTimeAdventure;             // +0x16D
    bool mMiniGameLocked;                 // +0x16E
    bool mCoopModeLocked;                 // +0x16F
    bool mPuzzleModeLocked;               // +0x170
    bool mSurvivalModeLocked;             // +0x171
    bool mVSModeLocked;                   // +0x172
    bool mStoreIntro;                     // +0x173  the store opens into its intro, then the next level
    bool mHandPlaying;                    // +0x174  the zombie hand is pushing the rock aside
    bool mCrowLeaving;                    // +0x175  the crow is off the menu after the exit starts
    bool mCrowTalking;                    // +0x176
    Sexy::Widget *mLastFocusedChild;      // +0x178
    int mFocusedChildCounter;             // +0x17C
    int mCrowIdleLoops;                   // +0x180  idle loops before the crow stretches or pecks
    int mUnseenMailCounter;               // +0x184
    int mButterflyAnimCounter;            // +0x188
    bool mRetainWidgetsOnExit;            // +0x18C
    bool mCreateUserDialogOpened;         // +0x18D
    float mFadeOriginX;                   // +0x190  the pressed button, where the iris closes
    float mFadeOriginY;                   // +0x194
    float mFadeCounterFloat;              // +0x198
    int mFadeHold;                        // +0x19C  reserved fade hold; the constructor clears it
    // TODO(recon): the three words at +0x1A0..+0x1AB are read in Update as one
    // object per menu scene, and each is called through vtable slot 7 with a
    // float alpha. Nothing in the binary ever stores a non-null value there, so
    // both loops are unreachable and no WidgetContainer slot 7 takes a scalar;
    // the real class cannot be recovered.
    Sexy::Widget *mSceneWidgets[3];       // +0x1A0 .. +0x1AB
    Sexy::Image *m2DMarkImage;            // +0x1AC
    // size 0x1B0
};
