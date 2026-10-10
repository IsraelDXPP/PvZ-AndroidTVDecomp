// FontData parses a font descriptor into layers. It extends DescParser, so
// the encoding and command slots come from that base. This class overrides
// HandleCommand and adds Error, which reports through the app Load was given.
//
// The define table is DescParser's. Its destructor deletes every mapped
// element, so this destructor must not delete those values again.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_FONTDATA_H
#define LAWNPROJECTTV_SEXY_INCLUDE_FONTDATA_H

#include "ReconTypes.h"
#include "DescParser.h"
#include "FontLayer.h"
#include "ListDataElement.h"
#include "PixelUtil.h"
#include "ResourceManager.h"
#include "SharedImageRef.h"

#include <list>
#include <map>
#include <string>
#include <vector>

int ReadCodePoint(const Sexy::String &theText, unsigned int &theIndex);
Sexy::String CodePointsToUtf8(const std::vector<int> &thePoints);

namespace Sexy
{

class FontData : public DescParser
{
public:
    virtual ~FontData();

    virtual bool HandleCommand(const Sexy::ListDataElement &theParams);
    virtual bool Error(const Sexy::String &theError);

    FontData();
    int Ref();
    FontData *DeRef();
    int DataToLayer(Sexy::DataElement *theElement, Sexy::FontLayer **theLayer);
    int GetColorFromDataElement(Sexy::DataElement *theElement, Color &theColor);
    int Load(Sexy::SexyAppBase *theApp, Sexy::String const &theFileName);
    int LoadLegacy(Sexy::Image *theImage, Sexy::String const &theFileName);

    bool ReportError(const char *theMessage);
    Sexy::FontLayer *AddLayer(Sexy::FontLayer const &theLayer);
    void RegisterLayer(Sexy::String const &theName, Sexy::FontLayer *theLayer);

    bool mIsListFont;
    int mRefCount;
    Sexy::SexyAppBase *mApp;
    int mDefaultPointSize;
    std::map<int, int> mCharMap;
    std::list<Sexy::FontLayer> mLayerList;
    std::map<Sexy::String, Sexy::FontLayer *> mLayerMap;
    Sexy::String mFileName;
    Sexy::String mErrorHeader;
    Sexy::String mImageGroup;
};

} // namespace Sexy

using Sexy::FontData;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_FONTDATA_H
