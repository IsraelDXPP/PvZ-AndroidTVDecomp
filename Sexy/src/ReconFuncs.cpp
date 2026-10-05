// Named-parameter functions the link still wants.
//
// Each symbol keeps the spelling its callers and the headers already use.
// Bodies that do real work reproduce the binary's purpose; empty hooks stay
// empty; converters that a caller then releases seed the destination so an
// uninitialised slot is never read as a string rep.

#include "SexyTypes.h"
#include "ReconExterns.h"
#include "ReconTypes.h"
#include "InputConnectManager.h"
#include "ReconFreeFunctions.h"
#include "SexyString.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cmath>
#include <cctype>
#include <vector>
#include <map>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <io.h>
#include <direct.h>

#include "MTRand.h"
#include "LogManager.h"
#include "MusicInterface.h"
#include "DescParser.h"
#include "PopAnim.h"
#include "ResourceManager.h"
#include "InputManager.h"
#include "SexyAppBase.h"
#include "DefaultProfileMgr.h"
#include "Buffer.h"
#include "PIEffect.h"
#include "SexyCommonApp.h"
#include "Win32GameCenter.h"
// Game/DriverFactory.h, not "DriverFactory.h": a quoted include is searched
// in this file's directory first, and Sexy\DriverFactory.h is a different class.
#include "Game/DriverFactory.h"
#include "GameCenter.h"
#include "Platform/Win32File.h"
#include "ShareManager.h"
#include "BridgeApp.h"
#include "NotificationManager.h"
#include "DrRpcEngine.h"
#include "SharedRenderTarget.h"
#include "SimpleMessageWidget.h"
#include "ToolTipWidget.h"
#include "VSSetupMenu.h"
#include "HelpBarWidget.h"
#include "JSONNode.h"
#include "LawnApp.h"
#include "ImageLib.h"
#include "Listener.h"
#include "CritSect.h"

// File-local shapes for types that have no header of their own.
namespace Sexy
{
bool        FileExists(Sexy::String const& thePath);
char const* GetEnv(char const* theName);
char const* logLevelName(LogLevel theLevel);
int         logLevelFromName(char const* theName, LogLevel& theLevel);
int         Split(Sexy::String const& theText, Sexy::String const& theSeparator,
                  std::vector<Sexy::String>& theOut);

int          StringToInt(Sexy::String theText, int* theDest);
int          StringToDouble(Sexy::String theText, double* theDest);
int          SexyStringToString(Sexy::String* theDest, Sexy::String const* theSource);
int          GetNextUTF8CharFromStream(void* theStream, char const** theCursor,
                                       int theLimit, wchar_t* theChar);
int          GetFileDate(Sexy::String const& thePath);
bool         Quantize8Bit(unsigned long const* thePixels, int theWidth, int theHeight,
                          unsigned char* theIndices, unsigned long* thePalette,
                          unsigned int* theCount);

int          setLocale(char const* theLocale);
int          SexyUtf8Validate(void* theState, char const* theText, int theLength);
// SetAppDataFolder is declared by ReconHelpers.h.

void         SexyMatrix3Multiply(float* theResult, SexyMatrix3 const& theMatrix,
                                 float const* theVector);
void         SexyWStringToString(Sexy::String* theDest, void const* theSource);
void         WStringToStringFast(Sexy::String* theDest, void const* theSource);

void         logtfd(void* theStream, char const* theFormat, ...);
int          logtfe(Sexy::String const& theTag, char const* theFormat, ...);

class PlatformIntegration
{
public:
    PlatformIntegration(Sexy::SexyAppBase* theApp);

    virtual ~PlatformIntegration();
    virtual void Init();
    virtual void Cleanup();
    virtual Sexy::String GetHardwareID();
    virtual Sexy::String GetUserID();
    virtual Sexy::String GetUserName();
    virtual Sexy::String GetSecretKey();
    virtual Sexy::String GetRechargeLimit();

    // The constructor stores the channel name, then the owning app.
    Sexy::String mChannel;
    Sexy::SexyAppBase* mApp;
};

class PopAnimModParser
{
public:
    PopAnimModParser();
};

// PATransform and PASpriteInst are completed by PopAnim.h.

// One node of the listener list. The value is the listener pointer.
struct InputConnectNode
{
    InputConnectNode*           mNext;
    InputConnectNode*           mPrev;
    Sexy::InputConnectListener* mListener;
};

static void LinkConnectNodeBefore(InputConnectNode* thePos, InputConnectNode* theNode)
{
    theNode->mNext = thePos;
    theNode->mPrev = thePos->mPrev;
    thePos->mPrev->mNext = theNode;
    thePos->mPrev = theNode;
}

class PreLoadWidget
{
public:
    unsigned char mObject[0x10C];

    PreLoadWidget(Sexy::SexyAppBase* theApp);
};

class ServiceManager
{
public:
    static ServiceManager* getInstance(ServiceManager* theSingleton);
    static int setName(ServiceManager* theManager, Sexy::String const& theName);
    static void release(ServiceManager* theManager);
};

class SysFont
{
public:
    void Release();
};

class PropertiesParser
{
public:
    PropertiesParser(Sexy::SexyAppBase* theApp);
    ~PropertiesParser();
    static int ParsePropertiesBuffer(PropertiesParser* theParser, Sexy::Buffer const* theBuffer);
    static void GetErrorText(PropertiesParser* theParser);
};

class FileLogListener
{
public:
    FileLogListener(Sexy::String const& thePath);
};

class TcpLogListener
{
public:
    TcpLogListener(Sexy::String const& theEndpoint);
};

class SimpleUdpLogListener
{
public:
    SimpleUdpLogListener(Sexy::String const& theEndpoint);
};

// Static factories, matching the mangled names the linker printed.
class IShareManager
{
public:
    static void* Create(SexyCommonApp* theApp);
};

class INotificationManager
{
public:
    static void* Create(SexyCommonApp* theApp);
};
} // namespace Sexy

// MultiplexLogListener is a global class. LogManager.cpp defines its own
// fan-out type inside namespace Sexy; this one is the unqualified symbol.
struct MultiplexLogNode
{
    MultiplexLogNode*  mNext;
    MultiplexLogNode*  mPrev;
    Sexy::LogListener* mListener;
};

static void LinkLogNodeBefore(MultiplexLogNode* thePos, MultiplexLogNode* theNode)
{
    theNode->mNext = thePos;
    theNode->mPrev = thePos->mPrev;
    thePos->mPrev->mNext = theNode;
    thePos->mPrev = theNode;
}

