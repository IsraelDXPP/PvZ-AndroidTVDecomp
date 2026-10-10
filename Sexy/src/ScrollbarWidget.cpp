// ScrollbarWidget draws a track, pages when the track is clicked, and scrolls from its arrow buttons.

#include "ScrollbarWidget.h"
#include "ReconData.h"

#include "Graphics.h"
#include "Image.h"
#include "ScrollListener.h"
#include "ScrollbuttonWidget.h"
#include "Widget.h"
#include "WidgetContainer.h"

void Sexy::ScrollbarWidget::MouseDown(int theX, int theY, int theClickCount)
{
    Widget::MouseDown(theX, theY, theClickCount);
}

void Sexy::ScrollbarWidget::MouseUp(int theX, int theY)
{
    Widget::MouseUp(theX, theY);
}

void Sexy::ScrollbarWidget::SetHorizontal(bool isHorizontal)
{
    mHorizontal = isHorizontal;
    mDownButton->mInvisIfNoScroll = isHorizontal;
    mUpButton->mInvisIfNoScroll = isHorizontal;
}

void Sexy::ScrollbarWidget::SetButtonImages(
    Sexy::Image *theButtonImage, Sexy::Image *theDownImage, Sexy::Image *theDisabledImage)
{
    mUpButton->mButtonImage = theButtonImage;
    mUpButton->mDownImage = theDownImage;
    mUpButton->mDisabledImage = theDisabledImage;
    mDownButton->mButtonImage = theButtonImage;
    mDownButton->mDownImage = theDownImage;
    mDownButton->mDisabledImage = theDisabledImage;
}

void Sexy::ScrollbarWidget::SetThumbImage(Sexy::Image *theThumbImage)
{
    mThumbImage = theThumbImage;
}

void Sexy::ScrollbarWidget::SetBarImages(Sexy::Image *theBarImage, Sexy::Image *thePagingImage)
{
    mBarImage = theBarImage;
    mPagingImage = thePagingImage;
}

void Sexy::ScrollbarWidget::SetButtonColors(int (*theColors)[3], int theCount)
{
    mUpButton->SetColors(theColors, theCount);
    mDownButton->SetColors(theColors, theCount);
}

void Sexy::ScrollbarWidget::SetButtonColors(int (*theColors)[4], int theCount)
{
    mUpButton->SetColors(theColors, theCount);
    mDownButton->SetColors(theColors, theCount);
}

void Sexy::ScrollbarWidget::SetButtonColor(int theIdx, Color const &theColor)
{
    mUpButton->SetColor(theIdx, theColor);
    mDownButton->SetColor(theIdx, theColor);
}

int Sexy::ScrollbarWidget::ResizeScrollbar(int theX, int theY, int theWidth, int theHeight)
{
    Resize(theX, theY, theWidth, theHeight);

    if (mHorizontal)
    {
        mUpButton->Resize(0, 0, theHeight, theHeight);
        mDownButton->Resize(theWidth - theHeight, 0, theHeight, theHeight);
    }
    else
    {
        mUpButton->Resize(0, 0, theWidth, theWidth);
        mDownButton->Resize(0, theHeight - theWidth, theWidth, theWidth);
    }

    return 0;
}

void Sexy::ScrollbarWidget::SetMaxValue(double theMaxValue)
{
    mMaxValue = theMaxValue;
    ClampValue();
    MarkDirty();
}

void Sexy::ScrollbarWidget::SetPageSize(double thePageSize)
{
    mPageSize = thePageSize;
    ClampValue();
    MarkDirty();
}

void Sexy::ScrollbarWidget::SetValue(double theValue)
{
    mValue = theValue;
    ClampValue();
    if (mScrollListener != nullptr)
        mScrollListener->ScrollPosition(mId, mValue);
    MarkDirty();
}

bool Sexy::ScrollbarWidget::AtBottom()
{
    return mMaxValue - mPageSize - mValue <= 1.0;
}

void Sexy::ScrollbarWidget::GoToBottom()
{
    mValue = mMaxValue - mPageSize;
    ClampValue();
    SetValue(mValue);
}

