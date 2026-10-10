// TodCurveWidgetAnimator -- reconstructed from libGameMain.so.
//
// Polymorphic class derived from Sexy::WidgetAnimator (Sexy\WidgetAnimator.h).
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x1b8e5c  TodCurveWidgetAnimator::~TodCurveWidgetAnimator
//   [  1] 0x1b8e74  TodCurveWidgetAnimator::~TodCurveWidgetAnimator
//   [  2] 0x17da48  Sexy::WidgetAnimator::Init            (inherited)
//   [  3] 0x1b8e94  TodCurveWidgetAnimator::Update
//   [  4] 0x1b8e44  TodCurveWidgetAnimator::IsCompleted
//   [  5] 0x17da60  Sexy::WidgetAnimator::HideOnCompletion (inherited)
// Slots 2 and 5 belong to the base and must NOT be re-declared here.
// Itanium/ARM gives the destructor two slots; MSVC x86 gives it one, so the
// generated absolute slot numbers are one lower than the comments above.
//
// Size 0x50, the allocation the transcription recorded. The base is 0x18 in the
// binary (vptr + two words + mWidget + mUpdateCounter + mHideOnCompletion), so
// the derived area runs from +0x18 to +0x4F.
//
// The constructor's signature is the recon dump's:
//     TodCurveWidgetAnimator(Sexy::Widget *a2, TodCurves a3, SexyVector2 &a4,
//                            SexyVector2 &a5, int a6)
// as recovered from _ZN21TodCurveWidgetAnimatorC1EPN4Sexy6WidgetE9TodCurvesRNS_11SexyVector2ES4_i.

#ifndef LAWNPROJECTTV_TODLIB_INCLUDE_TODCURVEWIDGETANIMATOR_H
#define LAWNPROJECTTV_TODLIB_INCLUDE_TODCURVEWIDGETANIMATOR_H

#include "ReconTypes.h"
#include "WidgetAnimator.h"

class TodCurveWidgetAnimator : public Sexy::WidgetAnimator
{
public:
    virtual ~TodCurveWidgetAnimator();

    virtual void Update();
    virtual bool IsCompleted();

    TodCurveWidgetAnimator(
        Sexy::Widget *theWidget, TodCurves theCurve, SexyVector2 &theFrom, SexyVector2 &theTo, int theDuration);

    // Derived area, binary +0x18 .. +0x4F.
    // TODO(recon): the curve animator has not been transcribed; the recon dump
    // only shows writes at binary +24, +28, +32, +44, +48, +52, +56 and +64, so
    // none of these words has a recovered name yet.
    TodCurves mCurve;
    int mDuration;
    bool mHasStart;
    float mStartX;
    float mStartY;
    float mFromX;
    float mFromY;
    float mToX;
    float mToY;
};

#endif // LAWNPROJECTTV_TODLIB_INCLUDE_TODCURVEWIDGETANIMATOR_H
