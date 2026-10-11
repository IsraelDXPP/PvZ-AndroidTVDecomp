// GLImage routes a draw to the live bgfx device when this image is the current
// draw target, and to MemoryImage otherwise. The check is the one in
// Sexy::GLImage::Blt @0x475888: the display's current target takes the
// hardware path, and every other destination stays on the software rasterizer.

#include "GLImage.h"
#include "BgfxRenderDevice.h"
#include <cmath>

namespace
{

Sexy::BgfxRenderDevice::DrawMode ToDeviceMode(int theDrawMode)
{
    if (theDrawMode == 1)
        return Sexy::BgfxRenderDevice::DRAWMODE_ADDITIVE;
    return Sexy::BgfxRenderDevice::DRAWMODE_NORMAL;
}

Sexy::MemoryImage *AsMemory(Sexy::Image *theImage)
{
    if (theImage == nullptr)
        return nullptr;
    return theImage->AsMemoryImage();
}

// Clip a source rect the way Graphics::DrawImage clips before Blt. The
// hardware quad has no software clip of its own, so the intersection is
// applied here and the destination moves with the clipped edge.
bool ClipSource(const Sexy::TRect<int> &theSource, int theX, int theY, const Sexy::TRect<int> &theClip,
    Sexy::TRect<int> &theOutSrc, int &theOutX, int &theOutY)
{
    int aLeft = theX;
    int aTop = theY;
    int aRight = theX + theSource.mWidth;
    int aBottom = theY + theSource.mHeight;
    int aClipRight = theClip.mX + theClip.mWidth;
    int aClipBottom = theClip.mY + theClip.mHeight;
    if (aLeft < theClip.mX)
        aLeft = theClip.mX;
    if (aTop < theClip.mY)
        aTop = theClip.mY;
    if (aRight > aClipRight)
        aRight = aClipRight;
    if (aBottom > aClipBottom)
        aBottom = aClipBottom;
    if (aRight <= aLeft || aBottom <= aTop)
        return false;

    theOutX = aLeft;
    theOutY = aTop;
    theOutSrc =
        Sexy::TRect<int>(theSource.mX + aLeft - theX, theSource.mY + aTop - theY, aRight - aLeft, aBottom - aTop);
    return true;
}

}

Sexy::GLImage::GLImage() :
    MemoryImage()
{
}

Sexy::GLImage::~GLImage() = default;

bool Sexy::GLImage::IsCurrentTarget() const
{
    BgfxRenderDevice *aDevice = BgfxRenderDevice::sCurrent;
    return aDevice != nullptr && aDevice->GetDrawTarget() == this;
}

bool Sexy::GLImage::Is3D()
{
    // GLImage::Is3D @0x4705cc answers the display's accelerated flag when this
    // image is the current target, and 0 otherwise.
    return IsCurrentTarget();
}

void Sexy::GLImage::PushTransform(const SexyMatrix3 &theTransform, bool theConcatenate)
{
    if (IsCurrentTarget())
        BgfxRenderDevice::sCurrent->PushTransform(theTransform, theConcatenate);
}

void Sexy::GLImage::PopTransform()
{
    if (IsCurrentTarget())
        BgfxRenderDevice::sCurrent->PopTransform();
}

void Sexy::GLImage::PopTransform(SexyMatrix3 &theTransform)
{
    if (IsCurrentTarget())
        BgfxRenderDevice::sCurrent->PopTransform(theTransform);
}

void Sexy::GLImage::FillRect(const TRect<int> &theRect, const Color &theColor, int theDrawMode)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::FillRect(theRect, theColor, theDrawMode);
        return;
    }
    BgfxRenderDevice::sCurrent->FillRect(theRect, theColor, ToDeviceMode(theDrawMode));
}

void Sexy::GLImage::ClearRect(const TRect<int> &theRect)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::ClearRect(theRect);
        return;
    }
    BgfxRenderDevice::sCurrent->FillRect(theRect, Color(0, 0, 0, 0), BgfxRenderDevice::DRAWMODE_NORMAL);
}

void Sexy::GLImage::DrawLine(
    double theStartX, double theStartY, double theEndX, double theEndY, const Color &theColor, int theDrawMode)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
        return;
    }
    BgfxRenderDevice::sCurrent->DrawLine(
        (float)theStartX, (float)theStartY, (float)theEndX, (float)theEndY, theColor, ToDeviceMode(theDrawMode));
}