class MultiplexLogListener
{
public:
    MultiplexLogListener();
    int addListener(Sexy::LogListener* theListener);
    int hasListener();
};

// Different parameter lists from the helpers ReconTypes.h already declares.
namespace Sexy
{
namespace RbTree
{
NodeBase* _M_copy(NodeBase* theNode, NodeBase* theParent);
void      equal_range(NodeBase** theOut, MapStorage* theTree, Sexy::String const* theKey);
} // namespace RbTree
} // namespace Sexy

class jsonSingletonEMPTY_STD_STRING
{
public:
    int getValue();
};

namespace
{

// Level names, indexed as level + 1. Anything outside -1..3 is "unknown".
const char* const kLogLevelNames[5] =
{
    "verbose", "debug", "info", "warn", "error"
};

// The eighteen vendor names the ad channel table holds, in initialiser order.
// getADType returns the index of the "xiaomi" entry.
const char* const kAdChannels[18] =
{
    "youmi", "wooboo", "nd", "uucun", "besttv", "huanwang", "aliyun",
    "transmension", "skyworth", "xiaomi", "zjtelecomitv", "routon", "letv",
    "unicom", "vtion", "egame", "ysten", "novel"
};

void SeedEmpty(Sexy::String* theDest)
{
    if (theDest != nullptr)
        theDest->mData = Sexy::gEmptyStringData;
}

void CopyText(Sexy::String* theDest, const char* theSource)
{
    if (theDest == nullptr)
        return;
    if (theSource == nullptr)
    {
        SeedEmpty(theDest);
        return;
    }
    *theDest = theSource;
}

int LastSeparator(const char* thePath)
{
    if (thePath == nullptr)
        return -1;
    const char* aSlash = std::strrchr(thePath, '\\');
    const char* aForward = std::strrchr(thePath, '/');
    const char* aHit = aSlash;
    if (aForward != nullptr && (aHit == nullptr || aForward > aHit))
        aHit = aForward;
    if (aHit == nullptr)
        return -1;
    return (int)(aHit - thePath);
}

} // anonymous namespace

// Predecessor in a red-black tree. A red node whose grandparent is itself is
// the header, and the predecessor is then the header's right child. Otherwise
// it is the rightmost node of the left subtree, or the first ancestor reached
// by climbing out of a right-child link.
void* TreeDecrement(void* theNodeWord)
{
    Sexy::RbTree::NodeBase* theNode = static_cast<Sexy::RbTree::NodeBase*>(theNodeWord);
    if (theNode == nullptr)
        return nullptr;

    if (theNode->mColor == 0)
    {
        Sexy::RbTree::NodeBase* aGrand = theNode->mParent->mParent;
        if (aGrand == theNode)
            return aGrand->mRight;
    }

    Sexy::RbTree::NodeBase* aLeft = theNode->mLeft;
    if (aLeft != nullptr)
    {
        while (aLeft->mRight != nullptr)
            aLeft = aLeft->mRight;
        return aLeft;
    }

    Sexy::RbTree::NodeBase* aParent = theNode->mParent;
    if (theNode != aParent->mRight)
        return aParent;

    Sexy::RbTree::NodeBase* aAncestor = aParent;
    for (;;)
    {
        Sexy::RbTree::NodeBase* aNext = aAncestor->mParent;
        const bool aKeepClimbing = (aNext->mRight == aAncestor);
        aAncestor = aNext;
        if (!aKeepClimbing)
            break;
    }
    return aAncestor;
}

// The num_put insertion one slot over from the integer writer. Same operation.
void* NumPutInsert(void* theStream, int theValue)
{
    return OStreamWriteInt(theStream, theValue);
}

// Exception destructors. The throw machinery calls these and ignores the
// result; neither object owns anything this port allocated.
std::exception* DestroyInvalidArgument(std::exception* theException)
{
    return theException;
}

std::exception* DestroyOutOfRange(std::exception* theException)
{
    return theException;
}

// The framework free function, not SexyAppBase::FileExists.
bool Sexy::FileExists(Sexy::String const& thePath)
{
    return ::_access(thePath.c_str(), 0) == 0;
}

unsigned int Sexy::TickCountDiff(unsigned int theEarlier, unsigned int theLater)
{
    return (theEarlier < theLater) ? (theLater - theEarlier) : (theEarlier - theLater);
}

// Milliseconds on the monotonic clock. QueryPerformanceCounter is that clock
// on Windows; the Win32 tick count is a different epoch.
int Sexy::GetTickCount()
{
    static LARGE_INTEGER sFrequency = []()
    {
        LARGE_INTEGER aValue;
        QueryPerformanceFrequency(&aValue);
        if (aValue.QuadPart == 0)
            aValue.QuadPart = 1;
        return aValue;
    }();

    LARGE_INTEGER aCounter;
    QueryPerformanceCounter(&aCounter);
    return (int)((aCounter.QuadPart * 1000) / sFrequency.QuadPart);
}

// Sexy::GetEnv @0x2FB880. The first call loads setup.env from the asset root
// through p_fopen. A name listed there wins; anything else falls through to
// the process environment. The table is read once and kept for the process.
namespace {

struct SetupEnvTable
{
    bool mLoaded;
    std::map<std::string, std::string> mValues;

