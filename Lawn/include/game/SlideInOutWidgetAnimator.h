#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_SLIDEINOUTWIDGETANIMATOR_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_SLIDEINOUTWIDGETANIMATOR_H

#include "ReconTypes.h"
#include "WidgetAnimator.h"

class SlideInOutWidgetAnimator : public Sexy::WidgetAnimator
{
public:
    enum SlideDir
    {
        SLIDE_FROM_BOTTOM = 0,
        SLIDE_FROM_TOP = 1,
        SLIDE_FROM_LEFT = 2,
        SLIDE_FROM_RIGHT = 3,
    };

    virtual ~SlideInOutWidgetAnimator();

    virtual void Init(Sexy::WidgetAnimationState theState);
    virtual void Update();
    virtual bool IsCompleted();

    SlideInOutWidgetAnimator(Sexy::Widget *theWidget, int theSlideDir, int theCounterOffset);

    int mSlideDir;
    int mCounterOffset;
    float mFromX;
    float mFromY;
    float mToX;
    float mToY;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_SLIDEINOUTWIDGETANIMATOR_H
