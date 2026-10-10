// A colour or alpha ramp: one keyframe list. GetValueAt maps the query onto
// the span between the first and last key time and blends the two colours that
// bracket it. GetKeyframeNum and GetKeyframeTime wrap the index.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PIINTERPOLATOR_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PIINTERPOLATOR_H

#include "ReconTypes.h"
#include <vector>

namespace Sexy
{

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

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PIINTERPOLATOR_H
