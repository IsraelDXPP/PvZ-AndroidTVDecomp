#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_HOUSECHOOSERDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_HOUSECHOOSERDIALOG_H

#include "ReconTypes.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnDialog.h"

class LawnApp;

class HouseChooserDialog : public LawnDialog
{
public:
    virtual ~HouseChooserDialog();

    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager) override;
    virtual void Update() override;
    virtual void Draw(Sexy::Graphics *g) override;
    virtual void Resize(int theX, int theY, int theWidth, int theHeight) override;
    virtual void GotFocus() override;
    int KeyDown(Sexy::KeyCode theKey) override;
    void ButtonDepress(int theId) override;
    void MouseDown(int x, int y, int theClickCount) override;

    HouseChooserDialog(LawnApp *theApp);
    void GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags);
    static bool IsHouseAvaliable(HouseType houseType);

    HouseType mSelectedHouseType;
    float mGrowTime;
    HelpBarWidget *mHelpBarWidget = nullptr;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_HOUSECHOOSERDIALOG_H
