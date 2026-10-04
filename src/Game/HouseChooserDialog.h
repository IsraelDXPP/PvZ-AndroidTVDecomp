#pragma once

#include "ReconTypes.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnDialog.h"

class LawnApp;

enum HouseType
{
    BLUEPRINT_INVALID = -1,
    BLUEPRINT_BLING = 0,
    BLUEPRINT_SCARY = 1,
    BLUEPRINT_TRAILER = 2,
    BLUEPRINT_FUTURE = 3,
    BLUEPRINT_CLOWN = 4,
};

class HouseChooserDialog : public LawnDialog
{
public:
    virtual ~HouseChooserDialog();

    virtual void RemovedFromManager(Sexy::WidgetManager* theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics* g);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual void GotFocus();

    HouseChooserDialog(LawnApp* theApp);
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    static bool IsHouseAvaliable(HouseType houseType);

    HouseType mSelectedHouseType;
    float mGrowTime;
    int mReserved;
};
