// StoreScreenOverlay -- reconstructed from libGameMain.so.
//
// Inheritance chain: `class StoreScreenOverlay : public Sexy::Widget`
// (phase-1 group table; confirmed by the binary's ZTV18StoreScreenOverlay
// @0x681db0, whose slot 36 is StoreScreenOverlay::Draw and whose destructor
// chains straight into `Sexy::Widget::~Widget()` -- StoreScreenOverlay.cpp:30).
//
// The primary vtable has 122 slots, exactly Sexy::Widget's count, and this class
// overrides only [0]/[1] (the destructor) and [36] (Draw). Every other slot still
// holds the Widget/WidgetContainer implementation.
//
//   [  0] 0x18d7bc  StoreScreenOverlay::~StoreScreenOverlay
//   [  1] 0x18d7dc  StoreScreenOverlay::~StoreScreenOverlay
//   [ 36] 0x18d7a8  StoreScreenOverlay::Draw
//   [  2.. 35] Sexy::WidgetContainer / Sexy::Widget
//   [ 37..121] Sexy::Widget / Sexy::WidgetContainer
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0; only the
// declaration order matters.
//
// Size 0x108, from the binary's allocation sites (the transcriber recorded that
// figure directly rather than the "largest this-relative offset" fallback).

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_STORESCREENOVERLAY_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_STORESCREENOVERLAY_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "Widget.h"

class StoreScreenOverlay : public Sexy::Widget
{
public:
    virtual ~StoreScreenOverlay(); // [0],[1] 0x000/0x004

    virtual void Draw(Sexy::Graphics *g); // [36] 0x090

    // +0x100: the widget StoreScreenOverlay forwards to. Draw() dispatches
    // virtual slot 54 (0xD8 = DrawOverlay) on it (StoreScreenOverlay.cpp:11).
    // TODO(recon): the authoritative project never named this member.
    Sexy::Widget *mStoreScreen; // +0x100
    int mOverlayWord; // +0x104  unread tail word of the 0x108 allocation
    // 0x108
};

// sizeof(StoreScreenOverlay) can only be asserted once Sexy::Widget carries its
// real 0x100-byte layout; it is still opaque storage of 0x7A0 bytes here.
// TODO(recon): enable `static_assert(sizeof(StoreScreenOverlay) == 0x108, ...)`.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_STORESCREENOVERLAY_H
