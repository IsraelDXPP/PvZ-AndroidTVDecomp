// ImageFont draws text from a font descriptor's layers. Metrics, measurement
// and drawing override Font. The extra virtuals build the active layers,
// draw with an optional rect list, and expose point size, scale and tags.

#pragma once

#include "ReconTypes.h"
#include "ActiveFontLayer.h"
#include "Buffer.h"
#include "CritSect.h"
#include "Font.h"
#include "FontData.h"
#include "FontLayer.h"
#include "Graphics.h"
#include "ResourceManager.h"
#include "SexyAppBase.h"
#include "SharedImageRef.h"

#include <list>
#include <map>
#include <string>
#include <vector>

namespace Sexy {

// The wide-string overloads share Sexy::String's one-word layout. Callers
// that already hold a widened string pass it through this name.
class WString;

class ImageFont : public Font
{
public:
    virtual ~ImageFont();
    virtual int StringWidth(Sexy::String const& theString);
    virtual int StringWidth(Sexy::WString const& theString);
    virtual int CharWidth(int theChar);
    virtual int CharWidthKern(int theChar, int thePrevChar);
    virtual void DrawString(Sexy::Graphics* theGraphics, int theX, int theY,
                            Sexy::String const& theString, Color const& theColor,
                            TRect<int> const& theClipRect);
    virtual void DrawString(Sexy::Graphics* theGraphics, int theX, int theY,
                            Sexy::WString const& theString, Color const& theColor,
                            TRect<int> const& theClipRect);
    void StringToGlyphs(std::wstring const& theString, std::vector<Glyph>& theGlyphs) override;
    virtual void DrawGlyphs(Sexy::Graphics* theGraphics, int theX, int theY,
                            std::vector<Glyph>& theGlyphs, unsigned int theStart,
                            unsigned int theCount, Color const& theColor,
                            TRect<int> const& theClipRect);
    void GlyphExtentsList(std::list<const Glyph*> const& theGlyphsList, TextExtents& theExtents) override;
    virtual bool IsComposited();
    virtual Sexy::ImageFont* Duplicate();
    virtual void GenerateActiveFontLayers();
    virtual void DrawStringEx(Sexy::Graphics* theGraphics, int theX, int theY,
                              Sexy::String const& theString, Color const& theColor,
                              TRect<int> const* theClipRect,
                              std::list<TRect<int>>* theDrawnAreas, int* theWidth);
    virtual void DrawStringEx(Sexy::Graphics* theGraphics, int theX, int theY,
                              Sexy::WString const& theString, Color const& theColor,
                              TRect<int> const* theClipRect,
                              std::list<TRect<int>>* theDrawnAreas, int* theWidth);
    virtual int MappedCharWidthKern(int theChar, int thePrevChar);
    virtual int SetPointSize(int thePointSize);
    virtual int GetPointSize();
    virtual int SetScale(double theScale);
    virtual int GetDefaultPointSize();
    virtual int AddTag(Sexy::WString const& theTag);
    virtual int RemoveTag(Sexy::WString const& theTag);
    virtual int HasTag(Sexy::WString const& theTag);
    virtual int AddTag(Sexy::String const& theTag);
    virtual int RemoveTag(Sexy::String const& theTag);
    virtual int HasTag(Sexy::String const& theTag);
    virtual Sexy::String GetDefine(Sexy::WString const& theName);
    virtual ImageFont* Prepare();

    Sexy::FontData* mFontData;
    int mPointSize;
    std::vector<Sexy::String> mTags;
    bool mRecalcFontSize;
    std::vector<ActiveFontLayer> mActiveLayerList;
    int mActiveListPad;
    double mScale;
    bool mForceScaledImagesWhite;
    Sexy::SexyAppBase* mApp;
    bool mDigestComputed;
    unsigned char mDigest[16];

    int SerializeWrite(void* theStream, int theSize);
    int GetMappedChar(int theChar);
    int GetLayerCount();
    void PopLayerColor(Sexy::String const& theLayerName);
    void PopLayerColor(int theLayerIndex);
    void PushLayerColor(Sexy::String const& theLayerName, Color const& theColor);
    void PushLayerColor(int theLayerIndex, Color const& theColor);
    ImageFont(Sexy::Image* theImage);
    int SerializeRead(void* theStream, int theSize);
    int SerializeReadEndian(void* theStream, int theSize);
    ImageFont(Sexy::ImageFont const& theOther);
    int UpdateDigest(Sexy::String const& theFileName);
    bool CheckCachedBuffer(Sexy::Buffer& theBuffer, Sexy::String const& theFileName);
    ImageFont(Sexy::SexyAppBase* theApp, Sexy::String const& theFileName);
    ImageFont(Sexy::Image* theImage, Sexy::String const& theFileName);
};

} // namespace Sexy

using Sexy::ImageFont;
