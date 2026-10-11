// BgfxRenderBackend.cpp -- backend implementations and the registry.

#include "BgfxRenderBackend.h"
#include "BgfxRenderDevice.h"
#include "ReconFreeFunctions.h"
#include "MemoryImage.h"
#include "Graphics.h"
#include "SexyTypes.h"
#include "AndroidShims.h"
#include "SexyString.h"
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

// The draw modes Graphics passes down as a plain int.
//
// Graphics.h declares SetDrawMode/GetDrawMode but carries no enumerators, and
// the backend interface takes the mode as an int. MemoryImage::Blt and
// StretchBlt dispatch on exactly these values (drawMode == 1 selects the
// additive blitter, see MemoryImage::Blt @0x321828), so the two names are
// spelled out here.
// TODO(recon): Sexy::Graphics wants DRAWMODE_NORMAL/DRAWMODE_ADDITIVE in
// Graphics.h; these local names stand in until then.
enum
{
    kDrawMode_Normal = 0,
    kDrawMode_Additive = 1
};

// ===========================================================================
// SoftwareBackend
// ===========================================================================

SoftwareBackend::SoftwareBackend() :
    mTarget(nullptr),
    mOwnTarget(false)
{
}

SoftwareBackend::~SoftwareBackend()
{
    Shutdown();
}

void SoftwareBackend::SetTarget(MemoryImage *theTarget, bool theOwn)
{
    if (mOwnTarget && mTarget && mTarget != theTarget)
        delete mTarget;
    mTarget = theTarget;
    mOwnTarget = theOwn;
}

bool SoftwareBackend::Init(int theWidth, int theHeight)
{
    if (!mTarget)
    {
        mTarget = new MemoryImage();
        mTarget->Create(theWidth, theHeight);
        mOwnTarget = true;
    }
    return mTarget->mBits != nullptr;
}

void SoftwareBackend::Shutdown()
{
    if (mOwnTarget)
        delete mTarget;
    mTarget = nullptr;
    mOwnTarget = false;
}

void SoftwareBackend::Resize(int theWidth, int theHeight)
{
    // MemoryImage exposes the dimensions as the Image data members mWidth and
    // mHeight (Image.h +0x24/+0x28); they are not accessor functions.
    if (mTarget && (mTarget->mWidth != theWidth || mTarget->mHeight != theHeight))
        mTarget->Create(theWidth, theHeight);
}

void SoftwareBackend::BeginFrame(const Color &theClearColor)
{
    if (!mTarget)
        return;
    // The original clears via Graphics before drawing; match that.
    mTarget->FillRect(Rect(0, 0, mTarget->mWidth, mTarget->mHeight), theClearColor, kDrawMode_Normal);
}

void SoftwareBackend::EndFrame() {}
void SoftwareBackend::Present() {}

void SoftwareBackend::SetDrawMode(int) {}
void SoftwareBackend::SetColorizeImages(bool) {}
void SoftwareBackend::SetClipRect(const Rect &) {}
void SoftwareBackend::ClearClipRect() {}
void SoftwareBackend::SetTransform(const SexyMatrix3 &) {}

void SoftwareBackend::DrawImage(MemoryImage *theImage, const Rect &theDestRect, const Rect &theSrcRect,
    const Color &theColor, int theDrawMode, bool theMirror)
{
    if (!mTarget || !theImage)
        return;

    // MemoryImage.h declares Blt/BltMirror/StretchBlt with the framework's
    // real types (Image *, int, int, TRect<int> const&, Color const&, int), so
    // the image and the rects are passed through unchanged -- the old raw
    // (_BYTE *)/(int *) casts were the decompiler's, not the header's.
    if (theMirror)
    {
        mTarget->BltMirror(theImage, theDestRect.mX, theDestRect.mY, theSrcRect, theColor, theDrawMode);
        return;
    }

    if (theDestRect.mWidth == theSrcRect.mWidth && theDestRect.mHeight == theSrcRect.mHeight)
    {
        mTarget->Blt(theImage, theDestRect.mX, theDestRect.mY, theSrcRect, theColor, theDrawMode);
    }
    else
    {
        mTarget->StretchBlt(theImage,
            theDestRect,
            theSrcRect,
            Rect(0, 0, mTarget->mWidth, mTarget->mHeight),
            theColor,
            theDrawMode,
            true);
    }
}