    SetupEnvTable() : mLoaded(false) {}
};

SetupEnvTable& SetupEnv()
{
    static SetupEnvTable aTable;
    return aTable;
}

void TrimEnvToken(std::string& theText)
{
    size_t aStart = 0;
    while (aStart < theText.size()
        && (theText[aStart] == ' ' || theText[aStart] == '\t'
            || theText[aStart] == '\r' || theText[aStart] == '\n'))
        ++aStart;
    size_t aEnd = theText.size();
    while (aEnd > aStart
        && (theText[aEnd - 1] == ' ' || theText[aEnd - 1] == '\t'
            || theText[aEnd - 1] == '\r' || theText[aEnd - 1] == '\n'))
        --aEnd;
    theText = theText.substr(aStart, aEnd - aStart);
}

void UnquoteEnvValue(std::string& theValue)
{
    if (theValue.size() >= 2 && theValue.front() == '\'' && theValue.back() == '\'')
    {
        theValue = theValue.substr(1, theValue.size() - 2);
        return;
    }
    if (theValue.size() < 2 || theValue.front() != '"' || theValue.back() != '"')
        return;

    std::string aPlain;
    for (size_t i = 1; i + 1 < theValue.size(); ++i)
    {
        if (theValue[i] == '\\' && i + 2 < theValue.size())
        {
            char aEscaped = theValue[i + 1];
            if (aEscaped == 'n')
                aPlain.push_back('\n');
            else if (aEscaped == 'r')
                aPlain.push_back('\r');
            else if (aEscaped == 't')
                aPlain.push_back('\t');
            else
                aPlain.push_back(aEscaped);
            ++i;
        }
        else
        {
            aPlain.push_back(theValue[i]);
        }
    }
    theValue.swap(aPlain);
}

void ApplySetupEnvLine(const std::string& theLine)
{
    if (theLine.empty() || theLine[0] == '#')
        return;

    bool aUnset = false;
    size_t aEqualsAt = std::string::npos;
    if (theLine.compare(0, 4, "set ") == 0)
        aEqualsAt = theLine.find('=', 4);
    else if (theLine.compare(0, 6, "unset ") == 0)
    {
        aUnset = true;
        aEqualsAt = theLine.find('=', 6);
    }
    else
        aEqualsAt = theLine.find('=', 3);
    if (aEqualsAt == std::string::npos)
        return;

    std::string aName = theLine.substr(0, aEqualsAt);
    std::string aValue = theLine.substr(aEqualsAt + 1);
    if (aName.compare(0, 4, "set ") == 0)
        aName.erase(0, 4);
    else if (aName.compare(0, 6, "unset ") == 0)
        aName.erase(0, 6);
    TrimEnvToken(aName);
    TrimEnvToken(aValue);
    UnquoteEnvValue(aValue);
    if (aName.empty())
        return;
    if (aUnset)
        SetupEnv().mValues.erase(aName);
    else
        SetupEnv().mValues[aName] = aValue;
}

void LoadSetupEnv()
{
    SetupEnvTable& aTable = SetupEnv();
    if (aTable.mLoaded)
        return;
    aTable.mLoaded = true;

    FILE* aFile = p_fopen("setup.env", "r");
    if (aFile == nullptr)
        return;

    std::string aLine;
    int aChar = fgetc(aFile);
    while (aChar != EOF)
    {
        if (aChar == '\n')
        {
            ApplySetupEnvLine(aLine);
            aLine.clear();
        }
        else if (aChar != '\r')
        {
            aLine.push_back((char)aChar);
        }
        aChar = fgetc(aFile);
    }
    ApplySetupEnvLine(aLine);
    fclose(aFile);
}

} // namespace

// Sexy::GetEnv @0x2FB880. setup.env is consulted first. A missing name falls
// through to the process environment, which is what EnvVarFile::GetEnv does
// when the file has no entry.
char const* Sexy::GetEnv(char const* theName)
{
    if (theName == nullptr || theName[0] == '\0')
        return nullptr;

    LoadSetupEnv();
    std::map<std::string, std::string>::iterator aFound = SetupEnv().mValues.find(theName);
    if (aFound != SetupEnv().mValues.end())
        return aFound->second.c_str();
    return ::getenv(theName);
}

char const* Sexy::logLevelName(Sexy::LogLevel theLevel)
{
    const int aIndex = (int)theLevel + 1;
    if (aIndex < 0 || aIndex > 4)
        return "unknown";
    return kLogLevelNames[aIndex];
}

// Lower-cases the name, then matches it against the five level names.
// 1 and the level written through the out-parameter on a hit, 0 otherwise.
int Sexy::logLevelFromName(char const* theName, Sexy::LogLevel& theLevel)
{
    if (theName == nullptr || theName[0] == 0)
        return 0;

    char aFolded[64];
    unsigned int aLength = 0;
    while (theName[aLength] != 0 && aLength + 1 < sizeof(aFolded))
    {
        aFolded[aLength] = (char)::tolower((unsigned char)theName[aLength]);
        ++aLength;
    }
    aFolded[aLength] = 0;

    for (int aIndex = 0; aIndex < 5; ++aIndex)
    {
        if (std::strcmp(aFolded, kLogLevelNames[aIndex]) == 0)
        {
            theLevel = (Sexy::LogLevel)(aIndex - 1);
            return 1;
        }
    }
    return 0;
}

// Split the text on every occurrence of the separator and append each piece.
// An empty separator cannot be found, so the whole text is the one piece.
int Sexy::Split(Sexy::String const& theText, Sexy::String const& theSeparator,
                std::vector<Sexy::String>& theOut)
{
    const char* aText = theText.c_str();
    const char* aSeparator = theSeparator.c_str();
    const size_t aSeparatorLength = std::strlen(aSeparator);

    if (aSeparatorLength == 0)
    {
        theOut.push_back(Sexy::String(aText));
        return (int)theOut.size();
    }

    const char* aCursor = aText;
    for (;;)
    {
        const char* aHit = std::strstr(aCursor, aSeparator);
        if (aHit == nullptr)
        {
            theOut.push_back(Sexy::String(aCursor));
            break;
        }
        theOut.push_back(Sexy::String(aCursor, (unsigned int)(aHit - aCursor)));
        aCursor = aHit + aSeparatorLength;
    }
    return (int)theOut.size();
}

int p_feof()
{
    return 0;
}

// MkDir lives in ReconHelpers.cpp.
// MTRand::Next lives in MTRand.cpp.

void StringToUpper(Sexy::String* theDest, char const* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return;
    Sexy::String aResult;
    for (const char* aCursor = theSource; *aCursor != 0; ++aCursor)
        aResult.push_back((char)::toupper((unsigned char)*aCursor));
    *theDest = aResult;
}

void StringToLower(Sexy::String* theDest, char const* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return;
    Sexy::String aResult;
    for (const char* aCursor = theSource; *aCursor != 0; ++aCursor)
        aResult.push_back((char)::tolower((unsigned char)*aCursor));
    *theDest = aResult;
}

void StringToSexyString(Sexy::String* theDest, char const* theSource)
{
    CopyText(theDest, theSource);
}

void ToString(Sexy::String* theDest, Sexy::String const* theSource)
{
    if (theDest == nullptr)
        return;
    *theDest = (theSource != nullptr) ? *theSource : Sexy::String();
}

