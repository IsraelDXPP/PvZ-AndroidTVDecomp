// RenderDevice -- reconstructed from libGameMain.so.
//
// The 2D render-device interface, and the SECOND base of Sexy::MemoryImage
// (MemoryImage.h: `class MemoryImage : public Image, public RenderDevice`).
// There was no .cpp for it, which is why every one of its out-of-line members
// was unresolved at once: MemoryImage.obj emits RenderDevice's own vftable
// (`??_7RenderDevice@Sexy@@6B@`) because MemoryImage's constructor stores the
// RenderDevice sub-object vptr at +0x6C, and that table points at each member
// below, so the linker wants all of them from the single object file.
//
// WHAT THE BINARY ACTUALLY HAS
//
// `_ZTVN4Sexy12RenderDeviceE` is at 0x68A370; slot k is at `0x68A370 + 8 + 4k`
// (offset-to-top at +0, typeinfo at +4). Reading it out gives 29 slots and
// splits them in two:
//
//   slot  byte  address     method                      body in the ARM?
//   [ 0]     0  0x307a64    ~RenderDevice               yes -- vptr store only
//   [ 2]  0x08  0x2c50d8    Get3D                       __cxa_pure_virtual
//   [ 3]  0x0C  0x2c50d8    CanFillPoly                 __cxa_pure_virtual
//   [ 4]  0x10  0x2c50d8    CreateContext               __cxa_pure_virtual
//   [ 5]  0x14  0x2c50d8    DeleteContext               __cxa_pure_virtual
//   [ 6]  0x18  0x2c50d8    SetCurrentContext           __cxa_pure_virtual
//   [ 7]  0x1C  0x2c50d8    GetCurrentContext           __cxa_pure_virtual
//   [ 8]  0x20  0x2c50d8    PushState                   __cxa_pure_virtual
//   [ 9]  0x24  0x2c50d8    PopState                    __cxa_pure_virtual
//   [10]  0x28  0x307a7c    IsPixelFormatSupported      yes -- 12 bytes
//   [11]  0x2C  0x2c50d8    ClearRect                   __cxa_pure_virtual
//   [12]  0x30  0x2c50d8    FillRect                    __cxa_pure_virtual
//   [13]  0x34  0x2c50d8    FillScanLinesWithCoverage   __cxa_pure_virtual
//   [14]  0x38  0x2c50d8    FillPoly                    __cxa_pure_virtual
//   [15]  0x3C  0x2c50d8    DrawLine                    __cxa_pure_virtual
//   [16]  0x40  0x2c50d8    Blt                         __cxa_pure_virtual
//   [17]  0x44  0x2c50d8    BltF                        __cxa_pure_virtual
//   [18]  0x48  0x2c50d8    BltRotated                  __cxa_pure_virtual
//   [19]  0x4C  0x2c50d8    BltMatrix                   __cxa_pure_virtual
//   [20]  0x50  0x2c50d8    BltTriangles                __cxa_pure_virtual
//   [21]  0x54  0x2c50d8    BltMirror                   __cxa_pure_virtual
//   [22]  0x58  0x2c50d8    BltStretched                __cxa_pure_virtual
//   [23]  0x5C  0x307a88    PushTransform               yes -- `BX LR`, empty
//   [24]  0x60  0x307a8c    PopTransform                yes -- `BX LR`, empty
//   [25]  0x64  0x307a90    PopTransform(matrix out)    yes -- `BX LR`, empty
//   [26]  0x68  0x307a94    DrawRect                    yes -- 182 bytes
//   [27]  0x6C  0x307b4c    FillScanLines               yes -- 82 bytes
//   [28]  0x70  0x307ba0    TakeScreenshot              yes -- 4 bytes, empty
//   [29]        (end of table)
//
// The same 29 slots, with the same bodies, are what the secondary vtable
// MemoryImage's constructor installs at +0x6C points at:
// slots 2..22 there are `_ZThn108_` MemoryImage thunks (108 = 0x6C, the Itanium
// offset-to-top), and slots 23..28 are these same `Sexy::RenderDevice::` bodies
// verbatim. So the seven real bodies below are shared between the interface's
// own table and every MemoryImage's device table.
//
// THE TWENTY PURE SLOTS
//
// Twenty of the twenty-eight methods hold `__cxa_pure_virtual` @0x2C50D8 in the
// ARM, i.e. the original declares them pure and provides no body at all. This
// tree's `MemoryImage.h` declares them NON-pure, and a non-pure virtual that
// MemoryImage.obj's vftable points at still has to be defined, so each gets an
// empty definition below carrying the vtable slot address it comes from.
//
// Nothing can reach them: a bare RenderDevice is never constructed (the
// framework's fallback device is the static NullRenderDevice, whose vptr is
// the hand-built table in ReconTables.cpp, not this C++ vtable), and every
// real device -- MemoryImage's sub-object and the GL device family --
// overrides the whole set.
//
// TODO(recon): the faithful fix is `= 0` on those twenty declarations in
// Sexy\MemoryImage.h, which makes the class abstract exactly as the ARM has it.
// That is a header change and is reported rather than made here; MemoryImage
// overrides all twenty, so it would stay concrete.

