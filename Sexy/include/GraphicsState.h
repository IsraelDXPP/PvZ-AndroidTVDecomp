// GraphicsState is the plain drawing state Graphics embeds and copies.
// There is no vtable. CopyStateFrom copies every field a later draw reads.

#pragma once

#include "ReconTypes.h"
#include <vector>

namespace Sexy {

class GraphicsState
{
public:
    Sexy::Image *mDestImage;
    float mTransX;
    float mTransY;
    float mScaleX;
    float mScaleY;
    float mScaleOrigX;
    float mScaleOrigY;
    Sexy::TRect<int> mClipRect;
    std::vector<Sexy::Color> mColorStack;
    // The colour after the multiply stack has been folded in. Drawers use it
    // while the stack is not empty and mColor while it is.
    Sexy::Color mFinalColor;
    Sexy::Color mColor;
    Sexy::Font *mFont;
    int mDrawMode;
    bool mColorizeImages;
    bool mFastStretch;
    bool mWriteColoredString;
    bool mLinearBlend;
    bool mIs3D;
    // Apply mScaleX / mScaleY around mScaleOrigX / mScaleOrigY on the way out.
    bool mApplyGlobalScale;
    // PushState / PopState also push and pop the destination device's state.
    bool mTrackDeviceState;

    void CopyStateFrom(Sexy::GraphicsState const* theSource);
};

} // namespace Sexy

using Sexy::GraphicsState;
