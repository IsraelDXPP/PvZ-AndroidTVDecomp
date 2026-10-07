// ImageLib. SDL_image decodes the asset; stb covers additional formats. Writers still
// report failure: this port does not emit PNG or JPEG files.

#include "ImageLib.h"
#include "SDLFile.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace ImageLib {

bool gAutoLoadAlpha = true;
unsigned int gAlphaComposeColor = 0;

Image::Image()
    : mOwnsBits(0),
      mWidth(0),
      mHeight(0),
      mBits(nullptr),
      mText1(nullptr),
      mText2(nullptr),
      mPalettized(false)
{
}

Image::~Image()
{
    if (mOwnsBits != 0)
        delete[] mBits;
}

unsigned int* Image::GetBits()
{
    return mBits;
}

char * WritePNGImage(const Sexy::String& thePath, Image * thePixels)
{
    (void)thePixels;
    (void)thePath;
    return nullptr;
}

char * WriteJPEGImage(const Sexy::String& thePath, Image * thePixels)
{
    (void)thePixels;
    (void)thePath;
    return nullptr;
}

namespace {

bool ExtensionMatches(const std::string& theExt, const char* theName)
{
    if (theExt.empty())
        return true;
    return SDL_strcasecmp(theExt.c_str(), theName) == 0;
}

Image* DecodeFile(const std::string& thePath)
{
    if (!Sexy::SDLFile::FileExists(thePath))
        return nullptr;

    std::vector<unsigned char> aBytes;
    if (!Sexy::SDLFile::ReadFile(thePath, aBytes) || aBytes.empty())
        return nullptr;

    SDL_Surface* decoded = IMG_Load_IO(SDL_IOFromConstMem(aBytes.data(), aBytes.size()), true);
    if (decoded != nullptr)
    {
        SDL_Surface* rgba = SDL_ConvertSurface(decoded, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(decoded);
        if (rgba != nullptr)
        {
            Image* image = new Image();
            image->mWidth = rgba->w;
            image->mHeight = rgba->h;
            image->mOwnsBits = 1;
            image->mBits = new unsigned int[size_t(rgba->w) * rgba->h];
            const unsigned char* pixels = static_cast<const unsigned char*>(rgba->pixels);
            for (int row = 0; row < rgba->h; ++row)
                for (int column = 0; column < rgba->w; ++column)
                {
                    const unsigned char* pixel = pixels + size_t(row) * rgba->pitch + column * 4;
                    image->mBits[size_t(row) * rgba->w + column] = (static_cast<unsigned int>(pixel[3]) << 24)
                        | (static_cast<unsigned int>(pixel[0]) << 16) | (static_cast<unsigned int>(pixel[1]) << 8)
                        | static_cast<unsigned int>(pixel[2]);
                }
            SDL_DestroySurface(rgba);
            return image;
        }
    }

    int aWidth = 0;
    int aHeight = 0;
    int aComponents = 0;
    unsigned char* aRgba = stbi_load_from_memory(
        aBytes.data(), (int)aBytes.size(), &aWidth, &aHeight, &aComponents, 4);
    if (aRgba == nullptr || aWidth <= 0 || aHeight <= 0)
        return nullptr;

    Image* aImage = new Image();
    aImage->mWidth = aWidth;
    aImage->mHeight = aHeight;
    aImage->mOwnsBits = 1;
    aImage->mBits = new unsigned int[(size_t)aWidth * (size_t)aHeight];

    int aCount = aWidth * aHeight;
    for (int aIndex = 0; aIndex < aCount; ++aIndex)
    {
        unsigned int aRed = aRgba[aIndex * 4];
        unsigned int aGreen = aRgba[aIndex * 4 + 1];
        unsigned int aBlue = aRgba[aIndex * 4 + 2];
        unsigned int aAlpha = aRgba[aIndex * 4 + 3];
        aImage->mBits[aIndex] = (aAlpha << 24) | (aRed << 16) | (aGreen << 8) | aBlue;
    }
    stbi_image_free(aRgba);
    return aImage;
}

void ApplyAlphaMask(Image* theColor, Image* theAlpha)
{
    int aCount = theColor->mWidth * theColor->mHeight;
    for (int aIndex = 0; aIndex < aCount; ++aIndex)
    {
        unsigned int aMask = theAlpha->mBits[aIndex] & 0xFFu;
        theColor->mBits[aIndex] = (theColor->mBits[aIndex] & 0x00FFFFFFu) | (aMask << 24);
    }
    theColor->mPalettized = false;
}

void ComposeSolid(Image* theAlpha, unsigned int theColor)
{
    int aCount = theAlpha->mWidth * theAlpha->mHeight;
    for (int aIndex = 0; aIndex < aCount; ++aIndex)
    {
        unsigned int aMask = theAlpha->mBits[aIndex] & 0xFFu;
        theAlpha->mBits[aIndex] = theColor | (aMask << 24);
    }
}

Image* LoadImageFile(const std::string& thePath, bool lookForAlpha)
{
    if (thePath.empty())
        return nullptr;

    int aDot = (int)thePath.rfind('.');
    int aSlash = (int)thePath.find_last_of("\\/");
    if (aDot == (int)std::string::npos)
        aDot = -1;
    if (aSlash == (int)std::string::npos)
        aSlash = -1;

    std::string aExt;
    std::string aBase = thePath;
    if (aDot > aSlash)
    {
        aExt = thePath.substr((size_t)aDot);
        aBase = thePath.substr(0, (size_t)aDot);
    }

    Image* aDecoded = nullptr;
    const char* aExtensions[] = { ".tga", ".jpg", ".png", ".gif" };
    for (int aKind = 0; aKind < 4; ++aKind)
    {
        if (!ExtensionMatches(aExt, aExtensions[aKind]))
            continue;
        aDecoded = DecodeFile(aBase + aExtensions[aKind]);
        if (aDecoded != nullptr)
            break;
    }

    if (!lookForAlpha)
        return aDecoded;

    // ImageLib::GetImage looks beside the colour file for a mask. The first
    // name is "_" plus the file (images/_SeedBank.gif). The second, which is
    // what the TV pack uses, is the file plus "_" (images/SeedBank_.gif).
    std::string aPrefixMask = thePath;
    if (aSlash >= 0)
        aPrefixMask.insert((size_t)aSlash + 1, "_");
    else
        aPrefixMask.insert(0, "_");
    Image* aAlpha = LoadImageFile(aPrefixMask, false);
    if (aAlpha == nullptr)
        aAlpha = LoadImageFile(thePath + "_", false);
    if (aAlpha == nullptr)
        return aDecoded;

    if (aDecoded == nullptr)
    {
        ComposeSolid(aAlpha, gAlphaComposeColor);
        return aAlpha;
    }

    if (aDecoded->mBits != nullptr && aAlpha->mBits != nullptr &&
        aDecoded->mWidth == aAlpha->mWidth && aDecoded->mHeight == aAlpha->mHeight)
        ApplyAlphaMask(aDecoded, aAlpha);
    delete aAlpha;
    return aDecoded;
}

} // namespace

Image* GetImage(Sexy::String const& thePath, bool lookForAlpha, bool thePurge, bool theFlags)
{
    (void)thePurge;
    (void)theFlags;
    if (!gAutoLoadAlpha)
        lookForAlpha = false;
    return LoadImageFile(thePath.c_str(), lookForAlpha);
}

} // namespace ImageLib
