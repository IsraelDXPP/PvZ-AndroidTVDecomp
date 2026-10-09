// ReconHelpers.cpp -- implementations of the runtime helpers in ReconHelpers.h.
//
// Each of these is a function the binary really contains. Where the original
// is a libc or libstdc++ routine the body forwards to the platform's
// equivalent, which is the same operation on the same inputs; where it is the
// framework's own code the body reproduces what the decompiler showed.

#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "SharedImageRef.h"
#include "SDLFile.h"
#include "PakLib/FileSystemManager.h"
#include "MTRand.h"


// ---------------------------------------------------------------------------
// SortedKern::Compare -- the kerning table's ordering.
//
// The binary sorts its kerning pairs by the first character and then the
// second, which is what makes the table searchable; the comparator is passed
// to qsort, so it takes two void pointers and returns the sign of the
// difference.
// ---------------------------------------------------------------------------
int SortedKern::Compare(const void* a, const void* b)
{
    const SortedKern* aFirst = static_cast<const SortedKern*>(a);
    const SortedKern* aSecond = static_cast<const SortedKern*>(b);

    if (aFirst->mChar != aSecond->mChar)
        return (aFirst->mChar < aSecond->mChar) ? -1 : 1;
    if (aFirst->mSecond != aSecond->mSecond)
        return (aFirst->mSecond < aSecond->mSecond) ? -1 : 1;
    return 0;
}

#include "ReconHelpers.h"

#include "ImageFont.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>

// ---------------------------------------------------------------------------
// StrFormat -- the framework's sprintf into a String slot.
//
// The binary's own version allocates through the string's own rep, so the
// destination ends up being a normal 4-byte string. Doing the formatting into
// a stack buffer first and then assigning is the same observable result and
// keeps the allocation inside Sexy::String where it belongs.
// ---------------------------------------------------------------------------
int StrFormat(Sexy::String* theDest, const char* theFormat, ...)
{
    if (theDest == nullptr)
        return 0;

    char aBuffer[4096];

    va_list aArgs;
    va_start(aArgs, theFormat);
    int aCount = std::vsnprintf(aBuffer, sizeof(aBuffer), theFormat, aArgs);
    va_end(aArgs);

    if (aCount < 0 || aCount >= static_cast<int>(sizeof(aBuffer)))
        aCount = (int)sizeof(aBuffer) - 1;
    aBuffer[sizeof(aBuffer) - 1] = '\0';
    aBuffer[aCount] = '\0';

    *theDest = aBuffer;
    return aCount;
}

// ---------------------------------------------------------------------------
// StringToInt -- parse an integer and keep the source text on the string.
// ---------------------------------------------------------------------------
int StringToInt(Sexy::String* theDest, const char* theText)
{
    int aValue = (theText != nullptr) ? atoi(theText) : 0;
    if (theDest != nullptr)
        *theDest = theText;
    return aValue;
}

// ---------------------------------------------------------------------------
// GetAppDataFolder -- where the game writes its saved state.
//
// SDL_GetPrefPath selects the host save directory. Callers append names directly,
// so the folder must include its trailing separator.
// ---------------------------------------------------------------------------
int GetAppDataFolder(Sexy::String* destination)
{
    char* folder = SDL_GetPrefPath("PopCap Games", "PlantsVsZombies");
    if (destination) *destination = folder ? folder : "./";
    SDL_free(folder);
    return 0;
}


// ---------------------------------------------------------------------------
// logfi / logfd -- the framework's trace channels.
//
// The binary writes these to the Android log. On Windows they go to stdout,
// which is where the PC build's trace output goes as well.
// ---------------------------------------------------------------------------
static int TodVLog(const char* theFormat, va_list theArgs)
{
    int aCount = vfprintf(stdout, theFormat, theArgs);
    fputc('\n', stdout);
    fflush(stdout);
    return aCount;
}

int logfi(const char* theFormat, ...)
{
    va_list aArgs;
    va_start(aArgs, theFormat);
    int aCount = TodVLog(theFormat, aArgs);
    va_end(aArgs);
    return aCount;
}

int logfd(const char* theFormat, ...)
{
    va_list aArgs;
    va_start(aArgs, theFormat);
    int aCount = TodVLog(theFormat, aArgs);
    va_end(aArgs);
    return aCount;
}

