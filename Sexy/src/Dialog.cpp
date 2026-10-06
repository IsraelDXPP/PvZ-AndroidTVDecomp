// Dialog is a modal panel with a header, wrapped lines, a footer and up to two buttons.

#include "Dialog.h"
#include "ReconData.h"
#include "ButtonListener.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NativeDisplay.h"
#include "SexyAppBase.h"
#include "TextLayout.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"

static Sexy::SexyAppBase *DialogApp()
{
    return gSexyAppBase;
}

void Sexy::Dialog::MouseUp(int theX, int theY)
{
    Widget::MouseUp(theX, theY);
}

void Sexy::Dialog::MouseDown(int theX, int theY, int theClickCount)
{
    Widget::MouseDown(theX, theY, theClickCount);
}

void Sexy::Dialog::MouseUp(int theX, int theY, int theClickCount)
{
    Widget::MouseUp(theX, theY, theClickCount);
}

void Sexy::Dialog::SetButtonFont(Sexy::Font * theFont)
{
    if (mYesButton != nullptr)
        mYesButton->SetFont(theFont);
    if (mNoButton != nullptr)
        mNoButton->SetFont(theFont);
}

void Sexy::Dialog::SetHeaderFont(Sexy::Font * theFont)
{
    delete mHeaderFont;
    mHeaderFont = theFont->Duplicate();
}

bool Sexy::Dialog::IsModal()
{
    return mIsModal;
}

int Sexy::Dialog::WaitForResult(bool autoKill)
{
    Sexy::SexyAppBase *theApp = DialogApp();

    ++theApp->mUpdateDepth;
    while (theApp->UpdateAppStep(nullptr) && mWidgetManager != nullptr && mResult == 0x7FFFFFFF)
        ;

    int aResult = mResult;
    if (autoKill)
        theApp->KillDialog(mId);

    --theApp->mUpdateDepth;
    return aResult;
}

void Sexy::Dialog::ButtonDownTick(int)
{
}

void Sexy::Dialog::SetLinesFont(Sexy::Font * theFont)
{
    delete mLinesFont;
    mLinesFont = theFont->Duplicate();
    mTextLayout.SetFont(mLinesFont);
}

void Sexy::Dialog::OrderInManagerChanged()
{
    Widget::OrderInManagerChanged();

    if (mYesButton != nullptr)
        mWidgetManager->PutInfront(mYesButton, this);
    if (mNoButton != nullptr)
        mWidgetManager->PutInfront(mNoButton, this);
}

void Sexy::Dialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    Widget::Resize(theX, theY, theWidth, theHeight);

    int offsetX = 0;
    int offsetY = 0;
    if (mAddToManager)
    {
        offsetX = mX;
        offsetY = mY;
    }

    if (mYesButton == nullptr)
        return;

    if (mNoButton != nullptr)
    {
        int buttonWidth = (mWidth - mContentInsets.mLeft - mContentInsets.mRight
                           - mBackgroundInsets.mLeft - mBackgroundInsets.mRight
                           - 2 * mButtonSidePadding - mButtonHorzSpacing) / 2;
        int buttonY = offsetY + mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - mButtonHeight;
        mYesButton->Resize(mButtonSidePadding + mContentInsets.mLeft + offsetX + mBackgroundInsets.mLeft,
                           buttonY, buttonWidth, mButtonHeight);
        mNoButton->Resize(mButtonHorzSpacing + buttonWidth + mYesButton->mX,
                          mYesButton->mY, buttonWidth, mButtonHeight);
    }
    else
    {
        mYesButton->Resize(mBackgroundInsets.mLeft + offsetX + mContentInsets.mLeft,
                           offsetY + mHeight - mContentInsets.mBottom - mBackgroundInsets.mBottom - mButtonHeight,
                           mWidth - mContentInsets.mLeft - mContentInsets.mRight
                             - mBackgroundInsets.mLeft - mBackgroundInsets.mRight,
                           mButtonHeight);
    }
}

void Sexy::Dialog::MouseDrag(int theX, int theY)
{
    if (!mDragging)
        return;

    int newX = mX + theX - mDragMouseX;
    int newY = mY + theY - mDragMouseY;
    if (newX < -8)
    {
        newX = -8;
    }
    else if (newX + mWidth > mWidgetManager->mWidth + 8)
    {
        newX = mWidgetManager->mWidth - mWidth + 8;
    }

    if (newY < -8)
    {
        newY = -8;
    }
    else if (newY + mHeight > mWidgetManager->mHeight + 8)
    {
        newY = mWidgetManager->mHeight - mHeight + 8;
    }

    mDragMouseX = mX + theX - newX;
    mDragMouseY = mY + theY - newY;
    if (mDragMouseX <= 7)
        mDragMouseX = 8;
    else if (mDragMouseX >= mWidth - 8)
        mDragMouseX = mWidth - 9;
    if (mDragMouseY <= 7)
        mDragMouseY = 8;
    else if (mDragMouseY >= mHeight - 8)
        mDragMouseY = mHeight - 9;

    Move(newX, newY);
}

