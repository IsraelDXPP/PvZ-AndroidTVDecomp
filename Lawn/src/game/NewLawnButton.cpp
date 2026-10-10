#include "NewLawnButton.h"

int TodIsPointInPolygon(void *thePoly, int theCount, float *thePoint);

#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Widget.h"

NewLawnButton::~NewLawnButton() {}

bool NewLawnButton::IsPointVisible(int x, int y)
{
    if (!mUsePolygonShape)
        return Sexy::Widget::IsPointVisible(x, y);

    float point[2];
    point[0] = (float)x;
    point[1] = (float)y;
    return TodIsPointInPolygon(mPolygonShape, 4, point) != 0;
}

void NewLawnButton::Draw(Sexy::Graphics *g)
{
    if (mBtnNoDraw)
        return;

    bool pressed = false;
    if (mIsDown)
    {
        if (mIsOver)
            pressed = !mDisabled;
        else
            pressed = mSelected;
    }
    else if (mReturnPressed)
    {
        if (mIsOver)
            pressed = !mDisabled;
        else
            pressed = mSelected;
    }

    int labelX = mTextOffsetX + mTranslateX;
    bool inverted = mInverted;
    int labelY = mTextOffsetY + mTranslateY;
    if (mFont != nullptr)
    {
        if (mLabelJustify == 1)
            labelX += mWidth - mFont->StringWidth(mLabel);
        else if (mLabelJustify == 0)
            labelX += (mWidth - mFont->StringWidth(mLabel)) / 2;

        int ascentSpan = mHeight + mFont->GetAscent();
        labelY += (mFont->GetAscent() / -6 + ascentSpan - 1) / 2;
    }

    if (inverted != pressed)
    {
        g->SetColorizeImages(true);
        g->SetColor(mColors[5]);
        if (HaveButtonImage(mDownImage, mDownRect))
            DrawButtonImage(g, mDownImage, mDownRect, mButtonOffsetX + mTranslateX, mTranslateY + mButtonOffsetY);
        else if (HaveButtonImage(mOverImage, mOverRect))
            DrawButtonImage(g, mOverImage, mOverRect, mButtonOffsetX + mTranslateX, mTranslateY + mButtonOffsetY);
        else if (HaveButtonImage(mButtonImage, mNormalRect))
            DrawButtonImage(g, mButtonImage, mNormalRect, mButtonOffsetX + mTranslateX, mTranslateY + mButtonOffsetY);

        g->SetColorizeImages(false);
        Sexy::Font *font = mHiliteFont;
        if (font == nullptr)
            font = mFont;
        g->SetFont(font);
        g->SetColor(mColors[1]);
        g->DrawString(mLabel, mTextDownOffsetX + labelX, mTextDownOffsetY + labelY);
        return;
    }

    g->SetColorizeImages(true);
    g->SetColor(mColors[5]);
    if (mDisabled && HaveButtonImage(mDisabledImage, mDisabledRect))
    {
        DrawButtonImage(g, mDisabledImage, mDisabledRect, mButtonOffsetX, mButtonOffsetY);
    }
    else if (mOverAlpha > 0.0 && HaveButtonImage(mOverImage, mOverRect))
    {
        if (HaveButtonImage(mButtonImage, mNormalRect) && mOverAlpha < 1.0)
            DrawButtonImage(g, mButtonImage, mNormalRect, mButtonOffsetX, mButtonOffsetY);

        g->mColor.mAlpha = (int)(mOverAlpha * 255.0);
        DrawButtonImage(g, mOverImage, mOverRect, mButtonOffsetX, mButtonOffsetY);
    }
    else if ((mIsOver || mIsDown || mSelected) && HaveButtonImage(mOverImage, mOverRect))
    {
        DrawButtonImage(g, mOverImage, mOverRect, mButtonOffsetX, mButtonOffsetY);
    }
    else if (HaveButtonImage(mButtonImage, mNormalRect))
    {
        DrawButtonImage(g, mButtonImage, mNormalRect, mButtonOffsetX, mButtonOffsetY);
    }

    g->SetColorizeImages(false);
    if (mIsOver || mSelected)
    {
        Sexy::Font *font = mHiliteFont;
        if (font == nullptr)
            font = mFont;
        g->SetFont(font);
        g->SetColor(mColors[1]);
    }
    else
    {
        g->SetFont(mFont);
        g->SetColor(mColors[0]);
    }
    g->DrawString(mLabel, labelX, labelY);
}

void NewLawnButton::SetLabel(Sexy::String const &theLabel)
{
    Sexy::String translated;
    TodStringTranslate(&translated, theLabel);
    mLabel = translated;
}

NewLawnButton::NewLawnButton(Sexy::Image *theComponentImage, int theId, Sexy::ButtonListener *theListener) :
    Sexy::DialogButton(theComponentImage, theId, theListener)
{
    mHiliteFont = nullptr;
    mTextDownOffsetX = 0;
    mTextDownOffsetY = 0;
    mButtonOffsetX = 0;
    mButtonOffsetY = 0;
    mUsePolygonShape = false;
    for (int i = 0; i < 4; ++i)
    {
        mPolygonShape[i].mX = 0;
        mPolygonShape[i].mY = 0;
    }
    Sexy::Widget::SetColor(5, Sexy::Color::White);
}