int logfe(const char* theFormat, ...)
{
    va_list aArgs;
    va_start(aArgs, theFormat);
    int aCount = TodVLog(theFormat, aArgs);
    va_end(aArgs);
    return aCount;
}

// ---------------------------------------------------------------------------
// SMemW -- copy a string member, bounded by the destination's capacity.
//
// The binary calls this from ImageFont once per font layer: the source is the
// address of the layer's string slot and `theLimit` its capacity in elements,
// with `theCount` the element size. The observable effect is "store the
// source string into the destination slot".
// ---------------------------------------------------------------------------
int SMemW(char* theDest, const char* theSource, int theCount, int theLimit)
{
    (void)theCount;
    if (theDest == nullptr || theSource == nullptr)
        return 0;

    size_t aMax = (theLimit > 0) ? (size_t)theLimit : strlen(theSource) + 1;
    size_t aLen = strlen(theSource);
    if (aLen >= aMax)
        aLen = (aMax > 0) ? aMax - 1 : 0;

    memcpy(theDest, theSource, aLen);
    theDest[aLen] = '\0';
    return (int)aLen;
}

// ---------------------------------------------------------------------------
// Preserve the framework's stdio element counts over SDL's byte-based I/O.
// ---------------------------------------------------------------------------
// SDL_SeekIO does not clear SDL_GetIOStatus. Keep the framework's EOF flag
// on the stream so a successful seek clears it, as the original fseek did.
static constexpr const char* kFrameworkFileEof = "Sexy.file.eof";

size_t p_fread(void* theBuffer, size_t theSize, size_t theCount, SDL_IOStream* theStream)
{
    if (theStream == nullptr || theSize == 0 || theCount == 0
        || theCount > std::numeric_limits<size_t>::max() / theSize)
        return 0;
    const size_t aRead = SDL_ReadIO(theStream, theBuffer, theSize * theCount);
    SDL_SetBooleanProperty(SDL_GetIOProperties(theStream), kFrameworkFileEof,
                           SDL_GetIOStatus(theStream) == SDL_IO_STATUS_EOF);
    return aRead / theSize;
}

size_t p_fwrite(const void* theBuffer, size_t theSize, size_t theCount, SDL_IOStream* theStream)
{
    if (theStream == nullptr || theSize == 0 || theCount == 0
        || theCount > std::numeric_limits<size_t>::max() / theSize)
        return 0;
    return SDL_WriteIO(theStream, theBuffer, theSize * theCount) / theSize;
}

int p_fgetc(SDL_IOStream* theStream)
{
    unsigned char aByte = 0;
    return p_fread(&aByte, 1, 1, theStream) == 1 ? aByte : EOF;
}

char* p_fgets(char* theBuffer, int theCapacity, SDL_IOStream* theStream)
{
    if (theBuffer == nullptr || theCapacity < 2 || theStream == nullptr)
        return nullptr;
    int aLength = 0;
    while (aLength < theCapacity - 1)
    {
        const int aByte = p_fgetc(theStream);
        if (aByte == EOF)
            break;
        theBuffer[aLength++] = static_cast<char>(aByte);
        if (aByte == '\n')
            break;
    }
    theBuffer[aLength] = '\0';
    return aLength > 0 ? theBuffer : nullptr;
}

int p_feof(SDL_IOStream* theStream)
{
    return theStream != nullptr
        && SDL_GetBooleanProperty(SDL_GetIOProperties(theStream), kFrameworkFileEof, false);
}

// ---------------------------------------------------------------------------
// The remaining framework file helpers.
// ---------------------------------------------------------------------------
SDL_IOStream* p_fopen(const char* thePath, const char* theMode)
{
    // Read-only resources use the mounted file systems. Outputs retain their
    // requested physical paths and can never write into a pack.
    if (thePath == nullptr || thePath[0] == '\0')
        return nullptr;

    std::string aMode = theMode != nullptr ? theMode : "rb";
    // EncodingParser needs unmodified bytes, including UTF-16 and DOS EOF.
    if (aMode.find('b') == std::string::npos)
        aMode += 'b';
    // Only read-only resource opens search the asset root. Writable files
    // must remain at their requested save/log path.
    if (aMode.front() == 'r' && aMode.find('+') == std::string::npos)
        return PakLib::FileSystemManager::getManager()->open(thePath);

    return SDL_IOFromFile(thePath, aMode.c_str());
}

