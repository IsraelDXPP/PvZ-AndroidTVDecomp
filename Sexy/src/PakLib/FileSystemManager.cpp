#include "PakLib/FileSystemManager.h"

#include <SDL3/SDL.h>
#include <minizip-ng/mz.h>
#include <minizip-ng/mz_strm.h>
#include <minizip-ng/mz_zip.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <new>
#include <vector>

namespace PakLib
{
namespace
{
using FileBytes = std::vector<unsigned char>;

std::string ForwardSlashes(std::string thePath)
{
    std::replace(thePath.begin(), thePath.end(), '\\', '/');
    return thePath;
}

// Case folding is deliberately ASCII-only: changing individual UTF-8 bytes
// through the locale's toupper/tolower would corrupt non-ASCII filenames.
std::string EntryKey(const std::string &thePath)
{
    const auto aPath = std::filesystem::u8path(ForwardSlashes(thePath)).lexically_normal();
    if (aPath.has_root_path())
        return {};
    std::string aKey = aPath.generic_u8string();
    while (!aKey.empty() && aKey.back() == '/')
        aKey.pop_back();
    if (aKey == "." || aKey == ".." || aKey.compare(0, 3, "../") == 0)
        return {};
    for (char &aChar : aKey)
        if (aChar >= 'A' && aChar <= 'Z')
            aChar += 'a' - 'A';
    return aKey;
}

std::string FullPath(const std::string &thePath)
{
    std::error_code aError;
    auto aPath = std::filesystem::absolute(std::filesystem::u8path(ForwardSlashes(thePath)), aError);
    return aError ? ForwardSlashes(thePath) : aPath.lexically_normal().generic_u8string();
}

std::string FoldPath(std::string thePath)
{
    for (char &aChar : thePath)
        if (aChar >= 'A' && aChar <= 'Z')
            aChar += 'a' - 'A';
    return thePath;
}

struct ArchiveEntry
{
    Sint64 mOffset = 0;
    Sint64 mSize = 0;
    std::weak_ptr<const FileBytes> mCachedBytes;
};

void SDLCALL ReleaseFileBytes(void *, void *theValue)
{
    delete static_cast<std::shared_ptr<const FileBytes> *>(theValue);
}

SDL_IOStream *OpenMemoryFile(const std::shared_ptr<const FileBytes> &theBytes)
{
    static const unsigned char aEmptyFile = 0;
    SDL_IOStream *aStream = SDL_IOFromConstMem(theBytes->empty() ? &aEmptyFile : theBytes->data(), theBytes->size());
    if (aStream == nullptr)
        return nullptr;
    auto *aOwner = new (std::nothrow) std::shared_ptr<const FileBytes>(theBytes);
    if (aOwner == nullptr)
    {
        SDL_CloseIO(aStream);
        SDL_SetError("Not enough memory for archive file stream");
        return nullptr;
    }
    // SDL runs this cleanup on close (and also if setting the property fails).
    if (!SDL_SetPointerPropertyWithCleanup(
            SDL_GetIOProperties(aStream), "PakLib.file.bytes", aOwner, ReleaseFileBytes, nullptr))
    {
        SDL_CloseIO(aStream);
        return nullptr;
    }
    return aStream;
}

class FileSystem
{
public:
    std::string mLocation;
    std::string mKind;
    std::string mPrefix;
    int mPriority;

    FileSystem(std::string theLocation, std::string theKind, std::string thePrefix, int thePriority) :
        mLocation(std::move(theLocation)),
        mKind(std::move(theKind)),
        mPrefix(std::move(thePrefix)),
        mPriority(thePriority)
    {
    }
    virtual ~FileSystem() = default;
    virtual SDL_IOStream *open(const std::string &thePath) = 0;
    virtual Sint64 getFileSize(const std::string &thePath) = 0;
};

class NativeFileSystem : public FileSystem
{
public:
    NativeFileSystem(const std::string &theDirectory, int thePriority) :
        FileSystem(theDirectory, "native", "", thePriority)
    {
    }

