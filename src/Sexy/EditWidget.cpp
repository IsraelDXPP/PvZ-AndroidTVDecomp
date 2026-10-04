#include "EditWidget.h"
#include "BaseAppDriver.h"
#include "EditListener.h"
#include "Font.h"
#include "Graphics.h"
#include "NativeDisplay.h"
#include "ReconData.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"

// EditState is only forward-declared. The incoming record is a string plus
// four ints: the text, its two selection ends, and the range it was built from.
struct IncomingEditState
{
    Sexy::String mText;
    int mSelStart;
    int mSelEnd;
    int mFrom;
    int mTo;
};

// BaseAppDriver::GetEditSource returns an int, and the Win32 driver stub
// returns 0. There is no typed pointer to recover here, so registration
// stays empty until a platform driver hands back a real EditSource.

static Sexy::DefaultEditController gDefaultEditController;

int Sexy::EditWidget::GetEditWidget()
{
    // The binary returns the receiver. The sink slot is an int, and no caller
    // in this tree reads it, so the override returns a non-zero token.
    return 1;
}

void Sexy::EditWidget::MouseDown(int theX, int theY, int theClickCount)
{
    Sexy::Widget::MouseDown(theX, theY, theClickCount);
}

void Sexy::EditWidget::MouseUp(int theX, int theY)
{
    Sexy::Widget::MouseUp(theX, theY);
}

void Sexy::EditWidget::MouseUp(int theX, int theY, int theClickCount)
{
    Sexy::Widget::MouseUp(theX, theY, theClickCount);
}

bool Sexy::EditWidget::WantsFocus()
{
    return true;
}

void Sexy::EditWidget::SetEditMode(int theMode)
{
    mEditMode = theMode;
}

int Sexy::EditWidget::GetEditMode()
{
    return mEditMode;
}

int Sexy::EditWidget::RegisterEditSink()
{
    if (mEditSource != nullptr)
        return mEditSource->RegisterEditSink(static_cast<Sexy::EditSink *>(this));
    return 0;
}

int Sexy::EditWidget::UnregisterEditSink()
{
    if (mEditSource != nullptr)
        return mEditSource->UnregisterEditSink(static_cast<Sexy::EditSink *>(this));
    return 0;
}

Sexy::Rect Sexy::EditWidget::GetEditArea()
{
    Point absPos = GetAbsPos();
    return Sexy::Rect(absPos.mX + 2, absPos.mY + 2, mWidth - 4, mHeight - 4);
}

void Sexy::EditWidget::UpdateEditText()
{
}

int Sexy::EditWidget::EditFinished()
{
    UpdateEditText();
    ProcessKey((Sexy::KeyCode)13, 0);

    Sexy::EditController *controller = GetEditController();
    if (controller != nullptr)
        return controller->EditFinished(static_cast<Sexy::EditSink *>(this));
    return 1;
}

void Sexy::EditWidget::EditCancelled()
{
    Sexy::EditController *controller = GetEditController();
    if (controller != nullptr)
        controller->EditCancelled(static_cast<Sexy::EditSink *>(this));
}

void Sexy::EditWidget::SetEditController(Sexy::EditController *theController)
{
    mEditController = theController;
}

Sexy::EditOption *Sexy::EditWidget::GetEditOption()
{
    return &mEditOption;
}

int Sexy::EditWidget::GetEditSelectionStart()
{
    int selectionPos = mSelectionPos;
    int cursorPos = mCursorPos;
    if (selectionPos >= 0 && selectionPos < cursorPos)
        return selectionPos;
    return cursorPos;
}

void Sexy::EditWidget::SetEditSelection(int theStart, int theEnd)
{
    int selectionEnd = (theStart == theEnd) ? -1 : theEnd;
    if (theStart == theEnd)
        mSelectionPos = -1;

    int length = (int)mString.size();
    if (theStart != theEnd)
        mSelectionPos = theEnd;
    mCursorPos = theStart;

    bool pastEnd = theStart > length;
    if (pastEnd)
        theStart = length;
    if (pastEnd)
        mCursorPos = theStart;
    if (selectionEnd > theStart)
        mSelectionPos = theStart;

    MarkDirty();
}

void Sexy::EditWidget::HideKeyboard()
{
    if (mEditSource != nullptr)
    {
        UpdateEditText();
        mEditSource->HideEditBox(static_cast<Sexy::EditSink *>(this));
        KeyboardWillBeHidden();
    }
    else if (gSexyAppBase != nullptr && gSexyAppBase->mAppDriver != nullptr)
    {
        gSexyAppBase->mAppDriver->HideKeyboard();
    }
    mKeyboardShown = false;
}

Sexy::String Sexy::EditWidget::GetTitle()
{
    return mTitle;
}

Sexy::String Sexy::EditWidget::GetHint()
{
    return mHint;
}

Sexy::String Sexy::EditWidget::GetEditText()
{
    Sexy::String text;
    StringToWString(&text, mString.c_str());
    return text;
}

