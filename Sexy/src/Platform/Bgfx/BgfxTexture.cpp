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
    std::vector<unsigned int> expanded;
    const unsigned int* uploadPixels = pixels;
    if (format != PIXEL_FORMAT_A8R8G8B8 || stride != image->mWidth * 4)
    {
        expanded.resize(size_t(image->mWidth) * image->mHeight);
        PixelData source(format, image->mWidth, image->mHeight, stride, pixels);
        PixelData destination(PIXEL_FORMAT_A8R8G8B8, image->mWidth, image->mHeight, image->mWidth * 4, expanded.data());
        if (!destination.Convert(source)) return false;
        uploadPixels = expanded.data();
    }
    // GLTexture::CreateTexture @0x470f88 extends edge texels into POT padding.
    int width = 1, height = 1;
    while (width < image->mWidth) width *= 2;
    while (height < image->mHeight) height *= 2;
    if (width > bgfx::getCaps()->limits.maxTextureSize || height > bgfx::getCaps()->limits.maxTextureSize) return false;
    size_t pixelCount = size_t(width) * height;
    for (int mipWidth = width, mipHeight = height; mipWidth > 1 || mipHeight > 1;)
    {
        mipWidth = std::max(1, mipWidth / 2);
        mipHeight = std::max(1, mipHeight / 2);
        pixelCount += size_t(mipWidth) * mipHeight;
    }
    std::vector<uint32_t> mipmaps(pixelCount);
    uint32_t* baseLevel = mipmaps.data();
    for (int row = 0; row < image->mHeight; ++row)
    {
        const unsigned int* sourceRow = uploadPixels + size_t(row) * image->mWidth;
        uint32_t* destinationRow = baseLevel + size_t(row) * width;
        for (int column = 0; column < image->mWidth; ++column)
        {
            uint32_t color = sourceRow[column];
            uint32_t alpha = color >> 24;
            if (alpha == 255)
                destinationRow[column] = color;
            else if (alpha == 0)
                destinationRow[column] = 0;
            else
            {
                uint32_t red = ((color >> 16) & 255) * alpha / 255;
                uint32_t green = ((color >> 8) & 255) * alpha / 255;
                uint32_t blue = (color & 255) * alpha / 255;
                destinationRow[column] = (alpha << 24) | (red << 16) | (green << 8) | blue;
            }
        }
        std::fill(destinationRow + image->mWidth, destinationRow + width, destinationRow[image->mWidth - 1]);
    }
    const uint32_t* lastRow = baseLevel + size_t(image->mHeight - 1) * width;
    for (int row = image->mHeight; row < height; ++row)
        std::memcpy(baseLevel + size_t(row) * width, lastRow, size_t(width) * sizeof(uint32_t));

    int mipWidth = width, mipHeight = height;
    const uint32_t* previousLevel = baseLevel;
    uint32_t* nextLevel = baseLevel + size_t(width) * height;
    while (mipWidth > 1 || mipHeight > 1)
    {
        int nextWidth = std::max(1, mipWidth / 2), nextHeight = std::max(1, mipHeight / 2);
        int columnOffset = mipWidth > 1 ? 1 : 0;
        int rowOffset = mipHeight > 1 ? mipWidth : 0;
        for (int row = 0; row < nextHeight; ++row)
        {
            const uint32_t* topRow = previousLevel + size_t(row * 2) * mipWidth;
            const uint32_t* bottomRow = topRow + rowOffset;
            uint32_t* destinationRow = nextLevel + size_t(row) * nextWidth;
            for (int column = 0; column < nextWidth; ++column)
            {
                uint32_t topLeft = topRow[column * 2], topRight = topRow[column * 2 + columnOffset];
                uint32_t bottomLeft = bottomRow[column * 2], bottomRight = bottomRow[column * 2 + columnOffset];
                // Two channels per word, with room for four 8-bit samples in
                // each 16-bit lane. Preserve the original rounded average.
                constexpr uint32_t channels = 0x00ff00ff;
                uint32_t redBlue = (topLeft & channels) + (topRight & channels)
                    + (bottomLeft & channels) + (bottomRight & channels);
                uint32_t alphaGreen = ((topLeft >> 8) & channels) + ((topRight >> 8) & channels)
                    + ((bottomLeft >> 8) & channels) + ((bottomRight >> 8) & channels);
                destinationRow[column] = (((redBlue + 0x00020002) >> 2) & channels)
                    | ((((alphaGreen + 0x00020002) >> 2) & channels) << 8);
            }
        }
        previousLevel = nextLevel;
        nextLevel += size_t(nextWidth) * nextHeight;
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
