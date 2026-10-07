#include "BgfxRenderDevice.h"
#include "ShaderPackage.h"
#include "SDLWindow.h"
#include "ReconFreeFunctions.h"
#include "AndroidShims.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>

namespace Sexy {
BgfxRenderDevice* BgfxRenderDevice::sCurrent = nullptr;
namespace {
constexpr bgfx::ViewId SceneView = 0, ReadbackView = 1, PresentView = 2;
bgfx::RendererType::Enum RequestedRenderer(bool& valid)
{
    const char* setting = GetEnv("SEXY_VIDEO_DRIVER");
    std::string name = setting ? setting : "auto";
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char letter) { return char(std::tolower(letter)); });
    valid = true;
    if (name == "dx11" || name == "d3d11") return bgfx::RendererType::Direct3D11;
    if (name == "dx12" || name == "d3d12") return bgfx::RendererType::Direct3D12;
    if (name == "gl" || name == "opengl") return bgfx::RendererType::OpenGL;
    if (name == "vk" || name == "vulkan") return bgfx::RendererType::Vulkan;
    if (name == "metal") return bgfx::RendererType::Metal;
    if (name.empty() || name == "auto" || name == "bgfx") return bgfx::RendererType::Count;
    valid = false;
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Invalid SEXY_VIDEO_DRIVER=%s", name.c_str());
    return bgfx::RendererType::Count;
}
}
BgfxRenderDevice::BgfxRenderDevice() { mTransform.LoadIdentity(); mVertices.reserve(24576); }
BgfxRenderDevice::~BgfxRenderDevice() { Shutdown(); }
bool BgfxRenderDevice::Init(int width, int height)
{
    SDLWindow* window = ActiveWindow();
    if (mInitialized) return true;
    if (sCurrent || !window || !window->mWindow || width <= 0 || height <= 0) return false;
    bool valid = false;
    bgfx::Init init;
    init.type = RequestedRenderer(valid);
    if (!valid) return false;
    if (init.type != bgfx::RendererType::Count)
    {
        bgfx::RendererType::Enum supported[bgfx::RendererType::Count];
        uint8_t count = bgfx::getSupportedRenderers(bgfx::RendererType::Count, supported);
        if (std::find(supported, supported + count, init.type) == supported + count)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Requested renderer %s is unavailable on this build", bgfx::getRendererName(init.type));
            return false;
        }
    }
    SDL_PropertiesID properties = SDL_GetWindowProperties(window->mWindow);
    init.platformData.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (!init.platformData.nwh)
        init.platformData.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    if (!init.platformData.nwh)
    {
        init.platformData.nwh = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        if (init.platformData.nwh)
        {
            init.platformData.ndt = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
            init.platformData.type = bgfx::NativeWindowHandleType::Wayland;
        }
    }
    if (!init.platformData.nwh)
    {
        Sint64 windowId = SDL_GetNumberProperty(properties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        // bgfx's X11 API specifies the numeric XID as its opaque nwh token.
        // This conversion exists only at that external API boundary.
        init.platformData.nwh = reinterpret_cast<void*>(static_cast<size_t>(windowId));
        init.platformData.ndt = SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    }
    if (!init.platformData.nwh)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL video driver %s has no supported bgfx native window", SDL_GetCurrentVideoDriver());
        return false;
    }
    mWindowWidth = window->mWidth; mWindowHeight = window->mHeight;
    mWindowVSync = window->mVSync;
    init.resolution.width = mWindowWidth;
    init.resolution.height = mWindowHeight;
    init.resolution.reset = mWindowVSync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
    init.limits.transientVbSize = 32 * 1024 * 1024;
    if (!bgfx::init(init)) { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "bgfx initialization failed"); return false; }
    mInitialized = true;
    if (init.type != bgfx::RendererType::Count && bgfx::getRendererType() != init.type)
    { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "bgfx did not select the requested renderer"); Shutdown(); return false; }
    mWidth = width; mHeight = height;
    mLayout.begin().add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true).end();
    mTransformUniform = bgfx::createUniform("u_transform", bgfx::UniformType::Mat3);
    mViewportUniform = bgfx::createUniform("u_viewport", bgfx::UniformType::Vec4);
    mBlitUniform = bgfx::createUniform("u_blitParams", bgfx::UniformType::Vec4);
    mTextureUniform = bgfx::createUniform("s_texture", bgfx::UniformType::Sampler);
    const char* samplers[] = {"s_y", "s_cb", "s_cr"};
    for (int index = 0; index < 3; ++index)
        mYuvUniforms[index] = bgfx::createUniform(samplers[index], bgfx::UniformType::Sampler);
    const char* fragments[] = {"main.frag.scbp", "solid.frag.scbp", "yuv.frag.scbp"};
    for (int index = 0; index < 3; ++index)
    {
        bgfx::ShaderHandle vertex = LoadShaderPackage("main.vert.scbp", ShaderStage::Vertex);
        bgfx::ShaderHandle fragment = LoadShaderPackage(fragments[index], ShaderStage::Fragment);
        if (bgfx::isValid(vertex) && bgfx::isValid(fragment))
            mPrograms[index] = bgfx::createProgram(vertex, fragment, false);
        if (bgfx::isValid(vertex)) bgfx::destroy(vertex);
        if (bgfx::isValid(fragment)) bgfx::destroy(fragment);
        if (!bgfx::isValid(mPrograms[index])) { Shutdown(); return false; }
    }
    bgfx::setViewName(SceneView, "Game offscreen");
    bgfx::setViewName(ReadbackView, "Offscreen readback");
    bgfx::setViewName(PresentView, "Window presentation");
    bgfx::setViewMode(SceneView, bgfx::ViewMode::Sequential);
    if (!CreateTarget()) { Shutdown(); return false; }
    sCurrent = this;
    SDL_Log("bgfx renderer: %s (%dx%d)", bgfx::getRendererName(bgfx::getRendererType()), width, height);
    return true;
}
void BgfxRenderDevice::Shutdown()
{
    if (sCurrent == this) sCurrent = nullptr;
    if (!mInitialized) return;
    mVertices.clear();
    mTextures.clear();
    { std::lock_guard<std::mutex> lock(mTextureRemovalMutex); mPendingTextureRemovals.clear(); }
    DestroyTarget();
    for (auto& program : mPrograms) { if (bgfx::isValid(program)) bgfx::destroy(program); program = BGFX_INVALID_HANDLE; }
    bgfx::UniformHandle* uniforms[] = {&mTransformUniform, &mViewportUniform, &mBlitUniform, &mTextureUniform,
        &mYuvUniforms[0], &mYuvUniforms[1], &mYuvUniforms[2]};
    for (auto uniform : uniforms) { if (bgfx::isValid(*uniform)) bgfx::destroy(*uniform); *uniform = BGFX_INVALID_HANDLE; }
    bgfx::shutdown();
    mInitialized = false;
    mFrameOpen = false;
    mDrawTarget = nullptr;
}
bool BgfxRenderDevice::CreateTarget()
{
    mColorTarget = bgfx::createTexture2D(uint16_t(mWidth), uint16_t(mHeight), false, 1,
        bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    if (!bgfx::isValid(mColorTarget)) return false;
    mTarget = bgfx::createFrameBuffer(1, &mColorTarget, true);
    if (!bgfx::isValid(mTarget)) { bgfx::destroy(mColorTarget); mColorTarget = BGFX_INVALID_HANDLE; return false; }
    bgfx::setViewFrameBuffer(SceneView, mTarget);
    bgfx::setViewRect(SceneView, 0, 0, uint16_t(mWidth), uint16_t(mHeight));
    return true;
}
void BgfxRenderDevice::DestroyTarget()
{
    if (bgfx::isValid(mReadback)) bgfx::destroy(mReadback);
    if (bgfx::isValid(mTarget)) bgfx::destroy(mTarget);
    mReadback = BGFX_INVALID_HANDLE; mTarget = BGFX_INVALID_HANDLE; mColorTarget = BGFX_INVALID_HANDLE;
}
void BgfxRenderDevice::Resize(int width, int height)
{
    if (width <= 0 || height <= 0 || (width == mWidth && height == mHeight)) return;
    FlushBatch(); DestroyTarget(); mWidth = width; mHeight = height; CreateTarget();
}
void BgfxRenderDevice::BeginFrame(const Color& clear)
{
    if (!mInitialized) return;
    FlushBatch();
    RemoveReleasedTextures();
    mDrawCalls = 0;
    uint32_t rgba = (uint32_t(std::clamp(clear.mRed, 0, 255)) << 24)
        | (uint32_t(std::clamp(clear.mGreen, 0, 255)) << 16)
        | (uint32_t(std::clamp(clear.mBlue, 0, 255)) << 8) | uint32_t(std::clamp(clear.mAlpha, 0, 255));
    bgfx::setViewClear(SceneView, BGFX_CLEAR_COLOR, rgba);
    bgfx::setViewFrameBuffer(SceneView, mTarget);
    bgfx::touch(SceneView);
    mClipped = false; mColorize = false; mTransform.LoadIdentity(); mFrameOpen = true;
}
void BgfxRenderDevice::EndFrame() { FlushBatch(); }
void BgfxRenderDevice::Present()
{
    if (!mInitialized) return;
    FlushBatch();
    SDLWindow* window = ActiveWindow();
    if (!window) return;
    if (mWindowWidth != window->mWidth || mWindowHeight != window->mHeight || mWindowVSync != window->mVSync)
    {
        mWindowWidth = window->mWidth; mWindowHeight = window->mHeight; mWindowVSync = window->mVSync;
        bgfx::reset(mWindowWidth, mWindowHeight, mWindowVSync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE);
    }
    bgfx::setViewRect(PresentView, 0, 0, uint16_t(mWindowWidth), uint16_t(mWindowHeight));
    bool flip = bgfx::getCaps()->originBottomLeft;
    float top = flip ? 1.0f : 0.0f, bottom = flip ? 0.0f : 1.0f;
    BgfxVertex quad[] = {{0,0,0,top,0xffffffff}, {float(mWindowWidth),0,1,top,0xffffffff},
        {float(mWindowWidth),float(mWindowHeight),1,bottom,0xffffffff}, {0,0,0,top,0xffffffff},
        {float(mWindowWidth),float(mWindowHeight),1,bottom,0xffffffff}, {0,float(mWindowHeight),0,bottom,0xffffffff}};
    SexyMatrix3 identity; identity.LoadIdentity();
    Submit(PresentView, Program::Blit, quad, 6, identity, mWindowWidth, mWindowHeight,
        false, DRAWMODE_NORMAL, false, mColorTarget, UINT32_MAX, false);
    bgfx::frame();
    mFrameOpen = false;
}
void BgfxRenderDevice::BlitImage(MemoryImage* image)
{
    BeginFrame(Color(0,0,0,255));
    DrawImage(image, Rect(0,0,mWidth,mHeight), Rect(0,0,image->mWidth,image->mHeight), Color::White, DRAWMODE_NORMAL);
    EndFrame();
}
void BgfxRenderDevice::SetDrawMode(DrawMode mode) { FlushBatch(); mBatchMode = mode; }
void BgfxRenderDevice::SetColorizeImages(bool colorize) { FlushBatch(); mColorize = colorize; }
void BgfxRenderDevice::SetTransform(const SexyMatrix3& transform) { FlushBatch(); mTransform = transform; }
void BgfxRenderDevice::SetClipRect(const Rect& clip)
{
    FlushBatch();
    int left = std::clamp(clip.mX, 0, mWidth), top = std::clamp(clip.mY, 0, mHeight);
    int right = std::clamp(clip.mX + clip.mWidth, left, mWidth), bottom = std::clamp(clip.mY + clip.mHeight, top, mHeight);
    mClip = Rect(left, top, right - left, bottom - top); mClipped = true;
}
void BgfxRenderDevice::ClearClipRect() { FlushBatch(); mClipped = false; }
void BgfxRenderDevice::BeginBatch(Program program, DrawMode mode, bgfx::TextureHandle texture, bool colorize, uint32_t sampler)
{
    if (!mVertices.empty() && (program != mBatchProgram || mode != mBatchMode || texture.idx != mBatchTexture.idx
        || colorize != mBatchColorize || sampler != mBatchSampler)) FlushBatch();
    mBatchProgram = program; mBatchMode = mode; mBatchTexture = texture;
    mBatchColorize = colorize; mBatchSampler = sampler;
}
void BgfxRenderDevice::Submit(bgfx::ViewId view, Program program, const BgfxVertex* vertices, uint32_t count,
    const SexyMatrix3& transform, int width, int height, bool blend, DrawMode mode, bool colorize,
    bgfx::TextureHandle texture, uint32_t sampler, bool clip, const bgfx::TextureHandle* planes)
{
    if (clip && (mClip.mWidth <= 0 || mClip.mHeight <= 0)) return;
    if (bgfx::getAvailTransientVertexBuffer(count, mLayout) < count)
    { SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "bgfx transient vertex buffer exhausted"); return; }
    bgfx::TransientVertexBuffer buffer;
    bgfx::allocTransientVertexBuffer(&buffer, count, mLayout);
    std::memcpy(buffer.data, vertices, size_t(count) * sizeof(BgfxVertex));
    float matrix[] = {transform.m00,transform.m10,transform.m20,transform.m01,transform.m11,transform.m21,transform.m02,transform.m12,transform.m22};
    float viewport[] = {float(width), float(height), 0, 0};
    float params[] = {colorize ? 1.0f : 0.0f, 0, 0, 0};
    bgfx::setUniform(mTransformUniform, matrix);
    bgfx::setUniform(mViewportUniform, viewport);
    if (program == Program::Blit) { bgfx::setUniform(mBlitUniform, params); bgfx::setTexture(0, mTextureUniform, texture, sampler); }
    if (program == Program::Yuv && planes)
        for (uint8_t index = 0; index < 3; ++index) bgfx::setTexture(index, mYuvUniforms[index], planes[index]);
    bgfx::setVertexBuffer(0, &buffer);
    if (clip) bgfx::setScissor(uint16_t(mClip.mX), uint16_t(mClip.mY), uint16_t(mClip.mWidth), uint16_t(mClip.mHeight));
    uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
    if (blend) state |= mode == DRAWMODE_ADDITIVE ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE)
        : BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
    bgfx::setState(state);
    bgfx::submit(view, mPrograms[static_cast<int>(program)]);
    ++mDrawCalls;
}
void BgfxRenderDevice::FlushBatch()
{
    if (mVertices.empty()) return;
    Submit(SceneView, mBatchProgram, mVertices.data(), uint32_t(mVertices.size()), mTransform, mWidth, mHeight,
        true, mBatchMode, mBatchColorize, mBatchTexture, mBatchSampler, mClipped);
    mVertices.clear();
}
void BgfxRenderDevice::AddQuad(float left, float top, float right, float bottom,
    float uStart, float vStart, float uEnd, float vEnd, uint32_t color)
{
    if (mVertices.size() + 6 > 24576) FlushBatch();
    BgfxVertex quad[] = {{left,top,uStart,vStart,color}, {right,top,uEnd,vStart,color}, {right,bottom,uEnd,vEnd,color},
        {left,top,uStart,vStart,color}, {right,bottom,uEnd,vEnd,color}, {left,bottom,uStart,vEnd,color}};
    mVertices.insert(mVertices.end(), std::begin(quad), std::end(quad));
}
uint32_t BgfxRenderDevice::PackColor(const Color& color) const
{
    int alpha = std::clamp(color.mAlpha, 0, 255);
    auto premultiply = [alpha](int component) {
        int product = std::clamp(component, 0, 255) * alpha + 128;
        return uint32_t((product + (product >> 8)) >> 8);
    };
    return premultiply(color.mRed) | (premultiply(color.mGreen) << 8) | (premultiply(color.mBlue) << 16) | (uint32_t(alpha) << 24);
}
BgfxTexture* BgfxRenderDevice::EnsureTexture(MemoryImage* image, int& originX, int& originY)
{
    RemoveReleasedTextures();
    originX = originY = 0;
    if (!image) return nullptr;
    MemoryImage* sheet = image;
    while (sheet->mParent) { originX += sheet->mParentX; originY += sheet->mParentY; sheet = sheet->mParent; }
    if (originX < 0 || originY < 0 || originX + image->mWidth > sheet->mWidth || originY + image->mHeight > sheet->mHeight) return nullptr;
    auto& texture = mTextures[sheet];
    if (!texture) texture = std::make_unique<BgfxTexture>();
    if (!bgfx::isValid(texture->mHandle) || texture->mSrcWidth != sheet->mWidth || texture->mSrcHeight != sheet->mHeight
        || sheet->mBitsChanged || sheet->mBitsChangedCount)
    { FlushBatch(); if (!texture->Upload(sheet)) { mTextures.erase(sheet); return nullptr; } }
    return texture.get();
}
void BgfxRenderDevice::RemoveTexture(const MemoryImage* image)
{
    // Images may be released by the resource-loading thread. GPU batches and
    // the cache are only touched by the thread that owns the render device.
    std::lock_guard<std::mutex> lock(mTextureRemovalMutex);
    mPendingTextureRemovals.push_back(image);
}
void BgfxRenderDevice::RemoveReleasedTextures()
{
    std::vector<const MemoryImage*> released;
    { std::lock_guard<std::mutex> lock(mTextureRemovalMutex); released.swap(mPendingTextureRemovals); }
    if (released.empty()) return;
    FlushBatch();
    for (const MemoryImage* image : released) mTextures.erase(image);
}
size_t BgfxRenderDevice::GetTextureBytes() const
{
    size_t bytes = 0;
    for (const auto& entry : mTextures) bytes += size_t(entry.second->mWidth) * entry.second->mHeight * 4 * 4 / 3;
    return bytes;
}
void BgfxRenderDevice::DrawImage(MemoryImage* image, const Rect& destination, const Rect& source,
    const Color& color, DrawMode mode, bool mirror, bool fastStretch, bool colorize)
{
    if (!mInitialized || destination.mWidth <= 0 || destination.mHeight <= 0 || source.mWidth <= 0 || source.mHeight <= 0) return;
    int originX, originY;
    BgfxTexture* texture = EnsureTexture(image, originX, originY);
    if (!texture) return;
    uint32_t sampler = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;
    if (fastStretch) sampler |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
    BeginBatch(Program::Blit, mode, texture->mHandle, colorize || mColorize, sampler);
    float left = float(originX + source.mX) / texture->mWidth, right = float(originX + source.mX + source.mWidth) / texture->mWidth;
    if (mirror) std::swap(left, right);
    AddQuad(float(destination.mX), float(destination.mY), float(destination.mX + destination.mWidth),
        float(destination.mY + destination.mHeight), left, float(originY + source.mY) / texture->mHeight,
        right, float(originY + source.mY + source.mHeight) / texture->mHeight, PackColor(color));
}
void BgfxRenderDevice::DrawImageMatrix(MemoryImage* image, const SexyMatrix3& matrix, float x, float y,
    const Rect& source, const Color& color, DrawMode mode, bool center)
{
    if (!mInitialized) return;
    int originX, originY;
    BgfxTexture* texture = EnsureTexture(image, originX, originY);
    if (!texture) return;
    // GLTexture::BltTransformed @0x4734ac, operator* @0x341b04.
    float left = center ? -source.mWidth * 0.5f : 0, top = center ? -source.mHeight * 0.5f : 0;
    float localX[] = {left, left, left + source.mWidth, left + source.mWidth};
    float localY[] = {top, top + source.mHeight, top, top + source.mHeight};
    BgfxVertex corners[4];
    uint32_t packed = PackColor(color);
    for (int index = 0; index < 4; ++index)
    {
        corners[index] = {localX[index]*matrix.m00 + localY[index]*matrix.m01 + matrix.m02 + x,
            localX[index]*matrix.m10 + localY[index]*matrix.m11 + matrix.m12 + y,
            float(originX + source.mX + (index >= 2 ? source.mWidth : 0))/texture->mWidth,
            float(originY + source.mY + (index % 2 ? source.mHeight : 0))/texture->mHeight, packed};
    }
    BeginBatch(Program::Blit, mode, texture->mHandle);
    if (mVertices.size() + 6 > 24576) FlushBatch();
    for (int index : {0,1,2,2,1,3}) mVertices.push_back(corners[index]);
}
void BgfxRenderDevice::DrawTriangles(MemoryImage* image, const SexyVertex2D (*triangles)[3], int count,
    const Color& color, DrawMode mode, float translateX, float translateY)
{
    if (!mInitialized || !triangles || count <= 0) return;
    int originX, originY;
    BgfxTexture* texture = EnsureTexture(image, originX, originY);
    if (!texture) return;
    BeginBatch(Program::Blit, mode, texture->mHandle);
    for (int triangle = 0; triangle < count; ++triangle)
    {
        if (mVertices.size() + 3 > 24576) FlushBatch();
        for (const auto& corner : triangles[triangle]) mVertices.push_back({corner.mX + translateX, corner.mY + translateY,
            (originX + corner.mU * image->mWidth)/texture->mWidth, (originY + corner.mV * image->mHeight)/texture->mHeight, PackColor(color)});
    }
}
void BgfxRenderDevice::FillRect(const Rect& rect, const Color& color, DrawMode mode)
{
    if (!mInitialized || rect.mWidth <= 0 || rect.mHeight <= 0) return;
    BeginBatch(Program::Solid, mode, BGFX_INVALID_HANDLE);
    AddQuad(float(rect.mX),float(rect.mY),float(rect.mX+rect.mWidth),float(rect.mY+rect.mHeight),0,0,0,0,PackColor(color));
}
void BgfxRenderDevice::DrawRect(const Rect& rect, const Color& color, DrawMode mode)
{
    FillRect(Rect(rect.mX,rect.mY,rect.mWidth,1),color,mode);
    FillRect(Rect(rect.mX,rect.mY+rect.mHeight-1,rect.mWidth,1),color,mode);
    FillRect(Rect(rect.mX,rect.mY,1,rect.mHeight),color,mode);
    FillRect(Rect(rect.mX+rect.mWidth-1,rect.mY,1,rect.mHeight),color,mode);
}
void BgfxRenderDevice::DrawLine(float startX, float startY, float endX, float endY, const Color& color, DrawMode mode)
{
    if (!mInitialized) return;
    float deltaX = endX-startX, deltaY = endY-startY, length = std::sqrt(deltaX*deltaX+deltaY*deltaY);
    if (length < 0.0001f) return;
    float normalX = -deltaY/length*0.5f, normalY = deltaX/length*0.5f;
    uint32_t packed = PackColor(color);
    BeginBatch(Program::Solid, mode, BGFX_INVALID_HANDLE);
    if (mVertices.size()+6 > 24576) FlushBatch();
    BgfxVertex vertices[] = {{startX+normalX,startY+normalY,0,0,packed},{startX-normalX,startY-normalY,0,0,packed},
        {endX-normalX,endY-normalY,0,0,packed},{startX+normalX,startY+normalY,0,0,packed},
        {endX-normalX,endY-normalY,0,0,packed},{endX+normalX,endY+normalY,0,0,packed}};
    mVertices.insert(mVertices.end(),std::begin(vertices),std::end(vertices));
}
void BgfxRenderDevice::DrawYuv(bgfx::TextureHandle y, bgfx::TextureHandle cb, bgfx::TextureHandle cr,
    int width, int height, const Rect& destination)
{
    if (!mInitialized || !bgfx::isValid(y) || !bgfx::isValid(cb) || !bgfx::isValid(cr)
        || width <= 0 || height <= 0 || destination.mWidth <= 0 || destination.mHeight <= 0) return;
    FlushBatch();
    float left = float(destination.mX), top = float(destination.mY), right = left+destination.mWidth, bottom = top+destination.mHeight;
    BgfxVertex vertices[] = {{left,top,0,0,0xffffffff},{right,top,1,0,0xffffffff},{right,bottom,1,1,0xffffffff},
        {left,top,0,0,0xffffffff},{right,bottom,1,1,0xffffffff},{left,bottom,0,1,0xffffffff}};
    bgfx::TextureHandle planes[] = {y,cb,cr};
    SexyMatrix3 identity; identity.LoadIdentity();
    Submit(SceneView,Program::Yuv,vertices,6,identity,mWidth,mHeight,true,DRAWMODE_NORMAL,false,BGFX_INVALID_HANDLE,UINT32_MAX,mClipped,planes);
}
bool BgfxRenderDevice::ReadPixels(MemoryImage& target)
{
    if (!mInitialized || !(bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_READ_BACK)
        || !(bgfx::getCaps()->supported & BGFX_CAPS_TEXTURE_BLIT)) return false;
    FlushBatch();
    if (!bgfx::isValid(mReadback)) mReadback = bgfx::createTexture2D(uint16_t(mWidth),uint16_t(mHeight),false,1,
        bgfx::TextureFormat::BGRA8, BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_BLIT_DST);
    if (!bgfx::isValid(mReadback)) return false;
    std::vector<unsigned char> pixels(size_t(mWidth)*mHeight*4);
    bgfx::blit(ReadbackView,mReadback,0,0,mColorTarget,0,0,uint16_t(mWidth),uint16_t(mHeight));
    uint32_t ready = bgfx::readTexture(mReadback,pixels.data());
    uint32_t frame = bgfx::frame();
    while (int32_t(frame-ready) < 0) frame = bgfx::frame();
    target.Create(mWidth,mHeight);
    if (!target.mBits) target.mBits = new unsigned long[size_t(mWidth)*mHeight];
    bool flip = bgfx::getCaps()->originBottomLeft;
    for (int row=0;row<mHeight;++row)
        for (int column=0;column<mWidth;++column)
        {
            size_t offset=(size_t(flip ? mHeight-1-row : row)*mWidth+column)*4;
            target.mBits[size_t(row)*mWidth+column]=uint32_t(pixels[offset])|(uint32_t(pixels[offset+1])<<8)
                |(uint32_t(pixels[offset+2])<<16)|(uint32_t(pixels[offset+3])<<24);
        }
    return true;
}
}