    std::string resolve(const std::string &thePath) const
    {
        return (std::filesystem::u8path(mLocation) / std::filesystem::u8path(thePath)).u8string();
    }
    SDL_IOStream *open(const std::string &thePath) override
    {
        return SDL_IOFromFile(resolve(thePath).c_str(), "rb");
    }
    Sint64 getFileSize(const std::string &thePath) override
    {
        SDL_PathInfo aInfo{};
        if (!SDL_GetPathInfo(resolve(thePath).c_str(), &aInfo) || aInfo.type != SDL_PATHTYPE_FILE ||
            aInfo.size > static_cast<Uint64>(std::numeric_limits<Sint64>::max()))
            return -1;
        return static_cast<Sint64>(aInfo.size);
    }
};

class ArchiveFileSystem : public FileSystem
{
protected:
    std::map<std::string, ArchiveEntry> mEntries;

    ArchiveEntry *find(const std::string &thePath)
    {
        const std::string aKey = EntryKey(thePath);
        if (aKey.empty())
            return nullptr;
        // APK/TV packages can include their files/ prefix. Explicit ::prefix
        // mounts select exactly that subtree; ordinary mounts also accept both
        // the logical resource name and the common APK directory prefixes.
        const std::array<std::string, 3> aNames{mPrefix + aKey, "files/" + aKey, "assets/files/" + aKey};
        for (size_t aIndex = 0; aIndex < (mPrefix.empty() ? aNames.size() : 1); ++aIndex)
        {
            auto aFound = mEntries.find(aNames[aIndex]);
            if (aFound != mEntries.end())
                return &aFound->second;
        }
        return nullptr;
    }

public:
    ArchiveFileSystem(
        const std::string &thePath, const std::string &theKind, const std::string &thePrefix, int thePriority) :
        FileSystem(thePath, theKind, thePrefix, thePriority)
    {
    }
    Sint64 getFileSize(const std::string &thePath) override
    {
        ArchiveEntry *aEntry = find(thePath);
        return aEntry != nullptr ? aEntry->mSize : -1;
    }
};

// minizip's stream ABI is backed by an actual mz_stream base and an SDL file;
// no CRT fopen or integer-encoded pointers enter archive loading.
struct ZipArchiveIo : mz_stream
{
    SDL_IOStream *mFile = nullptr;
    ~ZipArchiveIo()
    {
        if (mFile != nullptr)
            SDL_CloseIO(mFile);
    }

