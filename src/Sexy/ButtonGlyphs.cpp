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
    { "<LT>", 4, 8 },
    { "<RT>", 4, 7 },
    { "<LB>", 4, 6 },
    { "<RB>", 4, 5 },
    { "<A>", 3, 0 },
    { "<B>", 3, 1 },
    { "<X>", 3, 2 },
    { "<Y>", 3, 3 },
    { "<S>", 3, 4 },
    { "<L>", 3, 10 },
    { "<R>", 3, 11 },
    { "<D>", 3, 9 },
    { "<C>", 3, 4 },
    { "<b>", 3, 4 },
    { "<l>", 3, 10 },
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

// setup.env LAWN_BUTTON_MAP, written as ["A":"A","A":"ENTER","B":"S"].
// The same button may appear more than once. The first value is the caption.
struct KeyBinding
{
    const char* mButton;
    const char* mKey;
};

const KeyBinding kDefaultKeys[] = {
    { "A", "A" },
    { "A", "ENTER" },
    { "B", "S" },
    { "X", "X" },
    { "Y", "W" },
    { "S", "RETURN" },
    { "C", "RETURN" },
    { "L", "D" },
    { "R", "F" },
    { "LT", "E" },
    { "RT", "C" },
    { "LB", "D" },
    { "RB", "F" },
};

bool gMapLoaded = false;
char gMapText[512];
char gMapKeys[16][16];
const char* gMapButtons[16];
int gMapCount = 0;

void SkipMapSpace(const char*& theCursor)
{
    while (*theCursor == ' ' || *theCursor == '\t' || *theCursor == '\r' || *theCursor == '\n')
        ++theCursor;
}

bool ReadMapToken(const char*& theCursor, char* theOut, int theOutSize)
{
    SkipMapSpace(theCursor);
    if (*theCursor != '"')
        return false;
    ++theCursor;
    int aLength = 0;
    while (*theCursor != '\0' && *theCursor != '"')
    {
        if (aLength + 1 < theOutSize)
            theOut[aLength++] = *theCursor;
        ++theCursor;
    }
    if (*theCursor == '"')
        ++theCursor;
    theOut[aLength] = '\0';
    return aLength > 0;
}

void LoadButtonMap()
{
    if (gMapLoaded)
        return;
    gMapLoaded = true;
    gMapCount = 0;

    const char* aEnv = Sexy::GetEnv("LAWN_BUTTON_MAP");
    if (aEnv == nullptr)
        aEnv = "[\"A\":\"A\",\"A\":\"ENTER\",\"B\":\"S\",\"X\":\"X\",\"Y\":\"W\"]";
    std::strncpy(gMapText, aEnv, sizeof(gMapText) - 1);
    gMapText[sizeof(gMapText) - 1] = '\0';

    const char* aCursor = gMapText;
    SkipMapSpace(aCursor);
    if (*aCursor == '[')
        ++aCursor;
    while (*aCursor != '\0' && *aCursor != ']' && gMapCount < 16)
    {
        SkipMapSpace(aCursor);
        if (*aCursor == ',' || *aCursor == ':')
        {
            ++aCursor;
            continue;
        }
        char aButton[16];
        char aKey[16];
        if (!ReadMapToken(aCursor, aButton, (int)sizeof(aButton)))
            break;
        SkipMapSpace(aCursor);
        if (*aCursor == ':')
            ++aCursor;
        if (!ReadMapToken(aCursor, aKey, (int)sizeof(aKey)))
            break;
        std::strncpy(gMapKeys[gMapCount], aKey, sizeof(gMapKeys[0]) - 1);
        gMapKeys[gMapCount][sizeof(gMapKeys[0]) - 1] = '\0';
        gMapButtons[gMapCount] = nullptr;
        for (int aIndex = 0; aIndex < (int)(sizeof(kButtonTags) / sizeof(kButtonTags[0])); ++aIndex)
        {
            const char* aName = kButtonTags[aIndex].mName + 1;
            int aNameLength = kButtonTags[aIndex].mLength - 2;
            if (aNameLength == (int)std::strlen(aButton) && std::memcmp(aName, aButton, (size_t)aNameLength) == 0)
            {
                gMapButtons[gMapCount] = kButtonTags[aIndex].mName;
                break;
            }
        }
        ++gMapCount;
    }
}

