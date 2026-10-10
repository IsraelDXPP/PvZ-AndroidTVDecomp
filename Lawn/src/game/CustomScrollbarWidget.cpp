// CustomScrollbarWidget -- transcribed from libGameMain.so
// Rewritten from the IDA decompilation into the C++ the original programmer
// wrote: named members, real virtual calls, no this-relative byte offsets.

#include "CustomScrollbarWidget.h"

#include "ScrollListener.h"
#include "ScrollbarWidget.h"
// ==========================================================================
// CustomScrollbarWidget::ResizeScrollbar(int,int,int,int)
// @00167D04  112 bytes
// ==========================================================================
int CustomScrollbarWidget::ResizeScrollbar(int theX, int theY, int theWidth, int theHeight)
{
    Resize(theX, theY, theWidth, theHeight);

    if (mHorizontal)
    {
        if (mUpButton != nullptr)
            mUpButton->Resize(0, 0, theHeight, theHeight);
        if (mDownButton != nullptr)
            mDownButton->Resize(theWidth - theHeight, 0, theHeight, theHeight);
    }
    else
    {
        if (mUpButton != nullptr)
            mUpButton->Resize(0, 0, theWidth, theWidth);
        if (mDownButton != nullptr)
            mDownButton->Resize(0, theHeight - theWidth, theWidth, theWidth);
    }
    return 0;
}

// ==========================================================================
// CustomScrollbarWidget::ClampValue()
// @00167D74  228 bytes
// ==========================================================================
int CustomScrollbarWidget::ClampValue()
{
    double anOldValue = mValue;
    double aMaxValue = mMaxValue - mPageSize;

    if (mValue > aMaxValue)
        mValue = aMaxValue;
    if (mValue < 0.0)
        mValue = 0.0;

    // Nothing to scroll: grey the bar out. The binary computes `mMaxValue >
    // mPageSize` once and uses its complement for the three disabled flags.
    SetDisabled(mMaxValue <= mPageSize);
    if (mUpButton != nullptr)
        mUpButton->SetDisabled(mMaxValue <= mPageSize);
    if (mDownButton != nullptr)
        mDownButton->SetDisabled(mMaxValue <= mPageSize);

    if (mInvisIfNoScroll)
    {
        SetVisible(mMaxValue > mPageSize);
        // JUDGEMENT CALL: at 0x167e26..0x167e4e the binary tests mUpButton and then
        // calls SetVisible on mDownButton, and tests mDownButton and then calls it on
        // mUpButton -- the two receivers are crossed. Every other pair here is
        // symmetric, and a button is only touched when its own pointer is non-null,
        // so the symmetric form below is what the code has to mean; the crossed
        // registers are recorded here rather than reproduced.
        if (mUpButton != nullptr)
            mUpButton->SetVisible(mMaxValue > mPageSize);
        if (mDownButton != nullptr)
            mDownButton->SetVisible(mMaxValue > mPageSize);
    }

    if (mValue != anOldValue)
        mScrollListener->ScrollPosition(mId, mValue);

    return 0;
}

// ==========================================================================
// CustomScrollbarWidget::~CustomScrollbarWidget()
// @00167E60  34 bytes
// ==========================================================================
CustomScrollbarWidget::~CustomScrollbarWidget()
{
    // The binary stores this class's vptr and the secondary ButtonListener vptr
    // and then calls ~ScrollbarWidget explicitly; with real bases the compiler
    // emits all three.
}

// ==========================================================================
// CustomScrollbarWidget::GetThumbPosition()
// @00167EB0  106 bytes
// ==========================================================================
int CustomScrollbarWidget::GetThumbPosition()
{
    if (mPageSize > mMaxValue)
        return 0;

    // Slots [138] and [139]: GetTrackSize and GetThumbSize.
    return (int)((double)(GetTrackSize() - GetThumbSize()) * mValue / (mMaxValue - mPageSize) + 0.5);
}

// ==========================================================================
// CustomScrollbarWidget::GetTrackSize()
// @00167F1C  46 bytes
// ==========================================================================
int CustomScrollbarWidget::GetTrackSize()
{
    if (mHorizontal)
    {
        if (mUpButton != nullptr)
            return mWidth - 2 * mUpButton->mWidth;
        return mWidth;
    }
    else
    {
        // The binary reads the button's mWidth in this arm too (+0x3C, not +0x40);
        // the buttons are square -- ResizeScrollbar gives them a4 x a4 -- so the
        // two spellings are the same number.
        if (mUpButton != nullptr)
            return mHeight - 2 * mUpButton->mWidth;
        return mHeight;
    }
}

// ==========================================================================
// CustomScrollbarWidget::GetThumbSize()
// @00167F4C  82 bytes
// ==========================================================================
int CustomScrollbarWidget::GetThumbSize()
{
    if (mPageSize > mMaxValue)
        return 0;

    int aSize = (int)((double)GetTrackSize() * mPageSize / mMaxValue + 0.5);
    if (aSize < 8)
        return 8;
    return aSize;
}

// ==========================================================================
// CustomScrollbarWidget::CustomScrollbarWidget(int,Sexy::ScrollListener *)
// @00167FA0  36 bytes
// ==========================================================================
CustomScrollbarWidget::CustomScrollbarWidget(int theId, Sexy::ScrollListener *theListener) :
    ScrollbarWidget(theId, theListener)
{
    // The body of the binary only stores this class's vptr and the secondary
    // ButtonListener vptr over the base's; the base constructor is the real work
    // and the compiler runs it through the initialiser above.
}

// ==========================================================================
// CustomScrollbarWidget::RemoveScrollButtons()
// @00167FCC  68 bytes
// ==========================================================================
void CustomScrollbarWidget::RemoveScrollButtons()
{
    if (mUpButton != nullptr)
    {
        RemoveWidget(mUpButton);
        delete mUpButton;
        mUpButton = nullptr;
    }
    if (mDownButton != nullptr)
    {
        RemoveWidget(mDownButton);
        delete mDownButton;
        mDownButton = nullptr;
    }
}