void Sexy::Dialog::ButtonPress(int theId)
{
    if (theId >= 1000 && theId <= 1001)
        DialogApp()->DialogButtonPress(mId, theId);
}

void Sexy::Dialog::ButtonDepress(int theId)
{
    if (theId >= 1000 && theId <= 1001)
    {
        mResult = theId;
        DialogApp()->DialogButtonDepress(mId, theId);
    }
}

void Sexy::Dialog::SetColor(int theIdx, Color const& theColor)
{
    Widget::SetColor(theIdx, theColor);

    if (theIdx == 3)
    {
        if (mYesButton != nullptr)
            mYesButton->SetColor(0, theColor);
        if (mNoButton != nullptr)
            mNoButton->SetColor(0, theColor);
    }
    else if (theIdx == 4)
    {
        if (mYesButton != nullptr)
            mYesButton->SetColor(1, theColor);
        if (mNoButton != nullptr)
            mNoButton->SetColor(1, theColor);
    }
}

void Sexy::Dialog::AddedToManager(Sexy::WidgetManager * theWidgetManager)
{
    WidgetContainer::AddedToManager(theWidgetManager);

    if (mAddToManager)
    {
        if (mYesButton != nullptr)
            theWidgetManager->AddWidget(mYesButton);
        if (mNoButton != nullptr)
            theWidgetManager->AddWidget(mNoButton);
    }
    else
    {
        if (mYesButton != nullptr)
            AddWidget(mYesButton);
        if (mNoButton != nullptr)
            AddWidget(mNoButton);
    }
}

void Sexy::Dialog::RemovedFromManager(Sexy::WidgetManager * theWidgetManager)
{
    WidgetContainer::RemovedFromManager(theWidgetManager);

    if (mAddToManager)
    {
        if (mYesButton != nullptr)
            theWidgetManager->RemoveWidget(mYesButton);
        if (mNoButton != nullptr)
            theWidgetManager->RemoveWidget(mNoButton);
    }
    else
    {
        if (mYesButton != nullptr)
            RemoveWidget(mYesButton);
        if (mNoButton != nullptr)
            RemoveWidget(mNoButton);
    }
}

Sexy::Dialog::Dialog(Sexy::Image * theComponentImage, Sexy::Image * theButtonImage, int theId, bool isModal,
                     Sexy::String const& theHeader, Sexy::String const& theLines, Sexy::String const& theFooter,
                     int theButtonMode)
{
    mId = theId;
    mComponentImage = theComponentImage;
    mIsModal = isModal;
    mResult = 0x7FFFFFFF;
    mUsesWidgetColor = false;
    mContentInsets = Sexy::Insets(24, 24, 24, 24);
    mTextAlign = 0;
    mLineSpacingOffset = 0;
    mButtonSidePadding = 0;
    mSpaceAfterHeader = 10;
    mButtonHorzSpacing = 8;

    Sexy::SexyAppBase *theApp = DialogApp();
    mDialogListener = theApp;

    mDialogHeader = theHeader;
    mDialogFooter = theFooter;
    mButtonMode = theButtonMode;

    if (mButtonMode == 1 || mButtonMode == 2)
    {
        mYesButton = new Sexy::DialogButton(theButtonImage, 1000, this);
        mNoButton = new Sexy::DialogButton(theButtonImage, 1001, this);

        if (theApp != nullptr)
        {
            if (mButtonMode == 1)
            {
                mYesButton->mLabel = theApp->GetString("DIALOG_BUTTON_YES", DIALOG_YES_STRING);
                mNoButton->mLabel = theApp->GetString("DIALOG_BUTTON_NO", DIALOG_NO_STRING);
            }
            else
            {
                mYesButton->mLabel = theApp->GetString("DIALOG_BUTTON_OK", DIALOG_OK_STRING);
                mNoButton->mLabel = theApp->GetString("DIALOG_BUTTON_CANCEL", DIALOG_CANCEL_STRING);
            }
        }
    }
    else if (mButtonMode == 3)
    {
        mYesButton = new Sexy::DialogButton(theButtonImage, 1000, this);
        mYesButton->mLabel = mDialogFooter;
        mNoButton = nullptr;
    }
    else
    {
        mYesButton = nullptr;
        mNoButton = nullptr;
        mNumButtons = 0;
    }

    mDialogLines = theLines;
    mHasAlpha = true;
    mPriority = 1;
    mHasTransparencies = true;
    mHeaderFont = nullptr;
    mLinesFont = nullptr;
    mDragging = false;

    if (theButtonImage != nullptr)
    {
        mButtonHeight = theButtonImage->GetCelHeight();
    }
    else
    {
        mButtonHeight = 24;
        gDialogColors[3][0] = 0;
        gDialogColors[3][1] = 0;
        gDialogColors[3][2] = 0;
        gDialogColors[4][0] = 0;
        gDialogColors[4][1] = 0;
        gDialogColors[4][2] = 0;
    }

    SetColors(gDialogColors, 7);
    mTextLayout.SetText(mDialogLines, true);
    mTextLayout.SetSingleLine(false);
    mTextLayout.SetWrap(true);
    mCachedLines = mDialogLines;
}

