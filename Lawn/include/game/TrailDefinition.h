#pragma once

#include "ReconTypes.h"

// Loaded trail record. No vtable. Five 8-byte float tracks follow the header,
// which is the 0x38 bytes the board allocates for one definition.
class TrailDefinition
{
public:
    Sexy::Image*        mImage;
    int                 mMaxPoints;         // constructed to 2
    float               mMinPointDistance;  // constructed to 1.0f
    int                 mTrailFlags;        // bit 0 loops the trail instead of killing it
    FloatParameterTrack mTrailDuration;
    FloatParameterTrack mWidthOverLength;
    FloatParameterTrack mWidthOverTime;
    FloatParameterTrack mAlphaOverLength;
    FloatParameterTrack mAlphaOverTime;

    TrailDefinition();
    ~TrailDefinition();
};
