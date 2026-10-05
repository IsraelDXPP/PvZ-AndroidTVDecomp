// LabelWidget draws one string with one font. It adds the font, the text
// and the justification to Widget, and replaces Draw. A justification of 1
// is the centred case Draw tests.

#pragma once

#include "ReconTypes.h"
#include "Font.h"
#include "Graphics.h"
#include "Widget.h"

namespace Sexy {

class LabelWidget : public Widget
{
public:
    Font * mFont;
    Sexy::String mString;
    int mJustification;

    virtual ~LabelWidget();
    virtual void Draw(Sexy::Graphics * theGraphics);

    LabelWidget();
};

} // namespace Sexy

using Sexy::LabelWidget;