void Sexy::EditWidget::SetTitle(Sexy::String const &theTitle)
{
    mTitle = theTitle;
}

void Sexy::EditWidget::SetHint(Sexy::String const &theHint)
{
    mHint = theHint;
}

Sexy::EditController *Sexy::EditWidget::GetEditController()
{
    if (mEditController != nullptr)
        return mEditController;
    return &gDefaultEditController;
}

bool Sexy::EditWidget::IsPartOfWord(char theChar)
{
    unsigned char byte = (unsigned char)theChar;
    if ((unsigned char)((byte & 0xDF) - 65) <= 0x19u || (unsigned char)(byte - 48) <= 9u)
        return true;
    if (theChar == 95)
        return true;
    return theChar > 0xBF;
}

int Sexy::EditWidget::KeyChar(char theChar)
{
    if (theChar == 27)
        return 0;
    ProcessKey((Sexy::KeyCode)0, theChar);
    return 1;
}

void Sexy::EditWidget::KeyboardWasShown(Sexy::TRect<int> const &theRect)
{
    if (!mKeyboardShown)
        return;

    Sexy::EditController *controller = GetEditController();
    if (controller != nullptr)
        controller->KeyboardWasShown(static_cast<Sexy::EditSink *>(this), theRect);
}

void Sexy::EditWidget::KeyboardWillBeHidden()
{
    if (!mKeyboardShown)
        return;

    mKeyboardShown = false;
    Sexy::EditController *controller = GetEditController();
    if (controller != nullptr)
        controller->KeyboardWillBeHidden(static_cast<Sexy::EditSink *>(this));
}

int Sexy::EditWidget::GetEditSelectionEnd()
{
    int selectionPos = mSelectionPos;
    if (selectionPos < 0)
        return GetEditSelectionStart();

    int cursorPos = mCursorPos;
    if (selectionPos >= cursorPos)
        return selectionPos;
    return cursorPos;
}

void Sexy::EditWidget::SetText(Sexy::String const &theText, bool scrollToStart, bool clearSelection, bool tellListener)
{
    Sexy::String oldText = mString;
    int oldSelectionStart = GetEditSelectionStart();
    int oldSelectionEnd = GetEditSelectionEnd();

    mString = theText;

    unsigned int length = mString.size();
    mSelectionPos = clearSelection ? -1 : 0;
    mCursorPos = (int)length;
    if (scrollToStart)
        mScrollPos = 0;
    else
        FocusCursor(true);
    mHiliteStart = 0;
    mHiliteEnd = 0;

    if (tellListener && oldText != mString && mEditListener != nullptr)
    {
        Sexy::String wide;
        StringToWString(&wide, mString.c_str());
        mEditListener->TextChanged(mId, wide);
    }

    MarkDirty();

    if (mEditSource != nullptr && mKeyboardShown)
    {
        if (oldText == mString
            && oldSelectionStart == GetEditSelectionStart()
            && oldSelectionEnd == GetEditSelectionEnd())
            return;

        mEditSource->UpdateEditArea(static_cast<Sexy::EditSink *>(this));
    }
}

int Sexy::EditWidget::SetEditState(Sexy::EditState const &theState)
{
    const IncomingEditState *incoming =
        static_cast<const IncomingEditState *>(static_cast<const void *>(&theState));
    Sexy::String newText = incoming->mText;

    Sexy::String oldText = mString;
    int oldHiliteStart = mHiliteStart;
    int oldHiliteEnd = mHiliteEnd;

    mString.clear();

    int selStart = incoming->mSelStart;
    int selEnd = incoming->mSelEnd;
    int from = incoming->mFrom;
    int rangeEnd = from;
    int to = incoming->mTo;

    unsigned int index = 0;
    while (index < newText.size())
    {
        bool skip = false;
        if (rangeEnd > (int)index || to < (int)index)
        {
            unsigned char byte = (unsigned char)newText[index];
            if (byte > 127)
            {
                skip = true;
            }
            else if (mEditListener != nullptr
                     && mEditListener->AllowChar(mId, (char)byte) == 0)
            {
                break;
            }
        }

        if (skip)
        {
            if (selStart >= (int)index)
            {
                --selStart;
                --selEnd;
            }
            else if (selEnd >= (int)index)
            {
                --selEnd;
            }

            if (rangeEnd < (int)index)
            {
                if (to >= (int)index)
                    --to;
            }
            else
            {
                --from;
                --to;
            }
        }
        else
        {
            unsigned int length = mString.size();
            bool full = (mMaxChars > 0) ? (length + 1 > (unsigned int)(mMaxChars + to - from))
                                        : (mMaxChars != -1);
            if (full)
                break;

            mString.push_back((char)(unsigned char)newText[index]);
        }
        ++index;
    }

    unsigned int length = mString.size();
    if (selEnd >= (int)length)
        selEnd = (int)length;
    if (to >= (int)length)
        to = (int)length;

    int cursorPos = (selEnd < selStart) ? selEnd : selStart;
    mHiliteStart = (to >= from) ? from : to;
    mCursorPos = cursorPos;
    mSelectionPos = (selEnd == cursorPos) ? -1 : selEnd;
    mHiliteEnd = to;
    FocusCursor(false);

    if (mScrollPos > 0 && mScrollPos >= (int)mString.size())
    {
        mScrollPos = (int)mString.size();
        FocusCursor(false);
    }

    MarkDirty();

    Sexy::String wide;
    StringToWString(&wide, mString.c_str());

    if (mEditSource != nullptr
        && (wide != newText || incoming->mSelStart != cursorPos || incoming->mSelEnd != selEnd))
        mEditSource->UpdateEditArea(static_cast<Sexy::EditSink *>(this));

    if (mEditListener != nullptr
        && (oldText != mString || oldHiliteStart != mHiliteStart || oldHiliteEnd != mHiliteEnd))
        mEditListener->TextChanged(mId, wide);

    return 1;
}

