// HelpTextImageWidget -- reconstructed from libGameMain.so.
//
// Real polymorphic class: `class HelpTextImageWidget : public Sexy::Widget`.
// Widget occupies +0x00..+0xFF, so this class' single member is at +0x100.
//
// Size 0x10C, from the allocation site
//     LawnMenuParser.cpp:416   HelpTextImageWidget allocation, 0x10C bytes
// which is 0x100 (Widget) + 0x0C (Sexy::SharedImageRef, see
// Sexy\SharedImageRef.h). The old opaque blob said 0x110, one word too many.
//
// vtable order -- the RTTI export _ZTV19HelpTextImageWidget is the whole Widget
// table (122 slots) with ONE entry replaced:
//   [  0] 0x17c5d8  HelpTextImageWidget::~HelpTextImageWidget     (D2)
//   [  1] 0x17c60c  HelpTextImageWidget::~HelpTextImageWidget     (D0)
//   [ 36] 0x17c620  HelpTextImageWidget::Draw                     (override of Widget::Draw)
// Everything else still points at Widget/WidgetContainer code, so nothing else
// is redeclared here.
//
// The member at +0x100 is a Sexy::SharedImageRef, not a raw Image*, and the
// offsets were taken straight from the bodies in
// _recon\ida\HelpTextImageWidget.cpp: the constructor calls
// SharedImageRef::SharedImageRef(this + 0x100) and the destructor calls
// SharedImageRef::~SharedImageRef(this + 0x100), while Draw reads the image
// through SharedImageRef::operator Image* on the same address.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_HELPTEXTIMAGEWIDGET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_HELPTEXTIMAGEWIDGET_H

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept so other files' transitive dependencies do not change.
#include "Graphics.h"
#include "SharedImageRef.h"
#include "Widget.h"

class HelpTextImageWidget : public Sexy::Widget
{
public:
    virtual ~HelpTextImageWidget();

    // Overrides of Widget/WidgetContainer slots.
    virtual void Draw(Sexy::Graphics *theGraphics);

    HelpTextImageWidget();

    Sexy::SharedImageRef mImage; // +0x100, 0x0C bytes
    // size 0x10C
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_HELPTEXTIMAGEWIDGET_H
