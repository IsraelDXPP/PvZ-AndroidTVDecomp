#include "PIParticleDef.h"
#include "Bezier.h"

namespace
{
struct PIParticleDefRecord
{
    int mTimeBits;
    int mValueBits;
};
} // namespace

Sexy::PIParticleDef::PIParticleDef()
    : mUnused000(0),
      mUnused004(),
      mUnused008(0),
      mPtr7C8(nullptr),
      mColorKeyEnd(0),
      mPtr7D4(nullptr)
{
    for (int flag = 0; flag < 19; ++flag)
        mSpawnFlags[flag] = 0;
    for (int word = 0; word < 6; ++word)
        mAlphaKeys[word] = 0;
}

Sexy::PIParticleDef::PIParticleDef(Sexy::PIParticleDef const& other)
    : mUnused000(other.mUnused000),
      mUnused004(other.mUnused004),
      mUnused008(other.mUnused008),
      mValues{ other.mValues[0],  other.mValues[1],  other.mValues[2],  other.mValues[3],
               other.mValues[4],  other.mValues[5],  other.mValues[6],  other.mValues[7],
               other.mValues[8],  other.mValues[9],  other.mValues[10], other.mValues[11],
               other.mValues[12], other.mValues[13], other.mValues[14], other.mValues[15],
               other.mValues[16], other.mValues[17], other.mValues[18], other.mValues[19],
               other.mValues[20], other.mValues[21], other.mValues[22], other.mValues[23],
               other.mValues[24], other.mValues[25], other.mValues[26], other.mValues[27] }
{
    for (int flag = 0; flag < 18; ++flag)
        mSpawnFlags[flag] = other.mSpawnFlags[flag];

    const PIParticleDefRecord* colorSource =
        static_cast<const PIParticleDefRecord*>(other.mPtr7C8);
    const PIParticleDefRecord* colorEnd =
        static_cast<const PIParticleDefRecord*>(static_cast<const void*>(&other.mColorKeyEnd));
    mPtr7C8 = nullptr;
    unsigned int colorCount = (unsigned int)(colorEnd - colorSource);
    if (colorCount != 0)
    {
        PIParticleDefRecord* colorCopy = new PIParticleDefRecord[colorCount];
        for (unsigned int key = 0; key < colorCount; ++key)
            colorCopy[key] = colorSource[key];
        mPtr7C8 = colorCopy;
    }
    mColorKeyEnd = other.mColorKeyEnd;

    const PIParticleDefRecord* alphaSource =
        static_cast<const PIParticleDefRecord*>(static_cast<const void*>(&other.mAlphaKeys[0]));
    const PIParticleDefRecord* alphaEnd =
        static_cast<const PIParticleDefRecord*>(static_cast<const void*>(&other.mAlphaKeys[1]));
    mAlphaKeys[0] = 0;
    unsigned int alphaCount = (unsigned int)(alphaEnd - alphaSource);
    if (alphaCount != 0)
    {
        PIParticleDefRecord* alphaCopy = new PIParticleDefRecord[alphaCount];
        for (unsigned int key = 0; key < alphaCount; ++key)
            alphaCopy[key] = alphaSource[key];
        mAlphaKeys[0] = (int)alphaCopy;
    }
    for (int word = 1; word < 6; ++word)
        mAlphaKeys[word] = other.mAlphaKeys[word];
}