int Sexy::EditWidget::KeyUnicode(int theChar)
{
    if (theChar == 27)
        return 0;

    std::wstring wide(1, (wchar_t)theChar);
    Sexy::String narrow;
    WStringToString(&narrow, wide.c_str());

    for (unsigned int i = 0; i < narrow.size(); ++i)
        ProcessKey((Sexy::KeyCode)0, (unsigned char)narrow[i]);

    return 1;
}

Sexy::EditWidget::EditWidget(int theId, Sexy::EditListener *theListener)
{
    mId = theId;
    mEditListener = theListener;
    mFont = nullptr;
    mEditSource = nullptr;
    mEditController = nullptr;
    mEditOption.mMode = 0;
    mEditMode = 0;
    mCursorPos = 0;
    mSelectionPos = -1;
    mClipInset = 4;
    mTextOffsetX = 4;
    mHighlightOffsetX = 0;
    mCursorVisible = false;
    mDrawFocused = false;
    mMultiClick = false;
    mHiliteStart = 0;
    mHiliteEnd = 0;
    mBlinkTimer = 0;
    mBlinkPeriod = 40;
    mScrollPos = 0;
    mMaxChars = -1;
    mMaxPixels = -1;
    mPasswordChar = 0;
    mUndoCursorPos = 0;
    mUndoSelectionPos = 0;
    mLastCursorPos = 0;
    mKeyboardShown = false;

    static int defaultColors[8][3] =
    {
        { 0xFF, 0xFF, 0xFF }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x00 },
        { 0xFF, 0xFF, 0xFF }, { 0xE6, 0xE6, 0xE6 }, { 0xAD, 0xD8, 0xE6 }, { 0x00, 0x00, 0x00 },
    };
    SetColors(defaultColors, 8);
}

Sexy::Font *Sexy::EditWidget::ClearWidthCheckFonts()
{
    for (unsigned int i = 0; i < mWidthCheckFonts.size(); ++i)
        delete mWidthCheckFonts[i].mFont;
    mWidthCheckFonts.clear();
    return nullptr;
}

Sexy::Font *Sexy::EditWidget::AddWidthCheckFont(Sexy::Font *theFont, int theMaxPixels)
{
    WidthCheckFont entry;
    entry.mFont = (theFont != nullptr) ? theFont->Duplicate() : nullptr;
    entry.mMaxPixels = theMaxPixels;
    mWidthCheckFonts.push_back(entry);
    return entry.mFont;
}

Sexy::Font *Sexy::EditWidget::SetFont(Sexy::Font *theFont, Sexy::Font *theWidthCheckFont)
{
    if (mFont != nullptr)
        delete mFont;
    mFont = (theFont != nullptr) ? theFont->Duplicate() : nullptr;

    ClearWidthCheckFonts();
    if (theWidthCheckFont != nullptr)
        return AddWidthCheckFont(theWidthCheckFont, -1);
    return nullptr;
}

Sexy::String &Sexy::EditWidget::GetDisplayString()
{
    if (!mPasswordChar)
    {
        if (!mString.empty())
            return mString;
        return mHint;
    }

    if (mPasswordDisplayString.size() != mString.size())
        mPasswordDisplayString = Sexy::String(mString.size(), mPasswordChar);
    return mPasswordDisplayString;
}

int Sexy::EditWidget::HiliteWord()
{
    Sexy::String &displayString = GetDisplayString();
    int cursorPos = mCursorPos;
    int result = (int)displayString.size();

    if (cursorPos >= result)
        return result;

    mSelectionPos = cursorPos;
    if (cursorPos > 0)
    {
        while (true)
        {
            if (!IsPartOfWord(displayString[cursorPos - 1]))
                break;
            bool atStart = (mSelectionPos == 1);
            cursorPos = mSelectionPos - 1;
            mSelectionPos = cursorPos;
            if (cursorPos < 0 || atStart)
                break;
        }
        cursorPos = mCursorPos;
    }

    int length = (int)displayString.size();
    for (result = length - 1; length - 1 > cursorPos; mCursorPos = cursorPos)
    {
        result = IsPartOfWord(displayString[cursorPos + 1]);
        cursorPos = mCursorPos;
        length = (int)displayString.size();
        if (result == 0)
            break;
        ++cursorPos;
        result = length - 1;
    }
    if (cursorPos < length)
        mCursorPos = cursorPos + 1;
    return result;
}

