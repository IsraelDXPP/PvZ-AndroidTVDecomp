// SexyTransform2D composes translation, rotation and scale onto a 3x3 matrix.
// Each compose builds a factor and stores factor * *this, so the new transform
// is applied on top of the one already stored.

#include "SexyTransform2D.h"

#include <cmath>

Sexy::SexyTransform2D::SexyTransform2D()
{
    LoadIdentity();
}

Sexy::SexyTransform2D::SexyTransform2D(bool theLoadIdentity)
{
    if (theLoadIdentity)
        LoadIdentity();
}

Sexy::SexyTransform2D::SexyTransform2D(SexyMatrix3 const &theMatrix)
{
    m[0] = theMatrix.m[0];
    m[1] = theMatrix.m[1];
    m[2] = theMatrix.m[2];
    m[3] = theMatrix.m[3];
    m[4] = theMatrix.m[4];
    m[5] = theMatrix.m[5];
    m[6] = theMatrix.m[6];
    m[7] = theMatrix.m[7];
    m[8] = theMatrix.m[8];
}

Sexy::SexyTransform2D *Sexy::SexyTransform2D::Translate(float theX, float theY)
{
    SexyMatrix3 aFactor;
    aFactor.LoadIdentity();
    aFactor.m22 = 1.0f;
    aFactor.m02 = theX;
    aFactor.m12 = theY;
    *this = aFactor * *this;
    return this;
}

Sexy::SexyTransform2D *Sexy::SexyTransform2D::RotateRad(float theRadians)
{
    SexyMatrix3 aFactor;
    aFactor.LoadIdentity();
    const float aSin = sinf(theRadians);
    const float aCos = cosf(theRadians);
    aFactor.m00 = aCos;
    aFactor.m01 = aSin;
    aFactor.m10 = -aSin;
    aFactor.m11 = aCos;
    *this = aFactor * *this;
    return this;
}

Sexy::SexyTransform2D *Sexy::SexyTransform2D::RotateDeg(float theDegrees)
{
    return RotateRad(theDegrees * 0.017453294f);
}

float Sexy::SexyTransform2D::LoadRotateRad(float theRadians)
{
    LoadIdentity();
    const float aSin = sinf(theRadians);
    const float aCos = cosf(theRadians);
    m01 = aSin;
    m10 = -aSin;
    m00 = aCos;
    m11 = aCos;
    return aCos;
}

float Sexy::SexyTransform2D::LoadRotateDeg(float theDegrees)
{
    return LoadRotateRad(theDegrees * 0.017453294f);
}

Sexy::SexyTransform2D *Sexy::SexyTransform2D::Scale(float theX, float theY)
{
    SexyMatrix3 aFactor;
    aFactor.LoadIdentity();
    aFactor.m00 = theX;
    aFactor.m11 = theY;
    *this = aFactor * *this;
    return this;
}
