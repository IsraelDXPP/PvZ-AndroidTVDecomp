#include "ListWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "ListListener.h"
#include "ReconData.h"
#include "ScrollbarWidget.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include <cstring>
#include <vector>

int Sexy::ListWidget::GetLineCount()
{
    return (int)mLines.size();
}

void Sexy::ListWidget::SetColor(Sexy::String const &theLine, Color const &theColor)
{
    SetLineColor(GetLineIdx(theLine), theColor);
}

int Sexy::ListWidget::GetOptimalWidth()
{
    if (mLines.empty())
        return 16;

    int maxWidth = 0;
    for (unsigned int i = 0; i < mLines.size(); ++i)
    {
        int width = mFont->StringWidth(mLines[i]);
        if (maxWidth < width)
            maxWidth = width;
    }
    return maxWidth + 16;
}

int Sexy::ListWidget::GetOptimalHeight()
{
    int itemHeight = mItemHeight;
    if (itemHeight == -1)
        itemHeight = mFont->GetHeight();
    return (int)mLines.size() * itemHeight + 8;
}

void Sexy::ListWidget::ScrollPosition(int theId, double thePosition)
{
    if (mChild != nullptr)
        mChild->ScrollPosition(theId, thePosition);

    mPosition = thePosition;
    MarkDirty();
}

void Sexy::ListWidget::MouseDown(int theX, int theY, int theBtnNum, int theClickCount)
{
    (void)theX;
    (void)theY;
    (void)theBtnNum;
    if (mHiliteIdx != -1 && mListListener != nullptr)
        mListListener->ListClicked(mId, mHiliteIdx, theClickCount);
}

void Sexy::ListWidget::SetSelect(int theIdx)
{
    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    do
    {
        widget->mSelectIdx = theIdx;
        widget->MarkDirty();
        widget = widget->mChild;
    }
    while (widget != nullptr);
}

void Sexy::ListWidget::MouseWheel(int theDelta)
{
    if (mScrollbar == nullptr)
        return;

    if (theDelta > 0)
        mScrollbar->SetValue(mScrollbar->mValue - (double)mWheelLines);
    else if (theDelta < 0)
        mScrollbar->SetValue(mScrollbar->mValue + (double)mWheelLines);
}

int Sexy::ListWidget::MoveSelect(int theDelta)
{
    int oldSelect = mSelectIdx;
    int newSelect = oldSelect + theDelta;
    if (newSelect >= 0)
    {
        int count = (int)mLines.size();
        if (newSelect >= count)
            newSelect = count - 1;
    }
    else
    {
        newSelect = 0;
    }

    if (newSelect == oldSelect)
        return 0;

    SetSelect(mSelectIdx);
    mSelectIdx = newSelect;
    MarkDirty();
    return 1;
}

Sexy::String Sexy::ListWidget::GetStringAt(int theIdx)
{
    return mLines[theIdx];
}

void Sexy::ListWidget::SetLine(int theIdx, Sexy::String const &theLine)
{
    mLines[theIdx] = theLine;
    MarkDirty();
}

int Sexy::ListWidget::GetLineIdx(Sexy::String const &theLine)
{
    int count = (int)mLines.size();
    if (count == 0)
        return -1;

    for (int i = 0; i < count; ++i)
    {
        if (strcmp(mLines[i].c_str(), theLine.c_str()) == 0)
            return i;
    }
    return -1;
}

void Sexy::ListWidget::SetLineColor(int theIdx, Color const &theColor)
{
    if (theIdx < 0 || theIdx >= (int)mLineColors.size())
        return;

    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    do
    {
        widget->mLineColors[theIdx] = theColor;
        widget->MarkDirty();
        widget = widget->mChild;
    }
    while (widget != nullptr);
}

void Sexy::ListWidget::RemoveAll()
{
    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    do
    {
        widget->mLines.clear();
        widget->mLineColors.clear();
        widget->mSelectIdx = -1;
        widget->mHiliteIdx = -1;
        widget->MarkDirty();
        widget = widget->mChild;
    }
    while (widget != nullptr);

    if (mScrollbar != nullptr)
        mScrollbar->SetMaxValue((double)mLines.size());
}

