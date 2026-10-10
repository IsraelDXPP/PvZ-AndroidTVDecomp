// Transform is a SexyTransform2D plus the translation, rotation and scale
// that CalcMatrix folds in when they are pending.
//
// There is no vtable. Reset clears the pending flags and the translation, and
// writes identity scale. SetMatrix copies either a full 3x3 or the six-float
// simple matrix the particle effect stores.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_TRANSFORM_H
#define LAWNPROJECTTV_SEXY_INCLUDE_TRANSFORM_H

#include "ReconTypes.h"
#include "SexyTransform2D.h"

namespace Sexy
{

class Transform
{
public:
    SexyTransform2D mMatrix; // +0x00 ~ +0x23
    bool mNeedCalc; // +0x24
    bool mComplex; // +0x25
    bool mRotated; // +0x26
    bool mScaled; // +0x27
    float mTransX; // +0x28
    float mTransY; // +0x2C
    float mRotateCenterX; // +0x30
    float mRotateCenterY; // +0x34
    float mScaleX; // +0x38
    float mScaleY; // +0x3C
    float mRotation; // +0x40

    Transform();
    Transform *Reset();
    Transform *Translate(float theX, float theY);
    SexyMatrix3 *CalcMatrix();
    SexyMatrix3 *MakeComplex();
    Transform *RotateRad(float theRadians);
    Transform *RotateDeg(float theDegrees);
    Transform *Scale(float theX, float theY);
    Transform *GetMatrix();
    Transform *SetMatrix(Sexy::SexyTransform2D const &theMatrix);
    Transform *SetMatrix(const float *theSimple);
};

} // namespace Sexy

using Sexy::Transform;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_TRANSFORM_H
