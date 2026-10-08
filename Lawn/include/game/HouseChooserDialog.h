#pragma once

#include "ReconTypes.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnDialog.h"

class LawnApp;

class HouseChooserDialog : public LawnDialog
{
public:
    virtual ~HouseChooserDialog();

    virtual void RemovedFromManager(Sexy::WidgetManager* theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics* g);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual void GotFocus();
    void MouseDown(int x, int y, int theClickCount) override;

    HouseChooserDialog(LawnApp* theApp);
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    static bool IsHouseAvaliable(HouseType houseType);

    HouseType mSelectedHouseType;
    float mGrowTime;
    int mReserved;
};
