// CoreTypes.h -- the framework's small value types.
//
// These are taken from the PC source (SexyAppFramework/Point.h, Rect.h,
// Color.h, Insets.h) because the binary's use of them matches: the demangled
// signatures name Sexy::TRect<int>, Sexy::TPoint<int> and Sexy::Color, and the
// layouts the decompiler shows are the same four-int / two-int shapes.
//
// One thing is deliberately NOT copied: the PC versions call the Win32 macros
// max()/min() inside Intersection and Union, and ToRECT() returns a RECT. This
// project compiles with NOMINMAX (the transcribed bodies use std::min
// constantly, and the min/max macros would break them), so those two use
// std::max/std::min and ToRECT is dropped in favour of the four accessors the
// game actually uses.
//
// Sizes on the x86 target this project builds:
//     Point  8      Rect 16      Color 16      Insets 16
// which is what the binary's `*(_DWORD *)(this + 4 * n)` arithmetic assumes.

#ifndef __CORETYPES_H__
#define __CORETYPES_H__

#include "SexyTypes.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace Sexy
{

// ---------------------------------------------------------------------------
// TPoint / Point
// ---------------------------------------------------------------------------
template <class _T> class TPoint
{
public:
    _T  mX;
    _T  mY;

public:
    TPoint(_T theX, _T theY) : mX(theX), mY(theY) {}
    TPoint(const TPoint<_T>& thePoint) : mX(thePoint.mX), mY(thePoint.mY) {}
    TPoint() : mX(0), mY(0) {}

    bool operator==(const TPoint& p) const { return p.mX == mX && p.mY == mY; }
    bool operator!=(const TPoint& p) const { return p.mX != mX || p.mY != mY; }

    TPoint  operator+(const TPoint& p) const { return TPoint(mX + p.mX, mY + p.mY); }
    TPoint  operator-(const TPoint& p) const { return TPoint(mX - p.mX, mY - p.mY); }
    TPoint  operator*(const TPoint& p) const { return TPoint(mX * p.mX, mY * p.mY); }
    TPoint  operator/(const TPoint& p) const { return TPoint(mX / p.mX, mY / p.mY); }
    TPoint& operator+=(const TPoint& p) { mX += p.mX; mY += p.mY; return *this; }
    TPoint& operator-=(const TPoint& p) { mX -= p.mX; mY -= p.mY; return *this; }
    TPoint& operator*=(const TPoint& p) { mX *= p.mX; mY *= p.mY; return *this; }
    TPoint& operator/=(const TPoint& p) { mX /= p.mX; mY /= p.mY; return *this; }
    TPoint  operator*(_T s) const { return TPoint(mX * s, mY * s); }
    TPoint  operator/(_T s) const { return TPoint(mX / s, mY / s); }
};

typedef TPoint<int>    Point;
typedef TPoint<double> FPoint;

// ---------------------------------------------------------------------------
// TRect / Rect
// ---------------------------------------------------------------------------
template <class _T> class TRect
{
public:
    _T  mX;
    _T  mY;
    _T  mWidth;
    _T  mHeight;

public:
    TRect(_T theX, _T theY, _T theWidth, _T theHeight)
        : mX(theX), mY(theY), mWidth(theWidth), mHeight(theHeight) {}

    TRect(const TRect<_T>& theRect)
        : mX(theRect.mX), mY(theRect.mY),
          mWidth(theRect.mWidth), mHeight(theRect.mHeight) {}

    TRect() : mX(0), mY(0), mWidth(0), mHeight(0) {}

    bool Intersects(const TRect<_T>& theRect) const
    {
        return !((theRect.mX + theRect.mWidth <= mX) ||
                 (theRect.mY + theRect.mHeight <= mY) ||
                 (theRect.mX >= mX + mWidth) ||
                 (theRect.mY >= mY + mHeight));
    }

    TRect<_T> Intersection(const TRect<_T>& theRect) const
    {
        _T x1 = std::max(mX, theRect.mX);
        _T x2 = std::min(mX + mWidth, theRect.mX + theRect.mWidth);
        _T y1 = std::max(mY, theRect.mY);
        _T y2 = std::min(mY + mHeight, theRect.mY + theRect.mHeight);
        if ((x2 - x1) < 0 || (y2 - y1) < 0)
            return TRect<_T>(0, 0, 0, 0);
        return TRect<_T>(x1, y1, x2 - x1, y2 - y1);
    }

    TRect<_T> Union(const TRect<_T>& theRect) const
    {
        _T x1 = std::min(mX, theRect.mX);
        _T x2 = std::max(mX + mWidth, theRect.mX + theRect.mWidth);
        _T y1 = std::min(mY, theRect.mY);
        _T y2 = std::max(mY + mHeight, theRect.mY + theRect.mHeight);
        return TRect<_T>(x1, y1, x2 - x1, y2 - y1);
    }

    bool Contains(_T theX, _T theY) const
    {
        return theX >= mX && theX < mX + mWidth &&
               theY >= mY && theY < mY + mHeight;
    }

    bool Contains(const TPoint<_T>& thePoint) const
    {
        return Contains(thePoint.mX, thePoint.mY);
    }

    void Offset(_T theX, _T theY) { mX += theX; mY += theY; }
    void Offset(const TPoint<_T>& thePoint) { mX += thePoint.mX; mY += thePoint.mY; }

    TRect Inflate(_T theX, _T theY)
    {
        mX -= theX;
        mWidth += theX * 2;
        mY -= theY;
        mHeight += theY * 2;
        return *this;
    }

    bool operator==(const TRect<_T>& theRect) const
    {
        return mX == theRect.mX && mY == theRect.mY &&
               mWidth == theRect.mWidth && mHeight == theRect.mHeight;
    }
    bool operator!=(const TRect<_T>& theRect) const { return !(*this == theRect); }
};

typedef TRect<int>    Rect;
typedef TRect<double> FRect;

// ---------------------------------------------------------------------------
// Color
//
// The binary's ToInt @0x307984 is (R << 16) | (A << 24) | B | (G << 8), i.e.
// 0xAARRGGBB, matching the PC implementation.
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct SexyRGBA { unsigned char b, g, r, a; };
#pragma pack(pop)

class Color
{
public:
    int mRed;
    int mGreen;
    int mBlue;
    int mAlpha;

    static Color Black;
    static Color White;

public:
    Color() : mRed(0), mGreen(0), mBlue(0), mAlpha(255) {}
    Color(int theColor) { SetFromInt(theColor, 255); }
    Color(int theColor, int theAlpha) { SetFromInt(theColor, theAlpha); }
    Color(int theRed, int theGreen, int theBlue)
        : mRed(theRed), mGreen(theGreen), mBlue(theBlue), mAlpha(255) {}
    Color(int theRed, int theGreen, int theBlue, int theAlpha)
        : mRed(theRed), mGreen(theGreen), mBlue(theBlue), mAlpha(theAlpha) {}
    // -----------------------------------------------------------------------
    // The binary's own constructor, in the form the decompiler prints it.
    //
    // The dump shows the receiver as the first parameter:
    //
    //     // Sexy::Color::Color(int,int,int,int)
    //     _DWORD *__fastcall Sexy::Color::Color(_DWORD *this, int a2, int a3,
    //                                           int a4, int a5)
    //
    // so a call site reads
    //
    //     Sexy::Color::Color(&v24, 255, 255, 255, 64);
    //
    // which is "construct a Color at &v24". C++ has no operator for that, but a
    // constructor whose first parameter is the destination can do the same
    // work, and the call then compiles unchanged. Writing through the pointer
    // is exactly what the original does, so the colour really does land in the
    // caller's storage.
    // -----------------------------------------------------------------------
    Color(void* theDest, int theRed, int theGreen, int theBlue, int theAlpha)
        : mRed(theRed), mGreen(theGreen), mBlue(theBlue), mAlpha(theAlpha)
    {
        Store(theDest);
    }

    Color(void* theDest, int theRed, int theGreen, int theBlue)
        : mRed(theRed), mGreen(theGreen), mBlue(theBlue), mAlpha(255)
    {
        Store(theDest);
    }

    Color(void* theDest, int theColor)
    {
        SetFromInt(theColor, 255);
        Store(theDest);
    }

    Color(void* theDest)
    {
        mRed = 0; mGreen = 0; mBlue = 0; mAlpha = 255;
        Store(theDest);
    }

    Color(const SexyRGBA& theColor)
        : mRed(theColor.r), mGreen(theColor.g),
          mBlue(theColor.b), mAlpha(theColor.a) {}
    Color(const unsigned char* theElements)
        : mRed(theElements[0]), mGreen(theElements[1]),
          mBlue(theElements[2]), mAlpha(theElements[3]) {}
    Color(const int* theElements)
        : mRed(theElements[0]), mGreen(theElements[1]),
          mBlue(theElements[2]), mAlpha(theElements[3]) {}

    int   GetRed() const   { return mRed; }
    int   GetGreen() const { return mGreen; }
    int   GetBlue() const  { return mBlue; }
    int   GetAlpha() const { return mAlpha; }

    // @0x307984
    unsigned long ToInt() const
    {
        return ((unsigned long)mRed << 16) | ((unsigned long)mAlpha << 24) |
               (unsigned long)mBlue | ((unsigned long)mGreen << 8);
    }

    SexyRGBA ToRGBA() const
    {
        SexyRGBA r;
        r.r = (unsigned char)mRed;
        r.g = (unsigned char)mGreen;
        r.b = (unsigned char)mBlue;
        r.a = (unsigned char)mAlpha;
        return r;
    }

    int&       operator[](int theIdx)       { return (&mRed)[theIdx]; }
    int        operator[](int theIdx) const { return (&mRed)[theIdx]; }

    // Sexy::Color::FromInt(Color *theDest, int theColor) -- the binary's
    // unpacking helper, printed with the receiver as its first argument exactly
    // like the constructors above. It is the inverse of ToInt():
    // (R << 16) | (A << 24) | B | (G << 8), i.e. 0xAARRGGBB.
    //
    // It writes through the caller's pointer, so it is a public static rather
    // than a member: the bodies call it as `Sexy::Color::FromInt(&dest, value)`.
    static void FromInt(void* theDest, int theColor)
    {
        Color aColor;
        aColor.mRed   = (theColor >> 16) & 0xFF;
        aColor.mGreen = (theColor >> 8)  & 0xFF;
        aColor.mBlue  = theColor & 0xFF;
        aColor.mAlpha = (theColor >> 24) & 0xFF;
        aColor.Store(theDest);
    }

private:
    // Write this colour into the destination the decompiler passed as the
    // receiver. A null destination is what a value construction passes, and
    // writing nothing is right for it.
    void Store(void* theDest)
    {
        if (theDest != nullptr)
            *reinterpret_cast<Color*>(theDest) = *this;
    }

    void SetFromInt(int theColor, int theAlpha)
    {
        mRed   = (theColor >> 16) & 0xFF;
        mGreen = (theColor >> 8)  & 0xFF;
        mBlue  = theColor & 0xFF;
        mAlpha = theAlpha;
    }
};

inline bool operator==(const Color& a, const Color& b)
{
    return a.mRed == b.mRed && a.mGreen == b.mGreen &&
           a.mBlue == b.mBlue && a.mAlpha == b.mAlpha;
}
inline bool operator!=(const Color& a, const Color& b) { return !(a == b); }

// ---------------------------------------------------------------------------
// Insets -- Widget's +0x88 field (Widget::Widget @0x2F8Bxx stores four ints).
// ---------------------------------------------------------------------------
class Insets
{
public:
    int mLeft;
    int mTop;
    int mRight;
    int mBottom;

public:
    Insets() : mLeft(0), mTop(0), mRight(0), mBottom(0) {}
    Insets(int theLeft, int theTop, int theRight, int theBottom)
        : mLeft(theLeft), mTop(theTop), mRight(theRight), mBottom(theBottom) {}
    Insets(const Insets& theInsets)
        : mLeft(theInsets.mLeft), mTop(theInsets.mTop),
          mRight(theInsets.mRight), mBottom(theInsets.mBottom) {}

    // The packed form, so a word slot can hold one -- which is what the binary
    // does when it reuses a register for an Insets.
    int ToInt() const { return mLeft; }
};

} // namespace Sexy

