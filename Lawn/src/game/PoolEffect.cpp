#include "PoolEffect.h"
#include "Graphics.h"
#include "MemoryImage.h"
#include "TodHesitationBracket.h"
#include <cstring>
#include <cmath>

void PoolEffect::PoolEffectInitialize()
{
    TodHesitationBracket aBracket("PoolEffectInitialize");

    mPoolCounter = 0;
    mApp = (LawnApp *)gSexyAppBase;

    // 128 by 64 caustic image, opaque white.
    mCausticImage = new Sexy::MemoryImage();
    mCausticImage->Create(128, 64);
    mCausticImage->SetImageMode(true, true);
    memset(mCausticImage->GetBits(), 255, 0x8000u);

    // One byte per pixel of the 256 by 256 caustic texture.
    mCausticGrayscaleImage = new unsigned char[0x10000];

    const unsigned int *causticBits = IMAGE_POOL_CAUSTIC_EFFECT->GetBits();
    for (int i = 0; i < 0x10000; ++i)
        mCausticGrayscaleImage[i] = (unsigned char)causticBits[i];
}

void PoolEffect::PoolEffectDispose()
{
    delete mCausticImage;
    delete[] mCausticGrayscaleImage;
}

unsigned char PoolEffect::BilinearLookupFixedPoint(unsigned int u, unsigned int v)
{
    const unsigned char *mask = mCausticGrayscaleImage;
    unsigned int row = HIWORD(v);
    unsigned char column = BYTE2(u);
    unsigned int fracU = u & 0xFFFE;
    int nextRow = (unsigned char)(BYTE2(v) + 1);
    unsigned int fracV = v & 0xFFFE;
    return (unsigned char)((((((fracV + 1) * (0xFFFF - fracU)) >> 16) * mask[column + (nextRow << 8)]) >> 16) +
        (((((0xFFFF - fracV) * (fracU + 1)) >> 16) * mask[(unsigned char)(column + 1) + ((unsigned char)row << 8)]) >>
            16) +
        (((((0xFFFF - fracV) * (0xFFFF - fracU)) >> 16) * mask[column + ((unsigned char)row << 8)]) >> 16) +
        (((((fracV + 1) * (fracU + 1)) >> 16) * mask[(unsigned char)(column + 1) + (nextRow << 8)]) >> 16));
}

unsigned char PoolEffect::BilinearLookup(float u, float v)
{
    const unsigned char *mask = mCausticGrayscaleImage;
    float floorU = floorf(u);
    float floorV = floorf(v);
    unsigned char row = (int)floorV;
    const unsigned char *nextColumn = mask + (unsigned char)((int)floorU + 1);
    float weightU = (1.0f - u) + floorU;
    float weightV = (1.0f - v) + floorV;
    unsigned char column = (unsigned char)(int)floorU;
    unsigned char nextRow = (unsigned char)(row + 1);
    float top = (float)mask[column + (row << 8)] * weightU + (float)nextColumn[row << 8] * (u - floorU);
    float bottom = (float)mask[column + (nextRow << 8)] * weightU + (float)nextColumn[nextRow << 8] * (u - floorU);
    return (unsigned char)(unsigned int)(top * weightV + bottom * (v - floorV));
}

void PoolEffect::UpdateWaterEffect(Sexy::Graphics *) {}

void PoolEffect::PoolEffectUpdate()
{
    ++mPoolCounter;
}

void PoolEffect::PoolEffectDraw(Sexy::Graphics *, bool) {}