int Sexy::ScrollbarWidget::DrawThumb(Sexy::Graphics *theGraphics, TRect<int> theRect)
{
    return DrawThumb(theGraphics, theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

int Sexy::ScrollbarWidget::ClampValue()
{
    double oldValue = mValue;
    double limit = mMaxValue - mPageSize;

    if (oldValue > limit)
        mValue = limit;
    if (mValue < 0.0)
        mValue = 0.0;

    bool noScroll = mMaxValue <= mPageSize;
    SetDisabled(noScroll);
    mUpButton->SetDisabled(noScroll);
    mDownButton->SetDisabled(noScroll);
    if (mInvisIfNoScroll)
    {
        bool visible = mMaxValue > mPageSize;
        SetVisible(visible);
        mDownButton->SetVisible(visible);
        mUpButton->SetVisible(visible);
    }

    if (mValue != oldValue && mScrollListener != nullptr)
        mScrollListener->ScrollPosition(mId, mValue);

    return 0;
}

void Sexy::ScrollbarWidget::SetThumbPosition(int thePosition)
{
    int thumbOffset = thePosition - mUpButton->mWidth;
    double range = mMaxValue - mPageSize;
    int trackSize = GetTrackSize();
    int thumbSize = GetThumbSize();
    SetValue((double)thumbOffset * range / (double)(trackSize - thumbSize));
}

void Sexy::ScrollbarWidget::ButtonDepress(int) {}

int Sexy::ScrollbarWidget::ThumbCompare(int theX, int theY)
{
    int pos = mHorizontal ? theX - mUpButton->mWidth : theY - mUpButton->mHeight;
    if (pos < GetThumbPosition())
        return -1;
    return pos >= GetThumbSize() + GetThumbPosition();
}

void Sexy::ScrollbarWidget::Draw(Sexy::Graphics *theGraphics)
{
    int thumbSize = GetThumbSize();
    int trackExtent = mHorizontal ? mWidth : mHeight;
    int trackOffset = (trackExtent - GetTrackSize()) / 2;
    int thumbPos = GetThumbPosition();
    int halfThumb = thumbSize / 2;

    TRect<int> beforeThumb;
    TRect<int> afterThumb;
    TRect<int> thumbRect;

    if (mHorizontal)
    {
        beforeThumb = TRect<int>(trackOffset, 0, thumbPos + halfThumb, mHeight);
        afterThumb = TRect<int>(trackOffset + thumbPos + halfThumb, 0, GetTrackSize() - thumbPos - halfThumb, mHeight);
        thumbRect = TRect<int>(trackOffset + thumbPos, 0, thumbSize, mHeight);
    }
    else
    {
        beforeThumb = TRect<int>(0, trackOffset, mWidth, thumbPos + halfThumb);
        afterThumb = TRect<int>(0, trackOffset + thumbPos + halfThumb, mWidth, GetTrackSize() - thumbPos - halfThumb);
        thumbRect = TRect<int>(0, trackOffset + thumbPos, mWidth, thumbSize);
    }

    if (mUpdateMode == 1)
    {
        if (mPagingImage != nullptr)
            theGraphics->DrawImageBox(beforeThumb, mPagingImage);
        else
        {
            theGraphics->SetColor(GetColor(1));
            theGraphics->FillRect(beforeThumb);
        }
    }
    else if (mBarImage != nullptr)
        theGraphics->DrawImageBox(beforeThumb, mBarImage);
    else
    {
        theGraphics->SetColor(GetColor(0));
        theGraphics->FillRect(beforeThumb);
    }

    if (mUpdateMode == 2)
    {
        if (mPagingImage != nullptr)
            theGraphics->DrawImageBox(afterThumb, mPagingImage);
        else
        {
            theGraphics->SetColor(GetColor(1));
            theGraphics->FillRect(afterThumb);
        }
    }
    else if (mBarImage != nullptr)
        theGraphics->DrawImageBox(afterThumb, mBarImage);
    else
    {
        theGraphics->SetColor(GetColor(0));
        theGraphics->FillRect(afterThumb);
    }

    if (thumbSize > 0)
        DrawThumb(theGraphics, thumbRect);
}

Sexy::ScrollbarWidget::~ScrollbarWidget()
{
    RemoveAllWidgets(true, false, nullptr);
}

void Sexy::ScrollbarWidget::SetInvisIfNoScroll(bool invisIfNoScroll)
{
    mInvisIfNoScroll = invisIfNoScroll;
    if (invisIfNoScroll)
    {
        SetVisible(false);
        mDownButton->SetVisible(false);
        mUpButton->SetVisible(false);
    }
}

int Sexy::ScrollbarWidget::GetTrackSize()
{
    if (mHorizontal)
        return mWidth - 2 * mUpButton->mWidth;
    return mHeight - 2 * mUpButton->mWidth;
}

int Sexy::ScrollbarWidget::GetThumbPosition()
{
    if (mPageSize <= mMaxValue)
    {
        int trackRemainder = GetTrackSize() - GetThumbSize();
        return (int)((double)trackRemainder * mValue / (mMaxValue - mPageSize) + 0.5);
    }

    if (mHorizontal)
        return mUpButton->mWidth;
    return mUpButton->mHeight;
}

void Sexy::ScrollbarWidget::ButtonPress(int theId)
{
    mButtonAcc = 0;
    if (theId != 0)
        SetValue(mValue + 1.0);
    else
        SetValue(mValue - 1.0);
}

void Sexy::ScrollbarWidget::ButtonDownTick(int theId)
{
    mButtonAcc = mButtonAcc + 1;
    if (mButtonAcc <= 24)
        return;

    if (theId != 0)
        SetValue(mValue + 1.0);
    else
        SetValue(mValue - 1.0);

    mButtonAcc = 24;
}

int Sexy::ScrollbarWidget::DrawThumb(Sexy::Graphics *theGraphics, int theX, int theY, int theWidth, int theHeight)
{
    if (mThumbImage != nullptr)
    {
        theGraphics->DrawImageBox(TRect<int>(theX, theY, theWidth, theHeight), mThumbImage);
        return 0;
    }

    theGraphics->SetColor(GetColor(2));
    theGraphics->FillRect(theX, theY, theWidth, theHeight);

    theGraphics->SetColor(GetColor(5));
    theGraphics->FillRect(theX + 1, theY + 1, theWidth - 2, 1);
    theGraphics->FillRect(theX + 1, theY + 1, 1, theHeight - 2);

    theGraphics->SetColor(GetColor(3));
    theGraphics->FillRect(theX, theHeight + theY - 1, theWidth, 1);
    theGraphics->FillRect(theWidth + theX - 1, theY, 1, theHeight);

    theGraphics->SetColor(GetColor(4));
    theGraphics->FillRect(theX + 1, theHeight + theY - 2, theWidth - 2, 1);
    theGraphics->FillRect(theWidth + theX - 2, theY + 1, 1, theHeight - 2);
    return 0;
}

int Sexy::ScrollbarWidget::GetThumbSize()
{
    if (mPageSize > mMaxValue)
        return 0;

    int thumbSize = (int)((double)GetTrackSize() * mPageSize / mMaxValue + 0.5);
    if (thumbSize < 8)
        return 8;
    return thumbSize;
}

Sexy::ScrollbarWidget::ScrollbarWidget(int theId, Sexy::ScrollListener *theScrollListener)
{
    mId = theId;
    mScrollListener = theScrollListener;
    SetDisabled(true);

    mUpButton = new ScrollbuttonWidget(0, this, 0);
    mUpButton->SetDisabled(true);
    mDownButton = new ScrollbuttonWidget(1, this, 0);
    mDownButton->SetDisabled(true);

    mThumbImage = nullptr;
    mBarImage = nullptr;
    mPagingImage = nullptr;
    mInvisIfNoScroll = false;
    mPressedOnThumb = false;
    mUpdateAcc = 0;
    mButtonAcc = 0;
    mUpdateMode = 0;
    mHorizontal = false;
    mValue = 0.0;
    mMaxValue = 0.0;
    mPageSize = 0.0;

    AddWidget(mUpButton);
    AddWidget(mDownButton);
    SetColors(gScrollbarColors, 6);
}

void Sexy::ScrollbarWidget::Update()
{
    Widget::Update();

    if (mUpdateMode == 1)
    {
        if (ThumbCompare(mLastMouseX, mLastMouseY) != -1)
        {
            mUpdateMode = 0;
            MarkDirty();
            return;
        }

        mUpdateAcc = mUpdateAcc + 1;
        if (mUpdateAcc <= 24)
            return;

        SetValue(mValue - mPageSize);
        mUpdateAcc = 20;
    }
    else if (mUpdateMode == 2)
    {
        if (ThumbCompare(mLastMouseX, mLastMouseY) != 1)
        {
            mUpdateMode = 0;
            MarkDirty();
            return;
        }

        mUpdateAcc = mUpdateAcc + 1;
        if (mUpdateAcc <= 24)
            return;

        SetValue(mPageSize + mValue);
        mUpdateAcc = 20;
    }
}

void Sexy::ScrollbarWidget::MouseDown(int theX, int theY, int theBtnNum, int theClickCount)
{
    Widget::MouseDown(theX, theY, theBtnNum, theClickCount);

    if (!mDisabled)
    {
        int compared = ThumbCompare(theX, theY);
        if (compared == 1)
        {
            SetValue(mPageSize + mValue);
            mUpdateAcc = 0;
            mUpdateMode = 2;
        }
        else if (compared == -1)
        {
            SetValue(mValue - mPageSize);
            mUpdateAcc = 0;
            mUpdateMode = 1;
        }
        else
        {
            mPressedOnThumb = true;
            mMouseDownThumbPos = GetThumbPosition();
            mMouseDownX = theX;
            mMouseDownY = theY;
        }
    }

    mLastMouseX = theX;
    mLastMouseY = theY;
}

void Sexy::ScrollbarWidget::MouseUp(int theX, int theY, int theBtnNum, int theClickCount)
{
    Widget::MouseUp(theX, theY, theBtnNum, theClickCount);
    mUpdateMode = 0;
    mPressedOnThumb = false;
    MarkDirty();
}

void Sexy::ScrollbarWidget::MouseDrag(int theX, int theY)
{
    Widget::MouseDrag(theX, theY);

    if (mPressedOnThumb)
    {
        if (mHorizontal)
            SetThumbPosition(mMouseDownThumbPos + theX + mUpButton->mWidth - mMouseDownX);
        else
            SetThumbPosition(mUpButton->mHeight + mMouseDownThumbPos + theY - mMouseDownY);
    }

    mLastMouseX = theX;
    mLastMouseY = theY;
}