// ---------------------------------------------------------------------------
// SexyVector2 / SexyVector3 / SexyMatrix3
//
// The demangled signatures name these (Graphics::DrawImageMatrix takes a
// `SexyMatrix3 const&`), and the PC source has the same three types in
// SexyAppFramework/SEXYMATRIX.H. They are global, not in namespace Sexy: the
// binary writes `Sexy::SexyVector2` in some signatures and plain
// `SexyVector2` in others, which is the decompiler qualifying a global type,
// and the allocation sites confirm the global spelling.
//
// SexyVector2 is 8 bytes and SexyMatrix3 is 36, both POD, so the transcribed
// offset arithmetic over them holds.
// ---------------------------------------------------------------------------
struct SexyVector2
{
    float mX;
    float mY;

    SexyVector2() : mX(0.0f), mY(0.0f) {}
    SexyVector2(float theX, float theY) : mX(theX), mY(theY) {}
    SexyVector2(const SexyVector2& v) : mX(v.mX), mY(v.mY) {}

    SexyVector2& operator=(const SexyVector2& v)
    {
        mX = v.mX;
        mY = v.mY;
        return *this;
    }

    float Dot(const SexyVector2& v) const { return mX * v.mX + mY * v.mY; }
    float Length() const;
    float LengthSquared() const { return mX * mX + mY * mY; }

