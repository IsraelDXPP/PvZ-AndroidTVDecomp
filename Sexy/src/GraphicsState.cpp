// GraphicsState copies one drawing state onto another.

#include "GraphicsState.h"

void Sexy::GraphicsState::CopyStateFrom(Sexy::GraphicsState const *theSource)
{
    if (theSource == nullptr)
        return;

    mDestImage = theSource->mDestImage;
    mTransX = theSource->mTransX;
    mTransY = theSource->mTransY;
    mScaleX = theSource->mScaleX;
    mScaleY = theSource->mScaleY;
    mScaleOrigX = theSource->mScaleOrigX;
    mScaleOrigY = theSource->mScaleOrigY;
    mClipRect = theSource->mClipRect;
    mColorStack = theSource->mColorStack;
    mFinalColor = theSource->mFinalColor;
    mColor = theSource->mColor;
    mFont = theSource->mFont;
    mDrawMode = theSource->mDrawMode;
    mColorizeImages = theSource->mColorizeImages;
    mFastStretch = theSource->mFastStretch;
    mWriteColoredString = theSource->mWriteColoredString;
    mLinearBlend = theSource->mLinearBlend;
    mIs3D = theSource->mIs3D;
    mApplyGlobalScale = theSource->mApplyGlobalScale;
    mTrackDeviceState = theSource->mTrackDeviceState;
}
