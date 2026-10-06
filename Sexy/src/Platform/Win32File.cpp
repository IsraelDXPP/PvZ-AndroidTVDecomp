// Win32File.cpp -- asset path resolution and file IO.

#include "Win32File.h"
#include <string>
#include <windows.h>
#include <cstdio>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

std::string Win32File::sAssetRoot = "assets";

void Win32File::SetAssetRoot(const std::string& theRoot)
{
    sAssetRoot = theRoot;
    // Normalise a trailing separator so concatenation is predictable.
    while (!sAssetRoot.empty() &&
           (sAssetRoot.back() == '\\' || sAssetRoot.back() == '/'))
        sAssetRoot.pop_back();

    // The binary addresses assets relative to the APK's "files" directory:
    // MainMenu's constructor asks for "menu/MainMenu.menu.txt" (@0x19a776) and
    // the mark art is "images/mark/mainmenu_2d_mark_bg.png" (@0x19a8fe). On
    // Android the asset manager is rooted there. When the caller points at the
    // extracted "assets" directory instead, descend into "files" so the same
    // relative paths resolve -- this is the desktop stand-in for that root.
    if (!sAssetRoot.empty())
    {
        std::string filesDir = sAssetRoot + "/files";
        DWORD attrs = ::GetFileAttributesA(filesDir.c_str());
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY))
            sAssetRoot = filesDir;
    }
}

std::string Win32File::GetAssetRoot()
{
    return sAssetRoot;
}

std::string Win32File::ResolvePath(const std::string& thePath)
{
    if (thePath.empty()) return thePath;

    // Absolute paths (drive letter or UNC) are used as-is.
    if (thePath.size() >= 2 && thePath[1] == ':') return thePath;
    if (thePath.size() >= 2 && thePath[0] == '\\' && thePath[1] == '\\') return thePath;
    if (thePath[0] == '\\' || thePath[0] == '/') return thePath;

    return sAssetRoot + "/" + thePath;
}

bool Win32File::FileExists(const std::string& thePath)
{
    std::string p = ResolvePath(thePath);
    DWORD attrs = ::GetFileAttributesA(p.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

long Win32File::GetFileSize(const std::string& thePath)
{
    std::string p = ResolvePath(thePath);
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!::GetFileAttributesExA(p.c_str(), GetFileExInfoStandard, &fad))
        return -1;
    return (long)fad.nFileSizeLow;
}

bool Win32File::ReadFile(const std::string& thePath, std::vector<unsigned char>& theData)
{
    std::string p = ResolvePath(thePath);

    FILE* fp = fopen(p.c_str(), "rb");
    if (!fp) return false;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 0)
    {
        fclose(fp);
        return false;
    }

    theData.resize((size_t)size);
    if (size > 0)
    {
        size_t got = fread(theData.data(), 1, (size_t)size, fp);
        if (got != (size_t)size)
        {
            fclose(fp);
            theData.clear();
            return false;
        }
    }

    fclose(fp);
    return true;
}

bool Win32File::WriteFile(const std::string& thePath, const void* theData, size_t theSize)
{
    std::string p = ResolvePath(thePath);

    // Ensure the containing directory exists (save files live under userdata/).
    size_t slash = p.find_last_of("/\\");
    if (slash != std::string::npos)
        MakeDirs(p.substr(0, slash));

    FILE* fp = fopen(p.c_str(), "wb");
    if (!fp) return false;

    bool ok = true;
    if (theSize > 0)
        ok = fwrite(theData, 1, theSize, fp) == theSize;

    fclose(fp);
    return ok;
}

// Outputs (screenshots, save games, logs) must not be redirected into the
// asset tree, which is treated as read-only input. This variant resolves
// against the process working directory instead.
bool Win32File::WriteOutputFile(const std::string& thePath,
                                const void* theData, size_t theSize)
{
    if (thePath.empty() || (theSize > 0 && theData == nullptr))
        return false;
    std::string p = thePath;

    size_t slash = p.find_last_of("/\\");
    if (slash != std::string::npos && slash > 0 && !MakeDirs(p.substr(0, slash)))
        return false;

    FILE* fp = fopen(p.c_str(), "wb");
    if (!fp) return false;

    bool ok = true;
    if (theSize > 0)
        ok = fwrite(theData, 1, theSize, fp) == theSize;

    if (fclose(fp) != 0)
        ok = false;
    return ok;
}

bool Win32File::MakeDirs(const std::string& thePath)
{
    if (thePath.empty()) return false;

    std::string accum;
    for (size_t i = 0; i < thePath.size(); ++i)
    {
        char c = thePath[i];
        accum.push_back(c);

        bool isSep = (c == '\\' || c == '/');
        bool atEnd = (i + 1 == thePath.size());
        if (isSep || atEnd)
        {
            std::string dir = accum;
            while (!dir.empty() && (dir.back() == '\\' || dir.back() == '/'))
                dir.pop_back();
            if (dir.size() == 2 && dir[1] == ':') continue;   // drive root
            if (!dir.empty() && !::CreateDirectoryA(dir.c_str(), nullptr))
            {
                if (::GetLastError() != ERROR_ALREADY_EXISTS)
                    return false;
                DWORD attributes = ::GetFileAttributesA(dir.c_str());
                if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
                    return false;
            }
        }
    }
    return true;
}

} // namespace Sexy
