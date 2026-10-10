// ActiveFontLayer. PopColor drops the top of the color stack. PushColor
// multiplies that top by the new color, or stores the color alone when
// the stack is empty. A copied layer that owns its image asks the app
// for a private copy.

#include "ActiveFontLayer.h"
#include "FontData.h"
#include "FontLayer.h"
#include "Image.h"
#include "SexyAppBase.h"

Sexy::ActiveFontLayer::ActiveFontLayer() :
    mLayer(nullptr),
    mImage(nullptr),
    mOwnsImage(false)
{
}

int Sexy::ActiveFontLayer::PopColor()
{
    if (!mColorStack.empty())
        mColorStack.pop_back();
    return 0;
}

void Sexy::ActiveFontLayer::PushColor(Color const &theColor)
{
    if (mColorStack.empty())
    {
        mColorStack.push_back(theColor);
        return;
    }

    const Color &aTop = mColorStack.back();
    mColorStack.push_back(Color(aTop.mRed * theColor.mRed / 255,
        aTop.mGreen * theColor.mGreen / 255,
        aTop.mBlue * theColor.mBlue / 255,
        aTop.mAlpha * theColor.mAlpha / 255));
}

Sexy::ActiveFontLayer::ActiveFontLayer(Sexy::ActiveFontLayer const &theOther) :
    mLayer(theOther.mLayer),
    mImage(theOther.mImage),
    mOwnsImage(theOther.mOwnsImage),
    mCharRects(theOther.mCharRects),
    mColorStack(theOther.mColorStack)
{
    if (mOwnsImage && mLayer != nullptr && mLayer->mFontData != nullptr && mLayer->mFontData->mApp != nullptr)
        mImage = mLayer->mFontData->mApp->CopyImage(mImage);
}