void Sexy::EditWidget::Draw(Sexy::Graphics *theGraphics)
{
    if (mFont == nullptr && mWidgetManager != nullptr)
    {
        Sexy::SexyAppBase *theApp = mWidgetManager->mApp;
        if (theApp != nullptr && theApp->mPlatformDriverOrQueue != nullptr)
        {
            mFont = theApp->mPlatformDriverOrQueue->CreateFont(
                theApp, "Arial Unicode MS", 10, false, false, false);
        }
    }

    Sexy::String &displayString = GetDisplayString();

    int extents[6] = {0, 0, 0, 0, 0, 0};
    if (mFont != nullptr)
        mFont->StringExtents(displayString, extents[0]);

    theGraphics->SetColor(mColors[0]);
    theGraphics->FillRect(0, 0, mWidth, mHeight);

    int passCount = (mHiliteStart == mHiliteEnd) ? 2 : 3;

    for (int pass = 0; pass < passCount; ++pass)
    {
        Graphics *graphics = theGraphics->Create();
        graphics->SetFont(mFont);

        if (pass == 2 && mFont != nullptr)
        {
            int startX = mFont->StringWidth(displayString.substr(0, mHiliteStart))
                       - mFont->StringWidth(displayString.substr(0, mScrollPos));
            int endX;
            if (mHiliteEnd > mHiliteStart)
            {
                endX = mFont->StringWidth(displayString.substr(0, mHiliteEnd))
                     - mFont->StringWidth(displayString.substr(0, mScrollPos));
            }
            else
            {
                endX = startX;
            }

            int limit = mWidth - 8;
            if (startX < 0)
                startX = 0;
            if (startX > limit)
                startX = limit;
            if (endX < 0)
                endX = 0;
            if (endX > limit)
                endX = limit;

            int barWidth = (endX > startX) ? (endX - startX) : (startX - endX);
            int barX = (startX < endX) ? startX : endX;
            graphics->ClipRect(mHighlightOffsetX + mClipInset + barX,
                               (mHeight - mFont->GetHeight()) / 2,
                               barWidth,
                               mFont->GetHeight());

            graphics->ClipRect(mClipInset, 0, mWidth - 2 * mClipInset, mHeight);
        }
        else if (pass == 1 && mFont != nullptr)
        {
            int caretOffset = mFont->StringWidth(displayString.substr(0, mCursorPos))
                            - mFont->StringWidth(displayString.substr(0, mScrollPos));
            int caretEnd = caretOffset + 2;

            int otherEnd;
            if (mSelectionPos == -1 || mSelectionPos == mCursorPos)
            {
                otherEnd = caretEnd;
            }
            else
            {
                otherEnd = mFont->StringWidth(displayString.substr(0, mSelectionPos))
                         - mFont->StringWidth(displayString.substr(0, mScrollPos));
            }

            int firstEnd = mCursorVisible ? caretOffset : caretEnd;
            if (firstEnd < 0)
                firstEnd = 0;
            if (otherEnd < 0)
                otherEnd = 0;

            int limit = mWidth - 8;
            if (firstEnd > limit)
                firstEnd = limit;
            if (otherEnd > limit)
                otherEnd = limit;

            int caretWidth = (firstEnd > otherEnd) ? (firstEnd - otherEnd)
                                                   : (otherEnd - firstEnd);
            int caretX = (firstEnd < otherEnd) ? firstEnd : otherEnd;
            graphics->ClipRect(mHighlightOffsetX + mClipInset + caretX,
                               (mHeight - mFont->GetHeight()) / 2,
                               caretWidth,
                               mFont->GetHeight());
        }
        else
        {
            graphics->ClipRect(mClipInset, 0, mWidth - 2 * mClipInset, mHeight);
        }

        if (graphics->mClipRect.mWidth == 0 || graphics->mClipRect.mHeight == 0)
        {
            delete graphics;
            continue;
        }

        if (!(mHasFocus && mHasWindowFocus) && !mDrawFocused)
        {
            graphics->SetColor(mString.size() != 0 ? mColors[2] : mColors[5]);
        }
        else if (pass == 0)
        {
            graphics->SetColor(mString.size() != 0 ? mColors[2] : mColors[5]);
        }
        else if (pass == 1)
        {
            graphics->SetColor(mColors[3]);
            graphics->FillRect(0, 0, mWidth, mHeight);
            graphics->SetColor(mColors[4]);
        }
        else
        {
            graphics->SetColor(mColors[6]);
            graphics->FillRect(0, 0, mWidth, mHeight);
            graphics->SetColor(mColors[7]);
        }

        graphics->DrawString(displayString.substr(mScrollPos),
                             mTextOffsetX,
                             (int)(((float)mHeight - (float)extents[5]) * 0.5f
                                   - (float)extents[1]));
        delete graphics;
    }

    theGraphics->SetColor(mColors[1]);
    theGraphics->DrawRect(0, 0, mWidth - 1, mHeight - 1);
}