// Directory into the first string, file name into the second.
void GetPathFrom(Sexy::String* theOut, Sexy::String* theFirst, Sexy::String* theSecond)
{
    (void)theOut;
    if (theFirst == nullptr || theSecond == nullptr)
        return;

    const char* aDirectory = theFirst->c_str();
    const int aCut = LastSeparator(aDirectory);
    if (aCut < 0)
    {
        *theSecond = *theFirst;
        *theFirst = "";
        return;
    }
    *theSecond = Sexy::String(aDirectory + aCut + 1);
    *theFirst = Sexy::String(aDirectory, (unsigned int)aCut);
}

// ImageLib writers, the two-argument decoder and the JPEG2000 init live in
// ImageLib.cpp. The accessors and the constructor pair live there too.

// Stores the app, the default channel name, and the disabled flag. The six
// platform listener registrations have no Windows backend.
Sexy::AdProvider::AdProvider(Sexy::SexyCommonApp* theApp)
{
    mApp = theApp;
    mChannel = "default";
    mDisabled = false;
}

Sexy::AdProvider::~AdProvider()
{
}

void Sexy::AdProvider::show(int theGravity)
{
    if (mDisabled)
        return;
    int aAdType = getADType();
    if (aAdType == -1)
        return;
    AGShowAD(theGravity, aAdType);
}

void Sexy::AdProvider::hide()
{
    AGHideAD();
}

void Sexy::AdProvider::disable()
{
    mDisabled = true;
    hide();
}

int Sexy::AdProvider::getADType()
{
    for (int aIndex = 0; aIndex < 18; ++aIndex)
    {
        if (std::strcmp(kAdChannels[aIndex], "xiaomi") == 0)
            return aIndex;
    }
    return -1;
}

// App-offer ads are offered by the channels in slots 0 and 2..5. Slot 1 and
// everything past slot 5 are not. The probes of slots 6 and 7 do not change
// the answer.
bool Sexy::AdProvider::haveAppOffer()
{
    if (std::strcmp(kAdChannels[0], "xiaomi") == 0)
        return true;
    if (std::strcmp(kAdChannels[1], "xiaomi") == 0)
        return false;
    if (std::strcmp(kAdChannels[2], "xiaomi") == 0
        || std::strcmp(kAdChannels[3], "xiaomi") == 0
        || std::strcmp(kAdChannels[4], "xiaomi") == 0
        || std::strcmp(kAdChannels[5], "xiaomi") == 0)
        return true;
    (void)(std::strcmp(kAdChannels[6], "xiaomi") == 0);
    (void)(std::strcmp(kAdChannels[7], "xiaomi") == 0);
    return false;
}

void Sexy::AdProvider::showAppOffer(Sexy::String const& theOffer, int theX, int theY,
                                    int theWidth, int theHeight)
{
    if (mDisabled)
        return;
    int aAdType = getADType();
    if (aAdType == -1)
        return;
    AGShowOfferAD(theOffer.c_str(), theX, theY, theWidth, aAdType, theHeight);
}

void Sexy::AdProvider::showItemPay(Sexy::String const& theItem, int theCount,
                                   Sexy::String const& theName,
                                   Sexy::String const& theDescription,
                                   float thePrice, int theFlags)
{
    if (mDisabled)
        return;
    int aAdType = getADType();
    if (aAdType == -1)
        return;
    AGShowItems(theItem.c_str(), theCount, theName.c_str(), theDescription.c_str(),
                thePrice, theFlags, aAdType);
}

void Sexy::AdProvider::queryBalance(Sexy::String const& theAccount)
{
    if (mDisabled)
        return;
    int aAdType = getADType();
    if (aAdType == -1)
        return;
    AGQueryBalance(theAccount.c_str(), aAdType);
}

void Sexy::AdProvider::goToLogin(bool theForce)
{
    if (mDisabled)
        return;
    int aAdType = getADType();
    if (aAdType == -1)
        return;
    AGGoToLogin(theForce, aAdType);
}

void Sexy::AdProvider::getUserName(Sexy::String& theName)
{
    int aAdType = getADType();
    if (aAdType == -1)
        return;
    const char* aName = AGGetUserName(aAdType);
    if (aName == nullptr)
        aName = "";
    theName.assign(aName, (unsigned int)std::strlen(aName));
}

void Sexy::AdProvider::getNickname(Sexy::String& theName)
{
    (void)theName;
}

void Sexy::AdProvider::update()
{
}

void Sexy::AdProvider::recharge()
{
}

Sexy::PlatformIntegration::PlatformIntegration(Sexy::SexyAppBase* theApp)
    : mChannel("android"), mApp(theApp)
{
}

void Sexy::PlatformIntegration::Init()
{
}

void Sexy::PlatformIntegration::Cleanup()
{
}

Sexy::PlatformIntegration::~PlatformIntegration()
{
}

Sexy::String Sexy::PlatformIntegration::GetHardwareID()
{
    return Sexy::String(FixedGameCenterUserId());
}

Sexy::String Sexy::PlatformIntegration::GetUserID()
{
    return Sexy::String(FixedGameCenterUserId());
}

Sexy::String Sexy::PlatformIntegration::GetUserName()
{
    return Sexy::String(FixedGameCenterUserName());
}

Sexy::String Sexy::PlatformIntegration::GetSecretKey()
{
    return Sexy::String();
}

Sexy::String Sexy::PlatformIntegration::GetRechargeLimit()
{
    return Sexy::String();
}

Sexy::MusicInterface::~MusicInterface()
{
}

Sexy::SharedRenderTarget::Pool::~Pool()
{
}

Sexy::PopAnimModParser::PopAnimModParser()
{
}

// PATransform, PASpriteDef::GetLabelFrame and PASpriteInst::GetObjectInst
// are defined in PopAnim.cpp.

Sexy::FileLogListener::FileLogListener(Sexy::String const& thePath)
{
    (void)thePath;
}

Sexy::TcpLogListener::TcpLogListener(Sexy::String const& theEndpoint)
{
    (void)theEndpoint;
}

Sexy::SimpleUdpLogListener::SimpleUdpLogListener(Sexy::String const& theEndpoint)
{
    (void)theEndpoint;
}

// The listener list's sentinel is the pair eight bytes in. Both links point
// at that pair, which is an empty list. The word before it is the format.
MultiplexLogListener::MultiplexLogListener()
{
    void* aSelf = this;
    int* aFormat = static_cast<int*>(aSelf);
    aFormat[1] = -1;
    char* aBytes = static_cast<char*>(aSelf);
    MultiplexLogNode* aSentinel = static_cast<MultiplexLogNode*>(
        static_cast<void*>(aBytes + 8));
    aSentinel->mNext = aSentinel;
    aSentinel->mPrev = aSentinel;
}

