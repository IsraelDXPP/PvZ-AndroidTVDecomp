// DeviceImage -- a MemoryImage backed by a driver surface.
//
// `class DeviceImage : public MemoryImage`. The authoritative header declares
// it empty; the binary adds the driver, the surface flags and the lock
// descriptor after the base.
//
// Overrides of MemoryImage/Image, then this class's own LockSurface,
// UnlockSurface and SetSurface. Every override returns what the base returns.
// MSVC gives the destructor one slot where the binary gives two.

#pragma once

#include "ReconTypes.h"
#include "MemoryImage.h"
#include "SexyAppBase.h"

namespace Sexy {

// The object IGraphicsDriver::CreateDeviceSurface returns. The interface lives
// in DeviceImage.cpp, the only body that calls through it.
class IDeviceSurface;

class DeviceImage : public MemoryImage
{
public:
    virtual ~DeviceImage();

    virtual DeviceImage* AsDeviceImage();
    virtual int GetPitch();
    virtual int GetStride();
    virtual PixelFormat GetPixelFormat();
    virtual unsigned int* GetBits();
    virtual unsigned int GetPixel(int theX, int theY);
    virtual unsigned int* GetPixels();
    virtual void PurgeBits();
    virtual void DeleteExtraBuffers();
    virtual void ReInit();
    virtual void BitsChanged();
    virtual void CommitBits();
    virtual void DeleteNativeData();
    virtual Sexy::DeviceImage* CreateSubImage(int theX, int theY, int theWidth, int theHeight);
    virtual void Create(int theWidth, int theHeight);
    virtual int LockSurface();
    virtual int UnlockSurface();
    virtual int SetSurface(void* theSurface);

    IGraphicsDriver* mDriver;
    bool mHasSurface;
    // Set when the image holds no surface; the pixel accessors then answer nothing.
    bool mNoBits;
    // Set when ReInit should re-create the surface.
    bool mRecreateSurface;
    // Cleared by Init; no other body reads it.
    bool mSurfacePending;
    int mLockCount;
    int mLockInfo[3];
    int mSurfacePitch;
    unsigned int* mSurfaceBits;
    int mSurfaceReserved0[3];
    int mSurfaceBitsPerPixel;
    int mSurfaceReserved1[4];
    int mSurfaceFormat;
    IDeviceSurface* mSurface;

    int Init();
    DeviceImage(Sexy::SexyAppBase* theApp);
    DeviceImage(Sexy::IGraphicsDriver* theDriver);
    DeviceImage();
    int GenerateDeviceSurface();
    void DeleteDeviceSurface();
    void DeleteAllNonSurfaceData();
};

} // namespace Sexy

using Sexy::DeviceImage;
