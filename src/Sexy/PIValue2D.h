// A two-dimensional curve. Each key is a time and an (x, y) pair.
// GetValueAt and GetVelocityAt return a pointer to the cached pair they
// just wrote. mLastValueY follows mLastValueX, and mLastVelocityY follows
// mLastVelocityX, so the returned pointer is a two-float result.

#pragma once

#include "Bezier.h"

#include <vector>

namespace Sexy {

struct PIKeyframe2D
{
    float mTime = 0.0f;
    float mX = 0.0f;
    float mY = 0.0f;
};

class PIValue2D
{
public:
    std::vector<PIKeyframe2D> mKeyframes;
    Sexy::Bezier mBezier;
    float mLastValueTime = -1.0f;
    float mLastValueX = 0.0f;
    float mLastValueY = 0.0f;
    float mLastVelocityTime = 0.0f;
    float mLastVelocityX = 0.0f;
    float mLastVelocityY = 0.0f;

    PIValue2D() = default;
    PIValue2D(const PIValue2D& other);

    float* GetValueAt(float theTime);
    float* GetVelocityAt(float theTime);
};

} // namespace Sexy

using Sexy::PIKeyframe2D;
using Sexy::PIValue2D;