#include "MemoryImage.h"

#include "Image.h"

// Span is already complete on RenderDevice in MemoryImage.h. FillScanLines
// below walks that 12-byte record: position, then the covered pixel count.

// ==========================================================================
// Sexy::RenderDevice::~RenderDevice()
// @00307A64  14 bytes
// ==========================================================================
Sexy::RenderDevice::~RenderDevice()
{
    // The body is the compiler's own vptr store (`LDR R3, =_ZTVN4Sexy12RenderDeviceE
    // + 8; STR R3, [R0]`) and nothing else: RenderDevice is a pure interface with
    // no data members and no owned resources. A vptr store is what the compiler
    // emits and is not something to transcribe, so the body is empty.
}

// ==========================================================================
// Sexy::RenderDevice::Get3D()
// @0x68A380 (vtable slot [ 2])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
int *Sexy::RenderDevice::Get3D()
{
    // The binary holds __cxa_pure_virtual @0x2C50D8 in this slot, so there is no
    // ARM body to read. Declared non-pure by MemoryImage.h, so the definition is
    // required for the base vftable; MemoryImage::Get3D [68] overrides it.
    return nullptr;
}

// ==========================================================================
// Sexy::RenderDevice::CanFillPoly()
// @0x68A384 (vtable slot [ 3])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
bool Sexy::RenderDevice::CanFillPoly()
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::CanFillPoly [69] overrides it.
    return false;
}

// ==========================================================================
// Sexy::RenderDevice::CreateContext(Sexy::Image *,int const&)
// @0x68A388 (vtable slot [ 4])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
bool Sexy::RenderDevice::CreateContext(Image *theImage, const int &theContext)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::CreateContext [70] overrides it.
    (void)theImage;
    (void)theContext;
    return false;
}

// ==========================================================================
// Sexy::RenderDevice::DeleteContext(int const&)
// @0x68A38C (vtable slot [ 5])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::DeleteContext(const int &theContext)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::DeleteContext [71] overrides it.
    (void)theContext;
}

// ==========================================================================
// Sexy::RenderDevice::SetCurrentContext(int const&)
// @0x68A390 (vtable slot [ 6])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::SetCurrentContext(const int &theContext)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::SetCurrentContext [72] overrides it.
    (void)theContext;
}

// ==========================================================================
// Sexy::RenderDevice::GetCurrentContext() const
// @0x68A394 (vtable slot [ 7])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
int Sexy::RenderDevice::GetCurrentContext() const
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::GetCurrentContext [73] overrides it.
    return 0;
}

// ==========================================================================
// Sexy::RenderDevice::PushState()
// @0x68A398 (vtable slot [ 8])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::PushState()
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::PushState [74] overrides it.
}

