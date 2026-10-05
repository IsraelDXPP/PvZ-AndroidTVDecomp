// HyperlinkWidget is a button that draws its label as a link. It adds a
// resting colour, a hover colour, and the thickness and offset of the
// underline. Draw paints the link; MouseEnter and MouseLeave swap the
// colours.

#pragma once

#include "ReconTypes.h"
#include "ButtonWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "TextLayout.h"

namespace Sexy {

class HyperlinkWidget : public ButtonWidget
{
public:
    virtual ~HyperlinkWidget();

    virtual void Draw(Sexy::Graphics * theGraphics);
    virtual void MouseEnter();
    virtual void MouseLeave();

    Sexy::Color mColor;
    Sexy::Color mOverColor;
    int         mUnderlineSize;
    int         mUnderlineOffset;

    HyperlinkWidget(int theId, Sexy::ButtonListener * theListener);
};

} // namespace Sexy

using Sexy::HyperlinkWidget;
