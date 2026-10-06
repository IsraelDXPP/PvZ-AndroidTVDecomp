#include "GameButton.h"
#include "ReconData.h"

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "DialogButton.h"
#include "Font.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "Widget.h"
#include <cmath>

void GameButton::SetFont(Sexy::Font* theFont)
{
    delete mFont;
    mFont = theFont->Duplicate();
}

bool GameButton::IsButtonDown()
{
    if (!mIsDown && !mReturnPressed)
        return false;
    if (!mIsOver && !mSelected)
        return false;
    if (mDisabled)
        return false;
    return !mBtnNoDraw;
}

void GameButton::SetDisabled(bool isDisabled)
{
    mDisabled = isDisabled;
}

void GameButton::FocusDirectionHint(Sexy::WidgetFocusDir theDir)
{
    mFocusDirection = (int)theDir;
    mPulseFrame = 0;
    mApp->PlaySample(SOUND_BLEEP, true);
}

void GameButton::SetPadding(int theX, int theY)
{
    mInsets.mLeft = theX;
    mInsets.mTop = theY;
}

void GameButton::Resize(int theX, int theY, int theWidth, int theHeight)
{
    Sexy::Widget::Resize(theX, theY, theWidth, theHeight);
    mAnchorX = (float)mX;
    mAnchorY = (float)mY;
}

GameButton::~GameButton()
{
    delete mFont;
    mFont = nullptr;
}

void GameButton::Update()
{
    Sexy::ButtonWidget::Update();

    const bool hot = mIsOver || mIsDown;
    if (hot)
    {
        if (mOverAlphaFadeInSpeed > 0.0 && mOverAlpha < 1.0)
        {
            mOverAlpha += mOverAlphaFadeInSpeed;
            if (mOverAlpha > 1.0)
                mOverAlpha = 1.0;
        }
    }
    else if (mOverAlpha > 0.0)
    {
        if (mOverAlphaSpeed <= 0.0)
            mOverAlpha = 0.0;
        else
        {
            mOverAlpha -= mOverAlphaSpeed;
            if (mOverAlpha < 0.0)
                mOverAlpha = 0.0;
        }
    }

    ++mPulseFrame;

    if (!hot && !mSelected)
    {
        mPulseTime = 0;
        return;
    }
    if (mDisabled)
    {
        mPulseTime = 0;
        return;
    }
    if (++mPulseTime == 15)
        mPulseFrame = 0;
}

int GameButton::KeyDown(Sexy::KeyCode theKey)
{
    Sexy::Widget* topLevel = GetTopLevel();
    if (topLevel->mEvent != nullptr)
        mGamepadIndex = mApp->GetGamepadIndex(topLevel->mEvent->modifiers, 0);
    return Sexy::ButtonWidget::KeyDown(theKey);
}

void GameButton::SetLabel(const Sexy::String& theLabel)
{
    Sexy::String translated;
    TodStringTranslate(&translated, theLabel);
    mLabel = translated;

    TodReplaceButtons(&translated, &mLabel);
    mLabel = translated;
}

void GameButton::DrawButtonImage(Sexy::Graphics* theGraphics, Sexy::Image* theImage, const TRect<int>& theRect, int theX, int theY)
{
    bool inverted = mInverted;
    bool highlighted = false;

    if (!mIsOver)
    {
        highlighted = mSelected && !mDisabled;
    }
    else if (!mDisabled)
    {
        highlighted = true;
        if (mIsDown || mReturnPressed)
            inverted = !mInverted;
    }

    if (mDrawStoneButton)
    {
        DrawStoneButton(theGraphics, theX, theY, mWidth, mHeight, inverted, highlighted, mLabel, mDisabled);
        return;
    }

    if (theRect.mWidth != 0)
        theGraphics->DrawImageF(mButtonImage, (float)theX, (float)theY, theRect);
    else
        theGraphics->DrawImageF(theImage, (float)theX, (float)theY);

    if (!highlighted)
        return;

    theGraphics->SetDrawMode(1);
    theGraphics->SetColorizeImages(true);
    float pulse = sinf((float)mPulseFrame * 0.04f) * 0.5f + 1.0f;
    if (mPulseTime <= 15)
        pulse += mScaleX;
    float alpha = pulse * 0.2f;
    if (alpha > 0.25f)
        alpha = 0.25f;
    Color glow(255, 255, 255, (int)(alpha * 255.0f));
    theGraphics->SetColor(glow);
    theGraphics->DrawImageF(theImage, (float)theX, (float)theY);
}