// Appends the listener unless it is already present. A null listener is ignored.
int MultiplexLogListener::addListener(Sexy::LogListener* theListener)
{
    if (theListener == nullptr)
        return 0;

    void* aSelf = this;
    char* aBytes = static_cast<char*>(aSelf);
    MultiplexLogNode* aSentinel = static_cast<MultiplexLogNode*>(
        static_cast<void*>(aBytes + 8));
    for (MultiplexLogNode* aNode = aSentinel->mNext; aNode != aSentinel; aNode = aNode->mNext)
    {
        if (aNode->mListener == theListener)
            return 1;
    }

    MultiplexLogNode* aNewNode = new MultiplexLogNode;
    aNewNode->mListener = theListener;
    LinkLogNodeBefore(aSentinel, aNewNode);
    return 1;
}

int MultiplexLogListener::hasListener()
{
    void* aSelf = this;
    char* aBytes = static_cast<char*>(aSelf);
    MultiplexLogNode* aSentinel = static_cast<MultiplexLogNode*>(
        static_cast<void*>(aBytes + 8));
    return aSentinel->mNext != aSentinel;
}

Sexy::ServiceManager* Sexy::ServiceManager::getInstance(Sexy::ServiceManager* theSingleton)
{
    (void)theSingleton;
    return nullptr;
}

int Sexy::ServiceManager::setName(Sexy::ServiceManager* theManager, Sexy::String const& theName)
{
    (void)theManager;
    (void)theName;
    return 0;
}

void Sexy::ServiceManager::release(Sexy::ServiceManager* theManager)
{
    (void)theManager;
}

Sexy::PropertiesParser::PropertiesParser(Sexy::SexyAppBase* theApp)
{
    (void)theApp;
}

Sexy::PropertiesParser::~PropertiesParser()
{
}

int Sexy::PropertiesParser::ParsePropertiesBuffer(Sexy::PropertiesParser* theParser,
                                                  Sexy::Buffer const* theBuffer)
{
    (void)theParser;
    (void)theBuffer;
    return 0;
}

void Sexy::PropertiesParser::GetErrorText(Sexy::PropertiesParser* theParser)
{
    (void)theParser;
}

// Two empty listener lists, an unlocked section, and the two reconnect delays.
// The constructor's bool is the second byte of the flag word.
Sexy::InputConnectManager::InputConnectManager(bool theAutoReconnect)
{
    InputConnectNode* aListeners = static_cast<InputConnectNode*>(
        static_cast<void*>(&mListenerNext));
    aListeners->mNext = aListeners;
    aListeners->mPrev = aListeners;

    InputConnectNode* aSecond = static_cast<InputConnectNode*>(
        static_cast<void*>(&mSecondNext));
    aSecond->mNext = aSecond;
    aSecond->mPrev = aSecond;

    // The section is created on first lock. A null word is that unlocked state.
    mCritSect[0] = 0;
    mCritSect[1] = 0;
    mCritSect[2] = 0;
    mCritSect[3] = 0;
    mCritSect[4] = 0;
    mCritSect[5] = 0;
    mCritSect[6] = 0;
    mCritSect[7] = 0;

    mReconnectPending = 0;
    mConnectedCount = 0;
    mConnectingCount = 0;
    mReserved = 0;
    mReconnectDelay = 3000;
    mReconnectDelayLong = 20000;
    mFlags[0] = 0;
    mFlags[1] = theAutoReconnect ? 1 : 0;
}

Sexy::InputConnectManager::~InputConnectManager()
{
}

void Sexy::InputConnectManager::AddListener(Sexy::InputConnectListener* theListener)
{
    InputConnectNode* aNode = new InputConnectNode;
    aNode->mListener = theListener;
    LinkConnectNodeBefore(
        static_cast<InputConnectNode*>(static_cast<void*>(&mListenerNext)), aNode);
}

void Sexy::InputConnectManager::RemoveListener(Sexy::InputConnectListener* theListener)
{
    InputConnectNode* aEnd = static_cast<InputConnectNode*>(
        static_cast<void*>(&mListenerNext));
    for (InputConnectNode* aNode = aEnd->mNext; aNode != aEnd; aNode = aNode->mNext)
    {
        if (aNode->mListener != theListener)
            continue;
        aNode->mPrev->mNext = aNode->mNext;
        aNode->mNext->mPrev = aNode->mPrev;
        delete aNode;
        return;
    }
}

void Sexy::InputConnectManager::Start(Sexy::InputManager* theInput)
{
    (void)theInput;
}

void Sexy::InputConnectManager::Stop()
{
}

void Sexy::InputConnectManager::Update()
{
}

void Sexy::SysFont::Release()
{
}

// Directory half of a path. The flag selects whether the separator is kept.
// No separator yields an empty directory. The trailing unused argument is
// what the older four-argument call sites still pass.
int GetFileDir(Sexy::String* theDest, const void* thePath, int theKeepSeparator, int theUnused)
{
    (void)theUnused;
    if (theDest == nullptr || thePath == nullptr)
        return 0;

    // Call sites pass a Sexy::String*. The object is the pointer, not the
    // characters; reading it as a C string treats the data pointer's bytes
    // as the path.
    const Sexy::String* aString = static_cast<const Sexy::String*>(thePath);
    const char* aText = aString->c_str();
    if (aText == nullptr)
        aText = "";

    const int aCut = LastSeparator(aText);
    if (aCut < 0)
    {
        *theDest = "";
        return 1;
    }
    unsigned int aLength = (unsigned int)aCut;
    if (theKeepSeparator != 0)
        ++aLength;
    *theDest = Sexy::String(aText, aLength);
    return 1;
}

// File-name half. A non-zero flag stops the name at the last dot when that
// dot sits after the last separator, which drops the extension.
int GetFileName(Sexy::String* theDest, Sexy::String const* theName, int theStripExtension)
{
    if (theDest == nullptr || theName == nullptr)
        return 0;

    const char* aText = theName->c_str();
    const int aCut = LastSeparator(aText);
    const char* aFile = (aCut < 0) ? aText : aText + aCut + 1;
    if (theStripExtension == 0)
    {
        *theDest = aFile;
        return 1;
    }

    const char* aDot = std::strrchr(aFile, '.');
    if (aDot == nullptr || aDot == aFile)
    {
        *theDest = aFile;
        return 1;
    }
    *theDest = Sexy::String(aFile, (unsigned int)(aDot - aFile));
    return 1;
}

