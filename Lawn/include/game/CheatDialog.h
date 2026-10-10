// CheatDialog -- TV developer level selector (0x191AA0).
// The edit widget reports input through the dialog's EditListener base.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_CHEATDIALOG_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_CHEATDIALOG_H

#include "ReconTypes.h"
// These includes exist because the transcribed bodies call methods on complete
// types. Kept as-is so other files' transitive dependencies do not change.
#include "EditListener.h"
#include "Dialog.h"
#include "EditWidget.h"
#include "Font.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"

// LawnApp is used only as `LawnApp *mApp` and as the constructor's pointer
// parameter, so it is declared rather than included. The include closed the
// LawnApp.h <-> CheatDialog.h cycle, which left LawnApp undefined whenever this
// header was entered first.
class LawnApp;

class CheatDialog : public LawnDialog, public Sexy::EditListener
{
public:
    virtual ~CheatDialog();

    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager) override;
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager) override;
    virtual void Draw(Sexy::Graphics *theGraphics) override;
    virtual void Resize(int theX, int theY, int theWidth, int theHeight) override;
    virtual void GotFocus() override;

    void EditWidgetText(int theId, const Sexy::String &theText) override;
    int AllowChar(int theId, char theChar) override;
    CheatDialog(LawnApp *theApp);
    int ApplyCheat();
    int GetPreferredHeight(int theWidth) override;

    LawnApp *mApp;
    Sexy::EditWidget *mEditWidget;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_CHEATDIALOG_H