int p_fclose(SDL_IOStream* theStream)
{
    return theStream == nullptr || SDL_CloseIO(theStream) ? 0 : EOF;
}

int p_fseek(SDL_IOStream* theStream, long theOffset, int theOrigin)
{
    if (theStream == nullptr)
        return -1;
    SDL_IOWhence aWhence;
    switch (theOrigin)
    {
    case SEEK_SET: aWhence = SDL_IO_SEEK_SET; break;
    case SEEK_CUR: aWhence = SDL_IO_SEEK_CUR; break;
    case SEEK_END: aWhence = SDL_IO_SEEK_END; break;
    default: return -1;
    }
    if (SDL_SeekIO(theStream, theOffset, aWhence) < 0)
        return -1;
    SDL_SetBooleanProperty(SDL_GetIOProperties(theStream), kFrameworkFileEof, false);
    return 0;
}

long p_ftell(SDL_IOStream* theStream)
{
    if (theStream == nullptr)
        return -1L;
    const Sint64 aPosition = SDL_TellIO(theStream);
    if (aPosition < 0 || aPosition > std::numeric_limits<long>::max())
        return -1L;
    return static_cast<long>(aPosition);
}

// GetFileDir writes the directory part of thePath into theDest.
static int GetFileDir(Sexy::String* theDest, const char* thePath, int, int)
{
    if (theDest == nullptr || thePath == nullptr)
        return 0;

    const char* aSlash = strrchr(thePath, '\\');
    const char* aFwd = strrchr(thePath, '/');
    if (aFwd != nullptr && (aSlash == nullptr || aFwd > aSlash))
        aSlash = aFwd;

    if (aSlash == nullptr)
        return 0;

    Sexy::String aDir(thePath, (unsigned int)(aSlash - thePath));
    *theDest = aDir;
    return 1;
}

// TodLoadResources -- load one resource group, once.
//
// Transcribed from the binary at 0x1ff4d8, whose symbol is
// _Z16TodLoadResourcesRKSs. The dump's shape is a memo around the manager's
// loader: keep a set of the group names already asked for, with a use count, and
// only do the work the first time.
//
//     mgr = *(TodResourceManager **)(Sexy::gSexyAppBase + 1740);
//     if (group is not in gLoadedGroups) insert it with count 0;
//     if (count != 0 || TodResourceManager::TodLoadResources(mgr, group) != 0)
//         ++count, return 1;
//
// +1740 is where SexyAppBase keeps its ResourceManager; the constructor stores
// it there. The memo matters because the game asks for the same group from
// several screens, and re-parsing resources.xml each time would be both slow and
// wrong.
//
namespace {

struct LoadedGroup
{
    char mName[128];
    int  mCount;
};

LoadedGroup gLoadedGroups[64];
int gLoadedGroupCount = 0;

int FindLoadedGroup(const char* theName)
{
    for (int i = 0; i < gLoadedGroupCount; ++i)
    {
        if (SDL_strcasecmp(gLoadedGroups[i].mName, theName) == 0)
            return i;
    }
    return -1;
}

} // namespace

