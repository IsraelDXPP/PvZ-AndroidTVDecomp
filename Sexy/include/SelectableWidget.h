// SelectableWidget pulses a highlight colour while it is selected.
//
// It is a Widget. The vtable replaces the destructor and Update, then appends
// SetSelectColor, GetSelectAlpha and GetSelectColor. The highlight colour sits
// at +0x100 and the four pulse floats follow it.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SELECTABLEWIDGET_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SELECTABLEWIDGET_H

#include "ReconTypes.h"
#include "Widget.h"

namespace Sexy
{

class SelectableWidget : public Widget
{
public:
    virtual ~SelectableWidget();
    virtual void Update();

    virtual int SetSelectColor(Color const &theColor);
    virtual int GetSelectAlpha();
    virtual Sexy::Color GetSelectColor();

    Color mSelectColor; // +0x100
    float mSelectAlpha; // +0x110
    float mSelectAlphaMin; // +0x114
    float mSelectAlphaMax; // +0x118
    float mSelectAlphaDelta; // +0x11C

    SelectableWidget();
    int SetSelectAlphaRange(float theMinAlpha, float theMaxAlpha);
};

} // namespace Sexy

using Sexy::SelectableWidget;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SELECTABLEWIDGET_H
