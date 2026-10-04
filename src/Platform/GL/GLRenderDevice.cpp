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
      mCurrentBatchTexture(0), mFBO(0), mFBOTexture(0), mFBOColorRB(0),
      mTextureCacheBytes(0)
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

    // The window's back buffer, not the size the device was created at.
    // GLFW's initial viewport is the window's creation size, and a later
    // resize (or a framebuffer larger than the requested client area) leaves
    // it there. Blitting the 1280x720 target into that viewport fills only
    // the top-left corner and leaves the rest of the window black.
    GLint aViewport[4] = {0, 0, 0, 0};
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glGetIntegerv(GL_VIEWPORT, aViewport);
    int aWindowWidth = aViewport[2];
    int aWindowHeight = aViewport[3];
    if (aWindowWidth <= 0 || aWindowHeight <= 0)
    {
        aWindowWidth = mWidth;
        aWindowHeight = mHeight;
    }

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
        // Additive: src*a + dst. Matches the software blitter's DRAWMODE_ADDITIVE.
        mState.SetBlend(GLStateManager::BLEND_SRC_ALPHA, GLStateManager::BLEND_ONE);
    }
    else
    {
        // Normal: src*a + dst*(1-a).
        mState.SetBlend(GLStateManager::BLEND_SRC_ALPHA,
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

void GLRenderDevice::EnsureTexture(MemoryImage* theImage)
{
    if (!theImage) return;

    // Reuse the texture already uploaded from this image.
    //
    // Keying on the Image pointer is what makes this safe AND cheap: the
    // atlases and game images are long-lived objects owned by the atlas cache
    // and the resource manager, both of which outlive every draw. Uploading per
    // draw instead allocated a new GL texture every frame.
    auto it = mTextureCache.find(theImage);
    if (it != mTextureCache.end())
    {
        GLTexture* tex = it->second;

        // The image may have been repainted since the upload. A software
        // target such as the widget manager's image keeps the same object and
        // only sets mBitsChanged, so that flag has to re-upload too.
        if (tex->mDirty || theImage->mBitsChanged)
            tex->Upload(theImage);
        return;
    }

    GLTexture* tex = new GLTexture();
    if (!tex->CheckCreateTextures(theImage))
    {
        delete tex;
        return;
    }

    mTextureCache[theImage] = tex;
    // MemoryImage exposes the dimensions as the Image data members mWidth and
    // mHeight (Image.h +0x24/+0x28); they are not accessor functions.
    mTextureCacheBytes += (size_t)theImage->mWidth * theImage->mHeight * 4;
}

// ---------------------------------------------------------------------------
// drawing
// ---------------------------------------------------------------------------

void GLRenderDevice::DrawImage(MemoryImage* theImage, const Rect& theDestRect,
                               const Rect& theSrcRect, const Color& theColor,
                               DrawMode theMode, bool theMirror,
                               bool theFastStretch, bool theColorize)
{
    if (!mInitialized || !theImage) return;
    if (theDestRect.mWidth <= 0 || theDestRect.mHeight <= 0) return;
    if (theSrcRect.mWidth <= 0 || theSrcRect.mHeight <= 0) return;

    EnsureTexture(theImage);
    auto texIt = mTextureCache.find(theImage);
    GLTexture* tex = (texIt != mTextureCache.end()) ? texIt->second : nullptr;
    if (!tex || !tex->mTextureId) return;

    // Texture changes force a flush: one texture per batch.
    if (mBatch.GetCount() > 0 && mCurrentBatchTexture != tex->mTextureId)
        FlushBatch();

    ApplyDrawMode(theMode);

    if (mBatch.GetCount() == 0)
    {
        mShaders.mBlit.Bind();
        mShaders.mBlit.SetTexture(0);
        mShaders.mBlit.SetTransform(mCurrentTransform);
        mShaders.mBlit.SetColorize(theColorize);

        mState.BindTexture(0, tex->mTextureId);
        mBatch.Begin(tex->mTextureId);
        mCurrentBatchTexture = tex->mTextureId;
    }

    mShaders.mBlit.SetColor(theColor);

    // uv rect of the source region inside the texture.
    //
    // Half-destination-pixel correction. This is what makes the GPU match the
    // software blitter's sampling *exactly*, including under scaling.
    //
    // GL rasterises fragment centres at destX + d + 0.5 and interpolates uv
    // linearly, so with an edge-to-edge uv range the sampled source position is
    //     srcX + (d + 0.5) * srcW/destW        (NEAREST => floor of this)
    // whereas MemoryImage::FastStretchBlt uses a truncating step function
    //     srcX + (int)(d * srcW/destW)
    // The two differ from d = 1 onward whenever srcW/destW is not an integer
    // (e.g. the 1280->800 background: floor(1.6d + 0.8) vs (int)(1.6d)).
    //
    // Shifting the uv range back by half a destination pixel's worth of source
    // removes the +0.5 term and reduces the GPU's expression to floor(d * scale),
    // which is identical to the CPU's truncation. Measured on the 1280x720
    // background scaled to 800x600: channels differing by >30 dropped from 3742
    // to 455.
    float uRange = (float)theSrcRect.mWidth  / (float)tex->mWidth;
    float vRange = (float)theSrcRect.mHeight / (float)tex->mHeight;
    float uShift = (theDestRect.mWidth  > 0) ? 0.5f * uRange / (float)theDestRect.mWidth  : 0.0f;
    float vShift = (theDestRect.mHeight > 0) ? 0.5f * vRange / (float)theDestRect.mHeight : 0.0f;

    float u0 = tex->mU0 + (float)theSrcRect.mX / (float)tex->mWidth  - uShift;
    float v0 = tex->mV0 + (float)theSrcRect.mY / (float)tex->mHeight - vShift;
    float u1 = tex->mU0 + (float)(theSrcRect.mX + theSrcRect.mWidth)  / (float)tex->mWidth  - uShift;
    float v1 = tex->mV0 + (float)(theSrcRect.mY + theSrcRect.mHeight) / (float)tex->mHeight - vShift;

    if (theMirror)
        std::swap(u0, u1);

    mBatch.AddQuad((float)theDestRect.mX, (float)theDestRect.mY,
                   (float)(theDestRect.mX + theDestRect.mWidth),
                   (float)(theDestRect.mY + theDestRect.mHeight),
                   u0, v0, u1, v1);
}

void GLRenderDevice::DrawImageMatrix(MemoryImage* theImage, const SexyMatrix3& theMatrix,
                                     const Rect& theSrcRect, const Color& theColor,
                                     DrawMode theMode)
{
    // A rotated draw cannot share the axis-aligned batch: flush, then submit a
    // single transformed quad with its own matrix uniform.
    FlushBatch();

    if (!mInitialized || !theImage) return;

    EnsureTexture(theImage);
    auto texIt = mTextureCache.find(theImage);
    GLTexture* tex = (texIt != mTextureCache.end()) ? texIt->second : nullptr;
    if (!tex || !tex->mTextureId) return;

    ApplyDrawMode(theMode);
    mShaders.mBlit.Bind();
    mShaders.mBlit.SetTexture(0);
    mShaders.mBlit.SetTransform(theMatrix);
    mShaders.mBlit.SetColor(theColor);
    mShaders.mBlit.SetColorize(false);

    mState.BindTexture(0, tex->mTextureId);

    float u0 = tex->mU0 + (float)theSrcRect.mX / (float)tex->mWidth;
    float v0 = tex->mV0 + (float)theSrcRect.mY / (float)tex->mHeight;
    float u1 = tex->mU0 + (float)(theSrcRect.mX + theSrcRect.mWidth) / (float)tex->mWidth;
    float v1 = tex->mV0 + (float)(theSrcRect.mY + theSrcRect.mHeight) / (float)tex->mHeight;

    // The matrix already carries the placement, so draw at the origin.
    mBatch.Begin(tex->mTextureId);
    mBatch.AddQuad(0.0f, 0.0f,
                   (float)theSrcRect.mWidth, (float)theSrcRect.mHeight,
                   u0, v0, u1, v1);
    mBatch.Flush();
    mCurrentBatchTexture = 0;

    // Restore the frame transform for subsequent batched draws.
    mShaders.mBlit.SetTransform(mCurrentTransform);
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
                        (float)(theRect.mY + theRect.mHeight));
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

    // Two triangles forming the line quad.
    GLVertex* v = mBatch.mVertices + mBatch.mCount;
    if (mBatch.mCount + 6 > mBatch.mCapacity) mBatch.Flush();

    v[0] = { x1 + nx, y1 + ny, 0, 0 };
    v[1] = { x1 - nx, y1 - ny, 0, 0 };
    v[2] = { x2 - nx, y2 - ny, 0, 0 };
    v[3] = { x1 + nx, y1 + ny, 0, 0 };
    v[4] = { x2 - nx, y2 - ny, 0, 0 };
    v[5] = { x2 + nx, y2 + ny, 0, 0 };
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
