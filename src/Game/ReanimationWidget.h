#pragma once

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
