#include "ButtonGlyphs.h"

#include "Font.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "IInputDriver.h"
#include "Image.h"
#include "InputDriver.h"
#include "InputManager.h"
#include "ReconFreeFunctions.h"
#include "SexyAppBase.h"

#include <cstring>

namespace {

struct ButtonTag
{
    const char* mName;
    int mLength;
    int mCel;
};

// Longer names first so <LT> is not read as <L>.
const ButtonTag kButtonTags[] = {
    { "<LT>", 4, 9 },
    { "<RT>", 4, 8 },
    { "<LB>", 4, 6 },
    { "<RB>", 4, 5 },
    { "<A>", 3, 0 },
    { "<B>", 3, 1 },
    { "<X>", 3, 2 },
    { "<Y>", 3, 3 },
    { "<S>", 3, 4 },
    { "<L>", 3, 6 },
    { "<R>", 3, 5 },
    { "<D>", 3, 7 },
    { "<C>", 3, 4 },
    { "<b>", 3, 10 },
    { "<l>", 3, 12 },
    { "<r>", 3, 11 },
};

const ButtonTag* MatchTag(const char* theTag, int theLength)
{
    for (int aIndex = 0; aIndex < (int)(sizeof(kButtonTags) / sizeof(kButtonTags[0])); ++aIndex)
    {
        const ButtonTag& aTag = kButtonTags[aIndex];
        if (aTag.mLength == theLength && std::memcmp(theTag, aTag.mName, (size_t)theLength) == 0)
            return &aTag;
    }
    return nullptr;
}

} // namespace

bool ButtonGlyphsActive()
{
    if (gSexyAppBase == nullptr)
        return false;
    if (Sexy::GetEnvOption("LAWN_GAMEPAD_MODE", false) == 0)
        return false;
    // An icon needs a live SDL pad. A keyboard does not count.
    Sexy::GamepadApp* aApp = static_cast<Sexy::GamepadApp*>(gSexyAppBase);
    if (aApp->mInputManager == nullptr)
        return false;
    for (Sexy::IInputDriver* aDriver : aApp->mInputManager->Drivers())
    {
        if (aDriver == nullptr || aDriver->mOwner == nullptr)
            continue;
        if (aDriver->mOwner->mName != "SDLGamepad")
            continue;
        if (aDriver->GetNumDevices() > 0)
            return true;
    }
    return false;
}

int ButtonGlyphCel(const char* theTag, int theLength)
{
    const ButtonTag* aTag = MatchTag(theTag, theLength);
    if (aTag == nullptr)
        return -1;
    return aTag->mCel;
}

Sexy::String ButtonTagAsSpaces(Sexy::Font* theFont, const char* theTag, int theLength)
{
    Sexy::String aTag(theTag, (unsigned int)theLength);
    if (theFont == nullptr)
        return Sexy::String("   ");

    int aTagWidth = theFont->StringWidth(aTag);
    int aSpaceWidth = theFont->StringWidth(Sexy::String(" "));
    if (aSpaceWidth <= 0)
        aSpaceWidth = 1;
    int aCount = aTagWidth / aSpaceWidth;
    if (aCount < 1)
        aCount = 1;
    Sexy::String aSpaces;
    for (int aIndex = 0; aIndex < aCount; ++aIndex)
        aSpaces.push_back(' ');
    return aSpaces;
}

void DrawButtonGlyph(Sexy::Graphics* theGraphics, int theX, int theY, Sexy::Font* theFont,
                     int theCel, int theSlotWidth)
{
    if (theGraphics == nullptr || IMAGE_HELP_BUTTONS == nullptr || theCel < 0)
        return;

    int aCelWidth = IMAGE_HELP_BUTTONS->GetCelWidth();
    int aCelHeight = IMAGE_HELP_BUTTONS->GetCelHeight();
    int aHeight = theFont != nullptr ? theFont->GetHeight() : aCelHeight;
    int aAscent = theFont != nullptr ? theFont->GetAscent() : aCelHeight;
    // theY is the baseline. Center the disc on the font box, then center it
    // in the spaces the tag was turned into.
    int aTop = theY - aAscent + (aHeight - aCelHeight) / 2;
    int aLeft = theX + (theSlotWidth - aCelWidth) / 2;
    theGraphics->DrawImageCel(IMAGE_HELP_BUTTONS, aLeft, aTop, theCel % IMAGE_HELP_BUTTONS->mNumCols, 0);
}
