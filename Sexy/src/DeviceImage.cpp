// DeviceImage is a MemoryImage whose pixels live on a driver surface.
// LockSurface and UnlockSurface bracket access. When the surface is missing,
// unlocked, or shadowed by a software buffer, the accessors use MemoryImage.

#include "DeviceImage.h"
#include "IGraphicsDriver.h"
#include "LawnApp.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

Sexy::DeviceImage *Sexy::DeviceImage::AsDeviceImage()
{
    return this;
}

int Sexy::DeviceImage::SetSurface(void *theSurface)
{
    if (mDriver == nullptr)
        return 0;

    IDeviceSurface *surface = mSurface;
    mHasSurface = true;

    if (surface != nullptr)
    {
        if (theSurface != nullptr && mDriver->GetVersion() == surface->GetVersion())
        {
            surface->SetSurface(theSurface);
            const int result = surface->GetSize(mWidth, mHeight);
            mNoBits = false;
            return result;
        }

        delete surface;
        mSurface = nullptr;
    }

    if (theSurface == nullptr)
    {
        mWidth = 0;
        mHeight = 0;
        mNoBits = true;
        return 0;
    }

    surface = mDriver->CreateDeviceSurface();
    mSurface = surface;
    surface->SetSurface(theSurface);
    const int result = surface->GetSize(mWidth, mHeight);
    mNoBits = false;
    return result;
}

Sexy::DeviceImage::~DeviceImage()
{
    delete mSurface;
    if (mDriver != nullptr)
        mDriver->RemoveDeviceImage(this);
}

int Sexy::DeviceImage::UnlockSurface()
{
    if (mDriver == nullptr || (mDriver->Is3D() && (mCreateFlags & 0x10) != 0))
        return 0;

    --mLockCount;
    if (mLockCount == 0 && mSurface != nullptr)
        mSurface->Unlock();
    return 1;
}

int Sexy::DeviceImage::Init()
{
    mSurface = nullptr;
    int registered = 0;
    if (mDriver != nullptr)
    {
        mDriver->AddDeviceImage(this);
        registered = 1;
    }

    mNoBits = false;
    mRecreateSurface = false;
    mSurfacePending = false;
    mHasSurface = false;
    mLockCount = 0;
    return registered;
}

Sexy::DeviceImage::DeviceImage(Sexy::SexyAppBase *theApp) :
    MemoryImage(theApp)
{
    mDriver = theApp != nullptr ? theApp->mDDInterface : nullptr;
    Init();
}

Sexy::DeviceImage *Sexy::DeviceImage::CreateSubImage(int theX, int theY, int theWidth, int theHeight)
{
    MemoryImage *parent = mParent;
    int originX = theX;
    int originY = theY;
    if (parent != nullptr)
    {
        originX = theX + mParentX;
        originY = theY + mParentY;
    }
    else
    {
        parent = this;
    }

    DeviceImage *subImage = new DeviceImage(mApp);
    parent->Attach(subImage, originX, originY, theWidth, theHeight);
    return subImage;
}

Sexy::DeviceImage::DeviceImage(Sexy::IGraphicsDriver *theDriver) :
    MemoryImage(gSexyAppBase)
{
    mDriver = theDriver;
    Init();
}

Sexy::DeviceImage::DeviceImage() :
    MemoryImage(gSexyAppBase)
{
    mDriver = gSexyAppBase != nullptr ? gSexyAppBase->mDDInterface : nullptr;
    Init();
}

int Sexy::DeviceImage::GenerateDeviceSurface()
{
    if (mSurface != nullptr || mDriver == nullptr)
        return 0;

    IDeviceSurface *surface = mDriver->CreateDeviceSurface();
    if (surface == nullptr)
        return 0;

    surface->mCreateFlags = mCreateFlags;
    const int attached = surface->Attach(this);
    mSurface = surface;
    return attached;
}

int Sexy::DeviceImage::LockSurface()
{
    if (mDriver == nullptr)
        return 0;

    int flags = 0;
    if (mDriver->Is3D())
    {
        flags = mCreateFlags << 27;
        if ((mCreateFlags & 0x10) != 0)
            return 0;
    }

    if (mLockCount == 0)
    {
        if (mSurface == nullptr)
        {
            GenerateDeviceSurface();
            if (mSurface == nullptr)
                return 0;
        }
        if (!mSurface->Lock(mLockInfo, flags))
            return 0;
    }

    ++mLockCount;
    return 1;
}