    // The packed form, so a word slot can hold one.
    int ToInt() const
    {
        int aBits;
        std::memcpy(&aBits, &mX, 4);
        return aBits;
    }
};

struct SexyVector3
{
    float mX;
    float mY;
    float mZ;

    SexyVector3() : mX(0.0f), mY(0.0f), mZ(0.0f) {}
    SexyVector3(float theX, float theY, float theZ)
        : mX(theX), mY(theY), mZ(theZ) {}

    int ToInt() const
    {
        int aBits;
        std::memcpy(&aBits, &mX, 4);
        return aBits;
    }
};

// A 3x3 matrix stored row-major, matching SEXYMATRIX.H's SexyMatrix3.
struct SexyMatrix3
{
    // The nine elements, under both spellings the code uses. The PC source
    // names them m00..m22; the transcribed GL layer indexes them as m[0..8].
    // They are the same nine floats in the same order, so a union gives both
    // names to one layout.
    union
    {
        struct
        {
            float m00, m01, m02;
            float m10, m11, m12;
            float m20, m21, m22;
        };
        float m[9];
    };

    SexyMatrix3() { Zero(); }

    void Zero()
    {
        m00 = m01 = m02 = 0.0f;
        m10 = m11 = m12 = 0.0f;
        m20 = m21 = m22 = 0.0f;
    }

