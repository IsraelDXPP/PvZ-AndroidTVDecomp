// GLImage is the hardware surface. It is a MemoryImage so the software
// rasterizer is still there, and every draw asks the live GL device whether
// this image is the current draw target.
//
// That is the split in Sexy::GLImage::Blt @0x475888 and FillRect @0x475030:
// the display's current target takes the GLES path, and every other image
// falls through to MemoryImage. A cache canvas, a font atlas and a particle
// buffer stay software. The screen image does not.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_GLIMAGE_H
#define LAWNPROJECTTV_SEXY_INCLUDE_GLIMAGE_H

#include "MemoryImage.h"

namespace Sexy
{

class GLImage : public MemoryImage
{
public:
    GLImage();
    virtual ~GLImage();

    virtual bool Is3D();
    void PushTransform(const SexyMatrix3 &theTransform, bool theConcatenate) override;
    void PopTransform() override;
    void PopTransform(SexyMatrix3 &theTransform) override;
    virtual void FillRect(const TRect<int> &theRect, const Color &theColor, int theDrawMode);
    virtual void ClearRect(const TRect<int> &theRect);
    virtual void DrawLine(
        double theStartX, double theStartY, double theEndX, double theEndY, const Color &theColor, int theDrawMode);
    virtual void DrawLineAA(
        double theStartX, double theStartY, double theEndX, double theEndY, const Color &theColor, int theDrawMode);
    virtual void Blt(Image *theSource, int theX, int theY, const TRect<int> &theSourceRect, const Color &theColor,
        int theDrawMode, bool theColorize = false);
    virtual void BltF(Image *theSource, float theX, float theY, const TRect<int> &theSourceRect,
        const TRect<int> &theClipRect, const Color &theColor, int theDrawMode);
    virtual void BltRotated(Image *theSource, float theX, float theY, const TRect<int> &theSourceRect,
        const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, double theRotate, float theRotateCenterX,
        float theRotateCenterY);
    virtual void StretchBlt(Image *theSource, const TRect<int> &theDestRect, const TRect<int> &theSrcRect,
        const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, bool theFastStretch);
    virtual void BltMatrix(Image *theSource, float theX, float theY, const SexyMatrix3 &theMatrix,
        const TRect<int> &theSourceRect, const Color &theColor, int theDrawMode, const TRect<int> &theClipRect,
        bool theLinearBlend, bool theMirror);
    virtual void BltTrianglesTex(Image *theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount,
        const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, float theTx, float theTy,
        bool theLinearBlend);
    virtual void BltMirror(
        Image *theSource, int theX, int theY, const TRect<int> &theSourceRect, const Color &theColor, int theDrawMode);
    virtual void StretchBltMirror(Image *theSource, const TRect<int> &theDestRect, const TRect<int> &theSrcRect,
        const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, bool theFastStretch);

    // True only while this image is the device's current draw target. That is
    // the same question GLImage asks the display before it emits a primitive.
    bool IsCurrentTarget() const;
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_GLIMAGE_H