GameButton::GameButton(int theId, Sexy::ButtonListener* theListener)
    : Sexy::DialogButton(nullptr, theId, theListener)
{
    mWidgetId = theId;
    mFont = nullptr;
    mApp = (LawnApp*)gSexyAppBase;
    mLabelJustify = 0;
    mButtonImage = nullptr;
    mOverImage = nullptr;
    mDownImage = nullptr;
    mDisabledImage = nullptr;
    mOverOverlayImage = nullptr;
    mParentWidget = nullptr;

    mInverted = false;
    mBtnNoDraw = false;
    mFrameNoDraw = false;
    mDisabled = false;
    mIsDown = false;
    mIsOver = false;

    mX = 0;
    mY = 0;
    mUseFixedOrigin = false;
    mWidth = 0;
    mHeight = 0;
    mDrawStoneButton = false;
    mTextOffsetX = 0;
    mTextOffsetY = 0;
    mButtonOffsetX = 0;
    mButtonOffsetY = 0;
    mAnchorX = 0.0f;
    mAnchorY = 0.0f;
    mOverAlpha = 0.0;
    mOverAlphaSpeed = 0.0;
    mOverAlphaFadeInSpeed = 0.0;

    for (int colorIndex = 0; colorIndex < 6; ++colorIndex)
    {
        mColors[colorIndex] = Color(
            gGameButtonColors[colorIndex][0],
            gGameButtonColors[colorIndex][1],
            gGameButtonColors[colorIndex][2]);
    }

    mScaleX = 1.0f;
    mScaleY = 1.0f;
    mAppliedScaleX = 1.0f;
    mAppliedScaleY = 1.0f;
    mWobble = 0.0f;
    mFocusDirection = -1;
    mPulseFrame = 0;
    mPulseTime = 0;
    mSelected = false;
    mPressSoundId = -1;
    mGamepadIndex = -1;
}

bool GameButton::HaveButtonImage(Sexy::Image* theImage, const TRect<int>& theRect)
{
    return theImage != nullptr || theRect.mWidth != 0;
}

