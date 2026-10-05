// SharedImageRef names a shared image, an owned image, and an ownership flag.
//
// Release drops the shared count and, when the flag is set, deletes the owned
// image. The conversion operators and operator-> all return the owned image.

#pragma once

#include "ReconTypes.h"
#include "ReconHelpers.h"
#include "SharedImage.h"

namespace Sexy {

inline unsigned int& RefCountOf(Sexy::SharedImage* theSharedImage)
{
    return theSharedImage->mRefCount;
}

class SharedImageRef
{
public:
    SharedImage*    mSharedImage;   // +0
    Image*          mImage;         // +4
    bool            mOwnsImage;     // +8
    unsigned char   mOwnsPad[3];

    SharedImageRef(const SharedImageRef& theOther);
    SharedImageRef();
    SharedImageRef(Sexy::SharedImage * theSharedImage);
    ~SharedImageRef();

    SharedImageRef& operator=(const SharedImageRef& theOther);
    SharedImageRef& operator=(Sexy::SharedImage * theSharedImage);

    operator Sexy::Image*() const;
    operator Sexy::MemoryImage*() const;
    operator Sexy::DeviceImage*() const;
    Sexy::Image* operator->() const;
    Sexy::Image& operator*() const;

    void Release();
};

} // namespace Sexy

using Sexy::SharedImageRef;
