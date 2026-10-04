// FoleyTypeData -- restored from libGameMain.so.

#include "FoleyTypeData.h"

// ==========================================================================
// FoleyTypeData::FoleyTypeData()
// @001FF754  34 bytes
// ==========================================================================
FoleyTypeData::FoleyTypeData()
{
    for (int i = 0; i < MAX_FOLEY_INSTANCES; ++i)
    {
        mFoleyInstances[i] = FoleyTypeDataElement();
    }

    mLastVariationPlayed = -1;
}
