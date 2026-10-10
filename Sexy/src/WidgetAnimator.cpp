#include "WidgetAnimator.h"

Sexy::WidgetAnimator::~WidgetAnimator() {}

void Sexy::WidgetAnimator::Init(Sexy::WidgetAnimationState theState)
{
    mHideOnCompletion = ((int)theState == 2);
    mUpdateCounter = 0;
}

void Sexy::WidgetAnimator::Update()
{
    ++mUpdateCounter;
}

bool Sexy::WidgetAnimator::HideOnCompletion()
{
    return mHideOnCompletion;
}
