// ScrollbuttonWidget draws one arrow of a scrollbar. A button image uses a
// cel for the direction. Without an image it paints a beveled square and a
// four-step arrow.

#include "ScrollbuttonWidget.h"
#include "ButtonWidget.h"
#include "Graphics.h"
#include "Image.h"

Sexy::ScrollbuttonWidget::~ScrollbuttonWidget()
{
}

void Sexy::ScrollbuttonWidget::Draw(Sexy::Graphics* theGraphics)
{
    if (mButtonImage != nullptr || mDownImage != nullptr)
    {
        int cel = 0;
        if (mDirection <= 0)
        {
            if (mInvisIfNoScroll && mButtonImage != nullptr && mButtonImage->mNumCols > 2)
                cel = 2;
            if (mWidgetId == 1)
                ++cel;
        }
        else
        {
            cel = mDirection - 1;
            if (cel > 2 && mButtonImage != nullptr && mButtonImage->mNumCols <= 2)
                cel = mDirection - 3;
        }

        Sexy::Image* image = mButtonImage;
        if (mIsDown && mIsOver && !mDisabled)
            image = mDownImage;
        else if (mDisabled && mDisabledImage != nullptr)
            image = mDisabledImage;
        if (image == nullptr)
            image = mButtonImage;
        if (image != nullptr)
            theGraphics->DrawImageCel(image, 0, 0, cel);
        return;
    }

    theGraphics->SetColor(Sexy::Color(212, 212, 212));
    theGraphics->FillRect(0, 0, mWidth, mHeight);

    int nudge = 0;
    if (mIsDown && mIsOver && !mDisabled)
    {
        theGraphics->SetColor(Sexy::Color(132, 132, 132));
        nudge = 1;
        theGraphics->DrawRect(0, 0, mWidth - 1, mHeight - 1);
    }
    else
    {
        theGraphics->SetColor(Sexy::Color(255, 255, 255));
        theGraphics->FillRect(1, 1, mWidth - 2, 1);
        theGraphics->FillRect(1, 1, 1, mHeight - 2);
        theGraphics->SetColor(Sexy::Color::Black);
        theGraphics->FillRect(0, mHeight - 1, mWidth, 1);
        theGraphics->FillRect(mWidth - 1, 0, 1, mHeight);
        theGraphics->SetColor(Sexy::Color(132, 132, 132));
        theGraphics->FillRect(1, mHeight - 2, mWidth - 2, 1);
        theGraphics->FillRect(mWidth - 2, 1, 1, mHeight - 2);
    }

    if (mDisabled)
        theGraphics->SetColor(Sexy::Color(132, 132, 132));
    else
        theGraphics->SetColor(Sexy::Color::Black);

    const bool vertical = mInvisIfNoScroll || (unsigned)(mDirection - 3) <= 1u;
    if (vertical)
    {
        int span = 1;
        for (int step = 0; step != 4; ++step)
        {
            const bool pointUp = mWidgetId == 0 || mDirection == 3;
            const int arrowX = (pointUp ? step : 3 - step) + (mWidth - 4) / 2 + nudge;
            const int arrowY = mHeight / 2 - step - 1 + nudge;
            theGraphics->FillRect(arrowX, arrowY, 1, span);
            span += 2;
        }
        return;
    }

    int span = 1;
    for (int step = 0; step != 4; ++step)
    {
        const bool pointLeft = mWidgetId == 0 || mDirection == 1;
        const int arrowX = mWidth / 2 - step - 1 + nudge;
        const int arrowY = (pointLeft ? step : 3 - step) + (mHeight - 4) / 2 + nudge;
        theGraphics->FillRect(arrowX, arrowY, span, 1);
        span += 2;
    }
}

Sexy::ScrollbuttonWidget::ScrollbuttonWidget(int theId, Sexy::ButtonListener* theListener, int theDirection)
    : ButtonWidget(theId, theListener)
    , mInvisIfNoScroll(false)
    , mDirection(theDirection)
{
}
