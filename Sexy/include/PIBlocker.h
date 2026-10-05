// A particle blocker: a named polygon that particles bounce off or stop at.
// The loader builds one per CBlocker record and stores them on the layer.
// mActive is the first curve the reader fills, mSpin the second, mOffset the
// translation debug draw applies, and mPoints the polygon vertices.

#pragma once

#include "ReconTypes.h"
#include "Bezier.h"
#include "PIValue.h"
#include "PIValue2D.h"

#include <vector>

namespace Sexy {

class PIBlocker
{
public:
    Sexy::String mName;
    bool mFlag;
    Sexy::PIValue2D mOffset;
    Sexy::PIValue mActive;
    Sexy::PIValue mSpin;
    std::vector<Sexy::PIValue2D> mPoints;

    PIBlocker() : mName(), mFlag(false), mOffset(), mActive(), mSpin(), mPoints() {}
    PIBlocker(Sexy::PIBlocker const& other);
};

} // namespace Sexy

using Sexy::PIBlocker;
