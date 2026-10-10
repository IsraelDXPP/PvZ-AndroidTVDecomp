// TodCurveWidgetAnimator -- reconstructed from libGameMain.so
//
// The class animates one Sexy::Widget along a TodCurves-interpolated path: the
// x and y coordinates are evaluated independently between the two SexyVector2
// endpoints the constructor stores, and the widget is repositioned once per
// frame with Sexy::Widget::Move (vtable slot 52).
//
// Header note: Game\TodCurveWidgetAnimator.h still spells the derived words
// unk18..unk4C as `int`. The bodies show the real types -- unk18 is the
// TodCurves argument, unk20 is a bool flag and unk24/unk28/unk2C/unk30/unk34/
// unk38 hold floats -- so those six words are written through a float alias
// here. See the report for the header change that would make this exact.

#include "TodCurveWidgetAnimator.h"

#include "Widget.h"

// ==========================================================================
// TodCurveWidgetAnimator::IsCompleted()
// @001B8E44  22 bytes
// ==========================================================================
bool TodCurveWidgetAnimator::IsCompleted()
{
    // A negative duration loops and never reports complete.
    return mDuration >= 0 && mDuration < mUpdateCounter;
}

// ==========================================================================
// TodCurveWidgetAnimator::~TodCurveWidgetAnimator()
// @001B8E5C  14 bytes
// ==========================================================================
TodCurveWidgetAnimator::~TodCurveWidgetAnimator()
{
    // The binary's body only re-points the vptr at the base sub-object's table;
    // with real inheritance the compiler emits that itself.
}

// ==========================================================================
// TodCurveWidgetAnimator::Update()
// @001B8E94  356 bytes
// ==========================================================================
void TodCurveWidgetAnimator::Update()
{
    if (IsCompleted())
        return;

    // First tick: remember where the widget started, for the fade-in blend below.
    if (!mHasStart)
    {
        mHasStart = true;
        mStartX = (float)mWidget->mX;
        mStartY = (float)mWidget->mY;
    }

    int aBlendTicks = mBlendTicks;
    int aElapsed = mUpdateCounter;

    // While the first blend-in frames elapse, blend from the widget's original
    // position into the curve's position.
    int aBlending;
    float aBlendFraction;
    if (aBlendTicks <= 0)
    {
        aBlending = 0;
        aBlendFraction = 0.0f;
    }
    else if (aBlendTicks > aElapsed)
    {
        aBlending = 1;
        aBlendFraction = 1.0f - (float)(aBlendTicks - aElapsed) / (float)aBlendTicks;
        if (aBlendFraction < 0.0f)
            aBlendFraction = 0.0f;
        if (aBlendFraction > 1.0f)
            aBlendFraction = 1.0f;
    }
    else
    {
        aBlendFraction = 0.0f;
        aBlending = 0;
    }

    int aDuration = mDuration;
    int aNextElapsed = aElapsed + 1;
    mUpdateCounter = aNextElapsed;
    int aCurve = mCurve;
    float aFromX = mFromX;

    int aPosX;
    int aPosY;
    if (aDuration < 0)
    {
        // A negative budget loops. Both counters rewind when the loop closes.
        int aLoopLength = -aDuration;
        aPosX = (int)TodAnimateCurveFloat(0, -aDuration, aNextElapsed, aFromX, mToX, aCurve);
        aPosY = (int)TodAnimateCurveFloat(0, aLoopLength, mUpdateCounter, mFromY, mToY, aCurve);
        if (aLoopLength == mUpdateCounter)
        {
            mBlendTicks = 0;
            mUpdateCounter = 0;
        }
    }
    else
    {
        aPosX = (int)TodAnimateCurveFloat(0, aDuration, aNextElapsed, aFromX, mToX, aCurve);
        aPosY = (int)TodAnimateCurveFloat(0, mDuration, mUpdateCounter, mFromY, mToY, aCurve);
    }

    if (aBlending != 0)
    {
        aPosX = (int)(mStartX + ((float)aPosX - mStartX) * aBlendFraction);
        aPosY = (int)(mStartY + ((float)aPosY - mStartY) * aBlendFraction);
    }

    mWidget->Move(aPosX, aPosY);
}

// ==========================================================================
// TodCurveWidgetAnimator::TodCurveWidgetAnimator(Sexy::Widget *,TodCurves,Sexy::SexyVector2 &,Sexy::SexyVector2 &,int)
// @001B8FFC  92 bytes
// ==========================================================================
TodCurveWidgetAnimator::TodCurveWidgetAnimator(
    Sexy::Widget *theWidget, TodCurves theCurve, SexyVector2 &theFrom, SexyVector2 &theTo, int theDuration)
{
    mWidget = theWidget;
    mRefCount = 0;
    mUpdateCounter = 0;
    mCurve = theCurve;
    mDuration = theDuration;
    mHasStart = false;
    mStartX = 0.0f;
    mStartY = 0.0f;
    mFromX = theFrom.mX;
    mFromY = theFrom.mY;
    mToX = theTo.mX;
    mToY = theTo.mY;
}
