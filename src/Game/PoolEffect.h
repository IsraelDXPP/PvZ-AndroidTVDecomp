#pragma once

#include "ReconTypes.h"
#include "TodHesitationBracket.h"

class LawnApp;

class PoolEffect
{
public:
    unsigned char *mCausticGrayscaleImage;
    Sexy::MemoryImage *mCausticImage;
    LawnApp *mApp;
    int mPoolCounter;

    void PoolEffectInitialize();
    void PoolEffectDispose();
    unsigned char BilinearLookupFixedPoint(unsigned int u, unsigned int v);
    unsigned char BilinearLookup(float u, float v);
    void UpdateWaterEffect(Sexy::Graphics *g);
    void PoolEffectUpdate();
    void PoolEffectDraw(Sexy::Graphics *g, bool theIsNight);
};
