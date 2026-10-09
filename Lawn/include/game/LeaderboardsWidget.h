// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
#pragma once
#include "Widget.h"
#include "ButtonListener.h"
#include "Reanimation.h"
#include "TrashBin.h"
#include <array>
#include <memory>

class LawnApp;
class GameButton;

class LeaderboardsWidget : public Sexy::Widget, public Sexy::ButtonListener
{
public:
    LeaderboardsWidget(LawnApp* theApp, bool theFromGame);
    ~LeaderboardsWidget() override;
    void AddedToManager(Sexy::WidgetManager* theManager) override;
    void RemovedFromManager(Sexy::WidgetManager* theManager) override;
    void GotFocus() override;
    void Update() override;
    void Draw(Sexy::Graphics* theGraphics) override;
    void MouseDown(int x, int y, int theClickCount) override;
    int KeyDown(Sexy::KeyCode theKey) override;
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    void ButtonDepress(int theId) override;
    void ShowRecords();

private:
    void Close();
    void ChooseHouse();
    LawnApp* mApp;
    bool mFromGame;
    GameButton* mBackButton;
    std::array<std::unique_ptr<Reanimation>, 5> mBackgrounds;
    std::array<std::unique_ptr<Reanimation>, 12> mDecorations;
    std::unique_ptr<TrashBin> mPlantTrash;
    std::unique_ptr<TrashBin> mZombieTrash;
    int mFocusedDecoration = 0;
    bool mHighlightDecoration = false;
};