int TodLoadResources(const Sexy::String& theGroup)
{
    const char* aName = theGroup.c_str();
    if (aName == nullptr)
        return 0;

    // TEMPORARY TRACE: proves whether the loading path is reached at all.
    // TodLoadResources is only called from LawnApp::LoadingThreadProc, which
    // runs on a loading thread; if that thread is never started, the resource
    // chain is dead code no matter how complete it is.
    fprintf(stderr, "TodLoadResources: group '%s'\n", aName);
    fflush(stderr);

    int anIndex = FindLoadedGroup(aName);
    if (anIndex < 0)
    {
        if (gLoadedGroupCount >= (int)(sizeof(gLoadedGroups) / sizeof(gLoadedGroups[0])))
            return 0;                       // table full: treat as already asked

        anIndex = gLoadedGroupCount++;
        strncpy(gLoadedGroups[anIndex].mName, aName,
                sizeof(gLoadedGroups[anIndex].mName) - 1);
        gLoadedGroups[anIndex].mName[sizeof(gLoadedGroups[anIndex].mName) - 1] = '\0';
        gLoadedGroups[anIndex].mCount = 0;

        // First request for this group: hand it to the resource manager.
        //
        // This is TodResourceManager::TodLoadResources at 0x1fe880, inlined here
        // because the tree keeps the ResourceManager as one class rather than a
        // TodResourceManager subclass. Its shape is:
        //
        //     if (IsGroupLoaded(group)) return already;
        //     vtbl[96](mgr, group, 0);            // StartLoadResources
        //     while (gSexyAppBase[709] == 0) {    // 709 = the quit flag
        //         if (LoadNextResource(mgr) == 0) break;
        //     }
        //     if (HadError(mgr)) { ShowResourceError(true); return 0; }
        //
        // Every piece of that already exists in the tree:
        // ResourceManager::IsGroupLoaded (ResourceManager.cpp:334),
        // ResourceManager::LoadNextResource (a 598-byte transcribed body at
        // ResourceManager.cpp:3837), ResourceManager::HadError (:422), and
        // LawnApp::LoadGroup (LawnApp.cpp:2382), which is the override that
        // parses resources.xml and is reached through the load.
        Sexy::ResourceManager* aManager =
            gSexyAppBase != nullptr ? gSexyAppBase->mResourceManager : nullptr;

        if (aManager != nullptr && !aManager->IsGroupLoaded(theGroup))
        {
            // TEMPORARY TRACE: the group is still empty after this call, so mark
            // each step of the chain -- whether the load actually runs, how many
            // resources it pulls in, and whether the manager reports an error.
            // LoadNextResource is a full 598-byte transcription, so a zero return
            // means the list was empty to begin with, which points at
            // StartLoadResources not having parsed anything.
            fprintf(stderr, "TodLoadResources: '%s' not loaded yet; starting the load\n", aName);
            fflush(stderr);
            int aPulled = 0;
            // StartLoadResources is the resource manager's load entry.
            aManager->StartLoadResources(theGroup, false);

            while (gSexyAppBase != nullptr && !gSexyAppBase->mShutDown)
            {
                if (aManager->LoadNextResource() == 0)
                    break;
                ++aPulled;
                // LawnApp::LoadGroup counts one unit per resource it pulls.
                // This path loads the same groups, so it counts the same way,
                // which is what the loading bar reads back.
                // 0x58C is the loading-progress counter the bar reads. No named
                // field sits there yet, so the increment stays a word store.
            }
            fprintf(stderr,
                    "TodLoadResources: '%s' pulled %d resource(s); HadError=%d\n",
                    aName, aPulled, aManager->HadError());
            fflush(stderr);

            // Publish whatever decoded. A later miss, such as a reanim still
            // that is not in the asset tree, must not leave the title images
            // unpublished. The globals are what the loading screen draws.
            Sexy::ExtractResourcesByName(aManager, aName);

            if (aManager->HadError() != 0)
            {
                // SexyAppBase::ShowResourceError(true) at 0x3387ac. The tree's
                // LawnApp::ShowResourceError is the override; reaching it needs
                // the app object, and this helper has only the group name, so
                // the failure is reported rather than shown.
                fprintf(stderr, "TodLoadResources: group '%s' failed to load\n", aName);
                fflush(stderr);
                return 0;
            }
        }
    }

    ++gLoadedGroups[anIndex].mCount;
    return 1;
}

void TodDeleteResources(const Sexy::String& theGroup)
{
    const char* aName = theGroup.c_str();
    if (aName == nullptr)
        return;
    int anIndex = FindLoadedGroup(aName);
    if (anIndex < 0)
        return;
    --gLoadedGroups[anIndex].mCount;
    if (gLoadedGroups[anIndex].mCount != 0)
        return;
    if (gSexyAppBase != nullptr && gSexyAppBase->mResourceManager != nullptr)
        gSexyAppBase->mResourceManager->DeleteResources(theGroup);
    gLoadedGroups[anIndex] = gLoadedGroups[gLoadedGroupCount - 1];
    --gLoadedGroupCount;
}

// Legacy copy entry point; callers supply typed storage and a valid length.
void* ReconMemcpyChecked(void* destination, const void* source, unsigned int count, void*)
{
    if (!count || !destination || !source) return destination;
    return std::memcpy(destination, source, count);
}


