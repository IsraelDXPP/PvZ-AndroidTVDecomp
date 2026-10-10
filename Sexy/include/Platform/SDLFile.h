// SDLFile.h -- file access + asset path resolution.
//
// Maps extracted APK assets to a directory using SDL3's UTF-8 file APIs.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLFILE_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLFILE_H

#include <cstddef>
#include <string>
#include <vector>

namespace Sexy
{

class SDLFile
{
public:
    // Root directory holding the extracted APK assets (files/, levels/, paks/).
    static void SetAssetRoot(const std::string &theRoot);
    static std::string GetAssetRoot();

    // Resolve a logical asset path (e.g. "files/images/foo.png") against the
    // asset root. Returns the path unchanged if it is already absolute.
    static std::string ResolvePath(const std::string &thePath);

    static bool FileExists(const std::string &thePath);
    static bool ReadFile(const std::string &thePath, std::vector<unsigned char> &theData);
    static bool WriteFile(const std::string &thePath, const void *theData, size_t theSize);
    static long GetFileSize(const std::string &thePath);
    static bool MakeDirs(const std::string &thePath);

    // Write a file *without* asset-root redirection. Outputs (screenshots,
    // saves, logs) must never be written into the read-only asset tree, so
    // they go through here and resolve against the process working directory.
    static bool WriteOutputFile(const std::string &thePath, const void *theData, size_t theSize);

private:
    static std::string sAssetRoot;
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_SDLFILE_H
