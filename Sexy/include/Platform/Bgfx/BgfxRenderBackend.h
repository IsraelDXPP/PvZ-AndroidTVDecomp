// BgfxRenderBackend.h -- bridges Sexy::Graphics to the bgfx device.
//
// The binary selects a graphics backend through a *driver registry*
// (Sexy::VideoDriverFactory, populated from .init_array; the Android driver
// registers as "Android" with priority 10). BaseAppDriver::CreateGraphics()
// reads the SEXY_VIDEO_DRIVER environment variable and instantiates the match.
//
// We reproduce that shape so the same game code can run either way:
//
//     IGraphicsBackend                 (what Graphics talks to)
//       +-- SoftwareBackend            (MemoryImage blitters)
//       +-- BgfxBackend                  (BgfxRenderDevice, GPU)
//
// Graphics stays backend-agnostic: it transforms coordinates and forwards to
// Image primitives, which in turn forward here. That is exactly the split the
// original has between Graphics/Image and RenderDevice.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_BGFX_BGFXRENDERBACKEND_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_BGFX_BGFXRENDERBACKEND_H

#include "CoreTypes.h"
#include "SexyString.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class MemoryImage;
class BgfxRenderDevice;

// ---------------------------------------------------------------------------
// Backend interface
// ---------------------------------------------------------------------------
class IGraphicsBackend
{
public:
    virtual ~IGraphicsBackend() {}

    virtual bool Init(int theWidth, int theHeight) = 0;
    virtual void Shutdown() = 0;
    virtual void Resize(int theWidth, int theHeight) = 0;

    virtual void BeginFrame(const Color &theClearColor) = 0;
    virtual void EndFrame() = 0;
    virtual void Present() = 0;

    virtual void SetDrawMode(int theDrawMode) = 0;
    virtual void SetColorizeImages(bool theColorize) = 0;
    virtual void SetClipRect(const Rect &theRect) = 0;
    virtual void ClearClipRect() = 0;
    virtual void SetTransform(const SexyMatrix3 &theTransform) = 0;

    virtual void DrawImage(MemoryImage *theImage, const Rect &theDestRect, const Rect &theSrcRect,
        const Color &theColor, int theDrawMode, bool theMirror = false) = 0;
    virtual void FillRect(const Rect &theRect, const Color &theColor, int theDrawMode) = 0;

    // Read the composited frame back as 0xAARRGGBB pixels.
    virtual bool ReadPixels(MemoryImage &theTarget) = 0;

    virtual const char *GetName() const = 0;
};

// ---------------------------------------------------------------------------
// Software backend -- wraps a MemoryImage target.
//
// This is the reference implementation: every other backend must match it
// pixel-for-pixel, which the tests assert.
// ---------------------------------------------------------------------------
class SoftwareBackend : public IGraphicsBackend
{
public:
    MemoryImage *mTarget;
    bool mOwnTarget;

public:
    SoftwareBackend();
    virtual ~SoftwareBackend();

    // Use an externally owned surface (e.g. the window's back buffer).
    void SetTarget(MemoryImage *theTarget, bool theOwn = false);

    virtual bool Init(int theWidth, int theHeight) override;
    virtual void Shutdown() override;
    virtual void Resize(int theWidth, int theHeight) override;

    virtual void BeginFrame(const Color &theClearColor) override;
    virtual void EndFrame() override;
    virtual void Present() override;

    virtual void SetDrawMode(int theDrawMode) override;
    virtual void SetColorizeImages(bool theColorize) override;
    virtual void SetClipRect(const Rect &theRect) override;
    virtual void ClearClipRect() override;
    virtual void SetTransform(const SexyMatrix3 &theTransform) override;

    virtual void DrawImage(MemoryImage *theImage, const Rect &theDestRect, const Rect &theSrcRect,
        const Color &theColor, int theDrawMode, bool theMirror = false) override;
    virtual void FillRect(const Rect &theRect, const Color &theColor, int theDrawMode) override;

    virtual bool ReadPixels(MemoryImage &theTarget) override;
    virtual const char *GetName() const override
    {
        return "Software";
    }
};

// ---------------------------------------------------------------------------
// bgfx backend
// ---------------------------------------------------------------------------
class BgfxBackend : public IGraphicsBackend
{
public:
    BgfxRenderDevice *mDevice;

public:
    BgfxBackend();
    virtual ~BgfxBackend();

    BgfxRenderDevice *GetDevice()
    {
        return mDevice;
    }

    virtual bool Init(int theWidth, int theHeight) override;
    virtual void Shutdown() override;
    virtual void Resize(int theWidth, int theHeight) override;

    virtual void BeginFrame(const Color &theClearColor) override;
    virtual void EndFrame() override;
    virtual void Present() override;

    virtual void SetDrawMode(int theDrawMode) override;
    virtual void SetColorizeImages(bool theColorize) override;
    virtual void SetClipRect(const Rect &theRect) override;
    virtual void ClearClipRect() override;
    virtual void SetTransform(const SexyMatrix3 &theTransform) override;

    virtual void DrawImage(MemoryImage *theImage, const Rect &theDestRect, const Rect &theSrcRect,
        const Color &theColor, int theDrawMode, bool theMirror = false) override;
    virtual void FillRect(const Rect &theRect, const Color &theColor, int theDrawMode) override;

    virtual bool ReadPixels(MemoryImage &theTarget) override;
    virtual const char *GetName() const override
    {
        return "bgfx";
    }
};

// ---------------------------------------------------------------------------
// Backend registry -- mirrors Sexy::VideoDriverFactory / DriverFactory::Find.
// ---------------------------------------------------------------------------
class GraphicsBackendFactory
{
public:
    // "software" | "gl" | "auto". "auto" picks GL when a context is available.
    // The binary reads SEXY_VIDEO_DRIVER for the same purpose.
    static IGraphicsBackend *Create(const Sexy::String &theName);

    // Read the SEXY_VIDEO_DRIVER environment variable (default "auto").
    static Sexy::String GetRequestedName();
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_BGFX_BGFXRENDERBACKEND_H

// The global spelling, which the transcribed bodies use.
using Sexy::MemoryImage;
