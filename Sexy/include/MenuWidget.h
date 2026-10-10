// MenuWidget is a full-screen widget that loads a menu file and fades
// between screens. It also listens for buttons. Widget occupies the first
// base; the button listener is the second, so this class's own members
// start after that listener's vptr.
//
// Draw, update, focus and the manager hooks replace Widget's. UpdateExit,
// Enter, Exit, RetainWidgetsOnExit, OnExit and CreateMenuParser are this
// class's own. A game menu replaces those.
//
// Members: the fade flag, the fade-finished flag Update reads, the parser,
// the source file, the widget id the constructor stores as -1, and the
// background image. mExitFinished is set once the fade-out has run.
// mInitialFocusId is the widget SetInitialFocus names, or -1.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_MENUWIDGET_H
#define LAWNPROJECTTV_SEXY_INCLUDE_MENUWIDGET_H

#include "ReconTypes.h"
#include "ButtonListener.h"
#include "Graphics.h"
#include "MenuParser.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class MenuWidget : public Widget, public ButtonListener
{
public:
    virtual ~MenuWidget();

    virtual void AddWidget(Sexy::Widget *theWidget);
    virtual void AddedToManager(Sexy::WidgetManager *theManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void DrawAll(Sexy::ModalFlags *theFlags, Sexy::Graphics *theGraphics);
    virtual void GotFocus();
    virtual bool UpdateExit();
    virtual void Enter();
    virtual void Exit();
    virtual bool RetainWidgetsOnExit();
    virtual void OnExit();
    virtual int CreateMenuParser();

    bool mIsFading;
    bool mExitFinished;
    MenuParser *mMenuParser;
    Sexy::String mSourceFileName;
    int mInitialFocusId;
    Image *mImage;

    MenuWidget();
    MenuWidget(Sexy::String theFileName);
    int LoadMenuFile(Sexy::String theFileName);
};

} // namespace Sexy

using Sexy::MenuWidget;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_MENUWIDGET_H
