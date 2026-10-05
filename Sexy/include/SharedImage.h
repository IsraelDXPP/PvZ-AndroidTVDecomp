// SharedImage.h -- the reference-counted image handle the shared-image map stores.
//
// ReconTypes.h used to carry only `class SharedImage;`, and SexyAppBase.cpp
// worked around that with a file-local definition, so no other .cpp could read
// the object's words. ResourceManager.cpp reported C2027 for it.
//
// Size 0x8, and the two words are pinned by every body that touches one:
// `_ZN4Sexy11SharedImageC1Ev` @0x3418D4 writes both to zero, and the reference
// counting -- SharedImageRef's constructors, `Release`, and the helpers in
// SharedImageRef.h -- reads +0 as the Image being shared and +4 as the count.
// The class has no vtable and no other member.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class SharedImage
{
public:
    Image *      mImage;    // +0x00
    unsigned int mRefCount; // +0x04

    SharedImage()
        : mImage(nullptr), mRefCount(0)
    {
    }
};

} // namespace Sexy