int KeysForButton(const char* theTag, const char** theKeys, int theMax)
{
    LoadButtonMap();
    if (theTag == nullptr || theTag[0] != '<' || theKeys == nullptr || theMax <= 0)
        return 0;
    int aLength = 0;
    while (theTag[aLength] != '\0' && theTag[aLength] != '>')
        ++aLength;
    if (theTag[aLength] != '>')
        return 0;

    int aCount = 0;
    if (gMapCount > 0)
    {
        for (int aIndex = 0; aIndex < gMapCount && aCount < theMax; ++aIndex)
        {
            if (gMapButtons[aIndex] != nullptr
                && std::memcmp(gMapButtons[aIndex], theTag, (size_t)(aLength + 1)) == 0
                && gMapButtons[aIndex][aLength + 1] == '\0')
                theKeys[aCount++] = gMapKeys[aIndex];
        }
        return aCount;
    }
    for (int aIndex = 0; aIndex < (int)(sizeof(kDefaultKeys) / sizeof(kDefaultKeys[0])) && aCount < theMax; ++aIndex)
    {
        int aNameLength = (int)std::strlen(kDefaultKeys[aIndex].mButton);
        if (aNameLength == aLength - 1
            && std::memcmp(theTag + 1, kDefaultKeys[aIndex].mButton, (size_t)aNameLength) == 0)
            theKeys[aCount++] = kDefaultKeys[aIndex].mKey;
    }
    return aCount;
}

int KeyCodeFromName(const char* theName)
{
    if (theName == nullptr || theName[0] == '\0')
        return 0;
    if (std::strcmp(theName, "RETURN") == 0 || std::strcmp(theName, "ENTER") == 0)
        return 13;
    if (std::strcmp(theName, "ESCAPE") == 0 || std::strcmp(theName, "ESC") == 0)
        return 27;
    if (std::strcmp(theName, "SPACE") == 0)
        return 32;
    if (theName[1] == '\0')
    {
        char aLetter = theName[0];
        if (aLetter >= 'a' && aLetter <= 'z')
            aLetter = (char)(aLetter - 'a' + 'A');
        return (unsigned char)aLetter;
    }
    return 0;
}

} // namespace

bool ButtonGlyphsActive()
{
    if (gSexyAppBase == nullptr)
        return false;
    if (Sexy::GetEnvOption("LAWN_GAMEPAD_MODE", false) == 0)
        return false;
    // KeyboardGamepad also reports a stick, so HasGamepad is not enough.
    // An icon needs a live XInput or DirectInput pad.
    Sexy::GamepadApp* aApp = static_cast<Sexy::GamepadApp*>(gSexyAppBase);
    if (aApp->mInputManager == nullptr)
        return false;
    for (Sexy::IInputDriver* aDriver : aApp->mInputManager->Drivers())
    {
        if (aDriver == nullptr || aDriver->mOwner == nullptr)
            continue;
        if (aDriver->mOwner->mName != "Win32Gamepad")
            continue;
        if (aDriver->GetNumDevices() > 0)
            return true;
    }
    return false;
}

Sexy::String ButtonKeyCaption(const char* theTag, int theLength)
{
    const ButtonTag* aTag = MatchTag(theTag, theLength);
    if (aTag == nullptr)
        return Sexy::String();
    const char* aKeys[8];
    int aCount = KeysForButton(aTag->mName, aKeys, 8);
    if (aCount == 0)
        return Sexy::String();
    return Sexy::String(aKeys[0]);
}

int ButtonKeyCaptions(const char* theTag, int theLength, const char** theKeys, int theMax)
{
    const ButtonTag* aTag = MatchTag(theTag, theLength);
    if (aTag == nullptr)
        return 0;
    return KeysForButton(aTag->mName, theKeys, theMax);
}

bool ButtonKeyMatches(const char* theButton, int theKey)
{
    char aTag[8];
    aTag[0] = '<';
    int aLength = 1;
    if (theButton != nullptr)
    {
        while (theButton[aLength - 1] != '\0' && aLength < 6)
        {
            aTag[aLength] = theButton[aLength - 1];
            ++aLength;
        }
    }
    aTag[aLength++] = '>';
    aTag[aLength] = '\0';
    const char* aKeys[8];
    int aCount = KeysForButton(aTag, aKeys, 8);
    for (int aIndex = 0; aIndex < aCount; ++aIndex)
    {
        int aCode = KeyCodeFromName(aKeys[aIndex]);
        if (aCode == 0)
            continue;
        if (theKey == aCode)
            return true;
        if (aCode >= 'A' && aCode <= 'Z' && theKey == aCode + 32)
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
