#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_HELPTEXTSCREEN_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_HELPTEXTSCREEN_H

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

    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager) override;
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager) override;
    virtual void Update() override;
    virtual void Draw(Sexy::Graphics *g) override;
    virtual void DrawAll(Sexy::ModalFlags *theFlags, Sexy::Graphics *g) override;
    virtual void GotFocus() override;
    virtual int KeyDown(Sexy::KeyCode theKey) override;
    void MouseDown(int x, int y, int theClickCount) override;

    HelpTextScreen(LawnApp *theApp, int thePage);

    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theModifiers);

    LawnApp *mApp;
    float mScrollFrom;
    float mScrollTo;
    float mScrollTimer;
    int mPage;
    HelpBarWidget *mHelpBarWidget;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_HELPTEXTSCREEN_H