int Lower(Sexy::String* theDest, Sexy::String const* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return 0;
    StringToLower(theDest, theSource->c_str());
    return 1;
}

// The four-parameter equal_range lives in ReconHelpers. This spelling writes
// the same collapsed range through a two-element out array.
void Sexy::RbTree::equal_range(NodeBase** theOut, MapStorage* theTree, Sexy::String const* theKey)
{
    Sexy::RbTree::NodeBase* aFound = Sexy::RbTree::find(theTree, theKey);
    if (theOut != nullptr)
    {
        theOut[0] = aFound;
        theOut[1] = aFound;
    }
}

// Deep copy of a map node. The tree layer here does not allocate value nodes,
// so a copy has nothing to clone.
Sexy::RbTree::NodeBase* Sexy::RbTree::_M_copy(Sexy::RbTree::NodeBase* theNode,
                                              Sexy::RbTree::NodeBase* theParent)
{
    (void)theNode;
    (void)theParent;
    return nullptr;
}

void Sexy::SexyMatrix3Multiply(float* theResult, SexyMatrix3 const& theMatrix,
                               float const* theVector)
{
    (void)theResult;
    (void)theMatrix;
    (void)theVector;
}

namespace Sexy
{
Sexy::String SexyWStringFromString(Sexy::String const& theText)
{
    return theText;
}
} // namespace Sexy

void StringToWString(Sexy::String* theDest, char const* theSource)
{
    CopyText(theDest, theSource);
}

void StringToWStringFast(Sexy::String* theDest, char const* theSource)
{
    CopyText(theDest, theSource);
}

void WStringToString(Sexy::String* theDest, wchar_t const* theSource)
{
    if (theDest == nullptr)
        return;
    if (theSource == nullptr)
    {
        SeedEmpty(theDest);
        return;
    }
    Sexy::String aResult;
    for (const wchar_t* aCursor = theSource; *aCursor != 0; ++aCursor)
    {
        char aByte = (char)(*aCursor & 0xFF);
        aResult.push_back(aByte);
    }
    *theDest = aResult;
}

void WStringFrom(Sexy::String* theDest, wchar_t const* theSource)
{
    WStringToString(theDest, theSource);
}

void Sexy::SexyWStringToString(Sexy::String* theDest, void const* theSource)
{
    SeedEmpty(theDest);
    (void)theSource;
}

void Sexy::WStringToStringFast(Sexy::String* theDest, void const* theSource)
{
    if (theDest == nullptr)
        return;
    if (theSource == nullptr)
    {
        SeedEmpty(theDest);
        return;
    }
    WStringToString(theDest, static_cast<const wchar_t*>(theSource));
}

void SexyWStringToString(Sexy::String* theDest, Sexy::String const* theSource)
{
    if (theDest == nullptr)
        return;
    *theDest = (theSource != nullptr) ? *theSource : Sexy::String();
}

int Sexy::SexyStringToString(Sexy::String* theDest, Sexy::String const* theSource)
{
    if (theDest == nullptr)
        return 0;
    *theDest = (theSource != nullptr) ? *theSource : Sexy::String();
    return 1;
}

void XMLEncodeString(Sexy::String* theDest, char const* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return;
    Sexy::String aResult;
    for (const char* aCursor = theSource; *aCursor != 0; ++aCursor)
    {
        switch (*aCursor)
        {
        case '&':  aResult += "&amp;";  break;
        case '<':  aResult += "&lt;";   break;
        case '>':  aResult += "&gt;";   break;
        case '"':  aResult += "&quot;"; break;
        case '\'': aResult += "&apos;"; break;
        default:   aResult.push_back(*aCursor); break;
        }
    }
    *theDest = aResult;
}

void XMLDecodeString(Sexy::String* theDest, char const* theSource)
{
    if (theDest == nullptr || theSource == nullptr)
        return;
    Sexy::String aResult;
    for (const char* aCursor = theSource; *aCursor != 0; ++aCursor)
    {
        if (*aCursor != '&')
        {
            aResult.push_back(*aCursor);
            continue;
        }
        if (std::strncmp(aCursor, "&amp;", 5) == 0)  { aResult.push_back('&');  aCursor += 4; continue; }
        if (std::strncmp(aCursor, "&lt;", 4) == 0)   { aResult.push_back('<');  aCursor += 3; continue; }
        if (std::strncmp(aCursor, "&gt;", 4) == 0)   { aResult.push_back('>');  aCursor += 3; continue; }
        if (std::strncmp(aCursor, "&quot;", 6) == 0) { aResult.push_back('"');  aCursor += 5; continue; }
        if (std::strncmp(aCursor, "&apos;", 6) == 0) { aResult.push_back('\''); aCursor += 5; continue; }
        aResult.push_back('&');
    }
    *theDest = aResult;
}

int Sexy::StringToInt(Sexy::String theText, int* theDest)
{
    if (theDest == nullptr)
        return 0;
    *theDest = std::atoi(theText.c_str());
    return 1;
}

int Sexy::StringToDouble(Sexy::String theText, double* theDest)
{
    if (theDest == nullptr)
        return 0;
    *theDest = std::strtod(theText.c_str(), nullptr);
    return 1;
}

int StringToInt(Sexy::String theText, int* theDest)
{
    return Sexy::StringToInt(theText, theDest);
}

void ToSexyString(Sexy::String* theDest, Sexy::String const* theSource)
{
    if (theDest == nullptr)
        return;
    *theDest = (theSource != nullptr) ? *theSource : Sexy::String();
}

void ToWString(Sexy::String* theDest, Sexy::String const* theSource)
{
    if (theDest == nullptr)
        return;
    *theDest = (theSource != nullptr) ? *theSource : Sexy::String();
}

int SexySetLocale(void* theLocale, char const* theEncoding)
{
    (void)theLocale;
    (void)theEncoding;
    return 0;
}

int Sexy::setLocale(char const* theLocale)
{
    (void)theLocale;
    return 0;
}