int Sexy::EditWidget::GetCharAt(int theX, int theY)
{
    (void)theY;
    if (mString.size() == 0 || mFont == nullptr)
        return 0;

    Sexy::String &displayString = GetDisplayString();

    int scrollPos = mScrollPos;
    if (scrollPos >= (int)displayString.size())
        return 0;

    int result = 0;
    int end = scrollPos + 1;
    int start = 0;
    for (;;)
    {
        Sexy::String leftText = displayString.substr(scrollPos, start);
        Sexy::String rightText = displayString.substr(mScrollPos, end - mScrollPos);

        int leftWidth = mFont->StringWidth(leftText);
        int midPoint = (mFont->StringWidth(rightText) + leftWidth) / 2;

        if (midPoint + 4 < theX)
            result = end;

        if ((int)displayString.size() <= end)
            return result;

        scrollPos = mScrollPos;
        start = end - scrollPos;
        ++end;
    }
}

void Sexy::EditWidget::FocusCursor(bool byPage)
{
    for (int scrollPos = mScrollPos; mCursorPos < scrollPos; scrollPos = mScrollPos)
    {
        int step = byPage ? 10 : 1;
        mScrollPos = (scrollPos - step) < 0 ? 0 : (scrollPos - step);
        MarkDirty();
    }

    if (mFont == nullptr)
        return;

    Sexy::String &displayString = GetDisplayString();

    while (mScrollPos > 0)
    {
        if (mWidth - 7 <= mFont->StringWidth(displayString.substr(mScrollPos - 1)))
            break;
        --mScrollPos;
    }

    while (mWidth > 8)
    {
        int caretWidth = mFont->StringWidth(displayString.substr(0, mCursorPos));
        int scrollWidth = mFont->StringWidth(displayString.substr(0, mScrollPos));
        if (caretWidth - scrollWidth < mWidth - 8)
            break;

        int lastPos = (int)mString.size() - 1;
        int step = byPage ? 10 : 1;
        mScrollPos = (lastPos > mScrollPos + step) ? (mScrollPos + step) : lastPos;
        MarkDirty();
    }
}

void Sexy::EditWidget::UpdateCaretPos()
{
    Point absPos = GetAbsPos();
    (void)absPos;
}

void Sexy::EditWidget::EnforceMaxPixels()
{
    if (mMaxPixels <= 0 && mWidthCheckFonts.empty())
        return;

    if (mWidthCheckFonts.empty())
    {
        while (mFont != nullptr && mFont->StringWidth(mString) > mMaxPixels && !mString.empty())
            mString = mString.substr(0, mString.size() - 1);
        return;
    }

    for (unsigned int i = 0; i < mWidthCheckFonts.size(); ++i)
    {
        int maxPixels = mWidthCheckFonts[i].mMaxPixels;
        if (maxPixels <= 0)
        {
            maxPixels = mMaxPixels;
            if (maxPixels <= 0)
                continue;
        }

        Sexy::Font *font = mWidthCheckFonts[i].mFont;
        while (font != nullptr && font->StringWidth(mString) > maxPixels && !mString.empty())
            mString = mString.substr(0, mString.size() - 1);
    }
}

void Sexy::EditWidget::ShowKeyboard()
{
    if (mEditSource != nullptr)
    {
        StringToWString(&mEditOption.mTitle, mTitle.c_str());
        StringToWString(&mEditOption.mText, mString.c_str());
        StringToWString(&mEditOption.mHint, mHint.c_str());

        mEditOption.mMode = mPasswordChar ? 1 : mEditMode;
        mEditOption.mColor = mColors[2];

        mEditSource->SetEditOption(static_cast<Sexy::EditSink *>(this), mEditOption);
        mEditSource->ShowEditBox(static_cast<Sexy::EditSink *>(this));
    }
    else if (gSexyAppBase != nullptr && gSexyAppBase->mAppDriver != nullptr)
    {
        Sexy::String defaultText;
        int mode = mEditMode;
        if (mPasswordChar)
            mode = 1;
        else
            defaultText = mString;

        gSexyAppBase->mAppDriver->ShowKeyboard(mode, mTitle, mHint, defaultText);
    }

    mKeyboardShown = true;
}

