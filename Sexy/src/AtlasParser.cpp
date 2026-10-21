// Load opens the atlas file, walks to the atlases element, and records each
// atlas. A path already stored is rejected. Each image is stored under the
// base directory plus its lower-case name, pointing at the atlas file and
// the rectangle cut from it.

#include "AtlasParser.h"

#include "PixelUtil.h"
#include "ReconHelpers.h"
#include "SDLFile.h"
#include "XMLElement.h"

#include <cstdio>
#include <string>

namespace
{

int AttributeInt(Sexy::XMLElement &theElement, const char *theName, int &theOut)
{
    if (!theElement.HasAttribute(Sexy::String(theName)))
        return 0;
    return StringToInt(theElement.GetAttribute(Sexy::String(theName)), &theOut);
}

} // namespace

Sexy::AtlasParser::AtlasParser() = default;

Sexy::AtlasParser::~AtlasParser() = default;

Sexy::Atlas *Sexy::AtlasParser::Find(const Sexy::String &theName)
{
    std::map<Sexy::String, Atlas>::iterator aFound = mAtlases.find(theName);
    if (aFound == mAtlases.end())
        return nullptr;
    return &aFound->second;
}

int Sexy::AtlasParser::GetPixelFormat(const Sexy::String &theName)
{
    std::map<Sexy::String, int>::iterator aFound = mFormats.find(theName);
    if (aFound == mFormats.end())
        return 0;
    return aFound->second;
}

int Sexy::AtlasParser::ParseAtlas(Sexy::XMLParser &theParser, Sexy::XMLElement &theElement)
{
    if (!theElement.HasAttribute(Sexy::String("base")) || !theElement.HasAttribute(Sexy::String("path")))
        return 0;

    Sexy::String aBase = theElement.GetAttribute(Sexy::String("base"));
    if (!aBase.empty() && aBase[aBase.size() - 1] != '/')
        aBase.push_back('/');

    Sexy::String aPath = theElement.GetAttribute(Sexy::String("path"));
    Sexy::String aAtlasPath = aBase + aPath;
    if (mFormats.find(aAtlasPath) != mFormats.end())
        return 0;

    int aFormat = 0;
    if (theElement.HasAttribute(Sexy::String("format")))
    {
        Sexy::String aFormatName = theElement.GetAttribute(Sexy::String("format"));
        if (!aFormatName.empty())
        {
            aFormat = Sexy::PixelUtil::GetPixelFormat(aFormatName);
        }
    }
    mFormats[aAtlasPath] = aFormat;

    for (;;)
    {
        Sexy::XMLElement aImage;
        int aNext = theParser.NextElement(&aImage);
        if (aNext == 0)
            return 0;
        if (aImage.mType == 2)
            return aNext;
        if (aImage.mType != 1 || aImage.mInstruction != "image")
            continue;

        int aX = 0;
        int aY = 0;
        int aWidth = 0;
        int aHeight = 0;
        if (AttributeInt(aImage, "x", aX) == 0 || AttributeInt(aImage, "y", aY) == 0 ||
            AttributeInt(aImage, "w", aWidth) == 0 || AttributeInt(aImage, "h", aHeight) == 0)
            return 0;

        if (!aImage.HasAttribute(Sexy::String("name")))
            return 0;

        Sexy::String aName = aImage.GetAttribute(Sexy::String("name"));
        Sexy::String aLower;
        StringToLower(&aLower, aName.c_str());

        Atlas aRecord;
        aRecord.mPath = aAtlasPath;
        aRecord.mX = aX;
        aRecord.mY = aY;
        aRecord.mWidth = aWidth;
        aRecord.mHeight = aHeight;

        Sexy::String aStored = aBase + aLower;
        mAtlases[aStored] = aRecord;

        Sexy::XMLElement aEnd;
        if (theParser.NextElement(&aEnd) == 0 || aEnd.mType != 2)
            return 0;
    }
}

int Sexy::AtlasParser::ParseAtlases(Sexy::XMLParser &theParser, Sexy::XMLElement &)
{
    for (;;)
    {
        Sexy::XMLElement aElement;
        int aNext = theParser.NextElement(&aElement);
        if (aNext == 0)
            return 0;
        if (aElement.mType == 1)
        {
            if (aElement.mInstruction == "atlas" && ParseAtlas(theParser, aElement) == 0)
                return 0;
        }
        else if (aElement.mType == 2 && aElement.mInstruction == "atlases")
        {
            return aNext;
        }
    }
}

int Sexy::AtlasParser::Load(const Sexy::String &thePath)
{
    Sexy::String aFile(Sexy::SDLFile::ResolvePath(thePath.c_str()).c_str());
    Sexy::XMLParser aParser;
    if (aParser.OpenFile(aFile) == 0)
    {
        std::fprintf(stderr, "atlas: cannot open %s\n", aFile.c_str());
        return 0;
    }

    while (aParser.HasFailed() == 0)
    {
        Sexy::XMLElement aElement;
        int aNext = aParser.NextElement(&aElement);
        if (aNext == 0)
            break;
        if (aElement.mType != 1 || aElement.mInstruction != "atlases")
            continue;

        int aResult = ParseAtlases(aParser, aElement);
        if (aResult != 0)
        {
            std::fprintf(stderr, "atlas: %s loaded (%u images mapped in total)\n", thePath.c_str(),
                (unsigned)mAtlases.size());
            return aResult;
        }
    }

    Sexy::String aError = aParser.GetErrorText();
    // logtfe is a no-op in this port; a broken atlas map would otherwise make
    // every image it lists fail later with no hint of the cause.
    std::fprintf(stderr,
        "atlas: failed to parse %s: %s at line %d (%u images mapped in total)\n",
        thePath.c_str(),
        aError.c_str(),
        aParser.GetCurrentLineNum(),
        (unsigned)mAtlases.size());
    return 0;
}
