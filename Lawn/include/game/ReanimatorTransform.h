// ReanimatorTransform -- reconstructed from libGameMain.so.
//
// A plain 0x30-byte value record: no vtable (the RTTI export has no `_ZTV` for
// it) and no base class. The member table is the authoritative header
// PvZ\TodLib\Effect\Reanimator.h field for field, and the same header's
// ReanimatorTrackInstance proves the size: its `ReanimatorTransform mBlendTransform`
// spans words 2..13, i.e. twelve ints.
//
// The old placeholder said 0x20 because ReanimatorTransform.cpp reaches no `this`
// offset past +0x1C; the last four words are simply not written by the bodies it
// contains.

#pragma once

#include "ReconTypes.h"

class ReanimatorTransform
{
public:
    float mTransX;         // +0x00
    float mTransY;         // +0x04
    float mSkewX;          // +0x08
    float mSkewY;          // +0x0C
    float mScaleX;         // +0x10
    float mScaleY;         // +0x14
    float mFrame;          // +0x18
    float mAlpha;          // +0x1C
    Sexy::Image *mImage;   // +0x20
    Sexy::Image *mImage2;  // +0x24
    Sexy::Font *mFont;     // +0x28
    const char *mText;     // +0x2C
    // size 0x30

    ReanimatorTransform();
    ReanimatorTransform(float theTransX, float theTransY, float theSkewX, float theSkewY,
                        float theScaleX, float theScaleY, float theFrame, float theAlpha,
                        Sexy::Image *theImage, Sexy::Image *theImage2, Sexy::Font *theFont,
                        const char *theText);
};

