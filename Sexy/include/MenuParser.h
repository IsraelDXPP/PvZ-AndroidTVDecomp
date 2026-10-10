// MenuParser reads a menu descriptor and builds the widgets it names. It
// extends DescParser and is reference-counted, so the second base is the
// RefCount sub-object. CloseFile applies the focus links collected while
// the file was read. HandleCommand is the descriptor command. CreateWidget
// and CreateWidgetAnimator are the hooks a game menu replaces.
//
// After the two bases: the menu being filled, the parse widget the current
// command is building, and the pending focus links. Each link is the widget
// plus the four focus ids CloseFile resolves.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_MENUPARSER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_MENUPARSER_H

#include "ReconTypes.h"
#include "ButtonWidget.h"
#include "DescParser.h"
#include "ImageWidget.h"
#include "LabelWidget.h"
#include "ListDataElement.h"
#include "RefCount.h"
#include "Widget.h"
#include <vector>

class GameButton;
class HelpBarWidget;
class HelpTextImageWidget;
class HelpTextWidget;

namespace Sexy
{

class Color;
class MenuWidget;
class Widget;
class WidgetAnimator;

// The holder a command builds its widget in. CreateWidget fills the typed
// slots so later commands can tell a label from a button without a second
// lookup. LawnMenuParser::LawnParseWidget extends this and adds the help-bar
// slots the lawn menus write at +0x18..+0x24.
class MenuParser_ParseWidget : public RefCount
{
public:
    MenuParser_ParseWidget();
    ~MenuParser_ParseWidget() override = default;

    Widget *mWidget;
    Widget *mReservedWidget;
    LabelWidget *mLabelWidget;
    ImageWidget *mImageWidget;
    ButtonWidget *mButtonWidget;
    HelpBarWidget *mHelpBar;
    HelpTextImageWidget *mHelpImage;
    HelpTextWidget *mHelpText;
};

class MenuParser : public DescParser, public Sexy::RefCount
{
public:
    virtual ~MenuParser();

    virtual int CloseFile();
    virtual bool HandleCommand(Sexy::ListDataElement const &theParams);
    virtual Sexy::MenuParser_ParseWidget *CreateParseWidget();
    virtual Sexy::Widget *CreateWidget(Sexy::String theType, int theId, Sexy::MenuParser_ParseWidget *theParse);
    virtual Sexy::WidgetAnimator *CreateWidgetAnimator(
        Sexy::String theType, Sexy::ListDataElement const &theParams, Sexy::MenuParser_ParseWidget *theParse);

    MenuWidget *mMenuWidget;
    MenuParser_ParseWidget *mParseWidget;
    struct WidgetLinks
    {
        Sexy::Widget *mWidget;
        int mUpId;
        int mDownId;
        int mLeftId;
        int mRightId;
    };
    std::vector<WidgetLinks> mWidgetLinks;

    MenuParser(Sexy::MenuWidget *theMenu);
    int Error(Sexy::String const &theError, Sexy::String theDetail);
    Sexy::Widget *FindWidget(int theId);
    int DataToColor(Sexy::DataElement *theElement, Color &theColor);
};

} // namespace Sexy

using Sexy::MenuParser;
using Sexy::MenuParser_ParseWidget;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_MENUPARSER_H
