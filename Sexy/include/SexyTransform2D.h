// SexyTransform2D is a SexyMatrix3 with transform helpers and no extra data.
//
// The constructors build the matrix and optionally load the identity. Translate,
// Rotate and Scale compose a factor matrix into this one. There is no vtable.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class SexyTransform2D : public SexyMatrix3
{
public:
    SexyTransform2D();
    SexyTransform2D(bool theLoadIdentity);
    SexyTransform2D(SexyMatrix3 const& theMatrix);
    SexyTransform2D* Translate(float theX, float theY);
    SexyTransform2D* RotateRad(float theRadians);
    SexyTransform2D* RotateDeg(float theDegrees);
    float LoadRotateRad(float theRadians);
    float LoadRotateDeg(float theDegrees);
    SexyTransform2D* Scale(float theX, float theY);
};

} // namespace Sexy

using Sexy::SexyTransform2D;
