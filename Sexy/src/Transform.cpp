// Transform keeps a pending translation, rotation and scale, and folds them
// into the matrix the first time something asks for it. Once the transform is
// complex, later calls compose straight into that matrix.

#include "Transform.h"

Sexy::Transform::Transform() :
    mMatrix(false)
{
    Reset();
}

Sexy::Transform *Sexy::Transform::Reset()
{
    mNeedCalc = true;
    mComplex = false;
    mRotated = false;
    mScaled = false;
    mTransY = 0.0f;
    mTransX = 0.0f;
    mRotateCenterY = 0.0f;
    mRotateCenterX = 0.0f;
    mRotation = 0.0f;
    mScaleY = 1.0f;
    mScaleX = 1.0f;
    return this;
}

Sexy::Transform *Sexy::Transform::Translate(float theX, float theY)
{
    if (mComplex)
    {
        mMatrix.Translate(theX, theY);
        return this;
    }

    mNeedCalc = true;
    if (mRotated || mScaled)
    {
        mRotateCenterX += theX;
        mRotateCenterY += theY;
    }
    else
    {
        mTransX += theX;
        mTransY += theY;
    }
    return this;
}

Sexy::SexyMatrix3 *Sexy::Transform::CalcMatrix()
{
    if (!mNeedCalc)
        return &mMatrix;

    mNeedCalc = false;
    mMatrix.LoadIdentity();
    mMatrix.m02 = mTransX;
    mMatrix.m12 = mTransY;
    mMatrix.m22 = 1.0f;
    if (mScaled)
    {
        mMatrix.m00 = mScaleX;
        mMatrix.m11 = mScaleY;
    }
    else if (mRotated)
    {
        mMatrix.RotateRad(mRotation);
    }

    if (mRotateCenterX != 0.0f || mRotateCenterY != 0.0f)
        mMatrix.Translate(mRotateCenterX, mRotateCenterY);
    return &mMatrix;
}

Sexy::SexyMatrix3 *Sexy::Transform::MakeComplex()
{
    if (!mComplex)
    {
        mComplex = true;
        CalcMatrix();
    }
    return &mMatrix;
}

Sexy::Transform *Sexy::Transform::RotateRad(float theRadians)
{
    if (mComplex || mScaled)
    {
        MakeComplex();
        mMatrix.RotateRad(theRadians);
        return this;
    }

    mNeedCalc = true;
    mRotated = true;
    mRotation += theRadians;
    return this;
}

Sexy::Transform *Sexy::Transform::RotateDeg(float theDegrees)
{
    return RotateRad(theDegrees * 0.017453294f);
}

Sexy::Transform *Sexy::Transform::Scale(float theX, float theY)
{
    if (!mComplex)
    {
        const bool aUniformOrOrigin = (theX == theY) || (mRotateCenterX == 0.0f && mRotateCenterY == 0.0f);
        const bool aPositive = !mRotated && mTransX == 0.0f && mTransY == 0.0f &&
            (theX >= 0.0f || (theX * mScaleX) == -1.0f) && theY >= 0.0f;
        if (aPositive && aUniformOrOrigin)
        {
            mRotateCenterX *= theX;
            mRotateCenterY *= theY;
            mScaleX *= theX;
            mScaleY *= theY;
            mNeedCalc = true;
            mScaled = true;
            return this;
        }
        MakeComplex();
    }

    mMatrix.Scale(theX, theY);
    return this;
}

Sexy::Transform *Sexy::Transform::GetMatrix()
{
    CalcMatrix();
    return this;
}

Sexy::Transform *Sexy::Transform::SetMatrix(Sexy::SexyTransform2D const &theMatrix)
{
    mMatrix.m[0] = theMatrix.m[0];
    mMatrix.m[1] = theMatrix.m[1];
    mMatrix.m[2] = theMatrix.m[2];
    mMatrix.m[3] = theMatrix.m[3];
    mMatrix.m[4] = theMatrix.m[4];
    mMatrix.m[5] = theMatrix.m[5];
    mMatrix.m[6] = theMatrix.m[6];
    mMatrix.m[7] = theMatrix.m[7];
    mMatrix.m[8] = theMatrix.m[8];
    mNeedCalc = false;
    mComplex = true;
    return this;
}

Sexy::Transform *Sexy::Transform::SetMatrix(const float *theSimple)
{
    mMatrix.m[0] = theSimple[0];
    mMatrix.m[1] = theSimple[2];
    mMatrix.m[3] = theSimple[1];
    mMatrix.m[4] = theSimple[3];
    mMatrix.m[2] = theSimple[4];
    mMatrix.m[5] = theSimple[5];
    mMatrix.m[6] = 0.0f;
    mMatrix.m[7] = 0.0f;
    mMatrix.m[8] = 1.0f;
    mNeedCalc = false;
    mComplex = true;
    return this;
}
