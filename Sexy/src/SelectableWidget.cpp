// SelectableWidget pulses its highlight alpha while it is selected.
//
// Update walks mSelectAlpha between mSelectAlphaMin and mSelectAlphaMax by
// mSelectAlphaDelta, flipping the delta at either end, and repaints when the
// byte-quantised alpha changes.

#include "SelectableWidget.h"

#include "Widget.h"

int Sexy::SelectableWidget::SetSelectColor(Color const& theColor)
{
    mSelectColor = theColor;
    return mSelectColor.mRed;
}

int Sexy::SelectableWidget::GetSelectAlpha()
{
    return (int)(float)(mSelectAlpha * 255.0f);
}

// Alpha is the stored colour's alpha scaled by the current pulse, rounded
// with (v + 128 + ((unsigned)v >> 8)) >> 8.
Sexy::Color Sexy::SelectableWidget::GetSelectColor()
{
    Color aColor = mSelectColor;
    int aScaledAlpha = aColor.mAlpha * GetSelectAlpha();
    aColor.mAlpha = (aScaledAlpha + 128 + ((unsigned int)(aScaledAlpha + 128) >> 8)) >> 8;
    return aColor;
}

Sexy::SelectableWidget::~SelectableWidget()
{
}

Sexy::SelectableWidget::SelectableWidget()
{
    Color aWhite(255, 255, 255, 255);
    mSelectColor = aWhite;

    mSelectAlpha = 1.0f;
    mSelectAlphaMin = 0.4f;
    mSelectAlphaMax = 1.0f;
    mSelectAlphaDelta = 0.01f;
}

int Sexy::SelectableWidget::SetSelectAlphaRange(float theMinAlpha, float theMaxAlpha)
{
    if (theMinAlpha <= theMaxAlpha)
    {
        mSelectAlphaMin = theMinAlpha;
        mSelectAlphaMax = theMaxAlpha;
    }
    return 1;
}

void Sexy::SelectableWidget::Update()
{
    Widget::Update();

    // The pulse runs only while the widget is live and the window has focus.
    if (mSelected != 0 && HasWindowFocus())
    {
        float anOldAlpha = mSelectAlpha;
        int anOldByteAlpha = (int)(anOldAlpha * 255.0f);
        float aDelta = mSelectAlphaDelta;

        float aNewAlpha = anOldAlpha + aDelta;
        mSelectAlpha = aNewAlpha;

        if (aNewAlpha > mSelectAlphaMax)
        {
            mSelectAlpha = mSelectAlphaMax;
            mSelectAlphaDelta = -aDelta;
            MarkDirty();
            aNewAlpha = mSelectAlpha;
        }

        if (aNewAlpha < mSelectAlphaMin)
        {
            float aBouncedDelta = mSelectAlphaDelta;
            mSelectAlpha = mSelectAlphaMin;
            mSelectAlphaDelta = -aBouncedDelta;
            MarkDirty();
            aNewAlpha = mSelectAlpha;
        }

        if ((int)(aNewAlpha * 255.0f) != anOldByteAlpha)
            MarkDirty();
    }
    else if (mSelectAlpha != 1.0f)
    {
        mSelectAlpha = 1.0f;
        MarkDirty();
    }
}
