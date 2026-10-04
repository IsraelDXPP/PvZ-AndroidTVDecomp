// GLTexture.cpp -- texture creation and upload.

#include "GLTexture.h"
#include "MemoryImage.h"
#include "SexyTypes.h"
#include "Graphics.h"
#include "AndroidShims.h"
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

// Sexy::PixelFormat enumerators.
//
// The binary's values come from its own table at off_66D5A0 -- 11 entries of
// {name, bytes-per-pixel, has-alpha}:
//   0 NONE   1 LUT8   2 A8R8G8B8   3 A4R4G4B4   4 A1R5G5B5   5 R5G6R5
//   6 A8B8G8R8   7 R4G4B4A4   8 R5G5B5A1   9 X8R8G8B8   10 L8A8
// PixelUtil::GetPixelFormat(String) returns exactly those indices.
//
// ReconTypes.h declares Sexy::PixelFormat as a one-int wrapper WITHOUT the
// enumerators, so the values are spelled out here under local names to avoid
// colliding with the enumerators the header should eventually carry.
// TODO(recon): Sexy::PixelFormat wants these enumerators in ReconTypes.h;
// this local enum is the stand-in until then.
enum
{
    kPixelFormat_None     = 0,
    kPixelFormat_Lut8     = 1,
    kPixelFormat_A8R8G8B8 = 2,
    kPixelFormat_A4R4G4B4 = 3,
    kPixelFormat_A1R5G5B5 = 4,
    kPixelFormat_R5G6R5   = 5,
    kPixelFormat_A8B8G8R8 = 6,
    kPixelFormat_R4G4B4A4 = 7,
    kPixelFormat_R5G5B5A1 = 8,
    kPixelFormat_X8R8G8B8 = 9,
    kPixelFormat_L8A8     = 10
};

// Remember the texture bound to the active unit, bind another for an upload,
// then put the remembered one back. glTexImage writes to whatever is bound, and
// a draw may already be sampling that binding; replacing it without restoring
// it makes the draw sample the texture just uploaded.
struct TextureBindScope
{
    GLint mPrevious;

    explicit TextureBindScope(unsigned int theTexture)
    {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &mPrevious);
        glBindTexture(GL_TEXTURE_2D, theTexture);
    }

    ~TextureBindScope()
    {
        glBindTexture(GL_TEXTURE_2D, (unsigned int)mPrevious);
    }
};

GLTexture::GLTexture()
    : mTextureId(0), mWidth(0), mHeight(0), mSrcWidth(0), mSrcHeight(0),
      mU0(0), mV0(0), mU1(1), mV1(1),
      mFormat(kPixelFormat_A8R8G8B8), mDirty(true), mHasAlpha(true), mOwner(nullptr),
      mLinear(false)
{
}

GLTexture::~GLTexture()
{
    ReleaseTextures();
}

bool GLTexture::SetupTextureFormat(PixelFormat thePixelFormat,
                                   unsigned int& theInternalFormat,
                                   unsigned int& theDataFormat,
                                   unsigned int& theDataType)
{
    // The reconstructed pipeline stores everything as 32-bit ARGB, which
    // uploads as GL_RGBA8 / GL_BGRA. The remaining cases exist so the function
    // matches the original's contract; the loader only ever produces A8R8G8B8.
    switch ((int)thePixelFormat)
    {
    case kPixelFormat_A8R8G8B8:
        theInternalFormat = GL_RGBA8;
#ifdef LAWNTV_USE_GLES2
        theDataFormat = GL_RGBA;
#else
        theDataFormat = GL_BGRA;    // matches our 0xAARRGGBB memory layout
#endif
        theDataType = GL_UNSIGNED_BYTE;
        return true;

    case kPixelFormat_A8B8G8R8:
        theInternalFormat = GL_RGBA8;
        theDataFormat = GL_RGBA;
        theDataType = GL_UNSIGNED_BYTE;
        return true;

    case kPixelFormat_X8R8G8B8:
        theInternalFormat = GL_RGB8;
        theDataFormat = GL_BGRA;
        theDataType = GL_UNSIGNED_BYTE;
        return true;

    case kPixelFormat_R5G6R5:
        theInternalFormat = GL_RGB565;
        theDataFormat = GL_RGB;
        theDataType = GL_UNSIGNED_SHORT_5_6_5;
        return true;

    case kPixelFormat_A4R4G4B4:
        theInternalFormat = GL_RGBA4;
        theDataFormat = GL_BGRA;
        theDataType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
        return true;

    case kPixelFormat_R4G4B4A4:
        theInternalFormat = GL_RGBA4;
        theDataFormat = GL_RGBA;
        theDataType = GL_UNSIGNED_SHORT_4_4_4_4;
        return true;

    case kPixelFormat_A1R5G5B5:
        theInternalFormat = GL_RGB5_A1;
        theDataFormat = GL_BGRA;
        theDataType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
        return true;

    case kPixelFormat_R5G5B5A1:
        theInternalFormat = GL_RGB5_A1;
        theDataFormat = GL_RGBA;
        theDataType = GL_UNSIGNED_SHORT_5_5_5_1;
        return true;

    case kPixelFormat_Lut8:
        theInternalFormat = GL_R8;
        theDataFormat = GL_RED;
        theDataType = GL_UNSIGNED_BYTE;
        return true;

    default:
        return false;
    }
}

