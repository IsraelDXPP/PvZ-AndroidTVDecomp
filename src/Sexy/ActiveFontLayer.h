// ActiveFontLayer is a live font layer while ImageFont is drawing.
//
// It owns a vtable, so the destructor is virtual. It has no base class.
// PushColor and PopColor are ordinary methods.

#pragma once

#include "ReconTypes.h"
#include "SexyAppBase.h"

#include <map>
#include <vector>

namespace Sexy {

class FontLayer;

class ActiveFontLayer
{
public:
    virtual ~ActiveFontLayer() = default;

    FontLayer* mLayer;
    Sexy::Image* mImage;
    bool mOwnsImage;
    std::map<int, Sexy::TRect<int>> mCharRects;
    std::vector<Sexy::Color> mColorStack;

    ActiveFontLayer();
    int PopColor();
    void PushColor(Color const& theColor);
    ActiveFontLayer(Sexy::ActiveFontLayer const& theOther);
};

} // namespace Sexy

using Sexy::ActiveFontLayer;