void Sexy::ListWidget::Draw(Sexy::Graphics *theGraphics)
{
    theGraphics->SetColor(mColors[0]);
    theGraphics->FillRect(0, 0, mWidth, mHeight);
    theGraphics->PushState();
    theGraphics->ClipRect(4, 4, mWidth - 8, mHeight - 8);
    theGraphics->SetFont(mFont);

    int startLine = (int)mPosition;
    int pageSize = (int)mPageSize;
    int lastLine = (int)mLines.size() - 1;
    int itemHeight = mItemHeight;
    if (lastLine >= pageSize + startLine + 1)
        lastLine = pageSize + startLine + 1;
    if (itemHeight == -1)
        itemHeight = mFont->GetHeight();

    for (int i = startLine; i <= lastLine; ++i)
    {
        int rowY = (int)(((double)i - mPosition) * (double)itemHeight) + 4;

        bool selected = (mSelectIdx == i);
        bool hilited = (mHiliteIdx == i);

        if (selected || (hilited && mDrawSelectWhenHilited))
        {
            theGraphics->PushState();
            theGraphics->ClipRect(0, 4, mWidth, mHeight - 8);
            if (mItemImage != nullptr)
            {
                theGraphics->SetColor(Sexy::Color::White);
                TRect<int> rowRect(4, rowY, mWidth - 8, itemHeight);
                theGraphics->DrawImageBox(rowRect, mItemImage);
            }
            else
            {
                theGraphics->SetColor(mColors[4]);
                theGraphics->FillRect(0, rowY, mWidth, itemHeight);
            }
            theGraphics->PopState();
        }

        if (hilited)
            theGraphics->SetColor(mColors[3]);
        else if (selected && mColors.size() > 5)
            theGraphics->SetColor(mColors[5]);
        else
            theGraphics->SetColor(mLineColors[i]);

        Sexy::String text = mLines[i];
        Font::TextExtents textExtents;
        mFont->StringExtents(text, textExtents);

        int drawX;
        if (mJustify == 0)
            drawX = mTextOffset;
        else if (mJustify == 1)
            drawX = (mWidth - mFont->StringWidth(text)) / 2;
        else
            drawX = mWidth - mFont->StringWidth(text) - mTextOffset;

        int drawY = (int)(((float)itemHeight - textExtents.mHeight) * 0.5f
                          - textExtents.mMinY
                          + (float)rowY);
        theGraphics->DrawString(text, drawX, drawY);
    }

    theGraphics->PopState();
    if (mDrawOutline)
    {
        theGraphics->SetColor(mColors[1]);
        theGraphics->DrawRect(0, 0, mWidth - 1, mHeight - 1);
        if (mDrawOutline && mSelected)
        {
            theGraphics->SetColor(mColors[3]);
            theGraphics->DrawRect(0, 0, mWidth - 1, mHeight - 1);
        }
    }
}

void Sexy::ListWidget::RemoveLine(int theIdx)
{
    if (theIdx == -1)
        return;

    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    do
    {
        if (theIdx >= 0 && theIdx < (int)widget->mLines.size())
            widget->mLines.erase(widget->mLines.begin() + theIdx);

        if (theIdx >= 0 && theIdx < (int)widget->mLineColors.size())
            widget->mLineColors.erase(widget->mLineColors.begin() + theIdx);

        widget->MarkDirty();
        widget = widget->mChild;
    }
    while (widget != nullptr);

    if (mScrollbar != nullptr)
        mScrollbar->SetMaxValue((double)mLines.size());
}

int Sexy::ListWidget::SetHilite(int theIdx, bool notify)
{
    int oldHilite = mHiliteIdx;
    mHiliteIdx = theIdx;

    if (oldHilite != theIdx && notify && mListListener != nullptr)
        mListListener->ListHiliteChanged(mId, oldHilite, theIdx);

    return 0;
}

void Sexy::ListWidget::MouseLeave()
{
    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    do
    {
        widget->SetHilite(-1, true);
        widget->MarkDirty();
        widget = widget->mChild;
    }
    while (widget != nullptr);

    mWidgetManager->mApp->SetCursor(0);
}

void Sexy::ListWidget::MouseMove(int theX, int theY)
{
    (void)theX;
    int itemHeight = mItemHeight;
    if (itemHeight == -1)
        itemHeight = mFont->GetHeight();

    int hilite = (int)((double)(theY - 4) / (double)itemHeight + mPosition);
    if (hilite < 0 || hilite >= (int)mLines.size())
        hilite = -1;

    if (mHiliteIdx == hilite)
        return;

    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    do
    {
        widget->SetHilite(hilite, true);
        widget->MarkDirty();
        widget = widget->mChild;
    }
    while (widget != nullptr);

    if (mHiliteIdx != -1 && mDoFingerWhenHilited)
        mWidgetManager->mApp->SetCursor(1);
    else
        mWidgetManager->mApp->SetCursor(0);
}

Sexy::ListWidget::ListWidget(int theId, Sexy::Font *theFont, Sexy::ListListener *theListener)
{
    mId = theId;
    mFont = theFont;
    mScrollbar = nullptr;
    mJustify = 0;
    mWheelLines = 5;
    mPosition = 0.0;
    mPageSize = 0.0;
    mHiliteIdx = -1;
    mSelectIdx = -1;
    mListListener = theListener;
    mParent = nullptr;
    mChild = nullptr;
    mSortFromChild = false;
    mDrawOutline = true;
    mMaxNumericPlaces = 0;
    mItemHeight = (theFont != nullptr) ? theFont->GetHeight() : -1;
    mDrawSelectWhenHilited = false;
    mDoFingerWhenHilited = true;
    mTextOffset = 4;
    mItemImage = nullptr;

    SetColors(gListWidgetColors, 6);
}