// ==========================================================================
// Sexy::RenderDevice::PopState()
// @0x68A39C (vtable slot [ 9])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::PopState()
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::PopState [75] overrides it.
}

// ==========================================================================
// Sexy::RenderDevice::IsPixelFormatSupported(Sexy::PixelFormat)
// @00307A7C  12 bytes
// ==========================================================================
// One of the four slots the interface itself implements, and the only one whose
// body is a plain predicate:
//
//     SUBS R1, #1          ; R1 = theFormat - 1
//     CMP  R1, #1
//     ITE  HI
//     MOVHI R0, #0         ; unsigned (theFormat - 1) > 1  -> false
//     MOVLS R0, #1         ; unsigned (theFormat - 1) <= 1 -> true
//     BX   LR
//
// The comparison is UNSIGNED, so exactly two values answer true: 1 and 2. In the
// binary's own enum (recon-src\libGameMain115.h:7742, and the identical
// PvZ\SexyAppFramework\Misc\Common.h) those are kPixelFormat_RGBA8888 and
// kPixelFormat_RGBA4444 -- the two 32-bit and 16-bit RGBA layouts the software
// path can blit directly. kPixelFormat_None is -1 and kPixelFormat_Automatic is
// 0; both fall out as false, which is the point of the unsigned test.
//
// The comparison is written against the enum's numeric values rather than its
// names because Sexy::PixelFormat is a one-int surrogate struct in this tree
// (ReconTypes.h:1416) and the kPixelFormat_* names live only in the reference
// headers under PvZ\, which are not on this project's include path.
bool Sexy::RenderDevice::IsPixelFormatSupported(PixelFormat theFormat)
{
    const int aFormat = (int)theFormat;
    return (unsigned int)(aFormat - 1) <= 1u;
}

// ==========================================================================
// Sexy::RenderDevice::ClearRect(Sexy::TRect<int> const&)
// @0x68A3A4 (vtable slot [11])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::ClearRect(const TRect<int> &theRect)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::ClearRect [19] overrides it.
    (void)theRect;
}

// ==========================================================================
// Sexy::RenderDevice::FillRect(Sexy::TRect<int> const&,Sexy::Color const&,int)
// @0x68A3A8 (vtable slot [12])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::FillRect(const TRect<int> &theRect, const Color &theColor, int theDrawMode)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::FillRect [17] overrides it -- and it is also the slot
    // DrawRect and FillScanLines below dispatch their work through.
    (void)theRect;
    (void)theColor;
    (void)theDrawMode;
}

// ==========================================================================
// Sexy::RenderDevice::FillScanLinesWithCoverage(Sexy::RenderDevice::Span *,int,Sexy::Color const&,int,unsigned char const*,int,int,int,int)
// @0x68A3AC (vtable slot [13])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::FillScanLinesWithCoverage(Span *theSpans, int theSpanCount, const Color &theColor,
    int theDrawMode, const unsigned char *theCoverage, int theCoverageX, int theCoverageY, int theCoverageWidth,
    int theCoverageHeight)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage implements it as
    // FillScanLinesWithCoverageRenderDevice [56], which is why that method carries
    // the "RenderDevice" suffix -- the two take the same RenderDevice::Span but
    // land in different slots of the two vtables.
    (void)theSpans;
    (void)theSpanCount;
    (void)theColor;
    (void)theDrawMode;
    (void)theCoverage;
    (void)theCoverageX;
    (void)theCoverageY;
    (void)theCoverageWidth;
    (void)theCoverageHeight;
}

