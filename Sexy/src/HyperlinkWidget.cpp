// HyperlinkWidget draws its label as a centred link and underlines it.

#include "HyperlinkWidget.h"

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "NativeDisplay.h"
#include "SexyAppBase.h"
#include "TextLayout.h"
#include "WidgetManager.h"

Sexy::HyperlinkWidget::~HyperlinkWidget() {}

void Sexy::HyperlinkWidget::Draw(Sexy::Graphics *theGraphics)
{
    if (mFont != nullptr)
    {
        if (mUpdateCnt == 0)
            UpdateLabel();
    }
    else
    {
        Sexy::SexyAppBase *theApp = mWidgetManager->mApp;
        Sexy::NativeDisplay *display = theApp->mPlatformDriverOrQueue;
        mFont = display->CreateFont(theApp, "Arial Unicode MS", 10, false, false, false);
        mTextLayout.SetFont(mFont);
        UpdateLabel();
    }

    const int layoutWidth = mTextLayout.GetWidth();
    const int textX = (mWidth - layoutWidth) / 2;
    const int textY = (int)(((float)mHeight - mLabelExtents.mHeight) * 0.5f - mLabelExtents.mMinY);

    const bool hasFocus = mHasFocus;
    if (mIsOver || hasFocus)
        theGraphics->SetColor(mOverColor);
    else
        theGraphics->SetColor(mColor);

    if (hasFocus && mSelected)
    {
        const unsigned int scale = (unsigned int)(mSelectAlpha * 255.0f);
        Sexy::Color color = theGraphics->GetColor();
        color.mAlpha = (color.mAlpha * scale + 128 + ((color.mAlpha * scale + 128) >> 8)) >> 8;
        theGraphics->SetColor(color);
    }

    theGraphics->SetFont(mFont);
    mTextLayout.Draw(theGraphics, textX, textY, theGraphics->GetColor());

    const int underlineY = (int)(mLabelExtents.mMinY + mLabelExtents.mHeight + (float)mUnderlineOffset + (float)textY);
    for (int line = 0; line < mUnderlineSize; ++line)
        theGraphics->FillRect(textX, underlineY + line, layoutWidth, 1);
}

Sexy::HyperlinkWidget::HyperlinkWidget(int theId, Sexy::ButtonListener *theListener) :
    ButtonWidget(theId, theListener)
{
    mColor = Sexy::Color(255, 255, 255);
    mOverColor = Sexy::Color(255, 255, 255);
    mDoFinger = true;
    mUnderlineOffset = 1;
    mUnderlineSize = 1;
    mUsesWidgetColor = true;
}

void Sexy::HyperlinkWidget::MouseEnter()
{
    ButtonWidget::MouseEnter();
    MarkDirtyFull();
}

void Sexy::HyperlinkWidget::MouseLeave()
{
    ButtonWidget::MouseLeave();
    MarkDirtyFull();
}
