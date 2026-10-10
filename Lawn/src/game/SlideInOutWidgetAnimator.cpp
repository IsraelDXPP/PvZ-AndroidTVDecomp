#include "SlideInOutWidgetAnimator.h"

#include "LawnApp.h"
#include "Widget.h"

SlideInOutWidgetAnimator::~SlideInOutWidgetAnimator() {}

void SlideInOutWidgetAnimator::Init(Sexy::WidgetAnimationState theState)
{
    int dir = mSlideDir;
    mHideOnCompletion = (theState == 2);
    mUpdateCounter = 0;

    Sexy::Widget *widget = mWidget;
    float fromX = 0.0f;
    float fromY = 0.0f;
    float toX = (float)widget->mX;
    float toY = (float)widget->mY;

    switch (dir)
    {
        case SLIDE_FROM_BOTTOM:
            fromX = (float)widget->mX;
            toX = fromX;
            fromY = (float)(widget->mHeight + LawnApp::FULLSCREEN_RECT.mHeight + LawnApp::FULLSCREEN_RECT.mY);
            toY = (float)widget->mY;
            break;
        case SLIDE_FROM_TOP:
            fromX = (float)widget->mX;
            toY = (float)widget->mY;
            toX = fromX;
            fromY = (float)LawnApp::FULLSCREEN_RECT.mY - (float)((float)widget->mHeight + (float)widget->mHeight);
            break;
        case SLIDE_FROM_LEFT:
            fromY = (float)widget->mY;
            toX = (float)widget->mX;
            toY = fromY;
            fromX = (float)LawnApp::FULLSCREEN_RECT.mX - (float)((float)widget->mWidth + (float)widget->mWidth);
            break;
        case SLIDE_FROM_RIGHT:
            fromY = (float)widget->mY;
            toX = (float)widget->mX;
            fromX = (float)(widget->mWidth + LawnApp::FULLSCREEN_RECT.mWidth + LawnApp::FULLSCREEN_RECT.mX);
            toY = fromY;
            break;
        default:
            break;
    }

    if (theState == 1)
    {
        mFromX = fromX;
        mFromY = fromY;
        mToX = toX;
        mToY = toY;
    }
    else if (theState == 2)
    {
        mToX = fromX;
        mToY = fromY;
        fromX = toX;
        fromY = toY;
        mFromX = toX;
        mFromY = toY;
    }
    else if (theState == 0)
    {
        fromY = toY;
        fromX = toX;
        mFromY = toY;
        mFromX = toX;
        mToX = toX;
        mToY = toY;
    }

    widget->mX = (int)fromX;
    widget->mY = (int)fromY;
    mUpdateCounter = -mCounterOffset;
}

bool SlideInOutWidgetAnimator::IsCompleted()
{
    return mUpdateCounter > 49;
}

void SlideInOutWidgetAnimator::Update()
{
    mUpdateCounter++;

    float x = TodAnimateCurveFloat(0, 50, mUpdateCounter, mFromX, mToX, 4);
    float y = TodAnimateCurveFloat(0, 50, mUpdateCounter, mFromY, mToY, 4);
    mWidget->Move((int)x, (int)y);
}

SlideInOutWidgetAnimator::SlideInOutWidgetAnimator(Sexy::Widget *theWidget, int theSlideDir, int theCounterOffset)
{
    mWidget = theWidget;
    mSlideDir = theSlideDir;
    mCounterOffset = theCounterOffset;
    mUpdateCounter = 0;
    mFromX = 0.0f;
    mFromY = 0.0f;
    mToX = 0.0f;
    mToY = 0.0f;
}
