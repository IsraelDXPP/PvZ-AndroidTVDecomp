// GLRenderDevice.h -- the OpenGL 2D render device.
//
// This is the Windows counterpart of the binary's GL backend:
//
//     Sexy::RenderDevice            (29-slot interface, 2D)
//       -> Sexy::RenderDevice3D     (79-slot, adds 3D state)
//         -> BaseRenderDevice<...>
//           -> BaseOpenGLRenderDevice<...>   (102 slots)
//             -> Sexy::AndroidRenderDeviceES20 / ES11
//
// We implement the 2D half (RenderDevice) for desktop GL 3.3 core, which is what
// the game's Graphics/Image layer actually drives. The 3D half is present as a
// stub surface so the TodLib 3D paths link, and is filled in when needed.
//
// Key design point, taken from the original: the device owns no game state. It
// receives already-transformed pixel coordinates plus a texture and blend mode,
// and issues batched quads. Everything above it (Graphics, Widget, Board,
// Zombie) is unchanged between the software and GL backends, which is exactly
// why the reconstruction can share one game layer across both.

#ifndef __GLRENDERDEVICE_H__
#define __GLRENDERDEVICE_H__

#include "GLHeaders.h"
#include "GLShader.h"
#include "GLStateManager.h"
#include "GLTexture.h"
#include "GLVertexBuffer.h"
#include "MemoryImage.h"
#include "CoreTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class GLRenderDevice
{
public:
    // The device the frame loop presents. The game draws into the device's
    // offscreen target, and nothing in the transcribed frame path copies that
    // target to the window, so the loop calls Present on this one.
    static GLRenderDevice* sCurrent;
public:
    // Which blend the current draw uses; mirrors Graphics::DRAWMODE_*.
    enum DrawMode
    {
        DRAWMODE_NORMAL = 0,
        DRAWMODE_ADDITIVE
    };

    GLShaderSet         mShaders;
    GLStateManager      mState;
    GLVertexBuffer      mBatch;

    int                 mWidth;
    int                 mHeight;
    bool                mInitialized;

    // Scratch target for ReadPixels-based capture, so GL and software output
    // can be compared byte-for-byte.
    MemoryImage*        mReadbackImage;

    // Texture bound by the batch currently being assembled. A change forces a
    // flush, since one draw call can only sample one texture.
    unsigned int        mCurrentBatchTexture;

    // Transform applied to every vertex in the current batch. The Graphics
    // layer owns the stack; this is the top of it, mirrored here so the shader
    // uniform can be set once per batch instead of once per quad.
    SexyMatrix3         mCurrentTransform;

    // The offscreen framebuffer the game renders into before presentation.
    // Rendering to an FBO keeps the pipeline identical to the Android build,
    // which always drew into a texture-backed surface.
    unsigned int        mFBO;
    unsigned int        mFBOTexture;
    unsigned int        mFBOColorRB;

    // GPU textures, keyed by the Image they were uploaded from.
    //
    // The original keeps one GLImage per Image and reuses it across draws. An
    // earlier version of this device uploaded per draw instead, so every reanim
    // track allocated a fresh GL texture each frame and re-uploaded the whole
    // atlas -- a leak that grew the process by gigabytes.
    //
    // Textures are created lazily and live until Shutdown; the images they
    // mirror are owned by the resource manager / atlas cache, which outlives
    // every draw.
    std::map<const MemoryImage*, GLTexture*>    mTextureCache;
    size_t                                      mTextureCacheBytes;

public:
    GLRenderDevice();
    ~GLRenderDevice();

    // --- lifecycle ----------------------------------------------------------
    bool    Init(int theWidth, int theHeight);
    void    Shutdown();
    void    Resize(int theWidth, int theHeight);

    // --- frame --------------------------------------------------------------
    void    BeginFrame(const Color& theClearColor);
    void    EndFrame();
    void    Present();              // blit the FBO to the default framebuffer

    // Upload a finished software frame into the offscreen target. TitleScreen
    // and the other widgets paint a MemoryImage; this is how that image
    // reaches the window without a second compositor.
    void    BlitImage(MemoryImage* theImage);

    // --- state --------------------------------------------------------------
    void    SetDrawMode(DrawMode theMode);
    void    SetColorizeImages(bool theColorize);
    void    SetClipRect(const Rect& theRect);
    void    ClearClipRect();
    void    PushTransform(const SexyMatrix3& theTransform, bool concatenate);
    void    PopTransform();

    // Set the transform applied to subsequently queued geometry. The Graphics
    // layer calls this whenever its own transform changes.
    void    SetTransform(const SexyMatrix3& theTransform);
    const SexyMatrix3& GetTransform() const { return mCurrentTransform; }

    // --- drawing ------------------------------------------------------------
    // All coordinates are in pixels, top-left origin, matching the framework.
    void    DrawImage(MemoryImage* theImage, const Rect& theDestRect,
                      const Rect& theSrcRect, const Color& theColor,
                      DrawMode theMode, bool theMirror = false,
                      bool theFastStretch = true, bool theColorize = false);
    void    DrawImageMatrix(MemoryImage* theImage, const SexyMatrix3& theMatrix,
                            const Rect& theSrcRect, const Color& theColor,
                            DrawMode theMode);
    void    FillRect(const Rect& theRect, const Color& theColor, DrawMode theMode);
    void    DrawRect(const Rect& theRect, const Color& theColor, DrawMode theMode);
    void    DrawLine(float x1, float y1, float x2, float y2,
                     const Color& theColor, DrawMode theMode);

    // --- capture ------------------------------------------------------------
    // Read the current framebuffer back into an ARGB MemoryImage. Used by the
    // screenshot path and by the GL-vs-software comparison test.
    bool    ReadPixels(MemoryImage& theTarget);

    // --- diagnostics --------------------------------------------------------
    unsigned int GetDrawCalls() const { return mBatch.GetDrawCalls(); }
    unsigned int GetStateChanges() const { return mState.GetStateChanges(); }
    unsigned int GetAvoidedStateCalls() const { return mState.GetAvoidedCalls(); }

    // Number of GPU textures currently resident, and their approximate bytes.
    // Exposed so a leak shows up in a test rather than only in Task Manager.
    size_t  GetTextureCount() const { return mTextureCache.size(); }
    size_t  GetTextureBytes() const { return mTextureCacheBytes; }

private:
    void    FlushBatch();
    void    EnsureTexture(MemoryImage* theImage);
    void    ApplyDrawMode(DrawMode theMode);
    Rect    ApplyTransform(const Rect& theRect) const;
    void    CreateFBO(int theWidth, int theHeight);
    void    DestroyFBO();
};

} // namespace Sexy



#endif // __GLRENDERDEVICE_H__

