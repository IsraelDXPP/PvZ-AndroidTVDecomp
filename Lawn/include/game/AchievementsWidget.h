// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
#pragma once
#include "Widget.h"
#include "ButtonListener.h"

class LawnApp;
class GameButton;

class AchievementsWidget : public Sexy::Widget, public Sexy::ButtonListener
{
public:
    AchievementsWidget(LawnApp* theApp, bool theFromGame);
    ~AchievementsWidget() override;
    void AddedToManager(Sexy::WidgetManager* theManager) override;
    void RemovedFromManager(Sexy::WidgetManager* theManager) override;
    void GotFocus() override;
    void Update() override;
    void Draw(Sexy::Graphics* theGraphics) override;
    void MouseDown(int x, int y, int theClickCount) override;
    void MouseDrag(int x, int y) override;
    void MouseUp(int x, int y, int theClickCount) override;
    void MouseWheel(int theDelta) override;
    int KeyDown(Sexy::KeyCode theKey) override;
    void ButtonDepress(int theId) override;

private:
    void ScrollTo(int theY);
    LawnApp* mApp;
    GameButton* mBackButton;
    bool mFromGame;
    bool mDragging = false;
    int mScrollY = 0;
    int mDragStartY = 0;
    int mDragStartScroll = 0;
    float mScrollVelocity = 0.0f;
};
