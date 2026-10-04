// Curve1DUtil stores an easing preset. GetInterp is an empty hook in this
// build: the evaluation itself is not linked, so it returns 0.

#include "Curve1DUtil.h"

using namespace Sexy;

namespace {

void WriteDefaultCoeffs(float* theCoeffs)
{
    theCoeffs[0] = 0.0f;
    theCoeffs[1] = 0.25f;
    theCoeffs[2] = 0.5f;
    theCoeffs[3] = 0.75f;
    theCoeffs[4] = 0.25f;
    theCoeffs[5] = 0.5f;
    theCoeffs[6] = 0.75f;
    theCoeffs[7] = 1.01f;
}

} // namespace

Curve1DUtil::Curve1DUtil()
{
    for (int i = 0; i < 8; ++i)
        mParams[i] = 0.0f;
    WriteDefaultCoeffs(mCoeffs);
}

Curve1DUtil::Curve1DUtil(float theStart, float theEnd, float theControl0, float theControl1,
                         float theControl2, float theControl3, float theControl4)
{
    mParams[0] = theStart;
    mParams[1] = theEnd;
    mParams[2] = theControl0;
    mParams[3] = theControl1;
    mParams[4] = theControl2;
    mParams[5] = theControl3;
    mParams[6] = theControl4;
    mParams[7] = 0.0f;
    WriteDefaultCoeffs(mCoeffs);
}

Curve1DUtil* Curve1DUtil::SetBounce(float theAmount, float theHeight)
{
    mParams[0] = -4.0f;
    mParams[1] = 0.0f;
    mParams[2] = theAmount;
    mParams[3] = 1.0f;
    mParams[4] = 0.0f;

    mCoeffs[0] = 0.0f;
    mCoeffs[1] = theHeight;
    mCoeffs[2] = 50.0f;
    mCoeffs[3] = 50.0f;
    mCoeffs[4] = theHeight;
    mCoeffs[5] = 1.01f;
    mCoeffs[6] = 50.25f;
    mCoeffs[7] = 50.25f;
    return this;
}

Curve1DUtil* Curve1DUtil::SetBounce(float theAmount)
{
    return SetBounce(theAmount, 0.75f);
}

Curve1DUtil* Curve1DUtil::SetEaseIn()
{
    mParams[0] = 0.0f;
    mParams[1] = 0.0f;
    mParams[2] = 0.5f;
    mParams[3] = 1.0f;
    mParams[4] = 1.0f;

    mCoeffs[0] = 0.0f;
    mCoeffs[1] = 0.75f;
    mCoeffs[2] = 50.0f;
    mCoeffs[3] = 50.0f;
    mCoeffs[4] = 0.75f;
    mCoeffs[5] = 1.01f;
    mCoeffs[6] = 50.25f;
    mCoeffs[7] = 50.25f;
    return this;
}

Curve1DUtil* Curve1DUtil::SetEaseOut()
{
    mParams[0] = 1.0f;
    mParams[1] = 1.0f;
    mParams[2] = 0.5f;
    mParams[3] = 0.0f;
    mParams[4] = 0.0f;

    mCoeffs[0] = 0.0f;
    mCoeffs[1] = 0.75f;
    mCoeffs[2] = 50.0f;
    mCoeffs[3] = 50.0f;
    mCoeffs[4] = 0.75f;
    mCoeffs[5] = 1.01f;
    mCoeffs[6] = 50.25f;
    mCoeffs[7] = 50.25f;
    return this;
}

int Curve1DUtil::GetInterp(float theTime)
{
    (void)theTime;
    return 0;
}
