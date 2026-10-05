// Win32File.h -- file access + asset path resolution.
//
// Replaces Sexy::AndroidFileDriver / Sexy::AndroidFile. The Android build reads
// assets straight out of the APK via AAssetManager; on Windows the same tree is
// a directory, so this driver maps logical asset names onto a root folder.

#ifndef __WIN32FILE_H__
#define __WIN32FILE_H__

#include "CoreTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class Win32File
{
public:
    // Root directory holding the extracted APK assets (files/, levels/, paks/).
    static void         SetAssetRoot(const std::string& theRoot);
    static std::string  GetAssetRoot();

    // Resolve a logical asset path (e.g. "files/images/foo.png") against the
    // asset root. Returns the path unchanged if it is already absolute.
    static std::string  ResolvePath(const std::string& thePath);

    static bool         FileExists(const std::string& thePath);
    static bool         ReadFile(const std::string& thePath, std::vector<unsigned char>& theData);
    static bool         WriteFile(const std::string& thePath, const void* theData, size_t theSize);
    static long         GetFileSize(const std::string& thePath);
    static bool         MakeDirs(const std::string& thePath);

    // Write a file *without* asset-root redirection. Outputs (screenshots,
    // saves, logs) must never be written into the read-only asset tree, so
    // they go through here and resolve against the process working directory.
    static bool         WriteOutputFile(const std::string& thePath,
                                        const void* theData, size_t theSize);

private:
    static std::string  sAssetRoot;
};

} // namespace Sexy

#endif // __WIN32FILE_H__