void Sexy::GLImage::DrawLineAA(
    double theStartX, double theStartY, double theEndX, double theEndY, const Color &theColor, int theDrawMode)
{
    DrawLine(theStartX, theStartY, theEndX, theEndY, theColor, theDrawMode);
}

void Sexy::GLImage::Blt(Image *theSource, int theX, int theY, const TRect<int> &theSourceRect, const Color &theColor,
    int theDrawMode, bool theColorize)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::Blt(theSource, theX, theY, theSourceRect, theColor, theDrawMode);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    TRect<int> aClip(0, 0, mWidth, mHeight);
    TRect<int> aSrc;
    int aX = theX;
    int aY = theY;
    if (!ClipSource(theSourceRect, theX, theY, aClip, aSrc, aX, aY))
        return;

    // GLTexture::Blt modulates with the premultiplied glColor. Colorize is the
    // software replace path; the font outline is already in the atlas, so a
    // replace here erases it. The flag stays off on this path.
    (void)theColorize;
    BgfxRenderDevice::sCurrent->DrawImage(aSource,
        TRect<int>(aX, aY, aSrc.mWidth, aSrc.mHeight),
        aSrc,
        theColor,
        ToDeviceMode(theDrawMode),
        false,
        false);
}

void Sexy::GLImage::BltF(Image *theSource, float theX, float theY, const TRect<int> &theSourceRect,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::BltF(theSource, theX, theY, theSourceRect, theClipRect, theColor, theDrawMode);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    TRect<int> aSrc;
    int aX = 0;
    int aY = 0;
    if (!ClipSource(theSourceRect, (int)theX, (int)theY, theClipRect, aSrc, aX, aY))
        return;

    BgfxRenderDevice::sCurrent->DrawImage(aSource,
        TRect<int>(aX, aY, aSrc.mWidth, aSrc.mHeight),
        aSrc,
        theColor,
        ToDeviceMode(theDrawMode),
        false,
        false);
}

void Sexy::GLImage::BltRotated(Image *theSource, float theX, float theY, const TRect<int> &theSourceRect,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, double theRotate, float theRotateCenterX,
    float theRotateCenterY)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::BltRotated(theSource,
            theX,
            theY,
            theSourceRect,
            theClipRect,
            theColor,
            theDrawMode,
            theRotate,
            theRotateCenterX,
            theRotateCenterY);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    // Same placement MemoryImage::BltRotated uses: rotate around the center,
    // then translate by (theX, theY). The device multiplies that matrix.
    float aCos = std::cos((float)theRotate);
    float aSin = std::sin((float)theRotate);
    SexyMatrix3 aMatrix;
    aMatrix.LoadIdentity();
    aMatrix.m00 = aCos;
    aMatrix.m01 = aSin;
    aMatrix.m02 = theRotateCenterX - theRotateCenterX * aCos - theRotateCenterY * aSin;
    aMatrix.m10 = -aSin;
    aMatrix.m11 = aCos;
    aMatrix.m12 = theRotateCenterY + theRotateCenterX * aSin - theRotateCenterY * aCos;
    BltMatrix(theSource, theX, theY, aMatrix, theSourceRect, theColor, theDrawMode, theClipRect, false, false);
}

void Sexy::GLImage::StretchBlt(Image *theSource, const TRect<int> &theDestRect, const TRect<int> &theSrcRect,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, bool theFastStretch)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::StretchBlt(theSource, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, theFastStretch);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    TRect<int> aDest = theDestRect.Intersection(theClipRect);
    if (aDest.mWidth <= 0 || aDest.mHeight <= 0 || theDestRect.mWidth <= 0 || theDestRect.mHeight <= 0)
        return;

    float aScaleX = (float)theSrcRect.mWidth / (float)theDestRect.mWidth;
    float aScaleY = (float)theSrcRect.mHeight / (float)theDestRect.mHeight;
    TRect<int> aSrc(theSrcRect.mX + (int)((float)(aDest.mX - theDestRect.mX) * aScaleX),
        theSrcRect.mY + (int)((float)(aDest.mY - theDestRect.mY) * aScaleY),
        (int)((float)aDest.mWidth * aScaleX),
        (int)((float)aDest.mHeight * aScaleY));
    if (aSrc.mWidth <= 0 || aSrc.mHeight <= 0)
        return;

    // StretchBlt @0x475CB8 uses fastStretch only on the software path.
    // The hardware path goes through BltTransformed with linear filtering.
    BgfxRenderDevice::sCurrent->DrawImage(
        aSource, aDest, aSrc, theColor, ToDeviceMode(theDrawMode), false, false);
}