// ==========================================================================
// Sexy::RenderDevice::FillPoly(Sexy::TPoint<int> const*,int,Sexy::TRect<int> const*,Sexy::Color const&,int,int,int)
// @0x68A3B0 (vtable slot [14])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::FillPoly(const TPoint<int> *thePoints, int thePointCount, const TRect<int> *theClipRect,
    const Color &theColor, int theDrawMode, int theTx, int theTy)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::FillPoly [76] overrides it.
    (void)thePoints;
    (void)thePointCount;
    (void)theClipRect;
    (void)theColor;
    (void)theDrawMode;
    (void)theTx;
    (void)theTy;
}

// ==========================================================================
// Sexy::RenderDevice::DrawLine(double,double,double,double,Sexy::Color const&,int,bool)
// @0x68A3B4 (vtable slot [15])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::DrawLine(double theStartX, double theStartY, double theEndX, double theEndY,
    const Color &theColor, int theDrawMode, bool theAntiAlias)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::DrawLine [20] and DrawLineAA [21] override the two
    // aliasing modes; Graphics::DrawLine dispatches here through mRenderDevice.
    (void)theStartX;
    (void)theStartY;
    (void)theEndX;
    (void)theEndY;
    (void)theColor;
    (void)theDrawMode;
    (void)theAntiAlias;
}

// ==========================================================================
// Sexy::RenderDevice::Blt(Sexy::Image *,int,int,Sexy::TRect<int> const&,Sexy::Color const&,int)
// @0x68A3B8 (vtable slot [16])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::Blt(Image *theSource, int theX, int theY, const TRect<int> &theSourceRect,
    const Color &theColor, int theDrawMode, bool theColorize)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::Blt [24] overrides it; the binary's other
    // implementations are BltF (below) and the GL device's.
    (void)theSource;
    (void)theX;
    (void)theY;
    (void)theSourceRect;
    (void)theColor;
    (void)theDrawMode;
    (void)theColorize;
}

// ==========================================================================
// Sexy::RenderDevice::BltF(Sexy::Image *,float,float,Sexy::TRect<int> const&,Sexy::TRect<int> const&,Sexy::Color const&,int)
// @0x68A3BC (vtable slot [17])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::BltF(Image *theSource, float theX, float theY, const TRect<int> &theSourceRect,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::BltF [25] overrides it.
    (void)theSource;
    (void)theX;
    (void)theY;
    (void)theSourceRect;
    (void)theClipRect;
    (void)theColor;
    (void)theDrawMode;
}

// ==========================================================================
// Sexy::RenderDevice::BltRotated(Sexy::Image *,float,float,Sexy::TRect<int> const&,Sexy::TRect<int> const&,Sexy::Color const&,int,double,float,float)
// @0x68A3C0 (vtable slot [18])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::BltRotated(Image *theSource, float theX, float theY, const TRect<int> &theSourceRect,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, double theRotate, float theRotateCenterX,
    float theRotateCenterY)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::BltRotated [26] overrides it.
    (void)theSource;
    (void)theX;
    (void)theY;
    (void)theSourceRect;
    (void)theClipRect;
    (void)theColor;
    (void)theDrawMode;
    (void)theRotate;
    (void)theRotateCenterX;
    (void)theRotateCenterY;
}

// ==========================================================================
// Sexy::RenderDevice::BltMatrix(Sexy::Image *,float,float,SexyMatrix3 const&,Sexy::TRect<int> const&,Sexy::Color const&,int,Sexy::TRect<int> const&,bool,bool)
// @0x68A3C4 (vtable slot [19])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::BltMatrix(Image *theSource, float theX, float theY, const SexyMatrix3 &theMatrix,
    const TRect<int> &theSourceRect, const Color &theColor, int theDrawMode, const TRect<int> &theClipRect,
    bool theLinearBlend, bool theMirror)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::BltMatrix [28] overrides it.
    (void)theSource;
    (void)theX;
    (void)theY;
    (void)theMatrix;
    (void)theSourceRect;
    (void)theColor;
    (void)theDrawMode;
    (void)theClipRect;
    (void)theLinearBlend;
    (void)theMirror;
}

