// Screenshot.cpp -- dependency-free capture of the ARGB back buffer.
//
// Deliberately self-contained: the project's vcpkg dependencies are restored
// by MSBuild, but screenshot capture must also work in the headless self-test,
// which is compiled with plain cl.exe and no package manager. So we emit PNG
// using only zlib's *stored* (uncompressed) deflate blocks plus hand-rolled
// CRC-32 and Adler-32. The result is a fully valid PNG that any viewer opens;
// it is simply larger than a compressed one.
//
// PNG structure emitted:
//   signature, IHDR, IDAT (zlib stream, stored blocks), IEND

#include "Screenshot.h"
#include "SDLFile.h"
#include "SexyTypes.h"

#include "AndroidShims.h"
#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <set>

namespace Sexy
{

int Screenshot::sCounter = 0;
bool ScreenshotHotkey::sWasDown = false;

// ---------------------------------------------------------------------------
// checksums
// ---------------------------------------------------------------------------

static unsigned int Crc32(const unsigned char *buf, size_t len, unsigned int crc = 0xFFFFFFFF)
{
    static unsigned int table[256];
    static bool init = false;
    if (!init)
    {
        for (unsigned int n = 0; n < 256; ++n)
        {
            unsigned int c = n;
            for (int k = 0; k < 8; ++k)
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[n] = c;
        }
        init = true;
    }
    for (size_t i = 0; i < len; ++i)
        crc = table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

static unsigned int Adler32(const unsigned char *data, size_t len)
{
    unsigned int a = 1, b = 0;
    for (size_t i = 0; i < len; ++i)
    {
        a = (a + data[i]) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

// ---------------------------------------------------------------------------
// PNG chunk helpers
// ---------------------------------------------------------------------------

static void PutU32(std::vector<unsigned char> &v, unsigned int x)
{
    v.push_back((unsigned char)(x >> 24));
    v.push_back((unsigned char)(x >> 16));
    v.push_back((unsigned char)(x >> 8));
    v.push_back((unsigned char)(x));
}

static void WriteChunk(std::vector<unsigned char> &out, const char type[4], const std::vector<unsigned char> &data)
{
    PutU32(out, (unsigned int)data.size());
    size_t crcStart = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    unsigned int crc = Crc32(&out[crcStart], out.size() - crcStart) ^ 0xFFFFFFFF;
    PutU32(out, crc);
}

// Wrap raw bytes in a zlib stream using stored (type 00) deflate blocks.
static std::vector<unsigned char> ZlibStore(const std::vector<unsigned char> &raw)
{
    std::vector<unsigned char> z;
    z.push_back(0x78); // CMF: deflate, 32K window
    z.push_back(0x01); // FLG: no dict, fastest

    const size_t MAX_BLOCK = 65535;
    size_t pos = 0;
    while (pos < raw.size())
    {
        size_t n = raw.size() - pos;
        if (n > MAX_BLOCK)
            n = MAX_BLOCK;
        bool last = (pos + n >= raw.size());

        z.push_back(last ? 1 : 0); // BFINAL, BTYPE=00 (stored)
        z.push_back((unsigned char)(n & 0xFF));
        z.push_back((unsigned char)((n >> 8) & 0xFF));
        z.push_back((unsigned char)(~n & 0xFF));
        z.push_back((unsigned char)((~n >> 8) & 0xFF));
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
    }
    if (raw.empty())
    {
        z.push_back(1);
        z.push_back(0);
        z.push_back(0);
        z.push_back(0xFF);
        z.push_back(0xFF);
    }

    PutU32(z, Adler32(raw.data(), raw.size()));
    return z;
}

// ---------------------------------------------------------------------------
// capture
// ---------------------------------------------------------------------------

bool Screenshot::Save(MemoryImage *theImage, const std::string &thePath)
{
    if (!theImage || !theImage->mBits || theImage->mWidth <= 0 || theImage->mHeight <= 0)
        return false;

    const int w = theImage->mWidth;
    const int h = theImage->mHeight;

    // Build the raw scanlines PNG expects: one filter byte (0 = None) then RGB.
    std::vector<unsigned char> raw;
    raw.reserve((size_t)h * (1 + (size_t)w * 3));

    for (int y = 0; y < h; ++y)
    {
        raw.push_back(0);
        const std::uint32_t *row = theImage->mBits + (size_t)y * w;
        for (int x = 0; x < w; ++x)
        {
            unsigned p = row[x];
            raw.push_back((unsigned char)((p >> 16) & 0xFF)); // R
            raw.push_back((unsigned char)((p >> 8) & 0xFF)); // G
            raw.push_back((unsigned char)(p & 0xFF)); // B
        }
    }

    std::vector<unsigned char> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

    // IHDR
    std::vector<unsigned char> ihdr;
    PutU32(ihdr, (unsigned int)w);
    PutU32(ihdr, (unsigned int)h);
    ihdr.push_back(8); // bit depth
    ihdr.push_back(2); // colour type: truecolour RGB
    ihdr.push_back(0); // compression
    ihdr.push_back(0); // filter
    ihdr.push_back(0); // interlace
    WriteChunk(png, "IHDR", ihdr);

    // IDAT
    WriteChunk(png, "IDAT", ZlibStore(raw));

    // IEND
    WriteChunk(png, "IEND", {});

    if (!SDLFile::WriteOutputFile(thePath, png.data(), png.size()))
    {
        TOD_ERROR("Screenshot: failed to write %s", thePath.c_str());
        return false;
    }

    TOD_TRACE("Screenshot saved: %s (%dx%d, %zu bytes)", thePath.c_str(), w, h, png.size());
    return true;
}

bool Screenshot::SaveRaw(MemoryImage *theImage, const std::string &thePath)
{
    if (!theImage || !theImage->mBits)
        return false;
    size_t bytes = (size_t)theImage->mWidth * theImage->mHeight * 4;
    return SDLFile::WriteOutputFile(thePath, theImage->mBits, bytes);
}

bool Screenshot::SaveNext(MemoryImage *theImage)
{
    char name[128];
    sprintf(name, "screenshots/shot_%04d.png", sCounter++);
    return Save(theImage, name);
}

int Screenshot::GetCount()
{
    return sCounter;
}

bool ScreenshotHotkey::IsCaptureRequested(bool theKeyDown)
{
    return theKeyDown && !sWasDown;
}

void ScreenshotHotkey::Poll(MemoryImage *theScreen, bool theKeyDown)
{
    // Rising edge only.
    if (theScreen != nullptr && IsCaptureRequested(theKeyDown))
    {
        if (Screenshot::SaveNext(theScreen))
        {
            // Also drop a raw dump so two runs can be compared byte-for-byte.
            char rawName[128];
            sprintf(rawName, "screenshots/shot_%04d.argb", Screenshot::GetCount() - 1);
            Screenshot::SaveRaw(theScreen, rawName);
        }
    }
    sWasDown = theKeyDown;
}

} // namespace Sexy
