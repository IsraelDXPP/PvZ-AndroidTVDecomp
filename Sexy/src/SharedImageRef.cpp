// SharedImageRef counts a shared image and optionally owns a private copy.
// Copy and assign add a reference. Release drops it and, on the last reference,
// asks the app to sweep shared images. The owned image is deleted only when
// mOwnsImage is set.

#include "SharedImageRef.h"
#include "DeviceImage.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

Sexy::SharedImageRef::SharedImageRef(const SharedImageRef& theOther)
    : mSharedImage(theOther.mSharedImage)
    , mImage(theOther.mImage)
    , mOwnsImage(false)
{
    if (mSharedImage != nullptr)
        ++RefCountOf(mSharedImage);
}

Sexy::SharedImageRef::SharedImageRef()
    : mSharedImage(nullptr)
    , mImage(nullptr)
    , mOwnsImage(false)
{
}

Sexy::SharedImageRef::SharedImageRef(Sexy::SharedImage* theSharedImage)
    : mSharedImage(theSharedImage)
    , mImage(theSharedImage != nullptr ? theSharedImage->mImage : nullptr)
    , mOwnsImage(false)
{
    if (theSharedImage != nullptr)
        ++RefCountOf(theSharedImage);
}

void Sexy::SharedImageRef::Release()
{
    if (mOwnsImage && mImage != nullptr)
        delete mImage;

    SharedImage* shared = mSharedImage;
    mImage = nullptr;
    mOwnsImage = false;
    mSharedImage = nullptr;

    if (shared != nullptr && --RefCountOf(shared) == 0 && gSexyAppBase != nullptr)
        gSexyAppBase->mCleanupSharedImages = true;
}

Sexy::SharedImageRef::~SharedImageRef()
{
    Release();
}

SharedImageRef& SharedImageRef::operator=(const SharedImageRef& theOther)
{
    if (this == &theOther)
        return *this;

    SharedImageRef copy(theOther);
    Release();
    mSharedImage = copy.mSharedImage;
    mImage = copy.mImage;
    mOwnsImage = copy.mOwnsImage;
    copy.mSharedImage = nullptr;
    copy.mImage = nullptr;
    copy.mOwnsImage = false;
    return *this;
}

SharedImageRef& SharedImageRef::operator=(Sexy::SharedImage* theSharedImage)
{
    SharedImageRef copy(theSharedImage);
    Release();
    mSharedImage = copy.mSharedImage;
    mImage = copy.mImage;
    mOwnsImage = copy.mOwnsImage;
    copy.mSharedImage = nullptr;
    copy.mImage = nullptr;
    copy.mOwnsImage = false;
    return *this;
}

Sexy::Image* SharedImageRef::operator->() const
{
    return mImage;
}

Sexy::Image& SharedImageRef::operator*() const
{
    return *mImage;
}

SharedImageRef::operator Sexy::Image*() const
{
    return mImage;
}

SharedImageRef::operator Sexy::MemoryImage*() const
{
    return static_cast<Sexy::MemoryImage*>(mImage);
}

SharedImageRef::operator Sexy::DeviceImage*() const
{
    return static_cast<Sexy::DeviceImage*>(mImage);
}