    void LoadIdentity()
    {
        Zero();
        m00 = m11 = m22 = 1.0f;
    }

    int ToInt() const
    {
        int aBits;
        std::memcpy(&aBits, &m00, 4);
        return aBits;
    }

    // _ZNK4Sexy11SexyMatrix3mlERKNS_11SexyVector2E @0x341b04.
    // x' = x*m00 + y*m01 + m02. y' = x*m10 + y*m11 + m12.
    // GLTexture::BltTransformed calls this for every sod corner. Using m10 in
    // x and m01 in y negates the angle TitleScreen stores.
    SexyVector2 operator*(const SexyVector2& theVector) const
    {
        return SexyVector2(theVector.mX * m00 + theVector.mY * m01 + m02,
                           theVector.mX * m10 + theVector.mY * m11 + m12);
    }

    // _ZNK4Sexy11SexyMatrix3mlERKNS_11SexyVector3E @0x341b48.
    // x' = x*m00 + y*m01 + z*m02. y' uses m10/m11/m12. z' uses m20/m21/m22.
    SexyVector3 operator*(const SexyVector3& theVector) const
    {
        return SexyVector3(theVector.mX * m00 + theVector.mY * m01 + theVector.mZ * m02,
                           theVector.mX * m10 + theVector.mY * m11 + theVector.mZ * m12,
                           theVector.mX * m20 + theVector.mY * m21 + theVector.mZ * m22);
    }

    // _ZNK4Sexy11SexyMatrix3mlERKS0_ @0x341aa8. Result (i, j) is this row i
    // dotted with the other matrix's column j, the same product 0x1fd004 stores.
    SexyMatrix3 operator*(const SexyMatrix3& theMatrix) const
    {
        SexyMatrix3 aResult;
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                aResult.m[3 * i + j] = m[3 * i] * theMatrix.m[j]
                                     + m[3 * i + 1] * theMatrix.m[3 + j]
                                     + m[3 * i + 2] * theMatrix.m[6 + j];
            }
        }
        return aResult;
    }
};

// A vertex as the batcher takes it: position, texture coordinate, colour.
// Graphics::DrawTrianglesTex passes `SexyVertex2D const(*)[3]`, so the layout
// is what the caller's arrays already have.
struct SexyVertex2D
{
    float       mX;
    float       mY;
    float       mU;
    float       mV;
    int         mColor;

    int ToInt() const { return mColor; }
};

inline float SexyVector2::Length() const
{
    return sqrtf(LengthSquared());
}


#endif // __CORETYPES_H__
