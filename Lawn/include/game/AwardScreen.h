// AwardScreen -- reconstructed from libGameMain.so.
//
// Polymorphic class derived from Sexy::Widget (Sexy\Widget.h) plus a second
// polymorphic base at +0x100. The constructor stores two vptrs -- this + 0 and
// this + 256 -- and hands the pointer at this + 256, typed as ButtonListener*, to the
// GameButton constructor, so the second base is Sexy::ButtonListener, which is
// exactly where it lands once sizeof(Sexy::Widget) is 0x100.
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x145b04  AwardScreen::~AwardScreen
//   [  1] 0x145ba0  AwardScreen::~AwardScreen
//   [ 29] 0x146b18  AwardScreen::AddedToManager
//   [ 30] 0x146b1c  AwardScreen::RemovedFromManager
//   [ 31] 0x146acc  AwardScreen::Update
//   [ 36] 0x1441f4  AwardScreen::Draw
//   [ 50] 0x143e60  AwardScreen::Resize
//   [ 61] 0x146b14  AwardScreen::GotFocus
//   [ 67] 0x1459d8  AwardScreen::KeyChar
//   [ 71] 0x145934  AwardScreen::KeyDown
//   [ 76] 0x143e64  AwardScreen::MouseDown
//   [ 79] 0x145928  AwardScreen::MouseUp
// All twelve are overrides; AwardScreen adds no new slot, so every slot number
// above belongs to the inherited Widget / WidgetContainer / ButtonListener
// table. Itanium/ARM gives the destructor two slots, MSVC x86 gives it one, so
// the generated absolute slot numbers are one lower than the comments above.
//
// The return types below are the base's, not the transcription's: the .cpp
// currently returns int/_DWORD* for Draw/Resize/MouseDown/... because the
// decompiler guessed at every return type. TODO(recon): those .cpp definitions
// have to be reduced to void in the next pass; until then this header and the
// .cpp disagree, exactly like Game\Board.h and the other already-converted
// widget headers.
//
// Layout after the two bases (sizeof(Sexy::Widget) == 0x100, ButtonListener == 4):
//     +0x104  GameButton *mStartButton -- written as the word at this + 260
//     +0x108  GameButton *mMenuButton  -- never written by any transcribed body;
//             the field is kept because the restore tree declares it
//     +0x10C  LawnApp *mApp            -- written as the word at this + 268
//     +0x110  int mFadeInCounter       -- constructor stores 180
//     +0x114  AwardType mAwardType     -- constructor stores its third argument
//     +0x118  Sexy::RbTree::MapStorage mLoadedResources -- the set of resource
//             names the constructor and LoadResources maintain (this+280..this+303:
//             the constructor stores this+284 into +292 and +296, and the
//             destructor runs Sexy::RbTree::_M_erase over +288). 24 bytes because
//             the binary's container is libstdc++'s std::map header, not MSVC's.
//     +0x130  end of object

#pragma once

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "GameButton.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnPlayerInfo.h"
#include "StoreScreen.h"
#include "StretchableGameButton.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "ZenGarden.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// LawnApp is reached only through pointers here (`LawnApp *mApp` and the
// constructor's parameter), so it is declared rather than included. Including it
// closed the LawnApp.h <-> AwardScreen.h cycle, and a cycle is not harmless:
// entering from this side left LawnApp undefined while LawnApp.h was still being
// read, so every derived-of-LawnDialog class in the same translation unit
// reported C2504. Sexy\ReconTypes.h already carries this declaration.
class LawnApp;

class AwardScreen : public Sexy::Widget, public Sexy::ButtonListener
{
public:
    virtual ~AwardScreen();

    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);   // [ 29] 0x146b18
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager); // [ 30] 0x146b1c
    virtual void Update();                                  // [ 31] 0x146acc
    virtual void Draw(Sexy::Graphics *theGraphics);                  // [ 36] 0x1441f4
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);    // [ 50] 0x143e60
    virtual void GotFocus();                                // [ 61] 0x146b14
    virtual int KeyChar(char theChar);                          // [ 67] 0x1459d8
    virtual int KeyDown(Sexy::KeyCode theKey);                 // [ 71] 0x145934
    virtual void MouseDown(int theX, int theY, int theClickCount);         // [ 76] 0x143e64
    virtual void MouseUp(int theX, int theY, int theClickCount);           // [ 79] 0x145928

    // Not vtable slots: the signatures are the ones the transcribed bodies use.
    int IsPaperNote();
    int DrawBottom(Sexy::Graphics *theGraphics, Sexy::String const &theTitle, Sexy::String const &theSubtitle, Sexy::String const &theDescription);
    void DrawAwardSeed(Sexy::Graphics *theGraphics);
    void StartButtonPressed();
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    int LoadResources(Sexy::String const &theGroup);
    AwardScreen(LawnApp *theApp, AwardType theAward);

    GameButton *mStartButton; // +0x104
    GameButton *mMenuButton;  // +0x108
    LawnApp *mApp;            // +0x10C
    int mFadeInCounter;       // +0x110
    AwardType mAwardType;     // +0x114
    std::set<Sexy::String> mLoadedResources;
};
