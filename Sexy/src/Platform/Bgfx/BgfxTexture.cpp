#include "BgfxRenderDevice.h"
#include "PixelData.h"
#include "PixelUtil.h"
#include <algorithm>
#include <cstring>

namespace Sexy {
BgfxTexture::~BgfxTexture() { if (bgfx::isValid(mHandle)) bgfx::destroy(mHandle); }
bool BgfxTexture::Upload(MemoryImage* image)
{
    if (!image || image->mWidth <= 0 || image->mHeight <= 0) return false;
    unsigned int* pixels = image->GetPixels();
    if (!pixels) return false;
    PixelFormat format = image->GetPixelFormat();
    int stride = image->GetStride();
    if (stride <= 0) stride = image->mWidth * PixelUtil::GetBytePerPixel(format);
    std::vector<unsigned int> expanded(size_t(image->mWidth) * image->mHeight);
    PixelData source(format, image->mWidth, image->mHeight, stride, pixels);
    PixelData destination(PIXEL_FORMAT_A8R8G8B8, image->mWidth, image->mHeight, image->mWidth * 4, expanded.data());
    if (!destination.Convert(source)) return false;
    // GLTexture::CreateTexture @0x470f88 extends edge texels into POT padding.
    int width = 1, height = 1;
    while (width < image->mWidth) width *= 2;
    while (height < image->mHeight) height *= 2;
    if (width > bgfx::getCaps()->limits.maxTextureSize || height > bgfx::getCaps()->limits.maxTextureSize) return false;
    std::vector<uint32_t> level(size_t(width) * height);
    for (int row = 0; row < height; ++row)
        for (int column = 0; column < width; ++column)
        {
            uint32_t color = expanded[size_t(std::min(row, image->mHeight - 1)) * image->mWidth + std::min(column, image->mWidth - 1)];
            uint32_t alpha = color >> 24;
            uint32_t red = ((color >> 16) & 255) * alpha / 255;
            uint32_t green = ((color >> 8) & 255) * alpha / 255;
            uint32_t blue = (color & 255) * alpha / 255;
            level[size_t(row) * width + column] = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
    std::vector<uint32_t> mipmaps = level;
    int mipWidth = width, mipHeight = height;
    while (mipWidth > 1 || mipHeight > 1)
    {
        int nextWidth = std::max(1, mipWidth / 2), nextHeight = std::max(1, mipHeight / 2);
        std::vector<uint32_t> next(size_t(nextWidth) * nextHeight);
        for (int row = 0; row < nextHeight; ++row)
            for (int column = 0; column < nextWidth; ++column)
            {
                uint32_t color = 0;
                for (int shift = 0; shift <= 24; shift += 8)
                {
                    uint32_t sum = 0;
                    for (int offsetY = 0; offsetY < 2; ++offsetY)
                        for (int offsetX = 0; offsetX < 2; ++offsetX)
                            sum += (level[size_t(std::min(mipHeight - 1, row * 2 + offsetY)) * mipWidth
                                + std::min(mipWidth - 1, column * 2 + offsetX)] >> shift) & 255;
                    color |= ((sum + 2) / 4) << shift;
                }
                next[size_t(row) * nextWidth + column] = color;
            }
        mipmaps.insert(mipmaps.end(), next.begin(), next.end());
        level = std::move(next);
        mipWidth = nextWidth; mipHeight = nextHeight;
    }
    bgfx::TextureHandle replacement = bgfx::createTexture2D(uint16_t(width), uint16_t(height), true, 1,
        bgfx::TextureFormat::BGRA8, BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP,
        bgfx::copy(mipmaps.data(), uint32_t(mipmaps.size() * sizeof(uint32_t))));
    if (!bgfx::isValid(replacement)) return false;
    if (bgfx::isValid(mHandle)) bgfx::destroy(mHandle);
    mHandle = replacement;
    mWidth = width; mHeight = height;
    mSrcWidth = image->mWidth; mSrcHeight = image->mHeight;
    image->mBitsChanged = false;
    image->mBitsChangedCount = 0;
    return true;
}
}
