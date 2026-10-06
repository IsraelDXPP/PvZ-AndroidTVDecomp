#pragma once

#include "ReconTypes.h"
#include "HelpBarWidget.h"
#include "LawnMenuWidget.h"
#include "MenuWidget.h"
#include "Widget.h"

class LawnApp;

class HelpTextScreen : public LawnMenuWidget
{
public:
    virtual ~HelpTextScreen();

    virtual void AddedToManager(Sexy::WidgetManager* theWidgetManager);
    virtual void RemovedFromManager(Sexy::WidgetManager* theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics* g);
    virtual void DrawAll(Sexy::ModalFlags* theFlags, Sexy::Graphics* g);
    virtual void GotFocus();
    virtual int KeyDown(Sexy::KeyCode theKey);

    HelpTextScreen(LawnApp* theApp, int thePage);

    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifiers);

    LawnApp* mApp;
    float mScrollFrom;
    float mScrollTo;
    float mScrollTimer;
    int mPage;
    HelpBarWidget* mHelpBarWidget;
};