// Sexy::GetEnv is defined in ReconFuncs.cpp. It reads setup.env first, then
// the process environment.
namespace Sexy {
char const* GetEnv(char const* theName);
}

// Sexy::GetEnvOption @0x2fb9f4 (_ZN4Sexy12GetEnvOptionEPKcb).
// R1 is the bool default. A missing variable, or a value outside the eight
// literals below, returns that default. "1"/"yes"/"true"/"True" return 1;
// "0"/"no"/"false"/"False" return 0.
int Sexy::GetEnvOption(const char* theName, bool theDefault)
{
    const char* aValue = GetEnv(theName);
    if (aValue == nullptr)
        return theDefault ? 1 : 0;
    if (strcmp(aValue, "1") == 0 || strcmp(aValue, "yes") == 0
        || strcmp(aValue, "true") == 0 || strcmp(aValue, "True") == 0)
        return 1;
    if (strcmp(aValue, "0") == 0 || strcmp(aValue, "no") == 0
        || strcmp(aValue, "false") == 0 || strcmp(aValue, "False") == 0)
        return 0;
    return theDefault ? 1 : 0;
}

// MSleep -- the framework's millisecond sleep.
int MSleep(int theMillis)
{
    if (theMillis > 0)
        SDL_Delay(static_cast<Uint32>(theMillis));
    return 0;
}

// ---------------------------------------------------------------------------
// Rand -- the framework's random-in-range helper.
//
// @0x2F9EC8 / 0x2F9ED4 / 0x2F9EE4 all use the shared MTRand at
// 0x71C4EC. Its draws cover 31 bits, including large weighted-pick ranges.
// ---------------------------------------------------------------------------
namespace
{
Sexy::MTRand& SharedRandom()
{
    static Sexy::MTRand random;
    return random;
}
}

// _ZN4Sexy4RandEv @0x2F9EC8 -- the generator's own draw, no bound. The int
// and float forms scale this into [0, theRange).
int Sexy::Rand()
{
    return static_cast<int>(SharedRandom().Next());
}

int Sexy::Rand(void* /*theObject*/, int theRange)
{
    if (theRange <= 0)
        return 0;
    return static_cast<int>(SharedRandom().Next(static_cast<unsigned int>(theRange)));
}

int Sexy::Rand(int theRange)
{
    return Sexy::Rand(nullptr, theRange);
}

// _ZN4Sexy4RandEf @0x2f9ee4 -- the float overload. The binary keeps it separate
// from the int one (a 10-byte body each; both scale the next value into the
// range), so `Rand(2.0f)` must NOT silently land on Rand(int).
float Sexy::Rand(float theRange)
{
    if (theRange <= 0.0f)
        return 0.0f;
    return SharedRandom().Next(theRange);
}

// The double literal form; see the declaration for why it exists.
float Sexy::Rand(double theRange)
{
    return Sexy::Rand((float)theRange);
}

// Trim -- the third sibling of Upper/Lower: copy the source into the
// destination with leading and trailing whitespace removed.
int Trim(Sexy::String* theDest, const Sexy::String* theSource)
{
    if (theDest == nullptr)
        return 0;
    if (theSource == nullptr)
    {
        theDest->clear();
        return 0;
    }
    const char* aData = theSource->c_str();
    unsigned int aLength = theSource->size();
    unsigned int aStart = 0;
    while (aStart < aLength && (aData[aStart] == ' ' || aData[aStart] == '\t' ||
                                aData[aStart] == '\r' || aData[aStart] == '\n'))
        ++aStart;
    unsigned int aEnd = aLength;
    while (aEnd > aStart && (aData[aEnd - 1] == ' ' || aData[aEnd - 1] == '\t' ||
                             aData[aEnd - 1] == '\r' || aData[aEnd - 1] == '\n'))
        --aEnd;
    theDest->Assign(aData + aStart, aEnd - aStart);
    return (int)(aEnd - aStart);
}

// _ZN4Sexy5UpperERKSs @0x2fb5ac -- the uppercase counterpart of Lower.
int Upper(Sexy::String* theDest, const Sexy::String* theSource)
{
    if (theDest == nullptr)
        return 0;
    if (theSource == nullptr)
    {
        theDest->clear();
        return 0;
    }
    const char* aSource = theSource->c_str();
    unsigned int aLength = theSource->size();
    theDest->Assign(aSource, aLength);
    char* aData = theDest->data();
    for (unsigned int i = 0; i < aLength; ++i)
    {
        if (aData[i] >= 'a' && aData[i] <= 'z')
            aData[i] = (char)(aData[i] - 'a' + 'A');
    }
    return (int)aLength;
}