void Sexy::DeviceImage::DeleteDeviceSurface()
{
    if (mSurface == nullptr)
        return;

    if (mColorTable != nullptr || mBits != nullptr || mPixelsLive != 0 || GetBits() != nullptr)
        delete mSurface;
    mSurface = nullptr;
}

void Sexy::DeviceImage::DeleteAllNonSurfaceData()
{
    delete[] mColorTable;
    mColorTable = nullptr;
    delete[] mNativeAlphaData;
    mNativeAlphaData = nullptr;
    delete[] mRLAdditiveData;
    mRLAdditiveData = nullptr;
    delete[] mRLAlphaData;
    mRLAlphaData = nullptr;
    delete[] mColorIndices;
    mColorIndices = nullptr;
    delete[] mPixels;
    mPixels = nullptr;
}

void Sexy::DeviceImage::ReInit()
{
    MemoryImage::ReInit();
    if (mRecreateSurface)
        GenerateDeviceSurface();
}

void Sexy::DeviceImage::PurgeBits()
{
    if (mHasSurface)
        return;
    mPurgeBits = true;
    MemoryImage::PurgeBits();
}

void Sexy::DeviceImage::DeleteNativeData()
{
    if (mHasSurface)
        return;
    MemoryImage::DeleteNativeData();
    DeleteDeviceSurface();
}

void Sexy::DeviceImage::DeleteExtraBuffers()
{
    if (mHasSurface)
        return;
    MemoryImage::DeleteExtraBuffers();
    DeleteDeviceSurface();
}

void Sexy::DeviceImage::CommitBits()
{
    if (mSurface == nullptr)
        MemoryImage::CommitBits();
}

void Sexy::DeviceImage::Create(int theWidth, int theHeight)
{
    MemoryImage::Create(theWidth, theHeight);
}

void Sexy::DeviceImage::BitsChanged()
{
    if (mSurface == nullptr || mLockCount == 0 || mBits != nullptr || mColorIndices != nullptr || mPixels != nullptr)
    {
        MemoryImage::BitsChanged();
        delete mSurface;
        mSurface = nullptr;
    }
}

Sexy::PixelFormat Sexy::DeviceImage::GetPixelFormat()
{
    if (mSurface == nullptr || mLockCount == 0 || mBits != nullptr || mColorIndices != nullptr || mPixels != nullptr)
        return MemoryImage::GetPixelFormat();
    return (PixelFormat)mSurfaceFormat;
}

unsigned int *Sexy::DeviceImage::GetBits()
{
    if (mNoBits)
        return nullptr;
    if (mSurface == nullptr || mBits != nullptr)
        return MemoryImage::GetBits();
    if (mLockCount == 0 || mSurfaceFormat != 2)
    {
        mBits = (unsigned long *)mSurface->GetBits(this);
        mFormat = (PixelFormat)2;
        return MemoryImage::GetBits();
    }
    return mSurfaceBits;
}

unsigned int *Sexy::DeviceImage::GetPixels()
{
    if (mNoBits)
        return nullptr;
    if (mSurface == nullptr || mLockCount == 0 || mBits != nullptr || mColorIndices != nullptr || mPixels != nullptr)
        return MemoryImage::GetPixels();
    return mSurfaceBits;
}

int Sexy::DeviceImage::GetPitch()
{
    if (mNoBits)
        return 0;
    if (mSurface == nullptr || mLockCount == 0 || mBits != nullptr || mColorIndices != nullptr || mPixels != nullptr)
        return MemoryImage::GetPitch();
    if (mSurfaceBitsPerPixel < 8)
        return 0;
    return mSurfacePitch / (mSurfaceBitsPerPixel >> 3);
}

int Sexy::DeviceImage::GetStride()
{
    if (mNoBits)
        return 0;
    if (mSurface == nullptr || mLockCount == 0 || mBits != nullptr || mColorIndices != nullptr || mPixels != nullptr)
        return MemoryImage::GetStride();
    return mSurfacePitch;
}

unsigned int Sexy::DeviceImage::GetPixel(int theX, int theY)
{
    if (mParent != nullptr)
        return mParent->GetPixel(mParentX + theX, mParentY + theY);
    return (unsigned int)ReadDevicePixel(this, theX, theY);
}