int Sexy::SexyUtf8Validate(void* theState, char const* theText, int theLength)
{
    (void)theState;
    if (theText == nullptr || theLength <= 0)
        return 0;

    const unsigned char* aBytes = (const unsigned char*)theText;
    int aIndex = 0;
    while (aIndex < theLength)
    {
        unsigned char aLead = aBytes[aIndex];
        if (aLead < 0x80)
        {
            ++aIndex;
            continue;
        }

        int aNeed = 0;
        if ((aLead & 0xE0) == 0xC0)
            aNeed = 1;
        else if ((aLead & 0xF0) == 0xE0)
            aNeed = 2;
        else if ((aLead & 0xF8) == 0xF0)
            aNeed = 3;
        else
            return 0;

        if (aIndex + aNeed >= theLength)
            return 0;
        for (int aExtra = 1; aExtra <= aNeed; ++aExtra)
        {
            if ((aBytes[aIndex + aExtra] & 0xC0) != 0x80)
                return 0;
        }
        aIndex += aNeed + 1;
    }
    return theLength;
}

void Sexy::SetAppDataFolder(Sexy::String const& thePath)
{
    (void)thePath;
}

int Sexy::GetNextUTF8CharFromStream(void* theStream, char const** theCursor,
                                    int theLimit, wchar_t* theChar)
{
    (void)theStream;
    (void)theCursor;
    (void)theLimit;
    (void)theChar;
    return 0;
}

Sexy::String UTF8::filterUnicode(Sexy::String const& theText)
{
    (void)theText;
    return Sexy::String();
}

int Sexy::DividePoly(void* thePolygon, void* theLeft, void* theRight, int theCount, void* thePlane)
{
    (void)thePolygon;
    (void)theLeft;
    (void)theRight;
    (void)theCount;
    (void)thePlane;
    return 0;
}

int TodIsPointInPolygon(void* thePoint, int theCount, float* theVertices)
{
    (void)thePoint;
    (void)theCount;
    (void)theVertices;
    return 0;
}

int Sexy::GetFileDate(Sexy::String const& thePath)
{
    (void)thePath;
    return 0;
}

int logtfd(char const* theFormat, ...)
{
    (void)theFormat;
    return 0;
}

void Sexy::logtfd(void* theStream, char const* theFormat, ...)
{
    (void)theStream;
    (void)theFormat;
}

int logtfe(Sexy::String const& theTag, char const* theFormat, ...)
{
    (void)theTag;
    (void)theFormat;
    return 0;
}

void Sexy::logtfi(int theLevel, char* theFormat, ...)
{
    (void)theLevel;
    (void)theFormat;
}

int TodLogMessage(int theLevel, char const* theFormat, ...)
{
    (void)theLevel;
    (void)theFormat;
    return 0;
}

int p_addResource(char const* thePath, char const* theKind, int theFlags)
{
    (void)thePath;
    (void)theKind;
    (void)theFlags;
    return 0;
}

bool Sexy::Quantize8Bit(unsigned long const* thePixels, int theWidth, int theHeight,
                        unsigned char* theIndices, unsigned long* thePalette,
                        unsigned int* theCount)
{
    (void)thePixels;
    (void)theWidth;
    (void)theHeight;
    (void)theIndices;
    (void)thePalette;
    (void)theCount;
    return false;
}

int __imp__ZN6Native10GameCenter11reportScoreERKNS0_5ScoreE(
    Native::GameCenter* theCenter, Native::GameCenter_Score const& theScore)
{
    (void)theCenter;
    (void)theScore;
    return 0;
}

int __imp__ZN6Native10GameCenter11sendMessageERKNS0_7MessageE(
    Native::GameCenter* theCenter, Native::GameCenter_Message const& theMessage)
{
    (void)theCenter;
    (void)theMessage;
    return 0;
}

int __imp__ZN6Native10GameCenter8purchaseERKNS0_7PaymentE(
    Native::GameCenter* theCenter, Native::GameCenter_Payment const& thePayment)
{
    (void)theCenter;
    (void)thePayment;
    return 0;
}

int __imp__ZN6Native10GameCenter8rechargeERKNS0_12RechargeArgsE(
    Native::GameCenter* theCenter, Native::GameCenter_RechargeArgs const& theArgs)
{
    (void)theCenter;
    (void)theArgs;
    return 0;
}

int __imp__ZN6Native12ShareManager11getFeaturesEv(Native::ShareManager* theManager)
{
    (void)theManager;
    return 0;
}

int __imp__ZN6Native12ShareManager11setListenerEPNS0_8ListenerE(
    Native::ShareManager::Listener* theListener)
{
    (void)theListener;
    return 0;
}

int __imp__ZN6Native12ShareManager5shareERKNS0_4PostE(
    Native::ShareManager* theManager, Native::ShareManager_Post const& thePost)
{
    (void)theManager;
    (void)thePost;
    return 0;
}

int __imp__ZN6Native12ShareManager5shareERKSsS2_()
{
    return 0;
}

int __imp__ZN6Native12ShareManager6createEv(Native::ShareManager* theManager)
{
    (void)theManager;
    return 0;
}

int __imp__ZNK6Native12ShareManager7getNameEv(Native::ShareManager* theManager)
{
    (void)theManager;
    return 0;
}

int j_AGShow91MoreGame()
{
    return 0;
}

int j_AGShowMoreGame()
{
    return 0;
}

void* Sexy::IGameCenter::Create(Sexy::SexyCommonApp* theApp)
{
    (void)theApp;
    return new Sexy::Win32GameCenter();
}

void* Sexy::INotificationManager::Create(Sexy::SexyCommonApp* theApp)
{
    (void)theApp;
    return nullptr;
}

void* Sexy::IShareManager::Create(Sexy::SexyCommonApp* theApp)
{
    (void)theApp;
    return nullptr;
}

int DrCurlEngine::perform(Sexy::String const& theUrl, Sexy::String const& theBody,
                          Sexy::String& theResponse, DrCurlRequest::HttpMethod theMethod)
{
    (void)theUrl;
    (void)theBody;
    (void)theResponse;
    (void)theMethod;
    return 0;
}

SimpleMessageWidget::SimpleMessageWidget(LawnApp* theApp)
{
    (void)theApp;
}

SimpleMessageWidget::~SimpleMessageWidget()
{
    ClearReanim();
}

void SimpleMessageWidget::ClearReanim()
{
    for (int aIndex = 0; aIndex < 128; aIndex++)
    {
        Reanimation* aReanim = mApp->ReanimationTryToGet(mTextReanimID[aIndex]);
        if (aReanim == nullptr)
            continue;
        aReanim->ReanimationDie();
        mTextReanimID[aIndex] = REANIMATIONID_NULL;
    }
}

