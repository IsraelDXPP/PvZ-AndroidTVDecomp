#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_TRAIL_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_TRAIL_H

#include "ReconTypes.h"
#include "Graphics.h"
#include "SyncObject.h"

class TrailDefinition;
class TrailHolder;

// Ribbon of recent points. SyncObject owns the first 0x10 bytes; the rest of the
// object (points through the colour override) is one synchronised block.
class Trail : public SyncObject
{
public:
    virtual ~Trail();

    float mPoints[20][2]; // +0x10, 20 points of (x, y)
    int mNumPoints; // +0xB0
    bool mDead; // +0xB4
    int mRenderOrder; // +0xB8, stored 0; the allocator's order argument is unused
    int mTrailAge; // +0xBC
    int mTrailDuration; // +0xC0
    TrailDefinition *mDefinition; // +0xC4
    TrailHolder *mTrailHolder; // +0xC8
    float mTrailInterp[4]; // +0xCC, one Rand seed per float track
    float mOffsetX; // +0xDC, added to every point when the ribbon is drawn
    float mOffsetY; // +0xE0
    bool mIsAttachment; // +0xE4, EffectSystem skips Update while this is set
    Color mColor; // +0xE8, object ends at +0xF8

    void AddPoint(float theX, float theY);
    void Update();
    bool GetNormalAtPoint(int theIndex, SexyVector2 &theNormal);
    void Draw(Sexy::Graphics *theGraphics);
    Trail();
};

struct TrailSlot
{
    Trail mItem;
    unsigned int mID;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_TRAIL_H
