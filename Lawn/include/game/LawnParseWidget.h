// LawnMenuParser::LawnParseWidget -- reconstructed from libGameMain.so.
//
// The binary nests this class inside LawnMenuParser (symbols:
// _ZTVN14LawnMenuParser15LawnParseWidgetE @0x67f800, and its destructor pair at
// 0x149080 / 0x1490d8). This translation unit declares it at global scope, which
// is how the transcribed bodies spell it.
//
// Its vtable has exactly two slots, both destructors:
//   [  0] 0x149080  LawnMenuParser::LawnParseWidget::~LawnParseWidget
//   [  1] 0x1490d8  LawnMenuParser::LawnParseWidget::~LawnParseWidget
// The authoritative project derives it from `Sexy::MenuParser::ParseWidget`
// (a nested type of PvZ\SexyAppFramework\Widget\MenuParser.h), which this tree
// does not expose; the secondary vtable that follows it is labelled
// `typeinfo for'Sexy::MenuParser`, so ParseWidget contributes no virtual slots of
// its own.
// TODO(recon): derive from Sexy::MenuParser::ParseWidget once that class is
// declared in this tree.
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0.
//
// Size 0x8, from the binary's allocation sites: one vptr plus one four-byte
// member.
// TODO(recon): name the word at +0x04.

#pragma once

#include "ReconTypes.h"

class GameButton;
class HelpBarWidget;
class HelpTextWidget;
class HelpTextImageWidget;
namespace Sexy { class Widget; }

// The parser fills one of these per command. The command writes the widget it
// hands back, the button, the help bar, the image widget and the text widget.
// The binary's allocation is 0x28; this tree names the slots the commands use.
class LawnParseWidget
{
public:
    virtual ~LawnParseWidget();

    Sexy::Widget *mWidget;
    GameButton *mButton;
    HelpBarWidget *mHelpBar;
    HelpTextImageWidget *mImage;
    HelpTextWidget *mText;
};