void SimpleMessageWidget::Draw(Sexy::Graphics* theGraphics)
{
    (void)theGraphics;
}

void SimpleMessageWidget::SetLabel(Sexy::String const& theLabel, SimpleMessageStyle theStyle)
{
    (void)theLabel;
    (void)theStyle;
}

void SimpleMessageWidget::Update()
{
}

int HelpBarWidget::DrawButtons(Sexy::Graphics* theGraphics, std::vector<HelpBarWidget::ButtonDef>& theButtons,
                               int theSelected, float theAlpha)
{
    (void)theGraphics;
    (void)theButtons;
    (void)theSelected;
    (void)theAlpha;
    return 0;
}

void ToolTipWidget::GetLines(std::vector<Sexy::String>& theLines)
{
    // Wrap mLabel on spaces and newlines to mGetsLinesWidth, measuring each
    // character with the warning font. A word that itself exceeds the width
    // stays on the line that started it. The body is empty when there is no
    // label or no font to measure with.
    if (mLabel.empty() || mWarningTextFont == nullptr)
        return;

    unsigned int aStart = 0;
    const unsigned int aLength = mLabel.size();
    while (aStart != aLength)
    {
        unsigned int aEnd = aStart + 1;
        int aWidth = 0;
        unsigned int aBreak = aStart;
        for (;;)
        {
            aBreak = aEnd - 1;
            if (aBreak < aLength)
            {
                const unsigned char aChar = (unsigned char)mLabel[aBreak];
                if (aChar != ' ' && aChar != '\n')
                {
                    aWidth += mWarningTextFont->CharWidth(aChar);
                    ++aEnd;
                    continue;
                }
            }
            if (aBreak == aLength || aWidth >= mGetsLinesWidth)
                break;
            if (mLabel[aBreak] == '\n')
                break;
            aWidth += mWarningTextFont->CharWidth((unsigned char)mLabel[aBreak]);
            ++aEnd;
        }

        theLines.push_back(mLabel.substr(aStart, aBreak - aStart));
        if (aBreak < aLength && mLabel[aBreak] == '\n')
            aStart = aEnd;
        else
            aStart = aBreak;
        while (aStart < aLength && mLabel[aStart] == ' ')
            ++aStart;
    }
}

namespace
{

int TierForPick(int theIndex)
{
    if (theIndex < 2)
        return 0;
    if (theIndex <= 3)
        return 1;
    if (theIndex == 4)
        return 2;
    return 0;
}

int PoolLength(SeedType const* theRow)
{
    int aCount = 0;
    while (aCount < 8 && (int)theRow[aCount] != -1)
        ++aCount;
    return aCount;
}

bool AlreadyPicked(std::vector<SeedType> const& thePicked, SeedType theSeed)
{
    for (SeedType aHave : thePicked)
    {
        if (aHave == theSeed)
            return true;
    }
    return false;
}

SeedType DrawFromRow(SeedType const* theRow, std::vector<SeedType>& thePicked,
                     LawnApp* theApp, bool theZombie)
{
    const int aCount = PoolLength(theRow);
    if (aCount <= 0)
        return (SeedType)-1;

    SeedType aChoice = theRow[0];
    for (int aTry = 0; aTry < 32; ++aTry)
    {
        aChoice = theRow[Sexy::Rand(aCount)];
        if (AlreadyPicked(thePicked, aChoice))
            continue;
        if (theApp == nullptr || theApp->HasSeedType(aChoice, theZombie))
            return aChoice;
    }
    return aChoice;
}

} // namespace

void VSSetupMenu::PickRandomPlants(std::vector<SeedType>& thePicked,
                                   std::vector<SeedType> const& theTaken)
{
    // Five plant cards. Past level 20, one roll in five opens with seed 35 and
    // draws the rest from the later plant rows. Otherwise the five cards come
    // two, two, and one from the first three rows.
    int aPick = 0;
    int aRowBase = 0;
    if (mApp != nullptr && mApp->mPlayerInfo != nullptr
        && mApp->mPlayerInfo->mLevel > 20 && Sexy::Rand(5) == 1)
    {
        thePicked.push_back((SeedType)35);
        aPick = 1;
        aRowBase = 3;
    }

    while (aPick != 5)
    {
        const int aRow = aRowBase + TierForPick(aPick);
        const SeedType aChoice = DrawFromRow(msRandomPools[aRow], thePicked, mApp, false);
        if ((int)aChoice != -1)
            thePicked.push_back(aChoice);
        ++aPick;
    }

    // A taken zombie of type 75 swaps a picked peashooter (3) for 23 when the
    // player still has that seed.
    bool aHasSwapZombie = false;
    for (SeedType aTaken : theTaken)
    {
        if ((int)aTaken == 75)
        {
            aHasSwapZombie = true;
            break;
        }
    }
    if (!aHasSwapZombie || mApp == nullptr || !mApp->HasSeedType((SeedType)3, false))
        return;
    for (SeedType& aCard : thePicked)
    {
        if ((int)aCard == 3)
        {
            aCard = (SeedType)23;
            return;
        }
    }
}

void VSSetupMenu::PickRandomZombies(std::vector<SeedType>& thePicked)
{
    // Five zombie cards: two from the first zombie row, two from the second,
    // one from the third. Those rows sit after the six plant rows.
    for (int aPick = 0; aPick < 5; ++aPick)
    {
        const int aRow = 6 + TierForPick(aPick);
        const SeedType aChoice = DrawFromRow(msRandomPools[aRow], thePicked, mApp, true);
        if ((int)aChoice != -1)
            thePicked.push_back(aChoice);
    }
}

int TodTriangleGroup::DrawGroup(Sexy::Graphics* theGraphics)
{
    (void)theGraphics;
    return 0;
}

int TodTriangleGroup::AddTriangle(Sexy::Graphics* theGraphics, Sexy::Image* theImage,
                                  SexyMatrix3 const& theMatrix, Sexy::TRect<int> const& theSrc,
                                  Sexy::Color const& theColor, int theVertexCount,
                                  Sexy::TRect<int> const& theClip)
{
    (void)theGraphics;
    (void)theImage;
    (void)theMatrix;
    (void)theSrc;
    (void)theColor;
    (void)theVertexCount;
    (void)theClip;
    return 0;
}

int jsonSingletonEMPTY_STD_STRING::getValue()
{
    return 0;
}

int PakLib::DriverFactory::Find(Sexy::String theName)
{
    (void)theName;
    return 0;
}