int GLTexture::CreateTextureDimensions(int theSize)
{
    // The Android build padded to powers of two because GLES2 required it for
    // mipmapping and wrap modes. Desktop GL 3.3 supports NPOT, but padding keeps
    // the atlas maths identical to the original, so we keep it.
    int n = 1;
    while (n < theSize && n < 4096) n <<= 1;
    return n;
}

bool GLTexture::CreateTexture(MemoryImage* theImage)
{
    if (!theImage) return false;

    ReleaseTextures();

    // MemoryImage exposes the dimensions as the Image data members mWidth and
    // mHeight (Image.h +0x24/+0x28); they are not accessor functions.
    mSrcWidth  = theImage->mWidth;
    mSrcHeight = theImage->mHeight;
    mFormat    = theImage->GetPixelFormat();
    mOwner     = theImage;

    if (mSrcWidth <= 0 || mSrcHeight <= 0) return false;

    mWidth  = CreateTextureDimensions(mSrcWidth);
    mHeight = CreateTextureDimensions(mSrcHeight);

    unsigned int internalFormat, dataFormat, dataType;
    if (!SetupTextureFormat(mFormat, internalFormat, dataFormat, dataType))
    {
        TOD_ERROR("GLTexture: unsupported pixel format %d", (int)mFormat);
        return false;
    }

    glGenTextures(1, &mTextureId);
    if (!mTextureId) return false;

    TextureBindScope aBind(mTextureId);

    // Default to NEAREST to match the software blitter exactly. The framework
    // only asks for smoothing when Graphics::mFastStretch / mLinearBlend says
    // so, and SetFilter() switches this per texture.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    mLinear = false;

    // Allocate storage; the actual pixels arrive in Upload().
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, mWidth, mHeight, 0,
                 dataFormat, dataType, nullptr);

    // uv rect: with padding, the image occupies the top-left corner.
    mU0 = 0.0f;
    mV0 = 0.0f;
    mU1 = (float)mSrcWidth  / (float)mWidth;
    mV1 = (float)mSrcHeight / (float)mHeight;

    mDirty = true;

    GL_CHECK("GLTexture::CreateTexture");
    return true;
}

