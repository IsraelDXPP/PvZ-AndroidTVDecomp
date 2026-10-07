#pragma once
#include <bgfx/bgfx.h>
#include "MemoryImage.h"
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace Sexy {
struct BgfxVertex { float mX, mY, mU, mV; uint32_t mColor; };
struct BgfxTexture
{
    bgfx::TextureHandle mHandle = BGFX_INVALID_HANDLE;
    int mWidth = 0, mHeight = 0, mSrcWidth = 0, mSrcHeight = 0;
    ~BgfxTexture();
    bool Upload(MemoryImage* theImage);
};

class BgfxRenderDevice
{
public:
    static BgfxRenderDevice* sCurrent;
    enum DrawMode { DRAWMODE_NORMAL, DRAWMODE_ADDITIVE };
    int mWidth = 0, mHeight = 0;
    bool mInitialized = false;
    BgfxRenderDevice();
    ~BgfxRenderDevice();
    bool Init(int theWidth, int theHeight);
    void Shutdown();
    void Resize(int theWidth, int theHeight);
    void BeginFrame(const Color& theClearColor);
    void EndFrame();
    void Present();
    void BlitImage(MemoryImage* theImage);
    MemoryImage* GetDrawTarget() const { return mDrawTarget; }
    void SetDrawTarget(MemoryImage* theImage) { mDrawTarget = theImage; }
    void SetDrawMode(DrawMode theMode);
    void SetColorizeImages(bool theColorize);
    void SetFilterEffect(FilterEffect theEffect);
    void SetClipRect(const Rect& theRect);
    void ClearClipRect();
    void SetTransform(const SexyMatrix3& theTransform);
    const SexyMatrix3& GetTransform() const { return mTransform; }
    void DrawImage(MemoryImage* theImage, const Rect& theDestRect, const Rect& theSrcRect,
                   const Color& theColor, DrawMode theMode, bool theMirror = false,
                   bool theFastStretch = true, bool theColorize = false);
    void DrawImageMatrix(MemoryImage* theImage, const SexyMatrix3& theMatrix,
                         float theX, float theY, const Rect& theSrcRect,
                         const Color& theColor, DrawMode theMode, bool theCenter = true);
    void DrawTriangles(MemoryImage* theImage, const SexyVertex2D (*theTriangles)[3],
                       int theTriangleCount, const Color& theColor, DrawMode theMode,
                       float theTx, float theTy);
    void FillRect(const Rect& theRect, const Color& theColor, DrawMode theMode);
    void DrawRect(const Rect& theRect, const Color& theColor, DrawMode theMode);
    void DrawLine(float theStartX, float theStartY, float theEndX, float theEndY,
                  const Color& theColor, DrawMode theMode);
    void DrawYuv(bgfx::TextureHandle theY, bgfx::TextureHandle theCb, bgfx::TextureHandle theCr,
                 int theWidth, int theHeight, const Rect& theDestRect);
    bool ReadPixels(MemoryImage& theTarget);
    void RemoveTexture(const MemoryImage* theImage);
    size_t GetTextureCount() const { return mTextures.size(); }
    size_t GetTextureBytes() const;
    unsigned int GetDrawCalls() const { return mDrawCalls; }
private:
    enum class Program { Blit, Solid, Yuv };
    bgfx::ProgramHandle mPrograms[3] = {BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};
    bgfx::UniformHandle mTransformUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle mViewportUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle mBlitUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle mTextureUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle mYuvUniforms[3] = {BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE, BGFX_INVALID_HANDLE};
    bgfx::FrameBufferHandle mTarget = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle mColorTarget = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle mReadback = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout mLayout;
    std::vector<BgfxVertex> mVertices;
    std::map<const MemoryImage*, std::unique_ptr<BgfxTexture>> mTextures;
    std::mutex mTextureRemovalMutex;
    std::vector<const MemoryImage*> mPendingTextureRemovals;
    MemoryImage* mDrawTarget = nullptr;
    SexyMatrix3 mTransform;
    Rect mClip;
    bool mClipped = false, mColorize = false;
    FilterEffect mFilterEffect = FILTER_EFFECT_NONE;
    bool mFrameOpen = false;
    Program mBatchProgram = Program::Blit;
    DrawMode mBatchMode = DRAWMODE_NORMAL;
    bgfx::TextureHandle mBatchTexture = BGFX_INVALID_HANDLE;
    bool mBatchColorize = false;
    uint32_t mBatchSampler = UINT32_MAX;
    unsigned int mDrawCalls = 0;
    int mWindowWidth = 0, mWindowHeight = 0;
    bool mWindowVSync = true;
    void FlushBatch();
    void BeginBatch(Program theProgram, DrawMode theMode, bgfx::TextureHandle theTexture,
                    bool theColorize = false, uint32_t theSampler = UINT32_MAX);
    void AddQuad(float theLeft, float theTop, float theRight, float theBottom,
                  float theUStart, float theVStart, float theUEnd, float theVEnd, uint32_t theColor);
    void Submit(bgfx::ViewId theView, Program theProgram, const BgfxVertex* theVertices,
                uint32_t theCount, const SexyMatrix3& theTransform, int theWidth, int theHeight,
                bool theBlend, DrawMode theMode, bool theColorize,
                bgfx::TextureHandle theTexture, uint32_t theSampler,
                bool theClip, const bgfx::TextureHandle* thePlanes = nullptr,
                FilterEffect theFilterEffect = FILTER_EFFECT_NONE);
    BgfxTexture* EnsureTexture(MemoryImage* theImage, int& theOriginX, int& theOriginY);
    void RemoveReleasedTextures();
    bool CreateTarget();
    void DestroyTarget();
    void FinishGpuWork();
    uint32_t PackColor(const Color& theColor) const;
};
}