    static int32_t Open(void *theStream, const char *, int32_t)
    {
        return IsOpen(theStream);
    }
    static int32_t IsOpen(void *theStream)
    {
        return static_cast<ZipArchiveIo *>(theStream)->mFile != nullptr ? MZ_OK : MZ_OPEN_ERROR;
    }
    static int32_t Read(void *theStream, void *theBytes, int32_t theSize)
    {
        if (theSize < 0)
            return MZ_PARAM_ERROR;
        SDL_IOStream *aFile = static_cast<ZipArchiveIo *>(theStream)->mFile;
        const size_t aRead = SDL_ReadIO(aFile, theBytes, static_cast<size_t>(theSize));
        return aRead == 0 && SDL_GetIOStatus(aFile) == SDL_IO_STATUS_ERROR ? MZ_READ_ERROR
                                                                           : static_cast<int32_t>(aRead);
    }
    static int32_t Write(void *, const void *, int32_t)
    {
        return MZ_WRITE_ERROR;
    }
    static int64_t Tell(void *theStream)
    {
        return SDL_TellIO(static_cast<ZipArchiveIo *>(theStream)->mFile);
    }
    static int32_t Seek(void *theStream, int64_t theOffset, int32_t theOrigin)
    {
        SDL_IOWhence aWhence;
        switch (theOrigin)
        {
            case MZ_SEEK_SET:
                aWhence = SDL_IO_SEEK_SET;
                break;
            case MZ_SEEK_CUR:
                aWhence = SDL_IO_SEEK_CUR;
                break;
            case MZ_SEEK_END:
                aWhence = SDL_IO_SEEK_END;
                break;
            default:
                return MZ_PARAM_ERROR;
        }
        return SDL_SeekIO(static_cast<ZipArchiveIo *>(theStream)->mFile, theOffset, aWhence) >= 0 ? MZ_OK
                                                                                                  : MZ_SEEK_ERROR;
    }
    static int32_t Close(void *theStream)
    {
        auto *aStream = static_cast<ZipArchiveIo *>(theStream);
        const bool aClosed = SDL_CloseIO(aStream->mFile);
        aStream->mFile = nullptr;
        return aClosed ? MZ_OK : MZ_CLOSE_ERROR;
    }
    static int32_t Error(void *theStream)
    {
        return SDL_GetIOStatus(static_cast<ZipArchiveIo *>(theStream)->mFile) == SDL_IO_STATUS_ERROR ? MZ_READ_ERROR
                                                                                                     : MZ_OK;
    }
    ZipArchiveIo()
    {
        static mz_stream_vtbl aInterface{
            Open, IsOpen, Read, Write, Tell, Seek, Close, Error, nullptr, nullptr, nullptr, nullptr};
        vtbl = &aInterface;
        base = nullptr;
    }
};

class ZipFileSystem : public ArchiveFileSystem
{
    ZipArchiveIo mArchive;
    void *mZip = nullptr;
    std::mutex mReadMutex;

public:
    ZipFileSystem(const std::string &thePath, const std::string &thePrefix, int thePriority) :
        ArchiveFileSystem(thePath, "zip", thePrefix, thePriority)
    {
    }
    ~ZipFileSystem() override
    {
        if (mZip != nullptr)
        {
            mz_zip_close(mZip);
            mz_zip_delete(&mZip);
        }
    }
    bool initialize()
    {
        mArchive.mFile = SDL_IOFromFile(mLocation.c_str(), "rb");
        if (mArchive.mFile == nullptr)
            return false;
        mZip = mz_zip_create();
        if (mZip == nullptr || mz_zip_open(mZip, &mArchive, MZ_OPEN_MODE_READ) != MZ_OK)
            return SDL_SetError("Cannot read ZIP directory: %s", mLocation.c_str());
        int32_t aResult = mz_zip_goto_first_entry(mZip);
        while (aResult == MZ_OK)
        {
            mz_zip_file *aInfo = nullptr;
            if (mz_zip_entry_get_info(mZip, &aInfo) != MZ_OK || aInfo == nullptr || aInfo->filename == nullptr ||
                aInfo->uncompressed_size < 0)
                return SDL_SetError("Invalid ZIP entry: %s", mLocation.c_str());
            const std::string aKey = EntryKey(aInfo->filename);
            if (!aKey.empty() && mz_zip_entry_is_dir(mZip) != MZ_OK)
            {
                ArchiveEntry aEntry;
                aEntry.mOffset = mz_zip_get_entry(mZip);
                aEntry.mSize = aInfo->uncompressed_size;
                if (aEntry.mOffset < 0)
                    return false;
                mEntries.emplace(aKey, aEntry);
            }
            aResult = mz_zip_goto_next_entry(mZip);
        }
        return aResult == MZ_END_OF_LIST || SDL_SetError("Invalid ZIP directory: %s", mLocation.c_str());
    }
    SDL_IOStream *open(const std::string &thePath) override
    {
        std::lock_guard<std::mutex> aLock(mReadMutex);
        ArchiveEntry *aEntry = find(thePath);
        if (aEntry == nullptr)
            return nullptr;
        if (auto aCached = aEntry->mCachedBytes.lock())
            return OpenMemoryFile(aCached);
        // raw = 0 asks minizip to decode the entry's compression_method:
        // STORE (0) copies bytes; DEFLATE (8) uses the linked zlib decoder.
        if (static_cast<Uint64>(aEntry->mSize) > std::numeric_limits<size_t>::max() ||
            mz_zip_goto_entry(mZip, aEntry->mOffset) != MZ_OK || mz_zip_entry_read_open(mZip, 0, nullptr) != MZ_OK)
        {
            SDL_SetError("Cannot open ZIP entry: %s::%s", mLocation.c_str(), thePath.c_str());
            return nullptr;
        }
        std::shared_ptr<FileBytes> aBytes;
        try
        {
            aBytes = std::make_shared<FileBytes>(static_cast<size_t>(aEntry->mSize));
        }
        catch (const std::bad_alloc &)
        {
            mz_zip_entry_close(mZip);
            SDL_SetError("Not enough memory for ZIP entry: %s", thePath.c_str());
            return nullptr;
        }
        size_t aPosition = 0;
        bool aSuccess = true;
        while (aPosition < aBytes->size())
        {
            const int32_t aCount = static_cast<int32_t>(std::min<size_t>(aBytes->size() - aPosition, 65536));
            const int32_t aRead = mz_zip_entry_read(mZip, aBytes->data() + aPosition, aCount);
            if (aRead <= 0)
            {
                aSuccess = false;
                break;
            }
            aPosition += static_cast<size_t>(aRead);
        }
        // Finish the entry even on a failed read, and verify its CRC before
        // exposing any data to the game's image/audio/XML decoders.
        if (mz_zip_entry_close(mZip) != MZ_OK)
            aSuccess = false;
        if (!aSuccess)
        {
            SDL_SetError("Damaged ZIP entry: %s::%s", mLocation.c_str(), thePath.c_str());
            return nullptr;
        }
        aEntry->mCachedBytes = aBytes;
        return OpenMemoryFile(aBytes);
    }
};

struct PakFileSlice
{
    SDL_IOStream *mFile;
    Sint64 mStart;
    Sint64 mSize;
    Sint64 mPosition = 0;
    unsigned char mXorKey;