void SoftwareBackend::FillRect(const Rect &theRect, const Color &theColor, int theDrawMode)
{
    if (mTarget)
        mTarget->FillRect(theRect, theColor, theDrawMode);
}

bool SoftwareBackend::ReadPixels(MemoryImage &theTarget)
{
    if (!mTarget)
        return false;
    theTarget.Create(mTarget->mWidth, mTarget->mHeight);
    if (!theTarget.mBits)
        return false;
    memcpy(theTarget.mBits, mTarget->mBits, (size_t)mTarget->mWidth * mTarget->mHeight * 4);
    return true;
}

// ===========================================================================
// BgfxBackend
// ===========================================================================

BgfxBackend::BgfxBackend() :
    mDevice(nullptr)
{
}

BgfxBackend::~BgfxBackend()
{
    Shutdown();
}

bool BgfxBackend::Init(int theWidth, int theHeight)
{
    mDevice = new BgfxRenderDevice();
    if (!mDevice->Init(theWidth, theHeight))
    {
        delete mDevice;
        mDevice = nullptr;
        return false;
    }
    return true;
}

void BgfxBackend::Shutdown()
{
    if (mDevice)
    {
        mDevice->Shutdown();
        delete mDevice;
        mDevice = nullptr;
    }
}

void BgfxBackend::Resize(int theWidth, int theHeight)
{
    if (mDevice)
        mDevice->Resize(theWidth, theHeight);
}

void BgfxBackend::BeginFrame(const Color &theClearColor)
{
    if (mDevice)
        mDevice->BeginFrame(theClearColor);
}

void BgfxBackend::EndFrame()
{
    if (mDevice)
        mDevice->EndFrame();
}

void BgfxBackend::Present()
{
    if (mDevice)
        mDevice->Present();
}

void BgfxBackend::SetDrawMode(int theDrawMode)
{
    if (!mDevice)
        return;
    mDevice->SetDrawMode(
        theDrawMode == kDrawMode_Additive ? BgfxRenderDevice::DRAWMODE_ADDITIVE : BgfxRenderDevice::DRAWMODE_NORMAL);
}

void BgfxBackend::SetColorizeImages(bool theColorize)
{
    if (mDevice)
        mDevice->SetColorizeImages(theColorize);
}

void BgfxBackend::SetClipRect(const Rect &theRect)
{
    if (mDevice)
        mDevice->SetClipRect(theRect);
}

void BgfxBackend::ClearClipRect()
{
    if (mDevice)
        mDevice->ClearClipRect();
}

void BgfxBackend::SetTransform(const SexyMatrix3 &theTransform)
{
    if (mDevice)
        mDevice->SetTransform(theTransform);
}

void BgfxBackend::DrawImage(MemoryImage *theImage, const Rect &theDestRect, const Rect &theSrcRect,
    const Color &theColor, int theDrawMode, bool theMirror)
{
    if (!mDevice)
        return;
    mDevice->DrawImage(theImage,
        theDestRect,
        theSrcRect,
        theColor,
        theDrawMode == kDrawMode_Additive ? BgfxRenderDevice::DRAWMODE_ADDITIVE : BgfxRenderDevice::DRAWMODE_NORMAL,
        theMirror);
}

void BgfxBackend::FillRect(const Rect &theRect, const Color &theColor, int theDrawMode)
{
    if (!mDevice)
        return;
    mDevice->FillRect(theRect,
        theColor,
        theDrawMode == kDrawMode_Additive ? BgfxRenderDevice::DRAWMODE_ADDITIVE : BgfxRenderDevice::DRAWMODE_NORMAL);
}

bool BgfxBackend::ReadPixels(MemoryImage &theTarget)
{
    return mDevice ? mDevice->ReadPixels(theTarget) : false;
}

// ===========================================================================
// registry
// ===========================================================================

Sexy::String GraphicsBackendFactory::GetRequestedName()
{
    const char *setting = Sexy::GetEnv("SEXY_VIDEO_DRIVER");
    return setting && *setting ? setting : "auto";
}
IGraphicsBackend *GraphicsBackendFactory::Create(const Sexy::String &theName)
{
    if (theName == "software" || theName == "sw")
        return new SoftwareBackend();
    return new BgfxBackend();
}
} // namespace Sexy
