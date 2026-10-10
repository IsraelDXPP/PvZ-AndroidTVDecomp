#include "SDLFile.h"
#include "PakLib/FileSystemManager.h"
#include "ReconHelpers.h"
#include <SDL3/SDL.h>
#include <filesystem>
#include <limits>

namespace Sexy
{
std::string SDLFile::sAssetRoot = "assets";
bool SDLFile::sAssetRootConfigured = false;
void SDLFile::SetAssetRoot(const std::string &root)
{
    auto path = std::filesystem::u8path(root).lexically_normal();
    SDL_PathInfo info{};
    std::string files = (path / "files").u8string();
    if (SDL_GetPathInfo(files.c_str(), &info) && info.type == SDL_PATHTYPE_DIRECTORY)
        path /= "files";
    sAssetRoot = path.u8string();
    sAssetRootConfigured = true;
    // Read the external settings before opening packs, so language selection
    // cannot recurse through p_fopen or depend on the selected language pack.
    const char *language = Sexy::GetEnv("LAWN_LANGUAGE");
    const char *locale = Sexy::setLocale(nullptr, language != nullptr && language[0] != '\0' ? language : "en_us");
    PakLib::FileSystemManager::getManager()->setAssetRoot(sAssetRoot, locale);
}
std::string SDLFile::GetAssetRoot()
{
    return sAssetRoot;
}
bool SDLFile::IsAssetRootConfigured()
{
    return sAssetRootConfigured;
}
std::string SDLFile::ResolvePath(const std::string &name)
{
    if (name.empty())
        return name;
    auto path = std::filesystem::u8path(name);
    return path.is_absolute() ? name : (std::filesystem::u8path(sAssetRoot) / path).u8string();
}
bool SDLFile::FileExists(const std::string &name)
{
    return PakLib::FileSystemManager::getManager()->fileExists(name);
}
long SDLFile::GetFileSize(const std::string &name)
{
    const Sint64 size = PakLib::FileSystemManager::getManager()->getFileSize(name);
    return size >= 0 && size <= std::numeric_limits<long>::max() ? static_cast<long>(size) : -1;
}
bool SDLFile::ReadFile(const std::string &name, std::vector<unsigned char> &data)
{
    data.clear();
    size_t size = 0;
    SDL_IOStream *stream = p_fopen(name.c_str(), "rb");
    if (!stream)
        return false;
    void *loaded = SDL_LoadFile_IO(stream, &size, true);
    if (!loaded)
        return false;
    const unsigned char *bytes = static_cast<const unsigned char *>(loaded);
    data.assign(bytes, bytes + size);
    SDL_free(loaded);
    return true;
}
bool SDLFile::MakeDirs(const std::string &path)
{
    return !path.empty() && SDL_CreateDirectory(path.c_str());
}
bool SDLFile::WriteFile(const std::string &name, const void *data, size_t size)
{
    return WriteOutputFile(ResolvePath(name), data, size);
}
bool SDLFile::WriteOutputFile(const std::string &path, const void *data, size_t size)
{
    if (path.empty() || (size && !data))
        return false;
    auto parent = std::filesystem::u8path(path).parent_path();
    if (!parent.empty() && !MakeDirs(parent.u8string()))
        return false;
    // Replace a complete temporary file; a failed write preserves the old save.
    std::string temporary = path + ".tmp." + std::to_string(SDL_GetCurrentThreadID());
    SDL_IOStream *stream = SDL_IOFromFile(temporary.c_str(), "wb");
    if (!stream)
        return false;
    bool success = SDL_WriteIO(stream, data, size) == size;
    if (success)
        success = SDL_FlushIO(stream);
    if (!SDL_CloseIO(stream))
        success = false;
    if (success)
        success = SDL_RenamePath(temporary.c_str(), path.c_str());
    if (!success)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "File write failed: %s: %s", path.c_str(), SDL_GetError());
        SDL_RemovePath(temporary.c_str());
    }
    return success;
}
}
