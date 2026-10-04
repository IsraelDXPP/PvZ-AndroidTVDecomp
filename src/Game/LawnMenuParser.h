// LawnMenuParser -- reconstructed from libGameMain.so.
//
// Inheritance chain: this class derives from the descriptor-parser chain. The
// binary's primary vtable interleaves Sexy::EncodingParser and Sexy::MenuParser
// entries before LawnMenuParser's own:
//   [  2] 0x3bc4d0  Sexy::EncodingParser::SetEncodingType
//   [  3] 0x3bc8e0  Sexy::EncodingParser::OpenFile
//   [  4] 0x2af130  Sexy::MenuParser::CloseFile        <-- MenuParser overrides it here
//   [  5] 0x3bca00  Sexy::EncodingParser::EndOfFile
//   [  6] 0x3bcddc  Sexy::EncodingParser::SetStringSource
//   [  7] 0x3bca24  Sexy::EncodingParser::GetChar
//   [  8] 0x3bcc34  Sexy::EncodingParser::PutChar
//   [  9] 0x3bce30  Sexy::EncodingParser::PutString
//   [ 10] 0x2fffac  Sexy::DescParser::Error
//   [ 11] 0x300858  Sexy::DescParser::Dereference
//   [ 12] 0x149b78  LawnMenuParser::HandleCommand       (overrides DescParser's pure slot)
//   [ 13] 0x2ff9e4  Sexy::DescParser::LoadDescriptor
//   [ 14] 0x1490f8  LawnMenuParser::CreateParseWidget
//   [ 15] 0x149218  LawnMenuParser::CreateWidget
//   [ 16] 0x149b78  LawnMenuParser::CreateWidgetAnimator
// so the chain is EncodingParser -> MenuParser -> DescParser -> LawnMenuParser.
// This tree's Sexy\DescParser.h still derives DescParser straight from
// EncodingParser (its own file, another group's), so only `DescParser` is named
// as the primary base here.
//
// The secondary base IS identified now: the ZTV's second table (0x67f874, the
// one whose offset-to-top is -0x48) holds exactly TWO slots and its typeinfo is
// Sexy::RefCount's, i.e. the sub-object at +0x48 is a `Sexy::RefCount` --
// vptr at +0x48 and mRefCount at +0x4C. That also puts this class's own first
// member at +0x50, which is where `mUnk50` already is, and it is why
// LawnMenuWidget::CreateMenuParser releases a parser by decrementing +0x4C and
// then deleting it through the +0x48 sub-object's vtable slot 1.
// DescParser is 0x48 bytes (Sexy\DescParser.h's own closing comment), so the
// two bases tile exactly.
//
// MSVC x86 has ONE virtual-destructor slot where Itanium/ARM has D1+D0; only the
// declaration order matters.
//
// Size 0x68, from the constructor call site (`an allocation of 0x68u` followed by
// LawnMenuParser::LawnMenuParser(Sexy::MenuWidget*) @0x149190), which matches the
// recorder's figure.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "ButtonListener.h"
#include "DescParser.h"
#include "GameButton.h"
#include "HelpBarWidget.h"
#include "HelpTextImageWidget.h"
#include "HelpTextWidget.h"
#include "MenuParser.h"
#include "ReanimTrackWidgetAnimator.h"
#include "RefCount.h"
#include "ResourceManager.h"
#include "SharedImageRef.h"
#include "SlideInOutWidgetAnimator.h"
#include "TodCurveWidgetAnimator.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// Only a pointer to it is stored (`mLawnMenuWidget`), so a declaration is enough
// and it keeps this header out of LawnMenuWidget.h's cycle.
class LawnMenuWidget;
class LawnParseWidget;

class LawnMenuParser : public Sexy::DescParser, public Sexy::RefCount
{
public:
    virtual ~LawnMenuParser();                        // [  0],[1] 0x000/0x004
    virtual bool HandleCommand(Sexy::ListDataElement const& theParams); // [ 12] 0x030 (DescParser declares it pure)
    virtual LawnParseWidget * CreateParseWidget();             // [ 14] 0x038
    virtual Sexy::Widget * CreateWidget(Sexy::String theType, int theId, Sexy::MenuParser_ParseWidget *theParse); // [15] 0x03C
    virtual Sexy::WidgetAnimator * CreateWidgetAnimator(const char **theType, Sexy::ListDataElement *theParams, Sexy::MenuParser_ParseWidget *theParse); // [16] 0x040

    // DescParser is 0x48 bytes and the polymorphic secondary sub-object sits at
    // +0x48, so this class's own members start at +0x50. +0x50..+0x63 reproduce
    // Sexy::MenuParser's own area, and the authoritative layout names them the
    // way Game\MenuParser.h does: the MenuWidget the parser belongs to, the
    // ref-counted widget the current command is building, and the pending
    // focus-link vector. Declaring them is what makes this class the 0x68 bytes
    // the binary allocates for it.
    Sexy::MenuWidget *mMenuWidget;              // +0x50, parent of every button it makes
    Sexy::MenuParser_ParseWidget *mParseWidget; // +0x54, the widget being built
    int mWidgetLinks[3];                        // +0x58, MenuParser's vector header
    // +0x64 is LawnMenuParser's own copy of the same MenuWidget, typed the way
    // ReanimTrackWidgetAnimator's constructor wants it
    // (_ZN25ReanimTrackWidgetAnimatorC1EPN4Sexy6WidgetEP14LawnMenuWidgetSsSsSs).
    LawnMenuWidget *mLawnMenuWidget;            // +0x64
    // size 0x68

    // The remaining methods. None of these owns a vtable slot.
    LawnMenuParser(Sexy::MenuWidget *theMenu);
    int DataToVector2(Sexy::DataElement *theElement, float *theOut);
};