int Sexy::EditWidget::ProcessKey(Sexy::KeyCode theKey, int theChar)
{
    if ((unsigned int)(theKey - KEYCODE_SHIFT) <= 1
        || (unsigned int)(theKey - KEYCODE_GAMEPAD_LEFT) <= 7
        || (unsigned int)(theKey - KEYCODE_GAMEPAD_A) <= 1)
    {
        return 0;
    }

    bool shiftDown = false;
    bool controlDown = false;
    if (mWidgetManager != nullptr)
    {
        shiftDown = mWidgetManager->mKeyDown.Get(KEYCODE_SHIFT)->mDown;
        controlDown = mWidgetManager->mKeyDown.Get(KEYCODE_CONTROL)->mDown;
    }

    bool resetSelection = !shiftDown;
    if (shiftDown && mSelectionPos == -1)
        mSelectionPos = mCursorPos;

    Sexy::String oldText = mString;
    int cursorPos = mCursorPos;
    int selectionPos = mSelectionPos;
    int changed = 0;

    if (theChar == 24 || theChar == 3)
    {
        if (selectionPos != -1 && cursorPos != selectionPos && mWidgetManager != nullptr)
        {
            Sexy::String &displayString = GetDisplayString();
            if (cursorPos < selectionPos)
                mWidgetManager->mApp->CopyToClipboard(displayString.substr(cursorPos, selectionPos));
            else
                mWidgetManager->mApp->CopyToClipboard(displayString.substr(selectionPos, cursorPos));

            if (theChar == 3)
            {
                changed = 0;
                resetSelection = false;
            }
            else
            {
                int start = (mSelectionPos < mCursorPos) ? mSelectionPos : mCursorPos;
                int end = (mSelectionPos < mCursorPos) ? mCursorPos : mSelectionPos;
                mString = mString.substr(0, start) + mString.substr(end);

                changed = 1;
                int sel = mSelectionPos;
                int cur = mCursorPos;
                mSelectionPos = -1;
                mCursorPos = (cur > sel) ? sel : cur;
            }
        }
    }
    else if (theChar == 22)
    {
        Sexy::String clipboardText;
        if (mWidgetManager != nullptr)
            clipboardText = mWidgetManager->mApp->GetClipboard();
        changed = (int)clipboardText.size();

        if (changed != 0)
        {
            Sexy::String filteredText;
            for (unsigned int i = 0; i < clipboardText.size(); ++i)
            {
                char byte = clipboardText[i];
                if (byte == 13 || byte == 10)
                    break;
                if (mFont != nullptr && mFont->CharWidth(byte) != 0
                    && mEditListener != nullptr
                    && mEditListener->AllowChar(mId, (int)byte) != 0)
                    filteredText += byte;
            }

            if (mSelectionPos == -1)
            {
                int insertAt = mCursorPos;
                mString = mString.substr(0, mCursorPos) + filteredText + mString.substr(mCursorPos);
                mCursorPos = insertAt + (int)filteredText.size();
            }
            else
            {
                int start = (mSelectionPos >= mCursorPos) ? mCursorPos : mSelectionPos;
                int end = (mCursorPos >= mSelectionPos) ? mCursorPos : mSelectionPos;
                int insertAt = (mCursorPos < mSelectionPos) ? mCursorPos : mSelectionPos;

                mString = mString.substr(0, start) + filteredText + mString.substr(end);
                mSelectionPos = -1;
                mCursorPos = insertAt + (int)filteredText.size();
            }
            changed = 1;
        }
    }
    else if (theChar == 26)
    {
        mLastCursorPos = -1;

        Sexy::String newUndoText = mString;
        int newUndoCursorPos = mCursorPos;
        int newUndoSelectionPos = mSelectionPos;

        mString = mUndoString;
        mCursorPos = mUndoCursorPos;
        mSelectionPos = mUndoSelectionPos;

        mUndoString = newUndoText;
        mUndoCursorPos = newUndoCursorPos;
        mUndoSelectionPos = newUndoSelectionPos;

        resetSelection = false;
        changed = 0;
    }
    else
    {
        bool insertChar = false;
        switch (theKey)
        {
        case KEYCODE_LEFT:
            if (!controlDown)
            {
                if (shiftDown || selectionPos == -1)
                    mCursorPos = cursorPos - 1;
                else
                    mCursorPos = (selectionPos > cursorPos) ? cursorPos : selectionPos;
            }
            else
            {
                while (mCursorPos > 0 && IsPartOfWord(mString[mCursorPos - 1]))
                    --mCursorPos;
            }
            changed = 0;
            break;

        case KEYCODE_RIGHT:
            if (controlDown && (int)mString.size() - 1 > cursorPos)
            {
                while (mCursorPos < (int)mString.size() - 1 && IsPartOfWord(mString[mCursorPos + 1]))
                    ++mCursorPos;
                while (mCursorPos < (int)mString.size() - 1 && !IsPartOfWord(mString[mCursorPos + 1]))
                    ++mCursorPos;
            }

            if (shiftDown || mSelectionPos == -1)
                ++mCursorPos;
            else
                mCursorPos = (mCursorPos < mSelectionPos) ? mSelectionPos : mCursorPos;
            changed = 0;
            break;

        case KEYCODE_BACK:
            if (mString.size() == 0)
            {
                changed = 0;
                break;
            }

            if (mSelectionPos == -1 || cursorPos == mSelectionPos)
            {
                if (cursorPos <= 0)
                    mString = mString.substr(0);
                else
                    mString = mString.substr(0, cursorPos - 1) + mString.substr(mCursorPos);

                int lastCursorPos = mLastCursorPos;
                int oldCursorPos = mCursorPos;
                mSelectionPos = -1;
                mCursorPos = oldCursorPos - 1;
                changed = (oldCursorPos - 1 != lastCursorPos);
                mLastCursorPos = oldCursorPos - 2;
            }
            else
            {
                int start = (selectionPos >= cursorPos) ? mCursorPos : mSelectionPos;
                int end = (mCursorPos >= mSelectionPos) ? mCursorPos : mSelectionPos;
                mString = mString.substr(0, start) + mString.substr(end);

                changed = 1;
                int sel = mSelectionPos;
                int cur = mCursorPos;
                mSelectionPos = -1;
                mCursorPos = (cur > sel) ? sel : cur;
            }
            break;

        case KEYCODE_DELETE:
            if (mString.size() == 0)
            {
                changed = 0;
                break;
            }

            if (mSelectionPos == -1 || cursorPos == mSelectionPos)
            {
                if (cursorPos < (int)mString.size())
                    mString = mString.substr(0, cursorPos) + mString.substr(mCursorPos + 1);

                int oldCursorPos = mCursorPos;
                int lastCursorPos = mLastCursorPos;
                mLastCursorPos = oldCursorPos;
                changed = (lastCursorPos != oldCursorPos);
            }
            else
            {
                int start = (selectionPos >= cursorPos) ? mCursorPos : mSelectionPos;
                int end = (mCursorPos >= mSelectionPos) ? mCursorPos : mSelectionPos;
                mString = mString.substr(0, start) + mString.substr(end);

                changed = 1;
                int sel = mSelectionPos;
                int cur = mCursorPos;
                mSelectionPos = -1;
                mCursorPos = (cur > sel) ? sel : cur;
            }
            break;

        case KEYCODE_HOME:
            mCursorPos = 0;
            changed = 0;
            break;

        case KEYCODE_END:
            changed = 0;
            mCursorPos = (int)mString.size();
            break;

        case KEYCODE_RETURN:
            if (mEditListener != nullptr)
                mEditListener->EditWidgetText(mId, mString);
            changed = 0;
            break;

        case KEYCODE_CLEAR:
        default:
            insertChar = true;
            break;
        }

        if (insertChar)
        {
            if (theKey != KEYCODE_CLEAR)
            {
                char charBuffer[2] = {(char)theChar, 0};
                Sexy::String charString(charBuffer, 1);

                int maxChar = 127;
                if (mWidgetManager != nullptr && mWidgetManager->mApp->mAllowExtendedChars)
                    maxChar = 255;
                if (theChar > 0x1F && theChar <= maxChar
                    && mFont != nullptr && mFont->StringWidth(charString) > 0
                    && mEditListener != nullptr
                    && mEditListener->AllowChar(mId, theChar) != 0)
                {
                    if (mSelectionPos == -1 || mSelectionPos == mCursorPos)
                    {
                        int oldCursorPos = mCursorPos;
                        mString = mString.substr(0, oldCursorPos) + charString
                                + mString.substr(oldCursorPos);

                        int lastCursorPos = mLastCursorPos;
                        mSelectionPos = -1;
                        mLastCursorPos = oldCursorPos;
                        changed = (oldCursorPos != lastCursorPos + 1);
                    }
                    else
                    {
                        int end = (mCursorPos >= mSelectionPos) ? mCursorPos : mSelectionPos;
                        int newCursorPos = (mCursorPos < mSelectionPos) ? mCursorPos : mSelectionPos;

                        mString = mString.substr(0, end) + charString + mString.substr(end);
                        mSelectionPos = -1;
                        mCursorPos = newCursorPos + 1;
                        changed = 1;
                    }

                    FocusCursor(false);
                }
                else
                {
                    changed = 0;
                    resetSelection = false;
                }
            }
            else
            {
                SetText(Sexy::String(gEmptyCString), true, true, true);
                changed = 0;
            }
        }
    }

    if (mMaxChars != -1 && mMaxChars < (int)mString.size())
        mString = mString.substr(0, mMaxChars);

    EnforceMaxPixels();

    if (mCursorPos >= 0)
    {
        if (mCursorPos > (int)mString.size())
            mCursorPos = (int)mString.size();
    }
    else
    {
        mCursorPos = 0;
    }

    if (cursorPos != mCursorPos)
    {
        mCursorVisible = true;
        mBlinkTimer = 0;
    }
    FocusCursor(true);

    if (resetSelection || mSelectionPos == mCursorPos)
        mSelectionPos = -1;

    if (mEditListener == nullptr || mEditListener->AllowText(mId, mString) != 0)
    {
        if (changed != 0)
        {
            mUndoString = oldText;
            mUndoCursorPos = cursorPos;
            mUndoSelectionPos = selectionPos;
        }
    }
    else
    {
        mString = oldText;
        mCursorPos = cursorPos;
        mSelectionPos = selectionPos;
    }

    MarkDirty();
    return 1;
}

