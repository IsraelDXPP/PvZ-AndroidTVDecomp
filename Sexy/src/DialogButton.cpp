// DialogButton draws a three-cel image box and a centred label.

#include "DialogButton.h"

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "NativeDisplay.h"
#include "SexyAppBase.h"
#include "TextLayout.h"
#include "WidgetManager.h"

Sexy::DialogButton::~DialogButton() {}

Sexy::DialogButton::DialogButton(Sexy::Image *theImage, int theId, Sexy::ButtonListener *theListener) :
    ButtonWidget(theId, theListener)
{
    mImage = theImage;

    if (theImage != nullptr && theImage->GetCelCount() == 3)
    {
        mNormalRect = theImage->GetCelRect(0);
        mOverRect = theImage->GetCelRect(1);
        mDownRect = theImage->GetCelRect(2);
    }

    mTextOffsetY = 0;
    mTextOffsetX = 0;
    mTranslateY = 1;
    mTranslateX = 1;
    mDoFinger = true;
    SetColors(gDialogButtonColors, 6);
}

void Sexy::DialogButton::Draw(Sexy::Graphics *theGraphics)
{
    if (mBtnNoDraw)
        return;

    if (mImage == nullptr)
    {
        ButtonWidget::Draw(theGraphics);
        return;
    }

    if (mFont == nullptr && mLabel.length() != 0)
    {
        Sexy::SexyAppBase *theApp = mWidgetManager->mApp;
        Sexy::NativeDisplay *display = theApp->mPlatformDriverOrQueue;
        mFont = display->CreateFont(theApp, "Arial Unicode MS", 12, true, false, false);
    }

    bool isDown = IsButtonDown();
    bool hasFocus = mHasFocus && ShouldShowNavigationFocus();
    double overAlpha = mOverAlpha;
    TRect<int> dest(0, 0, mWidth, mHeight);

    if (mNormalRect.mWidth != 0)
    {
        if (mDisabled && mDisabledRect.mWidth > 0 && mDisabledRect.mHeight > 0)
        {
            theGraphics->DrawImageBox(mDisabledRect, dest, mImage);
        }
        else if (isDown)
        {
            theGraphics->DrawImageBox(mDownRect, dest, mImage);
        }
        else if (overAlpha > 0.0)
        {
            if (overAlpha < 1.0)
                theGraphics->DrawImageBox(mNormalRect, dest, mImage);

            theGraphics->SetColorizeImages(true);
            theGraphics->SetColor(Sexy::Color(255, 255, 255, (int)(overAlpha * 255.0)));
            theGraphics->DrawImageBox(mOverRect, dest, mImage);
            theGraphics->SetColorizeImages(false);
        }
        else if (mIsOver || hasFocus)
        {
            if (mSelected && ShouldShowNavigationFocus())
            {
                if (mSelectAlpha < 1.0f)
                    theGraphics->DrawImageBox(mNormalRect, dest, mImage);

                theGraphics->SetColorizeImages(true);
                theGraphics->SetColor(Sexy::Color(255, 255, 255, (int)(mSelectAlpha * 255.0f)));
                theGraphics->DrawImageBox(mOverRect, dest, mImage);
                theGraphics->SetColorizeImages(false);
            }
            else
            {
                theGraphics->DrawImageBox(mOverRect, dest, mImage);
            }
        }
        else
        {
            theGraphics->DrawImageBox(mNormalRect, dest, mImage);
        }

        if (isDown)
            theGraphics->Translate(mTranslateX, mTranslateY);
    }
    else
    {
        if (isDown)
            theGraphics->Translate(mTranslateX, mTranslateY);

        theGraphics->DrawImageBox(dest, mImage);
    }

    if (mFont != nullptr)
    {
        theGraphics->SetFont(mFont);

        if (mIsOver || hasFocus)
            theGraphics->SetColor(mColors[1]);
        else
            theGraphics->SetColor(mColors[0]);

        int innerWidth = mWidth - mInsets.mLeft - mInsets.mRight;
        int textX = mInsets.mLeft + (innerWidth - mTextLayout.GetWidth()) / 2;
        int textY;
        if (mInsets.mTop != 0 || mInsets.mBottom != 0)
        {
            float innerHeight = (float)(mHeight - mInsets.mTop - mInsets.mBottom);
            textY = (int)((float)mInsets.mTop - mLabelExtents.mMinY + (innerHeight - mLabelExtents.mHeight) * 0.5f);
        }
        else
        {
            int ascent = mFont->GetAscent();
            int base = mHeight + ascent - mFont->GetAscentPadding();
            textY = (base + ascent / -6 - 1) / 2;
        }

        textX += mTextOffsetX;
        textY += mTextOffsetY;
        mTextLayout.Draw(theGraphics, textX, textY, theGraphics->GetColor());
    }

    if (isDown)
        theGraphics->Translate(-mTranslateX, -mTranslateY);
}