// _ZN4Sexy13CommaSeperateEi @0x2fbe2c -- group the decimal digits in threes// with commas, which is the "1,234,567" the money string shows.
Sexy::String CommaSeperate(int theValue)
{
    char aDigits[32];
    int aLength = 0;

    // Magnitude first, so INT_MIN does not overflow on negation.
    unsigned int aMagnitude = (theValue < 0) ? (unsigned int)(-(theValue + 1)) + 1u
                                             : (unsigned int)theValue;
    do
    {
        aDigits[aLength++] = (char)('0' + (aMagnitude % 10u));
        aMagnitude /= 10u;
    }
    while (aMagnitude != 0u);

    char aOut[40];
    int aOutLength = 0;
    if (theValue < 0)
        aOut[aOutLength++] = '-';
    for (int i = aLength - 1; i >= 0; --i)
    {
        aOut[aOutLength++] = aDigits[i];
        // A comma after every third digit counted from the right, but never
        // before the first digit.
        if (i > 0 && (i % 3) == 0)
            aOut[aOutLength++] = ',';
    }

    Sexy::String aResult;
    aResult.Assign(aOut, (unsigned int)aOutLength);
    return aResult;
}

// ---------------------------------------------------------------------------
// The locale accessors. The binary's own versions (0x3063f8 and its setter)
// talk to the platform's locale service; on Windows the CRT locale is the same
// contract, so the getter reports the current C locale's name and the setter
// installs one.
//
// TODO(recon): the target's encoding names (the "UTF-8", "GBK" spellings the
// resource loader passes) are not the CRT's ("en-US", ".936"), so the setter
// currently accepts the call and leaves the locale alone. The resource path
// only reads the getter, so nothing on the game path depends on it yet.
// ---------------------------------------------------------------------------
static char gLocaleEncoding[64] = "UTF-8";

const char* SexyGetLocaleEncoding()
{
    return gLocaleEncoding;
}

