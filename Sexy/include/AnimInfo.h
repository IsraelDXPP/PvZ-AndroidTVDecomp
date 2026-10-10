// AnimInfo -- one image's animation record.
//
// A plain data record: no vtable and no base class. The field names are the
// ones the PC reference source uses, and the methods are the ones Image and
// ResourceManager already call.
//
// AnimType (AnimType_None / Once / PingPong / Loop) lives in ReconTypes.h.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_ANIMINFO_H
#define LAWNPROJECTTV_SEXY_INCLUDE_ANIMINFO_H

#include "ReconTypes.h"
#include <vector>

namespace Sexy
{

class AnimInfo
{
public:
    AnimType mAnimType;
    int mFrameDelay;
    int mNumCels;
    std::vector<int> mPerFrameDelay;
    std::vector<int> mFrameMap;
    int mTotalAnimTime;

    AnimInfo();
    int GetPerFrameCel(int theTick);
    int GetCel(int theTick);
    unsigned int SetPerFrameDelay(int theCel, int theDelay);
    // Returns void: the only caller, ResourceManager, discards the result.
    void Compute(int theNumCels, int theBeginFrameTime, int theEndFrameTime);
};

} // namespace Sexy

using Sexy::AnimInfo;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_ANIMINFO_H