void Sexy::GLImage::BltMatrix(Image *theSource, float theX, float theY, const SexyMatrix3 &theMatrix,
    const TRect<int> &theSourceRect, const Color &theColor, int theDrawMode, const TRect<int> &theClipRect,
    bool theLinearBlend, bool theMirror)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::BltMatrix(theSource,
            theX,
            theY,
            theMatrix,
            theSourceRect,
            theColor,
            theDrawMode,
            theClipRect,
            theLinearBlend,
            theMirror);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    (void)theLinearBlend;
    (void)theMirror;
    // GLImage::BltTransformed @0x474d7c, theCenter true. GLImage::BltMatrix
    // @0x475e8c always passes that. The matrix is applied to corners measured
    // from the middle of the source rect, then (theX, theY) is added.
    // GLTexture::BltTransformed @0x4734AC clips transformed quads too.
    // This is also the clip used by the seed packet cooldown overlay.
    TRect<int> aScreen(0, 0, mWidth, mHeight);
    TRect<int> aClip = theClipRect.Intersection(aScreen);
    if (aClip.mWidth <= 0 || aClip.mHeight <= 0)
        return;

    BgfxRenderDevice *aDevice = BgfxRenderDevice::sCurrent;
    bool aNeedsClip = aClip != aScreen;
    if (aNeedsClip)
        aDevice->SetClipRect(aClip);
    aDevice->DrawImageMatrix(aSource, theMatrix, theX, theY, theSourceRect, theColor, ToDeviceMode(theDrawMode), true);
    if (aNeedsClip)
        aDevice->ClearClipRect();
}

void Sexy::GLImage::BltTrianglesTex(Image *theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, float theTx, float theTy,
    bool theLinearBlend)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::BltTrianglesTex(theSource,
            theTriangles,
            theTriangleCount,
            theClipRect,
            theColor,
            theDrawMode,
            theTx,
            theTy,
            theLinearBlend);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    // GLImage::BltTrianglesTex (0x475F1C) takes its blend mode from theDrawMode.
    // The last argument selects linear sampling, not additive blending.

    // Vertices are already in screen space, and this clip is the screen clip
    // Graphics passed. Intersect it with the image so a widget clip cannot
    // become a scissor that covers the whole frame.
    TRect<int> aScreen(0, 0, mWidth, mHeight);
    TRect<int> aClip = theClipRect.Intersection(aScreen);
    if (aClip.mWidth <= 0 || aClip.mHeight <= 0)
        aClip = aScreen;
    BgfxRenderDevice *aDevice = BgfxRenderDevice::sCurrent;
    aDevice->SetClipRect(aClip);
    aDevice->DrawTriangles(aSource, theTriangles, theTriangleCount, theColor, ToDeviceMode(theDrawMode), theTx, theTy);
    aDevice->ClearClipRect();
}

void Sexy::GLImage::BltMirror(
    Image *theSource, int theX, int theY, const TRect<int> &theSourceRect, const Color &theColor, int theDrawMode)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::BltMirror(theSource, theX, theY, theSourceRect, theColor, theDrawMode);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    TRect<int> aClip(0, 0, mWidth, mHeight);
    TRect<int> aSrc;
    int aX = theX;
    int aY = theY;
    if (!ClipSource(theSourceRect, theX, theY, aClip, aSrc, aX, aY))
        return;

    BgfxRenderDevice::sCurrent->DrawImage(aSource,
        TRect<int>(aX, aY, aSrc.mWidth, aSrc.mHeight),
        aSrc,
        theColor,
        ToDeviceMode(theDrawMode),
        true,
        false);
}

void Sexy::GLImage::StretchBltMirror(Image *theSource, const TRect<int> &theDestRect, const TRect<int> &theSrcRect,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, bool theFastStretch)
{
    if (!IsCurrentTarget())
    {
        MemoryImage::StretchBltMirror(
            theSource, theDestRect, theSrcRect, theClipRect, theColor, theDrawMode, theFastStretch);
        return;
    }

    MemoryImage *aSource = AsMemory(theSource);
    if (aSource == nullptr)
        return;

    TRect<int> aDest = theDestRect.Intersection(theClipRect);
    if (aDest.mWidth <= 0 || aDest.mHeight <= 0)
        return;

    BgfxRenderDevice::sCurrent->DrawImage(
        aSource, aDest, theSrcRect, theColor, ToDeviceMode(theDrawMode), true, false);
}