void Sexy::Dialog::EnsureFonts()
{
    if (mHeaderFont == nullptr && mWidgetManager != nullptr)
    {
        Sexy::SexyAppBase *theApp = mWidgetManager->mApp;
        Sexy::NativeDisplay *display = theApp->mPlatformDriverOrQueue;
        mHeaderFont = display->CreateFont(theApp, "Arial Unicode MS", 14, false, false, false);
    }

    if (mLinesFont == nullptr && mWidgetManager != nullptr)
    {
        Sexy::SexyAppBase *theApp = mWidgetManager->mApp;
        Sexy::NativeDisplay *display = theApp->mPlatformDriverOrQueue;
        mLinesFont = display->CreateFont(theApp, "Arial Unicode MS", 12, false, false, false);
    }
}

void Sexy::Dialog::Draw(Sexy::Graphics * theGraphics)
{
    EnsureFonts();

    Sexy::TRect<int> boxRect(mBackgroundInsets.mLeft,
                             mBackgroundInsets.mTop,
                             mWidth - mBackgroundInsets.mLeft - mBackgroundInsets.mRight,
                             mHeight - mBackgroundInsets.mTop - mBackgroundInsets.mBottom);
    if (mComponentImage != nullptr)
    {
        theGraphics->DrawImageBox(boxRect, mComponentImage);
    }
    else
    {
        Sexy::Color backgroundDefault(gDialogColors[6][0], gDialogColors[6][1], gDialogColors[6][2]);
        theGraphics->SetColor(GetColor(6, backgroundDefault));
        theGraphics->DrawRect(12, 12, mWidth - 25, mHeight - 25);

        Sexy::Color outlineDefault(gDialogColors[5][0], gDialogColors[5][1], gDialogColors[5][2]);
        theGraphics->SetColor(GetColor(5, outlineDefault));
        theGraphics->FillRect(13, 13, mWidth - 26, mHeight - 26);

        theGraphics->SetColor(Sexy::Color(0, 0, 0, 128));
        theGraphics->FillRect(mWidth - 12, 24, 12, mHeight - 36);
        theGraphics->FillRect(24, mHeight - 12, mWidth - 24, 12);
    }

    int textY = mContentInsets.mTop + mBackgroundInsets.mTop;
    if (mDialogHeader.length() != 0)
    {
        int headerY = textY + mHeaderFont->GetAscent() - mHeaderFont->GetAscentPadding();
        theGraphics->SetFont(mHeaderFont);
        theGraphics->SetColor(mColors[0]);
        WriteCenteredLine(theGraphics, headerY, mDialogHeader);
        textY = headerY + mHeaderFont->GetHeight() - mHeaderFont->GetAscent() + mSpaceAfterHeader;
    }

    theGraphics->SetFont(mLinesFont);
    theGraphics->SetColor(mColors[1]);

    int textX = mContentInsets.mLeft + mBackgroundInsets.mLeft + 2;
    int layoutRect[4];
    layoutRect[0] = 0;
    layoutRect[1] = 0;
    layoutRect[2] = mWidth - mContentInsets.mLeft - mContentInsets.mRight
                    - mBackgroundInsets.mLeft - mBackgroundInsets.mRight - 4;
    layoutRect[3] = 0;
    mTextLayout.SetRect(layoutRect);
    mTextLayout.SetJustification(mTextAlign);

    if (mCachedLines != mDialogLines)
    {
        mTextLayout.SetText(mDialogLines, true);
        mCachedLines = mDialogLines;
    }

    mTextLayout.Draw(theGraphics, textX, textY, theGraphics->GetColor());
    int textHeight = mTextLayout.GetHeight();

    if (mDialogFooter.length() != 0 && mButtonMode != 3)
    {
        int footerY = textY + textHeight + 8 + mHeaderFont->GetLineSpacing();
        theGraphics->SetFont(mHeaderFont);
        theGraphics->SetColor(mColors[2]);
        WriteCenteredLine(theGraphics, footerY, mDialogFooter);
    }
}

