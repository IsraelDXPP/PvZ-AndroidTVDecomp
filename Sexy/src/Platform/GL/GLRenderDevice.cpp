// GLRenderDevice.cpp -- OpenGL 3.3 core implementation of the 2D render device.

#include "GLRenderDevice.h"
#include "SexyTypes.h"
#include "GLStateManager.h"
#include "GLTexture.h"
#include "GLVertexBuffer.h"
#include "Graphics.h"
#include "Image.h"
#include "MemoryImage.h"
#include "AndroidShims.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cmath>

namespace Sexy
{

GLRenderDevice* GLRenderDevice::sCurrent = nullptr;

GLRenderDevice::GLRenderDevice()
    : mWidth(0), mHeight(0), mInitialized(false), mReadbackImage(nullptr),
      mCurrentBatchTexture(0), mCurrentBatchMode(DRAWMODE_NORMAL), mCurrentBatchColorize(false),
      mDrawTarget(nullptr), mFBO(0), mFBOTexture(0),
      mFBOColorRB(0), mTextureCacheBytes(0)
{
    mCurrentTransform.LoadIdentity();
}

GLRenderDevice::~GLRenderDevice()
{
    Shutdown();
}

// ---------------------------------------------------------------------------
// lifecycle
// ---------------------------------------------------------------------------

bool GLRenderDevice::Init(int theWidth, int theHeight)
{
    sCurrent = this;
    mWidth = theWidth;
    mHeight = theHeight;

    if (!mShaders.Create())
    {
        TOD_ERROR("GLRenderDevice: shader creation failed");
        return false;
    }

    if (!mBatch.Create(4096 * 6))
    {
        TOD_ERROR("GLRenderDevice: vertex buffer creation failed");
        return false;
    }

    mState.Init();
    mState.SetViewport(0, 0, theWidth, theHeight);
    mState.SetClearColor(Color(0, 0, 0, 255));

    CreateFBO(theWidth, theHeight);

    mInitialized = true;

    TOD_TRACE("GLRenderDevice: %dx%d, GL '%s' / GLSL '%s'",
              theWidth, theHeight,
              (const char*)glGetString(GL_VERSION),
              (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    return true;
}

void GLRenderDevice::Shutdown()
{
    if (!mInitialized) return;

    // Release every GPU texture. GLTexture's destructor calls
    // ReleaseTextures(), which is what actually deletes the GL objects -- the
    // earlier code deleted the pointer without ever running that destructor,
    // because the type was incomplete at the delete site.
    for (auto& kv : mTextureCache)
        delete kv.second;
    mTextureCache.clear();
    mTextureCacheBytes = 0;

    mBatch.Destroy();
    mShaders.Destroy();
    DestroyFBO();

    delete mReadbackImage;
    mReadbackImage = nullptr;

    mInitialized = false;
}

void GLRenderDevice::Resize(int theWidth, int theHeight)
{
    if (theWidth == mWidth && theHeight == mHeight && mFBO != 0) return;

    mWidth = theWidth;
    mHeight = theHeight;

    mState.SetViewport(0, 0, theWidth, theHeight);
    CreateFBO(theWidth, theHeight);
}

void GLRenderDevice::CreateFBO(int theWidth, int theHeight)
{
    DestroyFBO();
    if (theWidth <= 0 || theHeight <= 0) return;

    glGenTextures(1, &mFBOTexture);
    GLint aPreviousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &aPreviousTexture);
    glBindTexture(GL_TEXTURE_2D, mFBOTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, theWidth, theHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    // Point sampling. A linear filter on a 1:1 blit picks the neighbour when
    // the edge is a fraction of a pixel, which is the grass strip shimmer.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, (unsigned int)aPreviousTexture);

    glGenFramebuffers(1, &mFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mFBOTexture, 0);

    unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        TOD_ERROR("GLRenderDevice: FBO incomplete (0x%04X)", status);
        DestroyFBO();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK("GLRenderDevice::CreateFBO");
}

void GLRenderDevice::DestroyFBO()
{
    if (mFBO)        { glDeleteFramebuffers(1, &mFBO); mFBO = 0; }
    if (mFBOTexture) { glDeleteTextures(1, &mFBOTexture); mFBOTexture = 0; }
    if (mFBOColorRB) { glDeleteRenderbuffers(1, &mFBOColorRB); mFBOColorRB = 0; }
}

// ---------------------------------------------------------------------------
// frame
// ---------------------------------------------------------------------------

void GLRenderDevice::BeginFrame(const Color& theClearColor)
{
    if (!mInitialized) return;

    // Render into the offscreen target, exactly as the Android build did.
    if (mFBO)
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

    // Set it explicitly. The viewport GLFW installed belongs to the window's
    // back buffer, and drawing the frame into the offscreen target with that
    // viewport shifts the whole frame within the target.
    glViewport(0, 0, mWidth, mHeight);
    mState.SetViewport(0, 0, mWidth, mHeight);
    mState.ApplyDefaultState();
    mState.SetClearColor(theClearColor);

    glClearColor(theClearColor.mRed / 255.0f, theClearColor.mGreen / 255.0f,
                 theClearColor.mBlue / 255.0f, theClearColor.mAlpha / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset batching state for the new frame.
    mCurrentBatchTexture = 0;
    mBatch.Begin(0);

    // The viewport uniform is set at link time; keep it correct after resizes.
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    if (mShaders.mBlit.IsValid())
    {
        mShaders.mBlit.Bind();
        int loc = glGetUniformLocation(mShaders.mBlit.mProgram, "uViewport");
        if (loc >= 0) glUniform2f(loc, (float)mWidth, (float)mHeight);
    }
    if (mShaders.mSolid.IsValid())
    {
        mShaders.mSolid.Bind();
        int loc = glGetUniformLocation(mShaders.mSolid.mProgram, "uViewport");
        if (loc >= 0) glUniform2f(loc, (float)mWidth, (float)mHeight);
    }
    if (mShaders.mYuv.IsValid())
    {
        mShaders.mYuv.Bind();
        int loc = glGetUniformLocation(mShaders.mYuv.mProgram, "uViewport");
        if (loc >= 0) glUniform2f(loc, (float)mWidth, (float)mHeight);
    }
}

void GLRenderDevice::EndFrame()
{
    FlushBatch();
}

void GLRenderDevice::BlitImage(MemoryImage* theImage)
{
    if (!mInitialized || theImage == nullptr || theImage->mWidth <= 0 || theImage->mHeight <= 0)
        return;

    BeginFrame(Color(0, 0, 0, 255));
    DrawImage(theImage,
              Rect(0, 0, mWidth, mHeight),
              Rect(0, 0, theImage->mWidth, theImage->mHeight),
              Color::White, DRAWMODE_NORMAL, false, true);
    EndFrame();
}

void GLRenderDevice::Present()
{
    if (!mInitialized) return;

    FlushBatch();

    // BeginFrame's viewport belongs to the game FBO. Query the drawable
    // framebuffer instead: its pixel size also accounts for Windows DPI.
    GLFWwindow* aWindow = glfwGetCurrentContext();
    if (aWindow == nullptr || mFBO == 0)
        return;
    int aWindowWidth = 0;
    int aWindowHeight = 0;
    glfwGetFramebufferSize(aWindow, &aWindowWidth, &aWindowHeight);
    if (aWindowWidth <= 0 || aWindowHeight <= 0)
        return;

    // A widget clip must not trim presentation. Keep the game target at its
    // logical size and stretch the finished frame across the whole window.
    mState.EnableScissor(false);
    mState.SetViewport(0, 0, aWindowWidth, aWindowHeight);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, mWidth, mHeight,
                      0, 0, aWindowWidth, aWindowHeight,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL_CHECK("GLRenderDevice::Present");
}

// ---------------------------------------------------------------------------
// state
// ---------------------------------------------------------------------------

void GLRenderDevice::ApplyDrawMode(DrawMode theMode)
{
    mState.EnableBlend(true);
    if (theMode == DRAWMODE_ADDITIVE)
    {
        // GLGraphics::SetDrawMode @0x476804: additive is glBlendFunc(ONE, ONE).
        // SetColor has already premultiplied RGB by alpha, matching
        // GLTexture::Blt. SRC_ALPHA on top of that washes a glow out to white.
        mState.SetBlend(GLStateManager::BLEND_ONE, GLStateManager::BLEND_ONE);
    }
    else
    {
        // Normal is glBlendFunc(ONE, ONE_MINUS_SRC_ALPHA). The shader has
        // already multiplied RGB by alpha, so the source factor is ONE.
        mState.SetBlend(GLStateManager::BLEND_ONE,
                        GLStateManager::BLEND_INV_SRC_ALPHA);
    }
}

void GLRenderDevice::SetDrawMode(DrawMode theMode)
{
    FlushBatch();
    ApplyDrawMode(theMode);
}

void GLRenderDevice::SetColorizeImages(bool theColorize)
{
    FlushBatch();
    mShaders.mBlit.SetColorize(theColorize);
}

void GLRenderDevice::SetClipRect(const Rect& theRect)
{
    FlushBatch();
    // GL's scissor origin is bottom-left; the framework's is top-left.
    int glY = mHeight - (theRect.mY + theRect.mHeight);
    mState.SetScissorRect(Rect(theRect.mX, glY, theRect.mWidth, theRect.mHeight));
    mState.EnableScissor(true);
}

void GLRenderDevice::ClearClipRect()
{
    FlushBatch();
    mState.EnableScissor(false);
}

void GLRenderDevice::PushTransform(const SexyMatrix3& theTransform, bool concatenate)
{
    // The Graphics layer owns the transform stack; the device just needs to
    // flush because queued vertices were transformed with the previous matrix.
    FlushBatch();
}

void GLRenderDevice::PopTransform()
{
    FlushBatch();
}

void GLRenderDevice::SetTransform(const SexyMatrix3& theTransform)
{
    // Queued vertices were transformed with the previous matrix, so they must
    // be submitted before the uniform changes.
    FlushBatch();
    mCurrentTransform = theTransform;
}

Rect GLRenderDevice::ApplyTransform(const Rect& theRect) const
{
    // The shader applies the transform, so the CPU-side rect passes through.
    return theRect;
}

// ---------------------------------------------------------------------------
// batching
// ---------------------------------------------------------------------------

void GLRenderDevice::FlushBatch()
{
    if (mBatch.GetCount() == 0) return;
    mBatch.Flush();
}

unsigned int GLRenderDevice::PackColor(const Color& theColor) const
{
    // GLTexture::Blt @0x471cd4: ToRGBA, premultiply RGB by alpha with the
    // (c*a+128+((c*a+128)>>8))>>8 rounding, then SetColor stores those bytes.
    // AddVertex copies that packed word onto the vertex. A negative sine
    // alpha from the pulse overlay clamps to 0, which is a transparent white.
    int aAlpha = theColor.mAlpha;
    if (aAlpha < 0)
        aAlpha = 0;
    if (aAlpha > 255)
        aAlpha = 255;
    int aRed = theColor.mRed;
    int aGreen = theColor.mGreen;
    int aBlue = theColor.mBlue;
    if (aRed < 0)
        aRed = 0;
    if (aGreen < 0)
        aGreen = 0;
    if (aBlue < 0)
        aBlue = 0;
    if (aRed > 255)
        aRed = 255;
    if (aGreen > 255)
        aGreen = 255;
    if (aBlue > 255)
        aBlue = 255;
    int aPremulRed = aRed * aAlpha + 128;
    int aPremulGreen = aGreen * aAlpha + 128;
    int aPremulBlue = aBlue * aAlpha + 128;
    aPremulRed = (aPremulRed + (aPremulRed >> 8)) >> 8;
    aPremulGreen = (aPremulGreen + (aPremulGreen >> 8)) >> 8;
    aPremulBlue = (aPremulBlue + (aPremulBlue >> 8)) >> 8;
    return (unsigned int)aPremulRed
         | ((unsigned int)aPremulGreen << 8)
         | ((unsigned int)aPremulBlue << 16)
         | ((unsigned int)aAlpha << 24);
}

GLTexture* GLRenderDevice::EnsureTexture(MemoryImage* theImage, int& theOriginX, int& theOriginY)
{
    theOriginX = 0;
    theOriginY = 0;
    if (theImage == nullptr)
        return nullptr;

    // EnsureSrcTexture @0x474cf4 reuses the parent texture and adds the
    // sub-image origin to the source rectangle instead of uploading a crop.
    MemoryImage* sheet = theImage;
    while (sheet->mParent != nullptr)
    {
        theOriginX += sheet->mParentX;
        theOriginY += sheet->mParentY;
        sheet = sheet->mParent;
    }
    if (theOriginX < 0 || theOriginY < 0
        || theOriginX + theImage->mWidth > sheet->mWidth
        || theOriginY + theImage->mHeight > sheet->mHeight)
        return nullptr;

    // This backend supports one texture per sheet, up to 4096 pixels. Keep
    // the existing cropped upload for pieces of sheets requiring tiled storage.
    if (sheet->mWidth > 4096 || sheet->mHeight > 4096)
    {
        sheet = theImage;
        theOriginX = 0;
        theOriginY = 0;
    }

    auto it = mTextureCache.find(sheet);
    if (it != mTextureCache.end())
    {
        GLTexture* tex = it->second;
        bool sizeChanged = tex->mSrcWidth != sheet->mWidth || tex->mSrcHeight != sheet->mHeight;
        // BitsChanged propagates from a sub-image to its parent sheet.
        // A reloaded image can also reuse a cached image address. Recreate
        // its texture when the source dimensions change; uploading through
        // the old dimensions clips font atlases and clamps glyphs to black.
        if (sizeChanged || tex->mDirty || sheet->mBitsChanged || sheet->mBitsChangedCount != 0)
        {
            FlushBatch();
            size_t previousBytes = (size_t)tex->mSrcWidth * tex->mSrcHeight * 4;
            // Deleting a bound GL texture unbinds it, and GL can reuse its
            // name immediately. Drop the cached binding before recreation
            // so BeginImageBatch will bind the replacement even with that name.
            if (sizeChanged)
                mState.BindTexture(0, 0);
            tex->MarkDirty();
            if (!tex->CheckCreateTextures(sheet))
            {
                mTextureCacheBytes -= previousBytes;
                mState.BindTexture(0, 0);
                delete tex;
                mTextureCache.erase(it);
                return nullptr;
            }
            mTextureCacheBytes = mTextureCacheBytes - previousBytes
                + (size_t)tex->mSrcWidth * tex->mSrcHeight * 4;
        }
        return tex;
    }

    GLTexture* tex = new GLTexture();
    if (!tex->CheckCreateTextures(sheet))
    {
        delete tex;
        return nullptr;
    }

    mTextureCache[sheet] = tex;
    mTextureCacheBytes += (size_t)sheet->mWidth * sheet->mHeight * 4;
    return tex;
}

// ---------------------------------------------------------------------------
// drawing
// ---------------------------------------------------------------------------

void GLRenderDevice::BeginImageBatch(GLTexture* theTexture, DrawMode theMode, bool theColorize)
{
    // GLGraphics::DrawTexture @0x476e80 queues transformed vertices too.
    // Only a changed draw state or a full buffer needs a new submission.
    if (mBatch.GetCount() > 0
        && (mCurrentBatchTexture != theTexture->mTextureId || mCurrentBatchMode != theMode
            || mCurrentBatchColorize != theColorize))
        FlushBatch();

    ApplyDrawMode(theMode);

    if (mBatch.GetCount() == 0)
    {
        mShaders.mBlit.Bind();
        mShaders.mBlit.SetTexture(0);
        mShaders.mBlit.SetTransform(mCurrentTransform);
        mShaders.mBlit.SetColorize(theColorize);

        mState.BindTexture(0, theTexture->mTextureId);
        mBatch.Begin(theTexture->mTextureId);
        mCurrentBatchTexture = theTexture->mTextureId;
        mCurrentBatchMode = theMode;
        mCurrentBatchColorize = theColorize;
    }
}

void GLRenderDevice::DrawImage(MemoryImage* theImage, const Rect& theDestRect,
                               const Rect& theSrcRect, const Color& theColor,
                               DrawMode theMode, bool theMirror,
                               bool theFastStretch, bool theColorize)
{
    if (!mInitialized || !theImage) return;
    if (theDestRect.mWidth <= 0 || theDestRect.mHeight <= 0) return;
    if (theSrcRect.mWidth <= 0 || theSrcRect.mHeight <= 0) return;

    int originX, originY;
    GLTexture* tex = EnsureTexture(theImage, originX, originY);
    if (!tex || !tex->mTextureId) return;

    BeginImageBatch(tex, theMode, theColorize);

    // The colour rides the vertex. SetColor stays so a caller that still
    // reads the uniform sees the same premultiplied value.
    mShaders.mBlit.SetColor(theColor);
    unsigned int aPacked = PackColor(theColor);

    // GLTexture::Blt @0x471cd4 feeds DrawTexture the tile uv from
    // GetTexture, with no half-texel shift. A 1:1 blit is
    // col / width .. (col + span) / width.
    float aTextureLeft = tex->mU0 + (float)(originX + theSrcRect.mX) / (float)tex->mWidth;
    float aTextureTop = tex->mV0 + (float)(originY + theSrcRect.mY) / (float)tex->mHeight;
    float aTextureRight = tex->mU0 + (float)(originX + theSrcRect.mX + theSrcRect.mWidth) / (float)tex->mWidth;
    float aTextureBottom = tex->mV0 + (float)(originY + theSrcRect.mY + theSrcRect.mHeight) / (float)tex->mHeight;

    if (theMirror)
        std::swap(aTextureLeft, aTextureRight);

    mBatch.AddQuad((float)theDestRect.mX, (float)theDestRect.mY,
                   (float)(theDestRect.mX + theDestRect.mWidth),
                   (float)(theDestRect.mY + theDestRect.mHeight),
                   aTextureLeft, aTextureTop, aTextureRight, aTextureBottom, aPacked);
}

void GLRenderDevice::DrawYuv(unsigned int theY, unsigned int theCb, unsigned int theCr,
                             int theWidth, int theHeight, const Rect& theDestRect)
{
    if (!mInitialized || theY == 0 || theCb == 0 || theCr == 0)
        return;
    if (theWidth <= 0 || theHeight <= 0)
        return;
    if (theDestRect.mWidth <= 0 || theDestRect.mHeight <= 0)
        return;
    if (!mShaders.mYuv.IsValid())
        return;

    FlushBatch();
    ApplyDrawMode(DRAWMODE_NORMAL);
    mShaders.mYuv.Bind();
    // The blit path leaves this at identity and bakes the centering shift
    // into the quad. A leftover matrix here slides the movie off the window.
    SexyMatrix3 aIdentity;
    aIdentity.LoadIdentity();
    mShaders.mYuv.SetTransform(aIdentity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, theY);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, theCb);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, theCr);
    glActiveTexture(GL_TEXTURE0);
    mState.BindTexture(0, 0);

    mBatch.Begin(theY);
    mBatch.AddQuad((float)theDestRect.mX, (float)theDestRect.mY,
                   (float)(theDestRect.mX + theDestRect.mWidth),
                   (float)(theDestRect.mY + theDestRect.mHeight),
                   0.0f, 0.0f, 1.0f, 1.0f, 0xFFFFFFFFu);
    FlushBatch();

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    mState.BindTexture(0, 0);
}

void GLRenderDevice::DrawImageMatrix(MemoryImage* theImage, const SexyMatrix3& theMatrix,
                                     float theX, float theY, const Rect& theSrcRect,
                                     const Color& theColor, DrawMode theMode, bool theCenter)
{
    if (!mInitialized || !theImage) return;

    int originX, originY;
    GLTexture* tex = EnsureTexture(theImage, originX, originY);
    if (!tex || !tex->mTextureId) return;

    // GLTexture::BltTransformed @0x4734ac. theCenter is the last argument of
    // GLImage::BltTransformed, which GLImage::BltMatrix always passes as true.
    // A centered quad is built from the source rect itself: the origin is the
    // middle of that rect, and (theX, theY) is added after the matrix multiply.
    // The four corners are passed to DrawTexture in the original order.
    float aOriginX = 0.0f;
    float aOriginY = 0.0f;
    if (theCenter)
    {
        aOriginX = (float)-theSrcRect.mWidth * 0.5f;
        aOriginY = (float)-theSrcRect.mHeight * 0.5f;
    }
    float aSpanX = (float)theSrcRect.mWidth;
    float aSpanY = (float)theSrcRect.mHeight;
    SexyVector2 aLocal[4] = {
        SexyVector2(aOriginX, aOriginY),
        SexyVector2(aOriginX, aOriginY + aSpanY),
        SexyVector2(aOriginX + aSpanX, aOriginY),
        SexyVector2(aOriginX + aSpanX, aOriginY + aSpanY)
    };
    // GLTexture::BltTransformed @0x4736a2 calls operator*(SexyVector2) @0x341b04.
    // x' = x*m00 + y*m01 + m02, y' = x*m10 + y*m11 + m12.
    float aScreenX[4];
    float aScreenY[4];
    for (int aCorner = 0; aCorner < 4; ++aCorner)
    {
        float aX = aLocal[aCorner].mX;
        float aY = aLocal[aCorner].mY;
        aScreenX[aCorner] = aX * theMatrix.m00 + aY * theMatrix.m01 + theMatrix.m02 + theX;
        aScreenY[aCorner] = aX * theMatrix.m10 + aY * theMatrix.m11 + theMatrix.m12 + theY;
    }

    BeginImageBatch(tex, theMode, false);
    unsigned int aPacked = PackColor(theColor);

    float aTextureLeft = tex->mU0 + (float)(originX + theSrcRect.mX) / (float)tex->mWidth;
    float aTextureTop = tex->mV0 + (float)(originY + theSrcRect.mY) / (float)tex->mHeight;
    float aTextureRight = tex->mU0 + (float)(originX + theSrcRect.mX + theSrcRect.mWidth) / (float)tex->mWidth;
    float aTextureBottom = tex->mV0 + (float)(originY + theSrcRect.mY + theSrcRect.mHeight) / (float)tex->mHeight;

    if (mBatch.mCount + 6 > mBatch.mCapacity)
        mBatch.Flush();
    // DrawTexture takes the four corners in the order written at 0x4734ac:
    // top-left, bottom-left, top-right, bottom-right.
    GLVertex* aOut = mBatch.mVertices + mBatch.mCount;
    aOut[0] = { aScreenX[0], aScreenY[0], aTextureLeft, aTextureTop, aPacked };
    aOut[1] = { aScreenX[1], aScreenY[1], aTextureLeft, aTextureBottom, aPacked };
    aOut[2] = { aScreenX[2], aScreenY[2], aTextureRight, aTextureTop, aPacked };
    aOut[3] = { aScreenX[2], aScreenY[2], aTextureRight, aTextureTop, aPacked };
    aOut[4] = { aScreenX[1], aScreenY[1], aTextureLeft, aTextureBottom, aPacked };
    aOut[5] = { aScreenX[3], aScreenY[3], aTextureRight, aTextureBottom, aPacked };
    mBatch.mCount += 6;
}

void GLRenderDevice::DrawTriangles(MemoryImage* theImage, const SexyVertex2D (*theTriangles)[3],
                                   int theTriangleCount, const Color& theColor, DrawMode theMode,
                                   float theTx, float theTy)
{
    FlushBatch();
    if (!mInitialized || theImage == nullptr || theTriangles == nullptr || theTriangleCount <= 0)
        return;

    int originX, originY;
    GLTexture* tex = EnsureTexture(theImage, originX, originY);
    if (tex == nullptr || tex->mTextureId == 0)
        return;

    ApplyDrawMode(theMode);
    mShaders.mBlit.Bind();
    mShaders.mBlit.SetTexture(0);
    mShaders.mBlit.SetTransform(mCurrentTransform);
    mShaders.mBlit.SetColor(theColor);
    mShaders.mBlit.SetColorize(false);
    unsigned int aPacked = PackColor(theColor);
    mState.BindTexture(0, tex->mTextureId);
    mBatch.Begin(tex->mTextureId);

    float aInvW = (tex->mWidth > 0) ? 1.0f / (float)tex->mWidth : 0.0f;
    float aInvH = (tex->mHeight > 0) ? 1.0f / (float)tex->mHeight : 0.0f;
    for (int aTri = 0; aTri < theTriangleCount; ++aTri)
    {
        if (mBatch.mCount + 3 > mBatch.mCapacity)
            mBatch.Flush();
        GLVertex* aOut = mBatch.mVertices + mBatch.mCount;
        for (int aCorner = 0; aCorner < 3; ++aCorner)
        {
            const SexyVertex2D& aIn = theTriangles[aTri][aCorner];
            aOut[aCorner].mX = aIn.mX + theTx;
            aOut[aCorner].mY = aIn.mY + theTy;
            // A vertex uv is already 0..1 across the image. The texture may be
            // a padded atlas page, so shift into that page.
            aOut[aCorner].mU = tex->mU0 + ((float)originX + aIn.mU * (float)theImage->mWidth) * aInvW;
            aOut[aCorner].mV = tex->mV0 + ((float)originY + aIn.mV * (float)theImage->mHeight) * aInvH;
            aOut[aCorner].mColor = aPacked;
        }
        mBatch.mCount += 3;
    }
    mBatch.Flush();
    mCurrentBatchTexture = 0;
}

void GLRenderDevice::FillRect(const Rect& theRect, const Color& theColor, DrawMode theMode)
{
    if (!mInitialized) return;
    if (theRect.mWidth <= 0 || theRect.mHeight <= 0) return;

    // Solid geometry uses the untextured program, so it cannot share a batch
    // with textured quads.
    if (mBatch.GetCount() > 0 && mCurrentBatchTexture != 0)
        FlushBatch();

    ApplyDrawMode(theMode);

    if (mBatch.GetCount() == 0)
    {
        mShaders.mSolid.Bind();
        mShaders.mSolid.SetTransform(mCurrentTransform);
        mBatch.Begin(0);
        mCurrentBatchTexture = 0;
    }

    mShaders.mSolid.SetColor(theColor);
    mBatch.AddSolidQuad((float)theRect.mX, (float)theRect.mY,
                        (float)(theRect.mX + theRect.mWidth),
                        (float)(theRect.mY + theRect.mHeight),
                        PackColor(theColor));
}

void GLRenderDevice::DrawRect(const Rect& theRect, const Color& theColor, DrawMode theMode)
{
    if (theRect.mWidth <= 0 || theRect.mHeight <= 0) return;

    // Four 1px edges, matching the software DrawRect.
    FillRect(Rect(theRect.mX, theRect.mY, theRect.mWidth, 1), theColor, theMode);
    FillRect(Rect(theRect.mX, theRect.mY + theRect.mHeight - 1, theRect.mWidth, 1),
             theColor, theMode);
    FillRect(Rect(theRect.mX, theRect.mY, 1, theRect.mHeight), theColor, theMode);
    FillRect(Rect(theRect.mX + theRect.mWidth - 1, theRect.mY, 1, theRect.mHeight),
             theColor, theMode);
}

void GLRenderDevice::DrawLine(float x1, float y1, float x2, float y2,
                              const Color& theColor, DrawMode theMode)
{
    // The game draws very few lines (mostly debug overlays), so a simple
    // axis-aligned approximation is not good enough; emit a thin quad along
    // the line instead.
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    // Perpendicular offset for a 1px-wide quad.
    float nx = -dy / len * 0.5f;
    float ny =  dx / len * 0.5f;

    if (!mInitialized) return;
    if (mBatch.GetCount() > 0 && mCurrentBatchTexture != 0)
        FlushBatch();

    ApplyDrawMode(theMode);
    if (mBatch.GetCount() == 0)
    {
        mShaders.mSolid.Bind();
        mShaders.mSolid.SetTransform(mCurrentTransform);
        mBatch.Begin(0);
        mCurrentBatchTexture = 0;
    }
    mShaders.mSolid.SetColor(theColor);
    unsigned int aPacked = PackColor(theColor);

    // Two triangles forming the line quad.
    GLVertex* v = mBatch.mVertices + mBatch.mCount;
    if (mBatch.mCount + 6 > mBatch.mCapacity) mBatch.Flush();

    v[0] = { x1 + nx, y1 + ny, 0, 0, aPacked };
    v[1] = { x1 - nx, y1 - ny, 0, 0, aPacked };
    v[2] = { x2 - nx, y2 - ny, 0, 0, aPacked };
    v[3] = { x1 + nx, y1 + ny, 0, 0, aPacked };
    v[4] = { x2 - nx, y2 - ny, 0, 0, aPacked };
    v[5] = { x2 + nx, y2 + ny, 0, 0, aPacked };
    mBatch.mCount += 6;
}

// ---------------------------------------------------------------------------
// capture
// ---------------------------------------------------------------------------

bool GLRenderDevice::ReadPixels(MemoryImage& theTarget)
{
    if (!mInitialized) return false;

    FlushBatch();

    // The frame was drawn into the offscreen target. A device that never
    // created one has been drawing to the window, which is empty when the
    // window is hidden, so build the target and read that.
    if (mFBO == 0 && mWidth > 0 && mHeight > 0)
        CreateFBO(mWidth, mHeight);

    theTarget.Create(mWidth, mHeight);
    // Create sets the dimensions and deliberately leaves the pixel buffer
    // empty; the transcribed software path allocates it on first use. The
    // capture has no such use, so allocate it here.
    if (theTarget.mBits == nullptr)
        theTarget.mBits = new unsigned long[(size_t)mWidth * mHeight];

    glBindFramebuffer(GL_FRAMEBUFFER, mFBO ? mFBO : 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    // Read as RGBA then repack to the framework's 0xAARRGGBB.
    std::vector<unsigned char> rgba((size_t)mWidth * mHeight * 4);
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    unsigned int aError = glGetError();
    if (aError != GL_NO_ERROR)
        fprintf(stderr, "ReadPixels: gl error 0x%X fbo=%u %dx%d\n",
                aError, mFBO, mWidth, mHeight);

    for (int y = 0; y < mHeight; ++y)
    {
        // GL's origin is bottom-left; the framework's is top-left.
        const unsigned char* src = rgba.data() + (size_t)(mHeight - 1 - y) * mWidth * 4;
        unsigned* dst = (unsigned*)theTarget.mBits + (size_t)y * mWidth;
        for (int x = 0; x < mWidth; ++x)
        {
            unsigned r = src[x * 4 + 0];
            unsigned g = src[x * 4 + 1];
            unsigned b = src[x * 4 + 2];
            unsigned a = src[x * 4 + 3];
            dst[x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    return true;
}

} // namespace Sexy