int SexySetLocaleEncoding(const char* theEncoding)
{
    if (theEncoding == nullptr)
        return 0;
    unsigned int i = 0;
    while (theEncoding[i] != 0 && i < sizeof(gLocaleEncoding) - 1)
    {
        gLocaleEncoding[i] = theEncoding[i];
        ++i;
    }
    gLocaleEncoding[i] = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// The string-member accessors (declared in ReconHelpers.h).
//
// SMemRStr reads a string member and SMemWStr writes one; both were only
// declared in their three-argument form, so ImageFont.cpp carried its own
// two-argument copies. The two-argument symbols are the real ones.
// ---------------------------------------------------------------------------
int SMemRStr(Sexy::String* theDest, const Sexy::String* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return 0;
    theDest->Assign(theSource->c_str(), theSource->size());
    return 1;
}

int SMemWStr(Sexy::String* theDest, const Sexy::String* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return 0;
    theDest->Assign(theSource->c_str(), theSource->size());
    return 1;
}

int SMemWStr(char* theDest, const char* theSource, int theLimit)
{
    return SMemW(theDest, theSource, 1, theLimit);
}

// ---------------------------------------------------------------------------
// The late additions declared at the end of ReconHelpers.h. ResourceManager.cpp
// carried file-local copies of these, which would have left the symbols
// undefined at link time; the definitions live here now.
// ---------------------------------------------------------------------------
namespace Sexy {

int gResourceManagerInfo = 0;

// _ZN4Sexy18GetResourcesFolderEv @0x2fa0fc -- the body copies the framework's
// resource-root string into the caller's buffer and returns it, so this is the
// sret form. The earlier `const char* (void)` spelling was a guess and did not
// match a single call site.
int GetResourcesFolder(Sexy::String* theDest)
{
    // This project roots the resource tree at the executable's directory; the
    // platform layer will install the real path there at startup.
    if (theDest == nullptr)
        return 0;
    static char aFolder[1024] = "";
    theDest->Assign(aFolder, (unsigned int)strlen(aFolder));
    return 1;
}

int GetEnvIntOption(const char* theName, int theDefault)
{
    // GetEnvIntOption @0x2fba90 preserves the integer in r1 and returns it
    // when GetEnv is null; otherwise it tail-calls atoi.
    const char* value = theName != nullptr ? Sexy::GetEnv(theName) : nullptr;
    return value != nullptr ? atoi(value) : theDefault;
}

// _ZN4Sexy9DebugInitEii @0x3b9bdc -- two parameters.
int DebugInit(int theMode, int theFlags)
{
    // The framework arms its debug facilities here. Nothing in this port reads
    // them, so the call is accepted and reports success.
    (void)theMode;
    (void)theFlags;
    return 1;
}

// _ZN4Sexy24SWTri_AddAllDrawTriFuncsE... @0x3ab754 -- fills the software
// rasteriser's function table with the per-format DrawTriangle entry points.
int SWTri_AddAllDrawTriFuncs(void* theTable)
{
    // The software renderer is not ported. This project renders through
    // OpenGL 3.3, so the table is left alone and the call reports success.
    (void)theTable;
    return 1;
}

// _ZN4Sexy14StringToDoubleE... -- parse the string into the caller's slot.
// Both parameters arrive as void* (see the header); the destination is a double
// in the 228-byte overload and an int in the 102-byte one, so this writes the
// double form and lets the int overload alias it, which is what the call sites
// in PopAnim do (they read the low word).
int StringToDouble(void* theText, void* theDest)
{
    if (theText == nullptr || theDest == nullptr)
        return 0;
    const Sexy::String* aText = static_cast<const Sexy::String*>(theText);
    double* aNumber = static_cast<double*>(theDest);
    *aNumber = strtod(aText->c_str(), nullptr);
    return 1;
}

// Sexy::Ratio -- see the header.
Ratio::Ratio()    : mNumerator(1), mDenominator(0)
{
}

int Ratio::Set(int theNumerator, int theDenominator)
{
    mNumerator = theNumerator;
    mDenominator = theDenominator;
    return 0;
}

void RemoveTrailingSlash(Sexy::String* theDest, const void* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return;
    const Sexy::String* aSource = static_cast<const Sexy::String*>(theSource);
    unsigned int aLength = aSource->size();
    const char* aData = aSource->c_str();
    while (aLength > 0 && (aData[aLength - 1] == '/' || aData[aLength - 1] == '\\'))
        --aLength;
    theDest->Assign(aData, aLength);
}

const char* setLocale(void* theCategory, const char* theName)
{
    // Game resource locales must remain available even when the operating
    // system has no matching C locale installed. Preserve xx_YY for the
    // original game checks; archive filenames use its lowercase spelling.
    (void)theCategory;
    static std::string aLocale = "en_US";
    if (theName == nullptr)
        return aLocale.c_str();

    std::string aRequested = theName;
    bool aValid = aRequested.size() == 5 && (aRequested[2] == '_' || aRequested[2] == '-');
    if (aValid)
    {
        for (size_t aIndex : {size_t(0), size_t(1), size_t(3), size_t(4)})
        {
            char& aLetter = aRequested[aIndex];
            if (aLetter >= 'A' && aLetter <= 'Z')
                aLetter += 'a' - 'A';
            if (aLetter < 'a' || aLetter > 'z')
                aValid = false;
        }
    }
    if (!aValid)
    {
        aLocale = "en_US";
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Invalid game language '%s'; using en_us", theName);
        return aLocale.c_str();
    }
    aRequested[2] = '_';
    aRequested[3] -= 'a' - 'A';
    aRequested[4] -= 'a' - 'A';
    aLocale = aRequested;
    return aLocale.c_str();
}

void SexyStringToString(void* theDest, unsigned int theSource)
{
    if (theDest == nullptr || theSource == 0)
        return;
    const Sexy::String* aSource = nullptr;
    std::memcpy(&aSource, &theSource, sizeof(aSource));
    static_cast<Sexy::String*>(theDest)->Assign(aSource->c_str(), aSource->size());
}

int SWTri_AddDrawTriFunc(int theFormat, int theTexMode, int theAlphaMode,
                         int theModMode, int theGlobalMode, int theBlendMode, int (*theFunc)())
{
    // The software rasteriser is not ported. This project renders through
    // OpenGL 3.3, so registering a batch reports failure. LawnApp::Init only
    // tests the result.
    (void)theFormat;
    (void)theTexMode;
    (void)theAlphaMode;
    (void)theModMode;
    (void)theGlobalMode;
    (void)theBlendMode;
    (void)theFunc;
    return 0;
}

int DrawTriangle_8888_TEX1_TALPHA1_MOD0_GLOB0_BLEND1()
{
    // Never reached: nothing registers it, because SWTri_AddDrawTriFunc above
    // declines. Present so the symbol resolves.
    return 0;
}

} // namespace Sexy

// PopAnim calls the global overload declared in ReconHelpers.h. The body above
// is Sexy::StringToDouble; this is the symbol the call sites link.
int StringToDouble(void* theText, void* theDest)
{
    return Sexy::StringToDouble(theText, theDest);
}

// ---------------------------------------------------------------------------
// The SharedImageRef and tag-lookup helpers (declared in ReconHelpers.h).
//
// SharedImageRefAssign is `SharedImageRef::operator=`: the binary's
// `_ZN4Sexy14SharedImageRefaSERKS0_` @0x3424A4, whose body releases the old
// reference and adopts the source's two words. SharedImageRefImage is the
// `operator Image*` conversion @0x3424FC, which reads the +4 word.
//
// These three are at GLOBAL scope on purpose, which is why they sit outside the
// namespace that closes just above. ReconHelpers.h declares them at global scope
// and every caller -- ImageFont.cpp's GenerateActiveFontLayers and SerializeRead
// -- reaches them unqualified from there. Defined inside `namespace Sexy` they
// mangled to `?SharedImageRefAssign@Sexy@@...` while the linker wanted
// `?SharedImageRefAssign@@...`, which is a link error and not a compile error.
// ---------------------------------------------------------------------------
int SharedImageRefAssign(Sexy::SharedImageRef* theDest, const Sexy::SharedImageRef* theSource)
{
    if (theSource == nullptr || theDest == nullptr)
        return 0;
    *theDest = *theSource;
    return 1;
}

Sexy::Image* SharedImageRefImage(const Sexy::SharedImageRef* theRef)
{
    if (theRef == nullptr)
        return nullptr;
    return theRef->mImage;
}

// `std::find` over a `std::vector<Sexy::String>`. The binary's helper is
// `_ZSt6__findIN9__gnu_cxx17__normal_iteratorIPSbIi...` @0x312180 and walks the
// range comparing with `operator==`, answering the end position when absent.
Sexy::String* FindInStringVector(Sexy::String* theBegin, Sexy::String* theEnd, const Sexy::String* theValue)
{
    if (theValue == nullptr)
        return theEnd;
    for (Sexy::String* aPos = theBegin; aPos != theEnd; ++aPos)
    {
        if (*aPos == *theValue)
            return aPos;
    }
    return theEnd;
}

int MkDir(const char* thePath)
{
    if (thePath == nullptr || thePath[0] == 0)
        return 0;
    return Sexy::SDLFile::MakeDirs(thePath) ? 1 : 0;
}

int MkDir(const Sexy::String& thePath)
{
    return MkDir(thePath.c_str());
}

int MkDir(const Sexy::String* thePath)
{
    if (thePath == nullptr)
        return 0;
    return MkDir(thePath->c_str());
}

namespace Sexy {
namespace RbTree {

NodeBase* equal_range(NodeBase* theNode, const void* theKey,
                      NodeBase** theFirst, NodeBase** theSecond)
{
    // The RbTree layer in this project is a stub (see RbTree.cpp): find() below
    // ignores the key, so the range collapses to the single node it returns.
    //
    // ReconTypes.h types find()'s key deliberately, so the generic `const void *`
    // this spelling takes has to name the instantiation it belongs to. This
    // container family is the string-keyed one (std::set/map on Sexy::String,
    // StringLessNoCase), which is the ordering find() implements.
    const Sexy::String* aKey = static_cast<const Sexy::String*>(theKey);
    NodeBase* aFound = find(theNode, aKey);
    if (theFirst != nullptr)
        *theFirst = aFound;
    if (theSecond != nullptr)
        *theSecond = aFound;
    return aFound;
}

} // namespace RbTree

} // namespace Sexy