int Sexy::Dialog::GetPreferredHeight(int theWidth)
{
    EnsureFonts();

    int height = mBackgroundInsets.mBottom + mContentInsets.mBottom
                 + mContentInsets.mTop + mBackgroundInsets.mTop;
    bool hasHeader = mDialogHeader.length() != 0;
    if (hasHeader)
    {
        if (mHeaderFont != nullptr)
            height += mHeaderFont->GetHeight() - mHeaderFont->GetAscentPadding();
        else
            hasHeader = false;
    }

    if (mDialogLines.length() != 0 && mLinesFont != nullptr)
    {
        if (hasHeader)
            height += mSpaceAfterHeader;

        Sexy::Graphics graphics(nullptr);
        graphics.SetFont(mLinesFont);

        int textWidth = theWidth - mContentInsets.mLeft - mContentInsets.mRight
                        - mBackgroundInsets.mLeft - mBackgroundInsets.mRight - 4;
        height += GetWordWrappedHeight(&graphics, textWidth, mDialogLines,
                                       mLineSpacingOffset + mLinesFont->GetLineSpacing());
        hasHeader = true;
    }

    if (mDialogFooter.length() == 0 || mButtonMode == 3 || mHeaderFont == nullptr)
    {
        if (mYesButton == nullptr)
            return height;
        if (hasHeader)
            height += 8;
    }
    else
    {
        if (hasHeader)
            height += 8;
        height += mHeaderFont->GetLineSpacing();
        if (mYesButton == nullptr)
            return height;
        height += 8;
    }

    return height + mButtonHeight + 2;
}

void Sexy::Dialog::MouseDown(int theX, int theY, int theBtnNum, int theClickCount)
{
    if (theClickCount == 1)
    {
        mWidgetManager->mApp->SetCursor(2);
        mDragging = true;
        mDragMouseX = theX;
        mDragMouseY = theY;
    }
    Widget::MouseDown(theX, theY, theBtnNum, theClickCount);
}

void Sexy::Dialog::MouseUp(int theX, int theY, int theBtnNum, int theClickCount)
{
    if (mDragging)
    {
        mWidgetManager->mApp->SetCursor(0);
        mDragging = false;
    }
    Widget::MouseUp(theX, theY, theBtnNum, theClickCount);
}

void Sexy::Dialog::Update()
{
    Widget::Update();
}

int Sexy::Dialog::KeyDown(Sexy::KeyCode theKey)
{
    int result = Widget::KeyDown(theKey);
    if (result == 0)
    {
        Sexy::DialogButton *button = nullptr;
        if (mYesButton != nullptr && theKey == Sexy::KEYCODE_RETURN)
            button = mYesButton;
        else if (mNoButton != nullptr && theKey == Sexy::KEYCODE_ESCAPE)
            button = mNoButton;

        if (button != nullptr)
        {
            mResult = button->mWidgetId;
            DialogApp()->DialogButtonPress(mId, mResult);
            return 1;
        }
    }
    return result;
}

int Sexy::Dialog::KeyUp(Sexy::KeyCode theKey)
{
    int result = Widget::KeyUp(theKey);
    if (result == 0 && mResult != 0x7FFFFFFF
        && ((mYesButton != nullptr && mResult == mYesButton->mWidgetId && theKey == Sexy::KEYCODE_RETURN)
         || (mNoButton != nullptr && mResult == mNoButton->mWidgetId && theKey == Sexy::KEYCODE_ESCAPE)))
    {
        ButtonDepress(mResult);
        return 1;
    }
    return result;
}

Sexy::Dialog::~Dialog()
{
    if (mYesButton != nullptr)
    {
        RemoveWidget(mYesButton);
        delete mYesButton;
    }
    if (mNoButton != nullptr)
    {
        RemoveWidget(mNoButton);
        delete mNoButton;
    }

    RemoveAllWidgets(true, false, mWidgetManager);
    delete mHeaderFont;
    delete mLinesFont;
}
