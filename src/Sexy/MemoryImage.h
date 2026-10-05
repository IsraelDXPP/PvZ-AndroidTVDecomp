// MemoryImage is an Image that is also a software RenderDevice. It owns a
// pixel buffer, optional palette and run-length alpha buffers, a parent, and
// the set of sub-images that share that parent. Drawing writes the pixel
// buffer; a sub-image is a view of a rectangle inside its parent.

#pragma once

#include "ReconTypes.h"
#include "CritSect.h"
#include "Image.h"
#include "PixelData.h"
#include "PixelUtil.h"
#include "SWHelper.h"
#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy {

class MemoryImage;
class SexyMatrix3;
class SexyAppBase;

// RenderDevice is the second base of MemoryImage. The slot order is the
// one MemoryImage's device table uses. Slots marked "not overridden" keep
// the base body; a memory image replaces the rest.
class RenderDevice
{
public:
    struct Span
    {
        int mY;
        int mX;
        int mCount;
    };

    virtual ~RenderDevice();

    virtual int * Get3D();
    virtual bool CanFillPoly();
    virtual bool CreateContext(Image * theImage, const int & theContext);
    virtual void DeleteContext(const int & theContext);
    virtual void SetCurrentContext(const int & theContext);
    virtual int GetCurrentContext() const;
    virtual void PushState();
    virtual void PopState();
    virtual bool IsPixelFormatSupported(PixelFormat theFormat);
    virtual void ClearRect(const TRect<int> & theRect);
    virtual void FillRect(const TRect<int> & theRect, const Color & theColor, int theDrawMode);
    virtual void FillScanLinesWithCoverage(Span * theSpans, int theSpanCount, const Color & theColor,
                                           int theDrawMode, const unsigned char * theCoverage,
                                           int theCoverageX, int theCoverageY, int theCoverageWidth,
                                           int theCoverageHeight);
    virtual void FillPoly(const TPoint<int> * thePoints, int thePointCount, const TRect<int> * theClipRect,
                          const Color & theColor, int theDrawMode, int theTx, int theTy);
    virtual void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY,
                          const Color & theColor, int theDrawMode, bool theAntiAlias);
    virtual void Blt(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect,
                     const Color & theColor, int theDrawMode, bool theColorize = false);
    virtual void BltF(Image * theSource, float theX, float theY, const TRect<int> & theSourceRect,
                      const TRect<int> & theClipRect, const Color & theColor, int theDrawMode);
    virtual void BltRotated(Image * theSource, float theX, float theY, const TRect<int> & theSourceRect,
                            const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                            double theRotate, float theRotateCenterX, float theRotateCenterY);
    virtual void BltMatrix(Image * theSource, float theX, float theY, const SexyMatrix3 & theMatrix,
                           const TRect<int> & theSourceRect, const Color & theColor, int theDrawMode,
                           const TRect<int> & theClipRect, bool theLinearBlend, bool theMirror);
    virtual void BltTriangles(Image * theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount,
                              const Color & theColor, int theDrawMode, float theTx, float theTy, bool theBlend,
                              const TRect<int> * theClipRect);
    virtual void BltMirror(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect,
                           const Color & theColor, int theDrawMode);
    virtual void BltStretched(Image * theSource, const TRect<int> & theDestRect, const TRect<int> & theSrcRect,
                              const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                              bool theFastStretch, bool theMirror);
    virtual void PushTransform(const SexyMatrix3 & theTransform, bool theConcatenate);
    virtual void PopTransform();
    virtual void PopTransform(SexyMatrix3 & theTransform);
    virtual void DrawRect(const TRect<int> & theRect, const Color & theColor, int theDrawMode);
    virtual void FillScanLines(Span * theSpans, int theSpanCount, const Color & theColor, int theDrawMode);
    virtual void TakeScreenshot(MemoryImage & theImage);
};

class MemoryImage : public Image, public RenderDevice
{
public:
    static const unsigned int kMemoryCheckId = 0x4BEEFADEu;

