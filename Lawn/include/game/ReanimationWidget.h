#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATIONWIDGET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATIONWIDGET_H

#include "ReconTypes.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "Widget.h"

class LawnApp;
class LawnDialog;

class ReanimationWidget : public Sexy::Widget
{
public:
    virtual ~ReanimationWidget();

    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);

    LawnApp *mApp;
    Reanimation *mReanim;
    LawnDialog *mLawnDialog;
    float mPosX;
    float mPosY;

    ReanimationWidget();
    void AddReanimation(float x, float y, ReanimationType theReanimationType);
    void Dispose();
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_REANIMATIONWIDGET_H
