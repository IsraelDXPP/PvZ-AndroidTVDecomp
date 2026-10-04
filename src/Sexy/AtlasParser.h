// AtlasParser reads an atlas XML file into two maps: image name to rectangle,
// and atlas path to pixel format.

#pragma once

#include "XMLParser.h"

#include <map>

namespace Sexy {

struct Atlas
{
    Sexy::String mPath;
    int mX;
    int mY;
    int mWidth;
    int mHeight;
};

class AtlasParser
{
public:
    AtlasParser();
    ~AtlasParser();

    Sexy::Atlas* Find(const Sexy::String& theName);
    int GetPixelFormat(const Sexy::String& theName);
    int ParseAtlas(Sexy::XMLParser& theParser, Sexy::XMLElement& theElement);
    int ParseAtlases(Sexy::XMLParser& theParser, Sexy::XMLElement& theElement);
    int Load(const Sexy::String& thePath);

    std::map<Sexy::String, Atlas> mAtlases;
    std::map<Sexy::String, int> mFormats;
};

} // namespace Sexy

using Sexy::AtlasParser;
