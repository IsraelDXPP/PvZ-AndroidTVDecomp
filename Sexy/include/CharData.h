// CharData -- one glyph's metrics inside a font layer.
//
// Plain class: no base and no virtual function. The constructor clears the
// first twenty bytes and leaves the rest alone. FontData and ImageFont read
// the same bytes as 16-bit halves, so each word is a union of the word and
// the two halves those bodies name.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_CHARDATA_H
#define LAWNPROJECTTV_SEXY_INCLUDE_CHARDATA_H

#include "ReconTypes.h"

namespace Sexy
{

class CharData
{
public:
    union
    {
        int mPackedOrigin;
        struct
        {
            unsigned short mX;
            unsigned short mY;
        };
    };
    union
    {
        int mPackedSize;
        struct
        {
            unsigned short mWidth;
            unsigned short mHeight;
        };
    };
    union
    {
        int mPackedOffset;
        struct
        {
            unsigned short mOffsetX;
            unsigned short mOffsetY;
        };
    };
    union
    {
        int mPackedKern;
        struct
        {
            unsigned short mKernIndex;
            unsigned short mKernCount;
        };
    };
    union
    {
        int mPackedAdvance;
        struct
        {
            unsigned short mAdvance;
            unsigned short mPageHeight;
        };
    };
    union
    {
        int mPackedBase;
        struct
        {
            unsigned short mBaseOrder;
            unsigned short mBaseOrderExtra;
        };
    };
    union
    {
        int mPackedOrder;
        struct
        {
            unsigned short mCharOrder;
            unsigned short mCharOrderExtra;
        };
    };
    int mTrailingWord;

    CharData();
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_CHARDATA_H