    LawnApp * mApp;
    unsigned int * mPixels;
    unsigned long * mBits;
    int mBitsChangedCount;
    void * mD3DData;
    unsigned int mD3DFlags;
    unsigned int * mColorTable;
    unsigned char * mColorIndices;
    bool mForcedMode;
    bool mHasTrans;
    bool mHasAlpha;
    bool mIsAlphaOnly;
    bool mIsVolatile;
    bool mPurgeBits;
    bool mWantPal;
    unsigned int * mNativeAlphaData;
    unsigned char * mRLAlphaData;
    unsigned char * mRLAdditiveData;
    bool mBitsChanged;
    MemoryImage * mParent;
    int mParentX;
    int mParentY;
    std::set<MemoryImage *> mSubImages;
    Sexy::CritSect mSubImagesLock;
    std::vector<void *> mTriRep[2];

    virtual ~MemoryImage();
    virtual MemoryImage * AsMemoryImage();
    virtual int GetPitch();
    virtual int GetStride();
    virtual PixelFormat GetPixelFormat();
    virtual void Finalize();
    virtual void SetBits(unsigned int * theBits, int theWidth, int theHeight, bool theOwnsBits);
    virtual unsigned int * GetBits();
    virtual unsigned int GetPixel(int theX, int theY);
    virtual unsigned int * GetPixels();
    virtual void FillRect(const TRect<int> & theRect, const Color & theColor, int theDrawMode);
    virtual void ClearRect(const TRect<int> & theRect);
    virtual void DrawLine(double theStartX, double theStartY, double theEndX, double theEndY,
                          const Color & theColor, int theDrawMode);
    virtual void DrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY,
                            const Color & theColor, int theDrawMode);
    virtual void Blt(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect,
                     const Color & theColor, int theDrawMode, bool theColorize = false);
    virtual void BltF(Image * theSource, float theX, float theY, const TRect<int> & theSourceRect,
                      const TRect<int> & theClipRect, const Color & theColor, int theDrawMode);
    virtual void BltRotated(Image * theSource, float theX, float theY, const TRect<int> & theSourceRect,
                            const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                            double theRotate, float theRotateCenterX, float theRotateCenterY);
    virtual void StretchBlt(Image * theSource, const TRect<int> & theDestRect, const TRect<int> & theSrcRect,
                            const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                            bool theFastStretch);
    virtual void BltMatrix(Image * theSource, float theX, float theY, const SexyMatrix3 & theMatrix,
                           const TRect<int> & theSourceRect, const Color & theColor, int theDrawMode,
                           const TRect<int> & theClipRect, bool theLinearBlend, bool theMirror);
    virtual void BltTrianglesTex(Image * theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount,
                                 const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                                 float theTx, float theTy, bool theBlend);
    virtual void BltMirror(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect,
                           const Color & theColor, int theDrawMode);
    virtual void StretchBltMirror(Image * theSource, const TRect<int> & theDestRect,
                                  const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                  const Color & theColor, int theDrawMode, bool theFastStretch);
    virtual void Palletize();

    virtual void * GetNativeAlphaData(int * theDisplay);
    virtual unsigned char * GetRLAlphaData();
    virtual unsigned char * GetRLAdditiveData(int * theDisplay);
    virtual void PurgeBits();
    virtual void DeleteSWBuffers();
    virtual void Delete3DBuffers();
    virtual void DeleteExtraBuffers();
    virtual void ReInit();
    virtual void BitsChanged();
    virtual void CommitBits();
    virtual void DeleteNativeData();
    virtual void Attach(MemoryImage * theSubImage, int theX, int theY, int theWidth, int theHeight);
    virtual void Detach(bool theCopyBits);
    virtual void DetachSubImages(bool theCopyBits);
    virtual MemoryImage * CreateSubImage(int theX, int theY, int theWidth, int theHeight);
    virtual void NormalStretchBltMirror(Image * theSource, const TRect<int> & theDestRect,
                                        const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                        const Color & theColor, bool theFastStretch);
    virtual void AdditiveStretchBltMirror(Image * theSource, const TRect<int> & theDestRect,
                                          const TRect<int> & theSrcRect, const TRect<int> & theClipRect,
                                          const Color & theColor, bool theFastStretch);
    virtual void FillScanLinesWithCoverageRenderDevice(Span * theSpans, int theSpanCount,
                                                       const Color & theColor, int theDrawMode,
                                                       const unsigned char * theCoverage, int theCoverageX,
                                                       int theCoverageY, int theCoverageWidth,
                                                       int theCoverageHeight);
    virtual void Clear();
    virtual void TakeBits(unsigned int * theBits, int theWidth, int theHeight, bool theCommitBits);
    virtual void TakePixels(PixelFormat theFormat, void * thePixels, int theWidth, int theHeight,
                            bool theCommitBits);
    virtual void Create(int theWidth, int theHeight);
    virtual void DrawLineEx(double theStartX, double theStartY, double theEndX, double theEndY,
                            const Color & theColor, int theDrawMode, bool theAntiAlias);
    virtual void BltTriangles(Image * theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount,
                              const Color & theColor, int theDrawMode, float theTx, float theTy,
                              bool theBlend, const TRect<int> * theClipRect);
    virtual void BltStretched(Image * theSource, const TRect<int> & theDestRect, const TRect<int> & theSrcRect,
                              const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                              bool theFastStretch, bool theMirror);
    virtual void SetImageMode(bool theHasTrans, bool theHasAlpha);
    virtual void SetVolatile(bool theIsVolatile);
    virtual void ForcePalletize();
    virtual unsigned int * GetPallete();
    virtual int * Get3D();
    virtual bool CanFillPoly();
    virtual bool CreateContext(Image * theImage, const int & theContext);
    virtual void DeleteContext(const int & theContext);
    virtual void SetCurrentContext(const int & theContext);
    virtual int GetCurrentContext() const;
    virtual void PushState();
    virtual void PopState();
    virtual void FillPoly(const TPoint<int> * thePoints, int thePointCount, const TRect<int> * theClipRect,
                          const Color & theColor, int theDrawMode, int theTx, int theTy);
    virtual void WriteToPng(std::string theFileName);
    virtual void WriteToJPEG(std::string theFileName);

    void Init();
    void NormalDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color & theColor);
    void AdditiveDrawLine(double theStartX, double theStartY, double theEndX, double theEndY, const Color & theColor);
    void NormalDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color & theColor);
    void AdditiveDrawLineAA(double theStartX, double theStartY, double theEndX, double theEndY, const Color & theColor);
    void AdditiveBlt(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect, const Color & theColor);
    void AdditiveBltMirror(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect, const Color & theColor);
    void NormalBlt(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect, const Color & theColor);
    void NormalBltMirror(Image * theSource, int theX, int theY, const TRect<int> & theSourceRect, const Color & theColor);
    bool BltRotatedClipHelper(float & theX, float & theY, const TRect<int> & theSourceRect,
                              const TRect<int> & theClipRect, double theRotate, TRect<float> & theRotatedRect,
                              float theRotateCenterX, float theRotateCenterY);
    bool StretchBltClipHelper(const TRect<int> & theDestRect, const TRect<int> & theSrcRect,
                              const TRect<int> & theClipRect, TRect<float> & theStretchedRect,
                              TRect<int> & theClippedDestRect);
    bool StretchBltMirrorClipHelper(const TRect<int> & theDestRect, const TRect<int> & theSrcRect,
                                    const TRect<int> & theClipRect, TRect<float> & theStretchedRect,
                                    TRect<int> & theClippedDestRect);
    void SlowStretchBlt(Image * theSource, const TRect<int> & theDestRect, const TRect<float> & theSrcRect,
                        const Color & theColor, int theDrawMode, bool theMirror);
    void FastStretchBlt(Image * theSource, const TRect<int> & theDestRect, const TRect<float> & theSrcRect,
                        const Color & theColor, int theDrawMode, bool theMirror);
    void BltMatrixHelper(Image * theSource, float theX, float theY, const SexyMatrix3 & theMatrix,
                         const TRect<int> & theSourceRect, const Color & theColor, int theDrawMode,
                         const TRect<int> & theClipRect, void * theBits, int theStride,
                         int theBlendMode, bool theMirror);
    void BltTrianglesTexHelper(Image * theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount,
                               const TRect<int> & theClipRect, const Color & theColor, int theDrawMode,
                               void * theBits, int theStride, int theBlendMode, float theAlpha, float theScale,
                               bool theAdditive);

    MemoryImage();
    MemoryImage(SexyAppBase * theApp);
    MemoryImage(const MemoryImage & theOther);
    void RemoveSubImage(MemoryImage * theSubImage);
    void AddSubImage(MemoryImage * theSubImage);
};

} // namespace Sexy

using Sexy::MemoryImage;
using Sexy::RenderDevice;
