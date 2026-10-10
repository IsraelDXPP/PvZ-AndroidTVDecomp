#include "MTRand.h"
#include "ReconData.h"

#include <cstring>

// MT19937 twist constant. Index 1 is the matrix A used when the low bit is set.
static const unsigned int kTwistMatrix[2] = {0x00000000u, 0x9908B0DFu};

static const int kTwistM = 397;
static const unsigned int kUpperMask = 0x80000000u;
static const unsigned int kLowerMask = 0x7FFFFFFFu;

Sexy::MTRand *Sexy::MTRand::SetRandAllowed(bool theAllowed)
{
    if (mRandAllowed == theAllowed)
        return this;

    mRandAllowed = theAllowed;
    if (theAllowed)
        ++gRandAllowedCount;
    else if (gRandAllowedCount > 0)
        --gRandAllowedCount;
    return this;
}

Sexy::MTRand *Sexy::MTRand::SRand(unsigned long theSeed)
{
    unsigned long aSeed = theSeed;
    if (aSeed == 0)
        aSeed = 4357;

    mt[0] = aSeed;
    for (int aIndex = 1; aIndex != MTRAND_N; ++aIndex)
    {
        aSeed = (unsigned long)aIndex + 1812433253UL * (aSeed ^ (aSeed >> 30));
        mt[aIndex] = aSeed;
    }
    mti = MTRAND_N;
    return this;
}

Sexy::MTRand::MTRand(unsigned long theSeed)
{
    mRandAllowed = false;
    SRand(theSeed);
}

Sexy::MTRand::MTRand()
{
    mRandAllowed = false;
    SRand(0x1105u);
}

Sexy::MTRand *Sexy::MTRand::SRand(Sexy::String const &theBlob)
{
    if (theBlob.size() == sizeof(unsigned long) * MTRAND_N)
    {
        memcpy(mt, theBlob.c_str(), sizeof(unsigned long) * MTRAND_N);
        return this;
    }
    return SRand(0x1105u);
}

Sexy::MTRand::MTRand(Sexy::String const &theBlob)
{
    mRandAllowed = false;
    SRand(theBlob);
    mti = MTRAND_N + 1;
}

unsigned int Sexy::MTRand::NextNoAssert()
{
    if (mti >= MTRAND_N)
    {
        for (int aIndex = 0; aIndex < MTRAND_N - kTwistM; ++aIndex)
        {
            const unsigned int aMixed = (mt[aIndex] & kUpperMask) | (mt[aIndex + 1] & kLowerMask);
            mt[aIndex] = mt[aIndex + kTwistM] ^ (aMixed >> 1) ^ kTwistMatrix[aMixed & 1];
        }
        for (int aIndex = MTRAND_N - kTwistM; aIndex < MTRAND_N - 1; ++aIndex)
        {
            const unsigned int aMixed = (mt[aIndex] & kUpperMask) | (mt[aIndex + 1] & kLowerMask);
            mt[aIndex] = mt[aIndex + (kTwistM - MTRAND_N)] ^ (aMixed >> 1) ^ kTwistMatrix[aMixed & 1];
        }
        const unsigned int aWrapped = (mt[MTRAND_N - 1] & kUpperMask) | (mt[0] & kLowerMask);
        mt[MTRAND_N - 1] = mt[kTwistM - 1] ^ (aWrapped >> 1) ^ kTwistMatrix[aWrapped & 1];
        mti = 0;
    }

    unsigned int aValue = mt[mti++];
    aValue ^= aValue >> 11;
    aValue ^= (aValue << 7) & 0x9D2C5680u;
    aValue ^= (aValue << 15) & 0xEFC60000u;
    aValue ^= aValue >> 18;
    return aValue & 0x7FFFFFFFu;
}

unsigned int Sexy::MTRand::Next()
{
    return NextNoAssert();
}

unsigned int Sexy::MTRand::NextNoAssert(unsigned long theRange)
{
    return NextNoAssert() % theRange;
}

unsigned int Sexy::MTRand::NextNoAssert(unsigned int theRange)
{
    return NextNoAssert() % theRange;
}

unsigned int Sexy::MTRand::Next(unsigned long theRange)
{
    return NextNoAssert(theRange);
}

unsigned int Sexy::MTRand::Next(unsigned int theRange)
{
    return NextNoAssert(theRange);
}

float Sexy::MTRand::NextNoAssert(float theRange)
{
    return theRange * 4.65661288e-10f * (float)NextNoAssert();
}

float Sexy::MTRand::Next(float theRange)
{
    return NextNoAssert(theRange);
}

Sexy::MTRand *Sexy::MTRand::Serialize()
{
    return this;
}
