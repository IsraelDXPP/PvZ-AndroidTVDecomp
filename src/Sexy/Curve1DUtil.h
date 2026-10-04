// Curve1DUtil -- a one-dimensional easing curve.
//
// Plain value type: no vtable. ChallengeScreen embeds one by value. Sixteen
// floats: the seven curve parameters plus a spare word, then the eight
// coefficients the setters fill in.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class Curve1DUtil
{
public:
    float mParams[8];
    float mCoeffs[8];

    Curve1DUtil();
    Curve1DUtil(float theStart, float theEnd, float theControl0, float theControl1,
                float theControl2, float theControl3, float theControl4);
    Curve1DUtil* SetBounce(float theAmount, float theHeight);
    Curve1DUtil* SetBounce(float theAmount);
    Curve1DUtil* SetEaseIn();
    Curve1DUtil* SetEaseOut();
    int GetInterp(float theTime);
};

} // namespace Sexy