void Sexy::EditWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
    Sexy::Widget::Resize(theX, theY, theWidth, theHeight);
    FocusCursor(false);
    MarkDirty();
}

void Sexy::EditWidget::GotFocus()
{
    Sexy::Widget::GotFocus();
    mCursorVisible = true;
    mBlinkTimer = 0;
    MarkDirty();
    ShowKeyboard();
}

void Sexy::EditWidget::LostFocus()
{
    Sexy::Widget::LostFocus();
    mCursorVisible = false;
    MarkDirty();
    HideKeyboard();
}

void Sexy::EditWidget::GotWindowFocus()
{
    Sexy::Widget::GotWindowFocus();
    if (mHasFocus && !mKeyboardShown)
        ShowKeyboard();
}

void Sexy::EditWidget::LostWindowFocus()
{
    Sexy::Widget::LostWindowFocus();
    if (mHasFocus)
        HideKeyboard();
}

void Sexy::EditWidget::Update()
{
    Sexy::Widget::Update();
    if (!mHasFocus)
        return;

    if (mWidgetManager != nullptr && mWidgetManager->mApp->mTabletPC)
        UpdateCaretPos();

    if (++mBlinkTimer > mBlinkPeriod)
    {
        MarkDirty();
        mBlinkTimer = 0;
        mCursorVisible = !mCursorVisible;
    }
}