void GameButton::Draw(Sexy::Graphics* theGraphics)
{
    bool inverted = mInverted;
    mApp = (LawnApp*)gSexyAppBase;
    if (mBtnNoDraw)
        return;

    // The glow face is the pointer-over state. Focus alone does not light
    // every button; the focused one gets the additive pulse in DrawButtonImage.
    const bool highlighted = (mIsOver || mSelected) && !mDisabled;
    if (mIsOver || mSelected)
        inverted = !mDisabled;

    mWobble = TodAnimateCurveFloat(0, 15, mPulseTime, 0.0f, 1.0f, CURVE_EASE_IN_OUT);
    theGraphics->PushState();

    if (mUseFixedOrigin)
    {
        theGraphics->mTransX = (mAnchorX + theGraphics->mTransX) - (float)mX;
        theGraphics->mTransY = (mAnchorY + theGraphics->mTransY) - (float)mY;
    }

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    // The press-offset index is the wobble word read as an integer. A curve
    // value in (0, 1] has an exponent, so its bits are far above 3 and the
    // table is skipped. Only a wobble of exactly 0 selects the first pair,
    // and that pair is then multiplied by zero, so the shake stays put.
    if (highlighted && mWobble == 0.0f)
    {
        offsetX = gButtonPressOffset[0];
        offsetY = gButtonPressOffset[4];
    }

    const int shakeX = (int)(offsetX * (mWobble + mWobble));
    const int shakeY = (int)(offsetY * (mWobble + mWobble));
    const float pulse = mWobble * 0.04f;
    const float pivotY = (float)shakeY + (float)mHeight * 0.5f;
    mAppliedScaleX = pulse + mScaleX;
    mAppliedScaleY = pulse + mScaleY;
    theGraphics->SetScale(mAppliedScaleX, mAppliedScaleY, (float)shakeX + (float)mWidth * 0.5f, pivotY);

    if (mDrawStoneButton)
    {
        DrawButtonImage(theGraphics, mDisabledImage, mDisabledRect, shakeX, shakeY);
        theGraphics->PopState();
        return;
    }

    Sexy::Font* font = mFont;
    int labelX = mTextOffsetX;
    int labelY = mTextOffsetY;
    if (font != nullptr)
    {
        if (mLabelJustify == 0)
            labelX += (mWidth - font->StringWidth(mLabel)) / 2;
        else if (mLabelJustify == 1)
            labelX += mWidth - font->StringWidth(mLabel);
        else if (mLabelJustify == 2)
            labelX = mWidth / 2;

        const int textHeight = mHeight + font->GetHeight();
        const int ascent = font->GetAscent();
        labelY += (textHeight + ascent / -6 - 1) / 2;
    }
    theGraphics->SetFont(font);

    const bool pressedLook = inverted != (mIsOver != 0);
    if (pressedLook)
    {
        if (HaveButtonImage(mOverImage, mOverRect))
            DrawButtonImage(theGraphics, mOverImage, mOverRect, shakeX, shakeY);
        else if (HaveButtonImage(mDownImage, mDownRect))
            DrawButtonImage(theGraphics, mDownImage, mDownRect, shakeX + 1, shakeY + 1);
        else
            DrawButtonImage(theGraphics, mButtonImage, mNormalRect, shakeX + 1, shakeY + 1);
        theGraphics->SetColor(mColors[1]);
    }
    else if (mDisabled && HaveButtonImage(mDisabledImage, mDisabledRect))
    {
        DrawButtonImage(theGraphics, mDisabledImage, mDisabledRect, shakeX, shakeY);
        theGraphics->SetColor(highlighted ? mColors[1] : mColors[0]);
    }
    else if (mOverAlpha > 0.0 && HaveButtonImage(mOverImage, mOverRect))
    {
        if (HaveButtonImage(mButtonImage, mNormalRect) && mOverAlpha < 1.0)
            DrawButtonImage(theGraphics, mButtonImage, mNormalRect, shakeX, shakeY);

        theGraphics->SetColorizeImages(true);
        Color fadeColor(255, 255, 255, (int)(mOverAlpha * 255.0));
        theGraphics->SetColor(fadeColor);
        DrawButtonImage(theGraphics, mOverImage, mOverRect, shakeX, shakeY);
        theGraphics->SetColorizeImages(false);
        theGraphics->SetColor(mColors[0]);
    }
    else if (mIsOver && HaveButtonImage(mOverImage, mOverRect))
    {
        DrawButtonImage(theGraphics, mOverImage, mOverRect, shakeX, shakeY);
        theGraphics->SetColor(mColors[1]);
    }
    else if (HaveButtonImage(mButtonImage, mNormalRect))
    {
        DrawButtonImage(theGraphics, mButtonImage, mNormalRect, shakeX, shakeY);
        theGraphics->SetColor(highlighted ? mColors[1] : mColors[0]);
    }
    else
    {
        theGraphics->SetColor(highlighted ? mColors[1] : mColors[0]);
    }

    if (mLabelJustify == 2)
    {
        const int textX = shakeX + (pressedLook ? 21 : 20);
        const int textY = shakeY + (pressedLook ? 17 : 16);
        TRect<int> wrapRect(textX, textY, mWidth - 40, mHeight - 32);
        const int lineSpacing = (int)((float)font->GetLineSpacing() * 0.8f);
        theGraphics->WriteWordWrapped(wrapRect, mLabel, lineSpacing, 0, nullptr, -1, nullptr);
    }
    else if (pressedLook)
    {
        theGraphics->DrawString(mLabel, labelX + shakeX + 1, shakeX + labelY + 1);
    }
    else
    {
        theGraphics->DrawString(mLabel, labelX + shakeX, shakeX + labelY);
    }

    if (mIsOver && mOverOverlayImage != nullptr)
    {
        theGraphics->SetDrawMode(1);
        DrawButtonImage(theGraphics, mOverOverlayImage, mNormalRect, shakeX, shakeY);
        theGraphics->SetDrawMode(0);
    }
    theGraphics->PopState();
}

bool GameButton::IsMouseOver()
{
    if (!mIsOver && !mSelected)
        return false;
    if (mDisabled)
        return false;
    return !mBtnNoDraw;
}

int GameButton::OnPressed()
{
    if (mPressSoundId == -1)
    {
        mApp->PlaySample(mDrawStoneButton ? SOUND_GRAVEBUTTON : SOUND_CERAMIC, true);
        return 0;
    }

    mApp->PlaySample(Sexy::GetSoundById(mPressSoundId), true);
    return 0;
}