    static Sint64 SDLCALL Size(void *theUser)
    {
        return static_cast<PakFileSlice *>(theUser)->mSize;
    }
    static Sint64 SDLCALL Seek(void *theUser, Sint64 theOffset, SDL_IOWhence theOrigin)
    {
        auto *aSlice = static_cast<PakFileSlice *>(theUser);
        Sint64 aBase;
        switch (theOrigin)
        {
            case SDL_IO_SEEK_SET:
                aBase = 0;
                break;
            case SDL_IO_SEEK_CUR:
                aBase = aSlice->mPosition;
                break;
            case SDL_IO_SEEK_END:
                aBase = aSlice->mSize;
                break;
            default:
                SDL_SetError("Invalid pak seek origin");
                return -1;
        }
        const Sint64 aPosition = theOffset > aSlice->mSize - aBase ? aSlice->mSize
            : theOffset < -aBase                                   ? 0
                                                                   : aBase + theOffset;
        if (SDL_SeekIO(aSlice->mFile, aSlice->mStart + aPosition, SDL_IO_SEEK_SET) < 0)
            return -1;
        aSlice->mPosition = aPosition;
        return aPosition;
    }
    static size_t SDLCALL Read(void *theUser, void *theBytes, size_t theSize, SDL_IOStatus *theStatus)
    {
        auto *aSlice = static_cast<PakFileSlice *>(theUser);
        const Uint64 aRemaining = static_cast<Uint64>(aSlice->mSize - aSlice->mPosition);
        const size_t aCount = static_cast<size_t>(std::min<Uint64>(aRemaining, theSize));
        const size_t aRead = SDL_ReadIO(aSlice->mFile, theBytes, aCount);
        auto *aBytes = static_cast<unsigned char *>(theBytes);
        for (size_t aIndex = 0; aIndex < aRead; ++aIndex)
            aBytes[aIndex] ^= aSlice->mXorKey;
        aSlice->mPosition += static_cast<Sint64>(aRead);
        if (aRead < theSize)
            *theStatus =
                SDL_GetIOStatus(aSlice->mFile) == SDL_IO_STATUS_ERROR ? SDL_IO_STATUS_ERROR : SDL_IO_STATUS_EOF;
        return aRead;
    }
    static bool SDLCALL Close(void *theUser)
    {
        auto *aSlice = static_cast<PakFileSlice *>(theUser);
        const bool aResult = SDL_CloseIO(aSlice->mFile);
        delete aSlice;
        return aResult;
    }
};

class PakFileSystem : public ArchiveFileSystem
{
    unsigned char mXorKey = 247;

