// MaskHelpWidget -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class MaskHelpWidget : public Sexy::Widget`
// (RTTI "14MaskHelpWidget", primary base "N4Sexy6WidgetE" at offset 0,
// vtable 0x686300). There is no secondary base, so Widget's 0x100 bytes are the
// whole base and this class's own members start right at +0x100.
//
// Object size 0x108: the constructor writes -1 to +0x100 and the LawnApp pointer
// to +0x104, and nothing else in the class touches a byte beyond +0x107. The
// transcribed class body was an opaque 0x1A0-byte block, a size guess.
//
// vtable order -- this class's own slots; the other 118 come from Widget:
//   [  0] 0x1ec1e8  MaskHelpWidget::~MaskHelpWidget    (D1)
//   [  1] 0x1ec208  MaskHelpWidget::~MaskHelpWidget    (D0)
//   [ 30] 0x1ec9f4  MaskHelpWidget::RemovedFromManager (override)
//   [ 31] 0x1ec1dc  MaskHelpWidget::Update             (override)
//   [ 36] 0x1ec258  MaskHelpWidget::Draw               (override)
// Slot [29] holds 0x1ec250, which is a bare tail-call thunk to
// `Sexy::WidgetContainer::AddedToManager` -- the binary's own slot still runs the
// base implementation, so AddedToManager is deliberately NOT declared here
// (map-lawn section 2.I).
// MSVC x86 gives a virtual destructor ONE slot where Itanium gives two (D1+D0).

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_MASKHELPWIDGET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_MASKHELPWIDGET_H

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "Graphics.h"
#include "Widget.h"
#include "WidgetContainer.h"

// LawnApp is reached only through `LawnApp *mApp` and the constructor's pointer
// parameter, so it is declared rather than included: the include was half of the
// LawnApp.h <-> MaskHelpWidget.h cycle. Sexy\ReconTypes.h already carries it.
class LawnApp;

class MaskHelpWidget : public Sexy::Widget
{
public:
    virtual ~MaskHelpWidget();

    // Both are `void` in the base chain -- WidgetContainer.h 150
    // (RemovedFromManager) and Widget.h 66 (Update) -- which is what the binary's
    // slots 30 and 31 need; the transcription's `int` made them C2555s.
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager);
    virtual void Update();
    // Slot 36 is `Draw(Sexy::Graphics *)` in the binary (0x1ec258, whose mangled
    // name is _ZN14MaskHelpWidget4DrawEPN4Sexy8GraphicsE) and it overrides
    // Widget's own slot 36, so this is a real override. The transcription's
    // `float Draw(float a3)` was the decompiler reusing its third argument
    // register; the body never reads it.
    virtual void Draw(Sexy::Graphics *g);

    // Widget occupies +0x00..+0xFF.
    int mHelpTextId; // +0x100  (the constructor and RemovedFromManager store -1)
    LawnApp *mApp; // +0x104
    // Size 0x108.

    explicit MaskHelpWidget(LawnApp *theApp);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_MASKHELPWIDGET_H