// ==========================================================================
// Sexy::RenderDevice::BltTriangles(Sexy::Image *,SexyVertex2D const (*)[3],int,Sexy::Color const&,int,float,float,bool,Sexy::TRect<int> const*)
// @0x68A3C8 (vtable slot [20])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::BltTriangles(Image *theSource, const SexyVertex2D (*theTriangles)[3], int theTriangleCount,
    const Color &theColor, int theDrawMode, float theTx, float theTy, bool theBlend, const TRect<int> *theClipRect)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::BltTriangles [62] overrides it; MemoryImage also has
    // the separate BltTrianglesTex [29] slot, which is the Image-chain spelling of
    // the same operation with the clip rect passed by reference.
    (void)theSource;
    (void)theTriangles;
    (void)theTriangleCount;
    (void)theColor;
    (void)theDrawMode;
    (void)theTx;
    (void)theTy;
    (void)theBlend;
    (void)theClipRect;
}

// ==========================================================================
// Sexy::RenderDevice::BltMirror(Sexy::Image *,int,int,Sexy::TRect<int> const&,Sexy::Color const&,int)
// @0x68A3CC (vtable slot [21])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::BltMirror(
    Image *theSource, int theX, int theY, const TRect<int> &theSourceRect, const Color &theColor, int theDrawMode)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::BltMirror [30] overrides it.
    (void)theSource;
    (void)theX;
    (void)theY;
    (void)theSourceRect;
    (void)theColor;
    (void)theDrawMode;
}

// ==========================================================================
// Sexy::RenderDevice::BltStretched(Sexy::Image *,Sexy::TRect<int> const&,Sexy::TRect<int> const&,Sexy::TRect<int> const&,Sexy::Color const&,int,bool,bool)
// @0x68A3D0 (vtable slot [22])  --  __cxa_pure_virtual in the ARM
// ==========================================================================
void Sexy::RenderDevice::BltStretched(Image *theSource, const TRect<int> &theDestRect, const TRect<int> &theSrcRect,
    const TRect<int> &theClipRect, const Color &theColor, int theDrawMode, bool theFastStretch, bool theMirror)
{
    // No ARM body: the slot holds __cxa_pure_virtual @0x2C50D8. See the file
    // comment. MemoryImage::BltStretched [63] overrides it. Note the parameter
    // order differs from the Image-chain StretchBlt [27]: here the destination
    // rect comes before the source rect.
    (void)theSource;
    (void)theDestRect;
    (void)theSrcRect;
    (void)theClipRect;
    (void)theColor;
    (void)theDrawMode;
    (void)theFastStretch;
    (void)theMirror;
}

// ==========================================================================
// Sexy::RenderDevice::PushTransform(SexyMatrix3 const&,bool)
// @00307A88  2 bytes
// ==========================================================================
void Sexy::RenderDevice::PushTransform(const SexyMatrix3 &theTransform, bool theConcatenate)
{
    // `BX LR` -- two bytes, an immediate return. The base transform stack is a
    // no-op: the devices that keep one (MemoryImage's sub-object, the GL
    // devices) override this slot with their own implementation, and the
    // interface's own body exists only so the slot is not pure.
    (void)theTransform;
    (void)theConcatenate;
}

// ==========================================================================
// Sexy::RenderDevice::PopTransform()
// @00307A8C  2 bytes
// ==========================================================================
void Sexy::RenderDevice::PopTransform()
{
    // `BX LR` -- empty, for the same reason as PushTransform above.
}

// ==========================================================================
// Sexy::RenderDevice::PopTransform(SexyMatrix3 &)
// @00307A90  2 bytes
// ==========================================================================
void Sexy::RenderDevice::PopTransform(SexyMatrix3 &theTransform)
{
    // `BX LR` -- empty. The out-parameter is left untouched, which is what the
    // ARM does: the base has no stack to pop a matrix off.
    (void)theTransform;
}