    bool readHeader(SDL_IOStream *theFile, void *theBytes, size_t theSize) const
    {
        if (SDL_ReadIO(theFile, theBytes, theSize) != theSize)
            return false;
        auto *aBytes = static_cast<unsigned char *>(theBytes);
        for (size_t aIndex = 0; aIndex < theSize; ++aIndex)
            aBytes[aIndex] ^= mXorKey;
        return true;
    }
    bool readWord(SDL_IOStream *theFile, Uint32 &theValue) const
    {
        unsigned char aBytes[4];
        if (!readHeader(theFile, aBytes, sizeof(aBytes)))
            return false;
        theValue = Uint32(aBytes[0]) | Uint32(aBytes[1]) << 8 | Uint32(aBytes[2]) << 16 | Uint32(aBytes[3]) << 24;
        return true;
    }

public:
    PakFileSystem(const std::string &thePath, const std::string &thePrefix, int thePriority) :
        ArchiveFileSystem(thePath, "pak", thePrefix, thePriority)
    {
    }
    bool initialize()
    {
        SDL_IOStream *aFile = SDL_IOFromFile(mLocation.c_str(), "rb");
        if (aFile == nullptr)
            return false;
        const Sint64 aFileSize = SDL_GetIOSize(aFile);
        Uint32 aMagic = 0;
        Uint32 aVersion = 0;
        bool aValid = aFileSize >= 9 && readWord(aFile, aMagic) && aMagic == 0xBAC04AC0 && readWord(aFile, aVersion) &&
            aVersion == 0;
        Sint64 aPayloadSize = 0;
        bool aHasEnd = false;
        while (aValid && !aHasEnd)
        {
            unsigned char aFlags = 0;
            if (!readHeader(aFile, &aFlags, 1))
            {
                aValid = false;
                break;
            }
            if ((aFlags & 128) != 0)
            {
                aHasEnd = true;
                break;
            }
            unsigned char aNameSize = 0;
            Uint32 aSize = 0;
            unsigned char aFileTime[8];
            char aName[256]{};
            aValid = readHeader(aFile, &aNameSize, 1) && aNameSize != 0 && readHeader(aFile, aName, aNameSize) &&
                readWord(aFile, aSize) && readHeader(aFile, aFileTime, sizeof(aFileTime)) &&
                aSize <= static_cast<Uint32>(std::numeric_limits<int>::max()) && aPayloadSize <= aFileSize - aSize;
            if (!aValid)
                break;
            const std::string aKey = EntryKey(std::string(aName, aNameSize));
            if (aKey.empty() || std::memchr(aName, '\0', aNameSize) != nullptr)
            {
                aValid = false;
                break;
            }
            ArchiveEntry aEntry;
            aEntry.mOffset = aPayloadSize;
            aEntry.mSize = aSize;
            mEntries[aKey] = aEntry;
            aPayloadSize += aSize;
        }
        const Sint64 aHeaderSize = SDL_TellIO(aFile);
        aValid = aValid && aHasEnd && aHeaderSize >= 0 && aPayloadSize <= aFileSize - aHeaderSize;
        SDL_CloseIO(aFile);
        if (!aValid)
            return SDL_SetError("Invalid PopCap PAK index: %s", mLocation.c_str());
        for (auto &aEntry : mEntries)
            aEntry.second.mOffset += aHeaderSize;
        return true;
    }
    SDL_IOStream *open(const std::string &thePath) override
    {
        ArchiveEntry *aEntry = find(thePath);
        if (aEntry == nullptr)
            return nullptr;
        SDL_IOStream *aFile = SDL_IOFromFile(mLocation.c_str(), "rb");
        if (aFile == nullptr)
            return nullptr;
        if (SDL_SeekIO(aFile, aEntry->mOffset, SDL_IO_SEEK_SET) < 0)
        {
            SDL_CloseIO(aFile);
            return nullptr;
        }
        auto *aSlice = new (std::nothrow) PakFileSlice{aFile, aEntry->mOffset, aEntry->mSize, 0, mXorKey};
        if (aSlice == nullptr)
        {
            SDL_CloseIO(aFile);
            SDL_SetError("Not enough memory for pak file stream");
            return nullptr;
        }
        SDL_IOStreamInterface aInterface;
        SDL_INIT_INTERFACE(&aInterface);
        aInterface.size = PakFileSlice::Size;
        aInterface.seek = PakFileSlice::Seek;
        aInterface.read = PakFileSlice::Read;
        aInterface.close = PakFileSlice::Close;
        SDL_IOStream *aStream = SDL_OpenIO(&aInterface, aSlice);
        if (aStream == nullptr)
            PakFileSlice::Close(aSlice);
        return aStream;
    }
};

}

struct FileSystemManager::ResourceLocations
{
    std::mutex mMutex;
    std::string mAssetRoot;
    std::vector<std::shared_ptr<FileSystem>> mFileSystems;

