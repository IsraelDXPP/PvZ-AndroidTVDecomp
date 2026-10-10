// ReanimTrackWidgetAnimator -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class ReanimTrackWidgetAnimator : public Sexy::WidgetAnimator`
// (RTTI "25ReanimTrackWidgetAnimator", primary base "N4Sexy14WidgetAnimatorE" at
// offset 0, vtable 0x684830). The authoritative header spells the base
// `Sexy::__WidgetAnimator`; this tree spells it `Sexy::WidgetAnimator`.
//
// WidgetAnimator is 6 words = 0x18, which is exactly where this class's first own
// member sits -- the authoritative header numbers mApp as ordinal 6.
//
// vtable order. Slots 0..5 are the base's; this class overrides 2, 3 and 4 and
// leaves 5 (Sexy::WidgetAnimator::HideOnCompletion) alone:
//   [  0] 0x1b8a5c  ReanimTrackWidgetAnimator::~ReanimTrackWidgetAnimator  (D1)
//   [  1] 0x1b8b1c  ReanimTrackWidgetAnimator::~ReanimTrackWidgetAnimator  (D0)
//   [  2] 0x1b8a20  ReanimTrackWidgetAnimator::Init(WidgetAnimationState)
//   [  3] 0x1b8be0  ReanimTrackWidgetAnimator::Update()
//   [  4] 0x1b8a1c  ReanimTrackWidgetAnimator::IsCompleted()
//   [  5] 0x17da60  Sexy::WidgetAnimator::HideOnCompletion()
// MSVC x86 emits ONE slot for a virtual destructor where Itanium emits two, so
// the declaration order below is one shorter and otherwise the same.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMTRACKWIDGETANIMATOR_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMTRACKWIDGETANIMATOR_H

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Reanimation.h"
#include "ReanimatorTransform.h"
#include "WidgetAnimator.h"
#include <string>
#include <vector>
#include <map>

// LawnApp is reached only through `LawnApp *mApp` (Update reads that pointer and
// the flag at its +0x118), so it is declared rather than included. Removing this
// include also breaks the path that made LawnMenuWidget.h report four C2504s:
// LawnMenuParser.h includes this header, LawnApp.h includes MainMenu.h, and
// MainMenu/VSSetupMenu/HelpTextScreen/VSResultsMenu derive from LawnMenuWidget.
// Sexy\ReconTypes.h already carries the declaration.
class LawnApp;
#include <set>

class LawnMenuWidget;

class ReanimTrackWidgetAnimator : public Sexy::WidgetAnimator
{
public:
    virtual ~ReanimTrackWidgetAnimator();
    // Slot 2, _ZN25ReanimTrackWidgetAnimator4InitEN4Sexy20WidgetAnimationStateE
    // @0x1b8a20.  The return type is the base's `void` now that
    // Sexy\WidgetAnimator.h declares the real parameter list: a different return
    // type would be C2555, and the binary's body returns nothing of its own (it
    // tail-calls Reanimation::AssignRenderGroupToTrack, which is void).
    virtual void Init(Sexy::WidgetAnimationState theState);
    virtual void Update();
    // Slot 4. The base is pure (`virtual bool IsCompleted() = 0`,
    // WidgetAnimator.h:44); the `int` was the transcriber's fallback and made this
    // a C2555 instead of the required override.
    virtual bool IsCompleted();

    LawnApp *mApp; // +0x18  (Update reads it and its +0x118 flag)
    Sexy::Widget *mLawnMenuWidget; // +0x1C  (Update reads its +0x118 field)
    // The authoritative header types these three as `int *`, which is how the
    // decompiler renders a 4-byte COW string rep; the constructor takes them as
    // `Sexy::String` by value and the destructor releases them as string reps
    // (`*(*(this + 9) - 12)`), so they are declared as the string they are.
    Sexy::String mTrackName1; // +0x20
    Sexy::String mTrackName2; // +0x24
    Sexy::String mTrackName3; // +0x28
    // Added to the track position when the widget is placed.
    float mOffsetX;
    float mOffsetY;

    ReanimTrackWidgetAnimator(Sexy::Widget *theWidget, LawnMenuWidget *theMenu, Sexy::String theTrackName1,
        Sexy::String theTrackName2, Sexy::String theTrackName3);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMTRACKWIDGETANIMATOR_H
