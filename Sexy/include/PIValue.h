// A one-dimensional curve. mSamples is the 256-entry table QuantizeCurve
// fills. mKeyframes is the authored (time, value) list. mBezier is used when
// it has points. The last four floats cache the previous query.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PIVALUE_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PIVALUE_H

#include "Bezier.h"

#include <vector>

namespace Sexy
{

struct PIKeyframe
{
    float mTime = 0.0f;
    float mValue = 0.0f;
};

class PIValue
{
public:
    std::vector<float> mSamples;
    std::vector<PIKeyframe> mKeyframes;
    Sexy::Bezier mBezier;
    float mLastQueryTime = -1.0f;
    float mLastValue = 0.0f;
    float mLastEvaluateTime = 0.0f;
    float mLastStepSize = 0.01f;

    PIValue();
    PIValue(const PIValue &other);

    // Value of the last keyframe at or before theTime. A query before every
    // keyframe, or an empty curve, is 0.
    float GetLastKeyframe(float theTime);
    float GetLastKeyframeTime(float theTime);
    float GetNextKeyframeTime(float theTime) const;
    int GetNextKeyframeIdx(float theTime);
    void QuantizeCurve();
    // theFallback is returned when the keyframes do not bracket theTime.
    float GetValueAt(float theTime, float theFallback);
};

} // namespace Sexy

using Sexy::PIKeyframe;
using Sexy::PIValue;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PIVALUE_H
