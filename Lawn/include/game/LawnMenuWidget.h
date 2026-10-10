// LawnMenuWidget -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class LawnMenuWidget : public Sexy::MenuWidget` (the
// authoritative PvZ\Lawn\Widget\LawnMenuWidget.h:25). MenuWidget itself derives
// from Sexy::Widget and Sexy::ButtonListener, which is why the constructor also
// writes a second vptr at +0x100 -- Widget is exactly 0x100 bytes, so the
// ButtonListener subobject starts there (LawnMenuWidget.cpp: `*((_DWORD *)this
// + 64) = &off_67F328;`). The destructor's non-virtual thunk adjusts `this` by
// -256 for the same reason.
//
// vtable order -- from the RTTI export _ZTV14LawnMenuWidgetE (128 function
// slots). Everything up to slot [126] belongs to Widget/WidgetContainer/
// MenuWidget; the class's own entries are, in declaration order (MSVC x86
// collapses the Itanium D2/D0 pair into one destructor slot):
//   [  0] 0x16930c  LawnMenuWidget::~LawnMenuWidget
//   [  1] 0x1693f0  LawnMenuWidget::~LawnMenuWidget
//   [127] 0x169141  LawnMenuWidget::CreateMenuParser
//
// Members: MenuWidget's own fields run to word 69 (its mImage at +0x114), so
// LawnMenuWidget's share starts at word 70. The old header spelled those three
// words `int data118[3]`. They are the menu backdrop reanimation and the two
// camera offsets the track animator adds when it places a button. A derived
// menu uses these words; it does not declare a second copy.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNMENUWIDGET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNMENUWIDGET_H

#include "ReconTypes.h"
// The transcribed bodies call methods on these, so the includes stay.
#include "LawnMenuParser.h"
#include "MenuWidget.h"
#include "Widget.h"

// LawnApp is not named anywhere in this file -- there is no member, no parameter
// and no return value of that type -- so the include was pure cycle. It mattered:
// MainMenu, VSSetupMenu, HelpTextScreen and VSResultsMenu all derive from
// LawnMenuWidget and LawnApp.h includes all four, so entering from this side
// reached LawnApp.h, came back here while this file was mid-parse, and left
// LawnMenuWidget undefined for all four derived classes (4 x C2504).
// Sexy\ReconTypes.h already carries this declaration.
class LawnApp;

class LawnMenuWidget : public Sexy::MenuWidget
{
public:
    virtual ~LawnMenuWidget();

    virtual int CreateMenuParser(); // [127]

    // The binary's HelpTextScreen constructor (HelpTextScreen.cpp:212 in the
    // transcription) calls Sexy::MenuWidget::MenuWidget on `this` with the menu
    // file name, so this class does have a one-line forwarding constructor and
    // the compiler inlined it at that call site. It is defined here because
    // Game\LawnMenuWidget.cpp (another workstream's file) carries no body for it.
    LawnMenuWidget() :
        mBackdropReanimID(ReanimationID::REANIMATIONID_NULL),
        mCameraOffsetX(0),
        mCameraOffsetY(0)
    {
    }
    explicit LawnMenuWidget(Sexy::String theMenuFile) :
        MenuWidget(theMenuFile),
        mBackdropReanimID(ReanimationID::REANIMATIONID_NULL),
        mCameraOffsetX(0),
        mCameraOffsetY(0)
    {
    }

    // Words 70..72, +0x118..+0x123. The destructor releases the reanimation.
    // The track animator reads the id and adds the two offsets to a track
    // transform. MainMenu stores its menu reanim and camera in these words.
    ReanimationID mBackdropReanimID; // +0x118
    float mCameraOffsetX; // +0x11C
    float mCameraOffsetY; // +0x120

    // Everything below belongs to MenuWidget, not to this class:
    //   bool  mIsFading        +0x104
    //   bool  unkBool2         +0x105
    //   void *mMenuParser      +0x108
    //   void *mSourceFileName  +0x10C
    //   int   unkInt1          +0x110
    //   Image *mImage          +0x114
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNMENUWIDGET_H
