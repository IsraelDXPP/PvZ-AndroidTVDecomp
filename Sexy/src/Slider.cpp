// Slider is a draggable 0..1 value widget.
//
// mHorizontal picks the axis. mDragging is set while the thumb is held.
// The grab offsets are the point inside the thumb the pointer landed on.
// Track and thumb coordinates are the ones recovered from the draw routine.

#include "Slider.h"

#include "Graphics.h"
#include "Image.h"
#include "SexyAppBase.h"
#include "Widget.h"

namespace {

double ClampUnit(double theValue)
{
    if (theValue < 0.0)
        return 0.0;
    if (theValue > 1.0)
        return 1.0;
    return theValue;
}

bool ThumbContains(int theOrigin, int theThumbSize, int thePos)
{
    return theOrigin <= thePos && thePos < theOrigin + theThumbSize;
}

} // namespace

void Sexy::Slider::SetValue(double theValue)
{
    mVal = ClampUnit(theValue);
    MarkDirtyFull();
}

bool Sexy::Slider::HasTransparencies()
{
    return true;
}

void Sexy::Slider::TouchDown(int theId, int x, int y)
{
    (void)theId;
    if (!mHorizontal)
    {
        int aThumbY = (int)((double)(mHeight - mThumbImage->GetHeight()) * mVal);
        if (ThumbContains(aThumbY, mThumbImage->GetHeight(), y))
        {
            mGrabOffsetY = y - aThumbY;
            mDragging = 1;
            return;
        }
        SetValue((double)y / (double)mHeight);
        if (mListener != nullptr)
            mListener->SliderVal(mId, mVal);
        return;
    }

    int aThumbX = (int)((double)(mWidth - mThumbImage->GetWidth()) * mVal);
    if (!ThumbContains(aThumbX, mThumbImage->GetWidth(), x))
    {
        SetValue((double)x / (double)mWidth);
        if (mListener != nullptr)
            mListener->SliderVal(mId, mVal);
        return;
    }
    mGrabOffsetX = x - aThumbX;
    mDragging = 1;
}

void Sexy::Slider::TouchUp(int theId, int x, int y)
{
    (void)theId;
    (void)x;
    (void)y;
    mDragging = 0;
    mListener->SliderVal(mId, mVal);
}

void Sexy::Slider::MouseDown(int x, int y, int theClickCount)
{
    (void)theClickCount;
    if (!mHorizontal)
    {
        int aThumbY = (int)((double)(mHeight - mThumbImage->GetHeight()) * mVal);
        if (ThumbContains(aThumbY, mThumbImage->GetHeight(), y))
        {
            mWidgetManager->mApp->SetCursor(2);
            mGrabOffsetY = y - aThumbY;
            mDragging = 1;
            return;
        }
        SetValue((double)y / (double)mHeight);
        if (mListener != nullptr)
            mListener->SliderVal(mId, mVal);
        return;
    }

    int aThumbX = (int)((double)(mWidth - mThumbImage->GetWidth()) * mVal);
    if (!ThumbContains(aThumbX, mThumbImage->GetWidth(), x))
    {
        SetValue((double)x / (double)mWidth);
        if (mListener != nullptr)
            mListener->SliderVal(mId, mVal);
        return;
    }
    mWidgetManager->mApp->SetCursor(2);
    mGrabOffsetX = x - aThumbX;
    mDragging = 1;
}

void Sexy::Slider::MouseMove(int x, int y)
{
    if (!mHorizontal)
    {
        int aThumbY = (int)((double)(mHeight - mThumbImage->GetHeight()) * mVal);
        mWidgetManager->mApp->SetCursor(ThumbContains(aThumbY, mThumbImage->GetHeight(), y) ? 2 : 0);
        return;
    }

    int aThumbX = (int)((double)(mWidth - mThumbImage->GetWidth()) * mVal);
    mWidgetManager->mApp->SetCursor(ThumbContains(aThumbX, mThumbImage->GetWidth(), x) ? 2 : 0);
}

void Sexy::Slider::MouseUp(int x, int y)
{
    (void)x;
    (void)y;
    mDragging = 0;
    mWidgetManager->mApp->SetCursor(0);
    mListener->SliderVal(mId, mVal);
}

Sexy::Slider::~Slider() = default;

void Sexy::Slider::MouseDrag(int x, int y)
{
    if (mDragging == 0)
        return;

    double aOld = mVal;
    double aNew = mHorizontal
        ? (double)(x - mGrabOffsetX) / (double)(mWidth - mThumbImage->GetWidth())
        : (double)(y - mGrabOffsetY) / (double)(mHeight - mThumbImage->GetHeight());
    aNew = ClampUnit(aNew);
    mVal = aNew;
    if (aOld != aNew)
    {
        mListener->SliderVal(mId, aNew);
        MarkDirtyFull();
    }
}

void Sexy::Slider::TouchMove(int theId, int x, int y)
{
    (void)theId;
    MouseDrag(x, y);
}