// ==========================================================================
// Sexy::RenderDevice::DrawRect(Sexy::TRect<int> const&,Sexy::Color const&,int)
// @00307A94  182 bytes
// ==========================================================================
// One of the four slots the interface itself implements. DrawRect is a 1-pixel
// outline built from four FillRect calls, dispatched through the vtable slot at
// byte 0x30 of this same object (that is RenderDevice slot [12], FillRect), so a
// device that overrides FillRect gets a working DrawRect for free.
//
// The four bands, read off the ARM's stack stores into the TRect at sp+0:
//
//     top     (mX,          mY,           mWidth + 1, 1        )
//     bottom  (mX,          mY + mHeight, mWidth + 1, 1        )
//     left    (mX,          mY + 1,       1,          mHeight-1)
//     right   (mX + mWidth, mY + 1,       1,          mHeight-1)
//
// The horizontal bands are mWidth + 1 wide, so they cover the pixel column at
// mX + mWidth and the rectangle closes at its right edge; the vertical bands
// then start one row down and stop one row short so they do not paint over the
// two horizontal ones. Each band's other dimension is exactly 1.
void Sexy::RenderDevice::DrawRect(const TRect<int> &theRect, const Color &theColor, int theDrawMode)
{
    const int aLeft = theRect.mX;
    const int aTop = theRect.mY;
    const int aWidth = theRect.mWidth;
    const int aHeight = theRect.mHeight;

    FillRect(TRect<int>(aLeft, aTop, aWidth + 1, 1), theColor, theDrawMode);
    FillRect(TRect<int>(aLeft, aTop + aHeight, aWidth + 1, 1), theColor, theDrawMode);
    FillRect(TRect<int>(aLeft, aTop + 1, 1, aHeight - 1), theColor, theDrawMode);
    FillRect(TRect<int>(aLeft + aWidth, aTop + 1, 1, aHeight - 1), theColor, theDrawMode);
}

// ==========================================================================
// Sexy::RenderDevice::FillScanLines(Sexy::RenderDevice::Span *,int,Sexy::Color const&,int)
// @00307B4C  82 bytes
// ==========================================================================
// One FillRect per span, each one scanline tall. The ARM walks the span array
// with a stride of 0xC and, for every entry, builds the same TRect from the
// three words before dispatching slot [12]:
//
//     LDR  R3, [R4,#8]     ; span[2] -> the rect's width
//     LDR  R7, [R4]        ; span[0] -> the rect's y
//     LDR  R3, [R4,#-8]    ; span[1] -> the rect's x   (after the 0xC advance)
//     MOVS R10, #1         ; the rect's height
//
// so the span record is { mY, mX, mCount } -- the same layout
// MemoryImage::FillScanLinesWithCoverageRenderDevice uses (MemoryImage.cpp).
// The dispatch is to byte 0x30 of the receiver's vtable, i.e. this interface's
// FillRect slot [12], not to a fixed target.
//
// The loop is a do/while entered only when theSpanCount > 0, so a zero or
// negative count draws nothing.
void Sexy::RenderDevice::FillScanLines(Span *theSpans, int theSpanCount, const Color &theColor, int theDrawMode)
{
    for (int aSpan = 0; aSpan < theSpanCount; ++aSpan)
    {
        const Span &aCurrent = theSpans[aSpan];
        FillRect(TRect<int>(aCurrent.mX, aCurrent.mY, aCurrent.mCount, 1), theColor, theDrawMode);
    }
}

// ==========================================================================
// Sexy::RenderDevice::TakeScreenshot(Sexy::MemoryImage &)
// @00307BA0  4 bytes
// ==========================================================================
void Sexy::RenderDevice::TakeScreenshot(MemoryImage &theImage)
{
    // `MOVS R0, #0; BX LR` -- four bytes. The base implementation does nothing
    // and leaves the destination image alone: a device that can read its
    // framebuffer back (the GL devices) overrides this slot, and there is no
    // software fallback to take a screenshot with.
    (void)theImage;
}