bool GLTexture::Upload(MemoryImage* theImage)
{
    if (!theImage || !mTextureId) return false;

    // A sub-image has no bits of its own. Its pixels are a rectangle in the
    // parent sheet, and the parent row stride is the sheet width, not the
    // sub-image width. Copy that rectangle into a packed buffer so the upload
    // below can treat every image as tightly packed.
    MemoryImage* aSheet = theImage;
    int aOriginX = 0;
    int aOriginY = 0;
    while (aSheet->mParent != nullptr)
    {
        aOriginX += aSheet->mParentX;
        aOriginY += aSheet->mParentY;
        aSheet = aSheet->mParent;
    }

    unsigned int* aSheetBits = aSheet->GetBits();
    if (aSheetBits == nullptr)
        return false;

    std::vector<unsigned int> aPacked;
    const unsigned int* aRows = aSheetBits;
    if (aSheet != theImage)
    {
        int aPitch = aSheet->GetPitch();
        if (aPitch <= 0)
            aPitch = aSheet->mWidth;
        if (mSrcWidth <= 0 || mSrcHeight <= 0 || aOriginX < 0 || aOriginY < 0
            || aOriginX + mSrcWidth > aSheet->mWidth
            || aOriginY + mSrcHeight > aSheet->mHeight)
            return false;

        aPacked.resize((size_t)mSrcWidth * (size_t)mSrcHeight);
        for (int aRow = 0; aRow < mSrcHeight; ++aRow)
        {
            const unsigned int* aSrc = aSheetBits
                + (size_t)(aOriginY + aRow) * (size_t)aPitch
                + (size_t)aOriginX;
            std::memcpy(aPacked.data() + (size_t)aRow * (size_t)mSrcWidth,
                        aSrc, (size_t)mSrcWidth * 4);
        }
        aRows = aPacked.data();
    }
    if (aRows == nullptr)
        return false;

    unsigned int internalFormat, dataFormat, dataType;
    if (!SetupTextureFormat(mFormat, internalFormat, dataFormat, dataType))
        return false;

    TextureBindScope aBind(mTextureId);

#ifndef LAWNTV_USE_GLES2
    // Our rows are tightly packed (pitch == width*4), so the default
    // GL_UNPACK_ALIGNMENT of 4 is already correct; set it explicitly anyway.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif

    if (mWidth == mSrcWidth && mHeight == mSrcHeight)
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mSrcWidth, mSrcHeight,
                        dataFormat, dataType, aRows);
    }
    else
    {
        // Padded texture. The padding region must be filled with transparent
        // black, not left as uninitialised storage: with NEAREST sampling and
        // the half-texel correction, edge texels can round into the padding,
        // and reading garbage there produced white fringes around the image
        // (observed as d=255 against the software reference at the left edge
        // of the title art).
        std::vector<unsigned int> padRow((size_t)mWidth, 0);

        // Rows inside the image: copy the row then clear the right padding.
        for (int y = 0; y < mSrcHeight; ++y)
        {
            memcpy(padRow.data(), aRows + (size_t)y * mSrcWidth,
                   (size_t)mSrcWidth * 4);
            memset(padRow.data() + mSrcWidth, 0, (size_t)(mWidth - mSrcWidth) * 4);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, mWidth, 1,
                            dataFormat, dataType, padRow.data());
        }

        // Rows below the image: fully transparent.
        memset(padRow.data(), 0, (size_t)mWidth * 4);
        for (int y = mSrcHeight; y < mHeight; ++y)
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, mWidth, 1,
                            dataFormat, dataType, padRow.data());
    }

    mDirty = false;
    theImage->mBitsChanged = false;
    GL_CHECK("GLTexture::Upload");
    return true;
}

bool GLTexture::CheckCreateTextures(MemoryImage* theImage)
{
    if (!theImage) return false;

    bool needCreate = (mTextureId == 0) ||
                      mWidth  != CreateTextureDimensions(theImage->mWidth) ||
                      mHeight != CreateTextureDimensions(theImage->mHeight);

    if (needCreate)
    {
        // CreateTexture allocates storage only; the pixels must be uploaded
        // before the texture is usable, otherwise the first draw samples an
        // uninitialised (all-zero) texture and renders nothing.
        if (!CreateTexture(theImage))
            return false;
        return Upload(theImage);
    }

    if (mDirty)
        return Upload(theImage);

    return true;
}

void GLTexture::GetTexture(int theX, int theY, int& theTexX, int& theTexY,
                           float& u0, float& v0, float& u1, float& v1)
{
    theTexX = theX;
    theTexY = theY;
    u0 = mU0 + (float)theX / (float)mWidth;
    v0 = mV0 + (float)theY / (float)mHeight;
    u1 = u0;
    v1 = v0;
}

void GLTexture::GetTextureF(float theX, float theY, float& u0, float& v0,
                            float& u1, float& v1)
{
    u0 = mU0 + theX / (float)mWidth;
    v0 = mV0 + theY / (float)mHeight;
    u1 = u0;
    v1 = v0;
}

void GLTexture::ReleaseTextures()
{
    if (mTextureId)
    {
        glDeleteTextures(1, &mTextureId);
        mTextureId = 0;
    }
    mWidth = mHeight = 0;
    mOwner = nullptr;
    mDirty = true;
}

// Switch between nearest (fast) and linear (smooth) sampling. The framework's
// default is nearest: Graphics::mFastStretch is false and mLinearBlend is
// false, and the software reference blitter is nearest-only for FastStretchBlt.
void GLTexture::SetFilter(bool theLinear)
{
    if (!mTextureId || mLinear == theLinear) return;

    TextureBindScope aBind(mTextureId);
    unsigned int mode = theLinear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);
    mLinear = theLinear;
}

} // namespace Sexy