void Sexy::Slider::MouseLeave()
{
    if (mDragging == 0)
        mWidgetManager->mApp->SetCursor(0);
}

Sexy::Slider::Slider(Sexy::Image *theTrack, Sexy::Image *theThumb, int theId, Sexy::SliderListener *theListener)
{
    mTrackImage = theTrack;
    mThumbImage = theThumb;
    mId = theId;
    mListener = theListener;
    mVal = 0.0;
    mHorizontal = true;
    mDragging = 0;
    mGrabOffsetX = 0;
    mGrabOffsetY = 0;
}

void Sexy::Slider::Draw(Sexy::Graphics *g)
{
    if (mTrackImage != nullptr)
    {
        if (mSelected && mThumbImage == nullptr)
        {
            g->SetColorizeImages(true);
            g->SetColor(GetSelectColor());
        }

        if (mHorizontal)
        {
            int aTrackW = mTrackImage->GetWidth() / 3;
            int aTrackH = mTrackImage->GetHeight();
            int aTrackY = (mHeight - aTrackH) / 2;

            g->DrawImage(mTrackImage, 0, aTrackY, TRect<int>(0, 0, aTrackW, aTrackH));

            {
                Graphics aClip(*g);
                aClip.ClipRect(aTrackW, aTrackY, mWidth - 2 * aTrackW, aTrackH);
                int aX = aTrackW;
                for (int i = 0; i < (mWidth - aTrackW - 1) / aTrackW; ++i)
                {
                    aClip.DrawImage(mTrackImage, aX, aTrackY, TRect<int>(aTrackW, 0, aTrackW, aTrackH));
                    aX += aTrackW;
                }
            }

            g->DrawImage(mTrackImage, mWidth - aTrackW, aTrackY, TRect<int>(2 * aTrackW, 0, aTrackW, aTrackH));
        }
        else
        {
            int aTrackW = mTrackImage->GetWidth();
            int aTrackH = mTrackImage->GetHeight() / 3;

            g->DrawImage(mTrackImage, 0, 0, TRect<int>(0, 0, aTrackW, aTrackH));

            {
                Graphics aClip(*g);
                aClip.ClipRect(0, aTrackH, aTrackW, mHeight - 2 * aTrackH);
                int aY = aTrackH;
                for (int i = 0; i < (mHeight - aTrackH - 1) / aTrackH; ++i)
                {
                    aClip.DrawImage(mTrackImage, 0, aY, TRect<int>(0, aTrackH, aTrackW, aTrackH));
                    aY += aTrackH;
                }
            }

            g->DrawImage(mTrackImage, 0, mHeight - aTrackH, TRect<int>(0, 2 * aTrackH, aTrackW, aTrackH));
        }

        if (mSelected)
            g->SetColorizeImages(false);
    }

    if (mThumbImage != nullptr)
    {
        int aThumbW = mThumbImage->GetWidth();
        int aThumbH = mThumbImage->GetHeight();
        if (mHorizontal)
            g->DrawImage(mThumbImage, (int)((double)(mWidth - aThumbW) * mVal), (mHeight - aThumbH) / 2);
        else
            g->DrawImage(mThumbImage, (mWidth - aThumbW) / 2, (int)((double)(mHeight - aThumbH) * mVal));
    }

    if (!mSelected)
        return;

    g->SetDrawMode(1);
    if (mThumbImage != nullptr)
    {
        int aThumbW = mThumbImage->GetWidth();
        int aThumbH = mThumbImage->GetHeight();
        if (mHorizontal)
            g->DrawImage(mThumbImage, (int)((double)(mWidth - aThumbW) * mVal), (mHeight - aThumbH) / 2);
        else
            g->DrawImage(mThumbImage, (mWidth - aThumbW) / 2, (int)((double)(mHeight - aThumbH) * mVal));
    }
    g->SetDrawMode(0);
}

int Sexy::Slider::KeyDown(Sexy::KeyCode theKey)
{
    if (mHorizontal)
    {
        if ((theKey & 0xFFFFFFFD) != KEYCODE_LEFT)
            return Widget::KeyDown(theKey);
    }
    else if (theKey != KEYCODE_DOWN && theKey != KEYCODE_UP)
    {
        return Widget::KeyDown(theKey);
    }

    double aOld = mVal;
    double aNew = ((unsigned int)(theKey - KEYCODE_LEFT) > 1) ? aOld + 0.05 : aOld - 0.05;
    aNew = ClampUnit(aNew);
    mVal = aNew;
    if (aNew == aOld)
        return Widget::KeyDown(theKey);

    mListener->SliderVal(mId, aNew);
    MarkDirtyFull();
    return 1;
}

void Sexy::Slider::TouchCancel(const std::vector<Sexy::Event> &theEvents)
{
    mListener->SliderVal(mId, mVal);
    mDragging = 0;
    Widget::TouchCancel(theEvents);
}
