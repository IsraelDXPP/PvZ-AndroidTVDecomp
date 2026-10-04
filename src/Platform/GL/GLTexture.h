// GLTexture.h -- GPU texture backing a MemoryImage.
//
// Mirrors Sexy::GLTexture (13 methods: SetupTextureFormat, CreateTexture,
// CreateTextureDimensions, CreateTextures, GetTexture, GetTextureF, Blt,
// BltTransformed, BltTriangles, CheckCreateTextures, ReleaseTextures).
//
// The original packs many small images into atlas textures and returns a uv
// sub-rect from GetTexture; that is why the game's blits pass a uv rect rather
// than binding a texture per image. We keep the same split between "the image's
// pixel data" (MemoryImage) and "its GPU residency" (GLTexture), so a
// MemoryImage can exist without a texture until it is first drawn.

#ifndef __GLTEXTURE_H__
#define __GLTEXTURE_H__

#include "GLHeaders.h"
#include "SexyTypes.h"
#include "CoreTypes.h"
#include "ReconTypes.h"   // Sexy::PixelFormat

namespace Sexy
{

class MemoryImage;
class GLStateManager;

class GLTexture
{
public:
    unsigned int    mTextureId;     // GL texture name (0 = not uploaded)
    int             mWidth;         // padded texture width (power of two or ATLAS)
    int             mHeight;
    int             mSrcWidth;      // the image's real width
    int             mSrcHeight;
    float           mU0, mV0, mU1, mV1;   // uv rect of the image inside the texture
    PixelFormat     mFormat;
    bool            mDirty;         // pixel data changed since upload
    bool            mHasAlpha;
    MemoryImage*    mOwner;

public:
    GLTexture();
    ~GLTexture();

    // Choose a GL format triple for a Sexy::PixelFormat.
    // NB: the out-param cannot be named theFormat -- that is the name of the
    // PixelFormat types parameter and C++ would treat it as a redeclaration.
    static bool     SetupTextureFormat(PixelFormat thePixelFormat,
                                       unsigned int& theInternalFormat,
                                       unsigned int& theDataFormat,
                                       unsigned int& theDataType);

    // Create (or recreate) the GL texture for an image.
    bool            CreateTexture(MemoryImage* theImage);

    // Round a dimension up to what the device wants for this format.
    static int      CreateTextureDimensions(int theSize);

    // Upload pixel data. Called lazily by CheckCreateTextures.
    bool            Upload(MemoryImage* theImage);

    // Ensure the texture exists and matches the image; cheap when clean.
    bool            CheckCreateTextures(MemoryImage* theImage);

    // uv rect for the whole image (or a sub-rect within an atlas).
    void            GetTexture(int theX, int theY, int& theTexX, int& theTexY,
                               float& u0, float& v0, float& u1, float& v1);
    void            GetTextureF(float theX, float theY, float& u0, float& v0,
                               float& u1, float& v1);

    void            ReleaseTextures();
    void            MarkDirty() { mDirty = true; }

    // Filtering. The framework distinguishes fast (nearest) from smooth
    // (linear) stretching via Graphics::mFastStretch / mLinearBlend; the GL
    // backend must honour the same flag or its output diverges from the
    // software reference at high-contrast edges.
    void            SetFilter(bool theLinear);
    bool            IsLinear() const { return mLinear; }

private:
    bool            mLinear;
};

} // namespace Sexy



#endif // __GLTEXTURE_H__

// The global spelling, which the transcribed bodies use.
using Sexy::MemoryImage;