int Sexy::EditWidget::KeyDown(Sexy::KeyCode theKey)
{
    bool direct = theKey > 0x2E;
    if (theKey != 46)
        direct = theKey - 35 > 1;
    if (!direct || (theKey & 0xFFFFFFFB) == 8)
        return ProcessKey(theKey, 0);

    if (!mSelected)
        return 0;
    if (theKey == 27)
        return Sexy::Widget::KeyDown(theKey);

    bool notAlnum = theKey - 65 > 0x19;
    if (theKey - 65 > 0x19)
        notAlnum = theKey - 48 > 9;
    if (notAlnum && mEditListener != nullptr && mEditListener->AllowKey(mId, theKey) != 0)
        return ProcessKey(theKey, 0);
    return Sexy::Widget::KeyDown(theKey);
}

void Sexy::EditWidget::MouseDown(int theX, int theY, int theBtnNum, int theClickCount)
{
    Sexy::Widget::MouseDown(theX, theY, theBtnNum, theClickCount);
    mSelectionPos = -1;
    mCursorPos = GetCharAt(theX, theY);

    if (theClickCount > 1)
    {
        mMultiClick = true;
        HiliteWord();
    }

    if (mEditSource != nullptr)
        mEditSource->UpdateEditArea(static_cast<Sexy::EditSink *>(this));
    MarkDirty();
    FocusCursor(false);
    if (mEditSource != nullptr)
        mEditSource->UpdateEditArea(static_cast<Sexy::EditSink *>(this));
}

void Sexy::EditWidget::MouseUp(int theX, int theY, int theBtnNum, int theClickCount)
{
    Sexy::Widget::MouseUp(theX, theY, theBtnNum, theClickCount);
    if (mSelectionPos == mCursorPos)
        mSelectionPos = -1;

    if (mMultiClick)
    {
        mSelectionPos = -1;
        mCursorPos = GetCharAt(theX, theY);
        mMultiClick = false;
        HiliteWord();
    }

    if (!mKeyboardShown)
    {
        ShowKeyboard();
        MarkDirty();
        return;
    }

    if (mEditSource != nullptr)
        mEditSource->UpdateEditArea(static_cast<Sexy::EditSink *>(this));
    MarkDirty();
}

void Sexy::EditWidget::MouseDrag(int theX, int theY)
{
    Sexy::Widget::MouseDrag(theX, theY);
    if (mSelectionPos == -1)
        mSelectionPos = mCursorPos;

    mCursorPos = GetCharAt(theX, theY);
    if (mEditSource != nullptr)
        mEditSource->UpdateEditArea(static_cast<Sexy::EditSink *>(this));
    MarkDirty();
    FocusCursor(false);
}

void Sexy::EditWidget::MouseEnter()
{
    Sexy::Widget::MouseEnter();
    if (mWidgetManager != nullptr)
        mWidgetManager->mApp->SetCursor(3);
}

void Sexy::EditWidget::MouseLeave()
{
    Sexy::Widget::MouseLeave();
    if (mWidgetManager != nullptr)
        mWidgetManager->mApp->SetCursor(0);
}

void Sexy::EditWidget::MarkDirty()
{
    if (mColors[0].mAlpha == 255)
        Sexy::WidgetContainer::MarkDirty();
    else
        Sexy::WidgetContainer::MarkDirtyFull();
}

void Sexy::EditWidget::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    Sexy::WidgetContainer::AddedToManager(theWidgetManager);
    RegisterEditSink();
}

void Sexy::EditWidget::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    Sexy::WidgetContainer::RemovedFromManager(theWidgetManager);
    UnregisterEditSink();
}

Sexy::EditWidget::~EditWidget()
{
    delete mFont;
    ClearWidthCheckFonts();
}