    std::string logicalPath(const std::string &thePath) const
    {
        const auto aPath = std::filesystem::u8path(ForwardSlashes(thePath)).lexically_normal();
        // ResolvePath can return a relative path already prefixed by a
        // relative --assets root. Recognize it before adding the root again.
        const std::string aAbsolute = FullPath(thePath);
        std::string aRoot = mAssetRoot;
        if (aRoot.empty())
            return aPath.has_root_path() ? std::string{} : aPath.generic_u8string();
        if (aRoot.back() != '/')
            aRoot += '/';
        if (FoldPath(aAbsolute).compare(0, aRoot.size(), FoldPath(aRoot)) == 0)
            return aAbsolute.substr(aRoot.size());
        return aPath.has_root_path() ? std::string{} : aPath.generic_u8string();
    }
};

FileSystemManager::FileSystemManager() :
    mLocations(std::make_unique<ResourceLocations>())
{
    setAssetRoot("assets");
}
FileSystemManager::~FileSystemManager() = default;
FileSystemManager *FileSystemManager::getManager()
{
    static FileSystemManager aManager;
    return &aManager;
}

void FileSystemManager::setAssetRoot(const std::string &theDirectory, const std::string &theLanguage)
{
    const std::string aRoot = FullPath(theDirectory);
    {
        std::lock_guard<std::mutex> aLock(mLocations->mMutex);
        mLocations->mFileSystems.clear();
        mLocations->mAssetRoot = aRoot;
    }
    addResource(aRoot, "native", 0);

    // The archive belongs to assets/main.pak, even when loose APK resources
    // have selected assets/files as their logical resource root.
    auto aPackageDirectory = std::filesystem::u8path(aRoot);
    if (FoldPath(aPackageDirectory.filename().u8string()) == "files")
        aPackageDirectory = aPackageDirectory.parent_path();
    const std::string aMain = (aPackageDirectory / "main.pak").u8string();
    SDL_PathInfo aInfo{};
    if (SDL_GetPathInfo(aMain.c_str(), &aInfo) && aInfo.type == SDL_PATHTYPE_FILE && !addResource(aMain, "auto", 10))
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot mount %s: %s", aMain.c_str(), SDL_GetError());

    // Mount only the selected language's supplementary assets. A language
    // package may be absent while resources are being prepared.
    std::string aLanguage = FoldPath(theLanguage);
    std::replace(aLanguage.begin(), aLanguage.end(), '-', '_');
    if (aLanguage.size() != 5 || aLanguage[2] != '_')
        return;
    for (size_t aIndex : {size_t(0), size_t(1), size_t(3), size_t(4)})
        if (aLanguage[aIndex] < 'a' || aLanguage[aIndex] > 'z')
            return;
    const std::string aLanguagePak = (aPackageDirectory / (aLanguage + ".pak")).u8string();
    if (SDL_GetPathInfo(aLanguagePak.c_str(), &aInfo) && aInfo.type == SDL_PATHTYPE_FILE &&
        !addResource(aLanguagePak, "auto", 5))
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot mount %s: %s", aLanguagePak.c_str(), SDL_GetError());
}

bool FileSystemManager::addResource(const std::string &theLocation, const std::string &theKind, int thePriority)
{
    if (theLocation.empty())
        return SDL_SetError("Resource location is empty");
    const size_t aSeparator = theLocation.find("::");
    const std::string aPath = FullPath(theLocation.substr(0, aSeparator));
    std::string aPrefix;
    if (aSeparator != std::string::npos)
    {
        aPrefix = EntryKey(theLocation.substr(aSeparator + 2));
        if (aPrefix.empty())
            return SDL_SetError("Invalid archive prefix: %s", theLocation.c_str());
        if (aPrefix.back() != '/')
            aPrefix += '/';
    }
    std::string aKind = FoldPath(theKind);
    if (aKind.empty() || aKind == "auto" || aKind == "pak")
    {
        SDL_IOStream *aProbe = SDL_IOFromFile(aPath.c_str(), "rb");
        if (aProbe == nullptr)
            return false;
        unsigned char aSignature[4]{};
        const size_t aRead = SDL_ReadIO(aProbe, aSignature, sizeof(aSignature));
        SDL_CloseIO(aProbe);
        aKind = aRead == sizeof(aSignature) && aSignature[0] == 'P' && aSignature[1] == 'K' ? "zip" : "pak";
    }
    {
        std::lock_guard<std::mutex> aLock(mLocations->mMutex);
        for (const auto &aFileSystem : mLocations->mFileSystems)
            if (aFileSystem->mLocation == aPath && aFileSystem->mKind == aKind && aFileSystem->mPrefix == aPrefix &&
                aFileSystem->mPriority == thePriority)
                return true;
    }
    std::shared_ptr<FileSystem> aFileSystem;
    if (aKind == "native")
        aFileSystem = std::make_shared<NativeFileSystem>(aPath, thePriority);
    else if (aKind == "zip")
    {
        auto aArchive = std::make_shared<ZipFileSystem>(aPath, aPrefix, thePriority);
        if (!aArchive->initialize())
            return false;
        aFileSystem = aArchive;
    }
    else if (aKind == "pak")
    {
        auto aArchive = std::make_shared<PakFileSystem>(aPath, aPrefix, thePriority);
        if (!aArchive->initialize())
            return false;
        aFileSystem = aArchive;
    }
    else
        return SDL_SetError("Unsupported resource driver: %s", theKind.c_str());
    std::lock_guard<std::mutex> aLock(mLocations->mMutex);
    for (const auto &aExisting : mLocations->mFileSystems)
        if (aExisting->mLocation == aPath && aExisting->mKind == aKind && aExisting->mPrefix == aPrefix &&
            aExisting->mPriority == thePriority)
            return true;
    mLocations->mFileSystems.push_back(aFileSystem);
    std::stable_sort(mLocations->mFileSystems.begin(),
        mLocations->mFileSystems.end(),
        [](const auto &theFirst, const auto &theSecond) { return theFirst->mPriority < theSecond->mPriority; });
    return true;
}

SDL_IOStream *FileSystemManager::open(const std::string &thePath)
{
    if (thePath.empty())
        return nullptr;
    std::vector<std::shared_ptr<FileSystem>> aFileSystems;
    std::string aLogical;
    {
        std::lock_guard<std::mutex> aLock(mLocations->mMutex);
        aFileSystems = mLocations->mFileSystems;
        aLogical = mLocations->logicalPath(thePath);
    }
    if (!aLogical.empty() && !EntryKey(aLogical).empty())
        for (const auto &aFileSystem : aFileSystems)
            if (SDL_IOStream *aFile = aFileSystem->open(aLogical))
                return aFile;
    return SDL_IOFromFile(ForwardSlashes(thePath).c_str(), "rb");
}

Sint64 FileSystemManager::getFileSize(const std::string &thePath)
{
    if (thePath.empty())
        return -1;
    std::vector<std::shared_ptr<FileSystem>> aFileSystems;
    std::string aLogical;
    {
        std::lock_guard<std::mutex> aLock(mLocations->mMutex);
        aFileSystems = mLocations->mFileSystems;
        aLogical = mLocations->logicalPath(thePath);
    }
    if (!aLogical.empty() && !EntryKey(aLogical).empty())
        for (const auto &aFileSystem : aFileSystems)
        {
            const Sint64 aSize = aFileSystem->getFileSize(aLogical);
            if (aSize >= 0)
                return aSize;
        }
    SDL_PathInfo aInfo{};
    if (SDL_GetPathInfo(ForwardSlashes(thePath).c_str(), &aInfo) && aInfo.type == SDL_PATHTYPE_FILE &&
        aInfo.size <= static_cast<Uint64>(std::numeric_limits<Sint64>::max()))
        return static_cast<Sint64>(aInfo.size);
    return -1;
}

bool FileSystemManager::fileExists(const std::string &thePath)
{
    return getFileSize(thePath) >= 0;
}
void FileSystemManager::release()
{
    std::lock_guard<std::mutex> aLock(mLocations->mMutex);
    mLocations->mFileSystems.clear();
}
}