Sexy::String Sexy::ListWidget::GetSortKey(int theIdx)
{
    Sexy::String key = mLines[theIdx];
    while (mMaxNumericPlaces > (int)key.length())
        key = "0" + key;

    if (mSortFromChild)
        return mChild->GetSortKey(theIdx) + key;

    if (mChild == nullptr)
        return key;

    return key + mChild->GetSortKey(theIdx);
}

int Sexy::ListWidget::AddLine(Sexy::String const &theLine, bool sorted)
{
    int lineIdx = (int)mLines.size();
    bool insertSorted = false;

    if (sorted && !mLines.empty())
    {
        lineIdx = 0;
        while (strcmp(theLine.c_str(), mLines[lineIdx].c_str()) >= 0)
        {
            ++lineIdx;
            if (lineIdx >= (int)mLines.size())
            {
                lineIdx = (int)mLines.size();
                break;
            }
        }
        insertSorted = (lineIdx < (int)mLines.size());
    }

    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    Sexy::String dash("-");
    for (; widget != nullptr; widget = widget->mChild)
    {
        const Sexy::String &text = (widget == this) ? theLine : dash;
        if (insertSorted)
            widget->mLines.insert(widget->mLines.begin() + lineIdx, text);
        else
            widget->mLines.push_back(text);

        widget->mLineColors.push_back(mColors[2]);
        widget->MarkDirty();
    }

    if (mScrollbar != nullptr)
        mScrollbar->SetMaxValue((double)mLines.size());

    return lineIdx;
}

void Sexy::ListWidget::Sort(bool descending)
{
    int count = (int)mLines.size();

    std::vector<int> order;
    std::vector<Sexy::String> keys;
    order.resize((size_t)count);
    for (int i = 0; i < count; ++i)
    {
        order[i] = i;
        keys.push_back(GetSortKey(i));
    }

    if (count > 1)
    {
        for (int pass = count - 1; pass != 0; --pass)
        {
            for (int i = 0; i != pass; ++i)
            {
                int cmp = strcmp(keys[i].c_str(), keys[i + 1].c_str());
                if ((descending && cmp > 0) || (!descending && cmp < 0))
                {
                    int swapIdx = order[i];
                    order[i] = order[i + 1];
                    order[i + 1] = swapIdx;

                    Sexy::String swapKey = keys[i];
                    keys[i] = keys[i + 1];
                    keys[i + 1] = swapKey;
                }
            }
        }
    }

    ListWidget *widget = this;
    while (widget->mParent != nullptr)
        widget = widget->mParent;

    do
    {
        std::vector<Sexy::String> sortedLines;
        std::vector<Sexy::Color> sortedColors;
        for (int i = 0; i < count; ++i)
        {
            int index = order[i];
            sortedLines.push_back(widget->mLines[index]);
            sortedColors.push_back(widget->mLineColors[index]);
        }

        widget->mLines = sortedLines;
        widget->mLineColors = sortedColors;
        widget->MarkDirty();
        widget = widget->mChild;
    }
    while (widget != nullptr);
}

void Sexy::ListWidget::MouseDown(int theX, int theY, int theClickCount)
{
    Widget::MouseDown(theX, theY, theClickCount);
}

void Sexy::ListWidget::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    WidgetContainer::RemovedFromManager(theWidgetManager);

    if (mListListener != nullptr)
        mListListener->ListClosed(mId);
}

void Sexy::ListWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
    Widget::Resize(theX, theY, theWidth, theHeight);

    int itemHeight = mItemHeight;
    if (itemHeight == -1)
        itemHeight = mFont->GetHeight();

    double pageSize = 1.0;
    if (mHeight > itemHeight + 8)
        pageSize = ((double)mHeight - 8.0) / (double)itemHeight;
    mPageSize = pageSize;

    if (mScrollbar != nullptr)
        mScrollbar->SetPageSize(pageSize);
}

void Sexy::ListWidget::SetColor(int theIdx, Color const &theColor)
{
    Widget::SetColor(theIdx, theColor);
}

void Sexy::ListWidget::OrderInManagerChanged()
{
    Widget::OrderInManagerChanged();

    if (mChild != nullptr)
        gSexyAppBase->mWidgetManager->PutInfront(mChild, this);
    if (mScrollbar != nullptr)
        gSexyAppBase->mWidgetManager->PutInfront(mScrollbar, this);
}

int Sexy::ListWidget::KeyDown(Sexy::KeyCode theKey)
{
    if (!mSelected)
        return Widget::KeyDown(theKey);

    switch (theKey)
    {
    case KEYCODE_UP:
        return MoveSelect(-1);
    case KEYCODE_DOWN:
        return MoveSelect(1);
    case KEYCODE_RETURN:
        if (mSelectIdx != -1 && mListListener != nullptr)
            mListListener->ListClicked(mId, mSelectIdx, 1);
        return (int)mSelected;
    default:
        return Widget::KeyDown(theKey);
    }
}

Sexy::ListWidget::~ListWidget()
{
    if (mItemImage != nullptr)
    {
        RemovedSharedImage(mItemImage);
        mItemImage = nullptr;
    }
}
