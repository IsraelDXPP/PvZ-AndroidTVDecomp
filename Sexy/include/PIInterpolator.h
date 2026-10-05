// A colour or alpha ramp: one keyframe list. GetValueAt maps the query onto
// the span between the first and last key time and blends the two colours that
// bracket it. GetKeyframeNum and GetKeyframeTime wrap the index.

#pragma once

#include "ReconTypes.h"
#include <vector>

namespace Sexy {

class PIInterpolator
{
public:
    std::vector<Sexy::PIInterpolatorPoint> mKeyframes;

    int GetValueAt(float time);
    unsigned int GetKeyframeNum(int index);
    int GetKeyframeTime(int index);
};

} // namespace Sexy

using Sexy::PIInterpolator;
