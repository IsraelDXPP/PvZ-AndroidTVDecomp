#pragma once

#include <SDL3/SDL_iostream.h>
#include <memory>
#include <string>

namespace PakLib
{

// Reconstructed resource search from FileSystemManager::addResource @0x4004A0,
// addDefaultLocations @0x400554 and open @0x400C54. Lower priorities are searched
// first; equal priorities retain their mount order. Writable files bypass it.
class FileSystemManager
{
public:
    static FileSystemManager* getManager();
    ~FileSystemManager();

    void setAssetRoot(const std::string& theDirectory);
    bool addResource(const std::string& theLocation, const std::string& theKind, int thePriority);
    SDL_IOStream* open(const std::string& thePath);
    bool fileExists(const std::string& thePath);
    Sint64 getFileSize(const std::string& thePath);
    void release();

private:
    FileSystemManager();
    struct ResourceLocations;
    std::unique_ptr<ResourceLocations> mLocations;
};

}
