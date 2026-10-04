// LogManager -- the framework's listener registry and the network log
// transport, rewritten from the decompilation of libGameMain.so.
//
// Sexy::LogManager is the listener registry. TM::LogManager is the transport
// LawnApp holds. The two share a name and nothing else.

#include "LogManager.h"
#include "CritSect.h"
#include "Listener.h"
#include "ReconFreeFunctions.h"
#include "SexyString.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

// ---------------------------------------------------------------------------
// The listener family. LogListener has formatMessage at slot 2 and log at
// slot 3. The base log is a no-op; every listener the manager builds
// replaces it.
// createListener builds one of five listeners from the endpoint the
// SEXY_LOG variable names.
// ---------------------------------------------------------------------------
namespace Sexy {

class LogListener
{
public:
    virtual ~LogListener();
    virtual int formatMessage(int theLevel, Sexy::String const& theTag,
                              Sexy::String const& theMessage);
    virtual int log(int theLevel, Sexy::String const& theTag,
                    Sexy::String const& theMessage);

    // -1 means "use the manager's default format". Every listener createListener
    // builds stores that here.
    int mFormat;

    LogListener() : mFormat(-1) {}
};

LogListener::~LogListener()
{
}

// 40D064. The line is the manager's name, then whichever of the tag, the
// pid and the timestamp the format bits ask for, then the message. The bits
// are read off the body: 0x1 the tag, 0x2 the timestamp, 0x4 the pid.
int LogListener::formatMessage(int theLevel, Sexy::String const& theTag,
                               Sexy::String const& theMessage)
{
    LogManager* aManager = (LogManager*)LogManager::getInstance();

    int aFormat = mFormat;
    if (aFormat == -1)
        aFormat = aManager->getDefaultFormat();

    Sexy::String aResult = aManager->getName();

    if ((aFormat & 1) != 0 && !theTag.empty())
    {
        Sexy::String aBracketed;
        StrFormat(&aBracketed, "[%s]", theTag.c_str());
        aResult += aBracketed;
    }

    if ((aFormat & 4) != 0)
    {
        Sexy::String aPid;
        StrFormat(&aPid, "/%u", (unsigned)_getpid());
        aResult += aPid;
    }

    if ((aFormat & 2) != 0)
    {
        unsigned aTicks = ::GetTickCount();
        Sexy::String aTime;
        StrFormat(&aTime, " %u.%03u", aTicks / 1000, aTicks % 1000);
        aResult += aTime;
    }

    (void)theLevel;
    aResult += theMessage;
    return 0;
}

// The base's slot 3 is __cxa_pure_virtual. Nothing calls it: every listener
// the manager builds overrides it.
int LogListener::log(int theLevel, Sexy::String const& theTag,
                     Sexy::String const& theMessage)
{
    (void)theLevel;
    (void)theTag;
    (void)theMessage;
    return 0;
}

// "stdout" and "default" both allocate 8 bytes and differ only in the vtable
// createListener stores. Neither has a body of its own in this port.
class StdoutLogListener : public LogListener
{
};

class DefaultLogListener : public LogListener
{
};

// silent() builds the same 8-byte object from a third listener table.
class SilentLogListener : public LogListener
{
};

class FileLogListener : public LogListener
{
public:
    explicit FileLogListener(Sexy::String const& thePath);
};

class TcpLogListener : public LogListener
{
public:
    explicit TcpLogListener(Sexy::String const& theEndpoint);
};

class SimpleUdpLogListener : public LogListener
{
public:
    explicit SimpleUdpLogListener(Sexy::String const& theEndpoint);
};

// A listener that fans one line out to several. The binary keeps its children
// in a std::list; the standard list is that container. Named apart from the
// global MultiplexLogListener, which ReconTypes.h only forward-declares.
class FanOutLogListener : public LogListener
{
public:
    FanOutLogListener();
    int addListener(LogListener* theListener);
    int hasListener();

    std::list<LogListener*> mListeners;
};

FanOutLogListener::FanOutLogListener()
{
}

// 3C7250. A listener already present, or a null one, is ignored.
int FanOutLogListener::addListener(LogListener* theListener)
{
    if (theListener == nullptr)
        return 0;
    for (std::list<LogListener*>::iterator aIt = mListeners.begin();
         aIt != mListeners.end(); ++aIt)
    {
        if (*aIt == theListener)
            return 0;
    }
    mListeners.push_back(theListener);
    return 1;
}

// 3C7244. Non-empty exactly when the sentinel's next link is not itself.
int FanOutLogListener::hasListener()
{
    return mListeners.empty() ? 0 : 1;
}

} // namespace Sexy

// ---------------------------------------------------------------------------
// Sexy::LogManager
// ---------------------------------------------------------------------------
namespace Sexy {

LogManager LogManager::msLogManager;

// 3C38CC. The singleton is the object itself, so the address of it is the
// instance.
void* LogManager::getInstance()
{
    return &msLogManager;
}

// 3C38E0. The endpoint selects the listener. A scheme prefix wins over the
// bare word, and anything unrecognised yields nothing.
LogListener* LogManager::createListener(Sexy::String const& theSpec)
{
    const char* aSpec = theSpec.c_str();

    if (std::strncmp(aSpec, "udp://", 6) == 0 || std::strcmp(aSpec, "udp") == 0)
        return new SimpleUdpLogListener(theSpec);
    if (std::strncmp(aSpec, "tcp://", 6) == 0 || std::strcmp(aSpec, "tcp") == 0)
        return new TcpLogListener(theSpec);
    if (std::strncmp(aSpec, "file://", 7) == 0 || std::strcmp(aSpec, "file") == 0)
        return new FileLogListener(theSpec);
    if (std::strcmp(aSpec, "stdout") == 0)
        return new DefaultLogListener();
    if (std::strcmp(aSpec, "default") == 0)
        return new DefaultLogListener();
    return nullptr;
}

// 3C3A8C.
int LogManager::setListener(LogListener* theListener)
{
    mListener = theListener;
    return 1;
}

// 3C3AB0.
int LogManager::resetListener()
{
    mListener = nullptr;
    return 1;
}

// 3C3AC0. Reinstalls the listener the SEXY_LOG variable named.
int LogManager::resetToPlatformListener()
{
    const char* aSpec = std::getenv("SEXY_LOG");
    Sexy::String aName(aSpec != nullptr ? aSpec : "default");

    if (mListenerRoot == nullptr)
        mListenerRoot = new FanOutLogListener();

    LogListener* aListener = createListener(aName);
    ((FanOutLogListener*)mListenerRoot)->addListener(aListener);
    mListener = mListenerRoot;
    return 1;
}

// 3C3BD4.
int LogManager::release()
{
    return msLogManager.resetToPlatformListener();
}

// 3C3BE8.
int LogManager::silent()
{
    return setListener(new SilentLogListener());
}

// 3C3C1C.
int LogManager::setVerboseLevel(int theLevel)
{
    mVerboseLevel = theLevel;
    return theLevel;
}

// 3C3C20.
int LogManager::getVerboseLevel()
{
    return mVerboseLevel;
}

// 3C3C24.
Sexy::String LogManager::getDefaultTag()
{
    return mDefaultTag;
}

// 3C3C34.
void LogManager::setDefaultTag(Sexy::String const& theTag)
{
    mDefaultTag = theTag;
}

// 3C3C3C.
Sexy::String LogManager::getName()
{
    return mName;
}

// 3C3C4C.
void LogManager::setName(Sexy::String const& theName)
{
    mName = theName;
}

// 3C3C54.
int LogManager::getDefaultFormat()
{
    return mDefaultFormat;
}

// 3C3C58.
int LogManager::setDefaultFormat(int theFormat)
{
    mDefaultFormat = theFormat;
    return theFormat;
}

// 3C3C6C. The per-tag table wins when the tag is in it; otherwise the
// manager's own level stands. The binary walks its _Rb_tree for this, which
// is what std::map::find does.
int LogManager::getVerboseLevel(Sexy::String const& theTag)
{
    std::map<std::string, int>::iterator aFound = mLevelByTag.find(theTag.c_str());
    if (aFound == mLevelByTag.end())
        return mVerboseLevel;
    return aFound->second;
}

// 3C3CC8. The first log() call installs the listeners SEXY_LOG names, one
// per ';'-separated entry, falling back to "default" when none of them took.
void LogManager::setupDefaultListener()
{
    if (mListener != nullptr || mListenerNesting != 0)
        return;

    mCritSect1.Lock();
    ++mListenerNesting;

    if (mListenerRoot == nullptr)
        mListenerRoot = new FanOutLogListener();

    const char* aSpec = mListenerSpec.c_str();
    const char* aCursor = aSpec;
    for (;;)
    {
        const char* aSeparator = std::strchr(aCursor, ';');
        size_t aLength = (aSeparator != nullptr)
            ? (size_t)(aSeparator - aCursor)
            : std::strlen(aCursor);
        if (aLength != 0)
        {
            Sexy::String aEntry(aCursor, (unsigned)aLength);
            ((FanOutLogListener*)mListenerRoot)->addListener(createListener(aEntry));
        }
        if (aSeparator == nullptr)
            break;
        aCursor = aSeparator + 1;
    }

    if (((FanOutLogListener*)mListenerRoot)->hasListener() == 0)
        ((FanOutLogListener*)mListenerRoot)->addListener(createListener(Sexy::String("default")));

    if (mListener == nullptr)
        mListener = mListenerRoot;

    mCritSect1.Unlock();
}

// 3C4060. An empty tag is replaced by the manager's default before the
// listener sees the line.
int LogManager::log(int theLevel, Sexy::String const& theTag,
                    Sexy::String const& theMessage)
{
    setupDefaultListener();
    if (mListener == nullptr)
        return 0;

    const Sexy::String& aTag = theTag.empty() ? mDefaultTag : theTag;
    return mListener->log(theLevel, aTag, theMessage);
}

// 3C4090.
int LogManager::close()
{
    mCritSect2.Lock();
    if (mListener != nullptr)
    {
        delete mListener;
        mListener = nullptr;
    }
    if (mListenerRoot != nullptr)
    {
        delete mListenerRoot;
        mListenerRoot = nullptr;
    }
    mCritSect2.Unlock();
    return 1;
}

// 3C4554. Reads SEXY_LOG and SEXY_LOG_LEVEL, then installs the defaults the
// tail of the binary's constructor writes.
LogManager::LogManager()
    : mListener(nullptr),
      mListenerRoot(nullptr),
      mListenerNesting(0),
      mVerboseLevel(1),
      mDefaultFormat(7)
{
    const char* aLog = std::getenv("SEXY_LOG");
    if (aLog != nullptr)
        mListenerSpec.assign(aLog);

    const char* aLevel = std::getenv("SEXY_LOG_LEVEL");
    if (aLevel != nullptr && aLevel[0] != '\0')
    {
        // "tag:level" pairs override the default; a bare number replaces it.
        const char* aCursor = aLevel;
        for (;;)
        {
            const char* aSpace = std::strchr(aCursor, ' ');
            size_t aLength = (aSpace != nullptr)
                ? (size_t)(aSpace - aCursor)
                : std::strlen(aCursor);

            const char* aColon = (const char*)std::memchr(aCursor, ':', aLength);
            if (aColon != nullptr)
            {
                std::string aTag(aCursor, aColon - aCursor);
                int aTagLevel = std::atoi(aColon + 1);
                if (aTag != "*")
                    mLevelByTag[aTag] = aTagLevel;
                else
                    mVerboseLevel = aTagLevel;
            }
            else if (aLength != 0)
            {
                mVerboseLevel = std::atoi(aCursor);
            }

            if (aSpace == nullptr)
                break;
            aCursor = aSpace + 1;
        }
    }

    mName.assign("default");
    mDefaultTag.assign("default");
    mDefaultFormat = 7;
    mListenerRoot = new FanOutLogListener();
    mListener = nullptr;
    mListenerNesting = 0;
}

} // namespace Sexy

// ---------------------------------------------------------------------------
// TM::LogManager -- the network transport.
//
// The binary's send thread, socket protocol and local-file spool are the
// bodies from sendLog onward. What the game calls is sendLog, and what that
// does first is queue the line; the thread drains the queue. The queue and
// the file spool are reproduced here. The socket exchange with the metrics
// server is not: this port has no such server, and the binary's own fallback
// for a failed send is the local file.
// ---------------------------------------------------------------------------
namespace TM {

namespace {

// Milliseconds since the epoch, which is what gettimeofday gives the binary.
int NowMillis()
{
    return (int)((long long)::GetTickCount());
}

} // namespace

// 4EF070. Stamps the clock and starts the send thread. The thread body is
// threadSendProc, which the binary passes to Thread::CreateThread.
int LogManager::initialize()
{
    mLastSendTime = NowMillis();
    Thread::Create(this, &LogManager::SendThreadMain, this);
    return 1;
}

// The entry Thread::Create calls. The binary's threadSendProc is a member, so
// the trampoline recovers the receiver from the argument.
void LogManager::SendThreadMain(void* theSelf)
{
    LogManager* aSelf = (LogManager*)theSelf;
    aSelf->threadSendProc(theSelf);
}

// 4EF0D8. Resolves the host the caller named. An unresolvable name leaves
// the result empty, which is what the binary's gethostbyname failure does.
Sexy::String LogManager::getServerIp(Sexy::String const& theHost)
{
    WSADATA aData;
    ::WSAStartup(MAKEWORD(2, 2), &aData);

    Sexy::String aResult;
    struct hostent* aHost = ::gethostbyname(theHost.c_str());
    if (aHost != nullptr && aHost->h_addr_list != nullptr && aHost->h_addr_list[0] != nullptr)
    {
        struct in_addr aAddress;
        std::memcpy(&aAddress, aHost->h_addr_list[0], sizeof(aAddress));
        const char* aText = ::inet_ntoa(aAddress);
        if (aText != nullptr)
            aResult.assign(aText);
    }
    return aResult;
}

// 4EF120.
void LogManager::setServerAddr(Sexy::String const& theAddress)
{
    mServerAddr = theAddress;
}

// 4EF130. A non-empty directory replaces the local one, and the file path
// becomes that directory plus "/trans_log".
void LogManager::setLocalLogPath(Sexy::String const& theDirectory)
{
    if (theDirectory.empty())
        return;

    mLocalLogDir = theDirectory;
    Sexy::String aPath = theDirectory;
    aPath += "/";
    aPath += "trans_log";
    mLocalLogPath = aPath;
}

// 4EF1EC. Closes the socket the caller holds and clears it.
int LogManager::myClose(int& theSocket)
{
    if (theSocket >= 0)
    {
        ::closesocket((SOCKET)theSocket);
        theSocket = -1;
    }
    return 1;
}

// 4EF2A8.
LogManager::LogManager(Sexy::String const& theServer, Sexy::String theDirectory)
    : mSocket(-1),
      mLastSendTime(0),
      mStopRequested(false)
{
    mLocalLogPath.assign("trans_log", 9);
    setLocalLogPath(theDirectory);
    mServerAddr = theServer;
    initialize();
}

// 4EF3F0. The line is stamped, logged locally and queued for the send
// thread. The binary's stringstream preamble is the fixed text below.
int LogManager::sendLog(Sexy::String const& theLine)
{
    ::OutputDebugStringA("push message to vector!\n");

    Sexy::String aEntry = theLine;
    aEntry += mRemoteLogPath;
    aEntry += "|\n";

    mSendMutex.Acquire();
    mRetryQueue.push_back(aEntry.c_str());
    mSendMutex.Release();
    return 1;
}

// 4EF5C0. One line, appended to the local spool.
int LogManager::appendLogToLocalFile(Sexy::String const& theLine)
{
    if (mLocalLogPath.empty())
        return 0;

    std::ofstream aFile(mLocalLogPath.c_str(), std::ios::app | std::ios::binary);
    if (!aFile)
        return 0;
    aFile << theLine.c_str();
    return aFile.good() ? 1 : 0;
}

// 4EF8F0. The binary writes the line to the metrics socket and, on failure,
// to the local spool. This port has no metrics server, so the line takes the
// failure path.
void LogManager::sendLogToServer(Sexy::String const& theLine)
{
    appendLogToLocalFile(theLine);
}

// 4F00C0. Opens the socket to the metrics server. There is no such server
// here, so the attempt reports failure and the caller spools locally.
int LogManager::connectToServer()
{
    return 0;
}

// 4F0A80. Writes every queued line to the local spool.
int LogManager::batchAppendLogToLocalFile(std::list<std::string> const& theLines)
{
    int aWritten = 0;
    for (std::list<std::string>::const_iterator aIt = theLines.begin();
         aIt != theLines.end(); ++aIt)
    {
        Sexy::String aLine(aIt->c_str());
        if (appendLogToLocalFile(aLine) != 0)
            ++aWritten;
    }
    return aWritten;
}

// 4F0E40. Stops the send thread and flushes every queue to the local spool.
int LogManager::destroy()
{
    mStopRequested = true;
    Join();

    batchAppendLogToLocalFile(mSendQueue);
    batchAppendLogToLocalFile(mRetryQueue);
    batchAppendLogToLocalFile(mBackupQueue);
    batchAppendLogToLocalFile(mIncomingQueue);

    mSendQueue.clear();
    mRetryQueue.clear();
    mBackupQueue.clear();
    mIncomingQueue.clear();
    return 1;
}

// 4F0F40. Reads the local spool back into the incoming queue, one line per
// entry, which is what the send thread drains first.
int LogManager::readLogFromLocalFile()
{
    if (mLocalLogPath.empty())
        return 0;

    std::ifstream aFile(mLocalLogPath.c_str(), std::ios::binary);
    if (!aFile)
        return 0;

    std::string aLine;
    int aCount = 0;
    while (std::getline(aFile, aLine))
    {
        mIncomingQueue.push_back(aLine);
        ++aCount;
    }
    return aCount;
}

// 4F1440. Drains the incoming queue, then the retry queue, then the send
// queue, handing each line to the server. The binary polls mStopRequested
// between lines so destroy() can cut the loop short.
int LogManager::threadSendMsg()
{
    readLogFromLocalFile();

    while (!mStopRequested && !mIncomingQueue.empty())
    {
        Sexy::String aLine(mIncomingQueue.front().c_str());
        mIncomingQueue.pop_front();
        sendLogToServer(aLine);
    }

    while (!mStopRequested && !mRetryQueue.empty())
    {
        Sexy::String aLine(mRetryQueue.front().c_str());
        mRetryQueue.pop_front();
        sendLogToServer(aLine);
    }

    while (!mStopRequested && !mSendQueue.empty())
    {
        Sexy::String aLine(mSendQueue.front().c_str());
        mSendQueue.pop_front();
        sendLogToServer(aLine);
    }
    return 1;
}

// 4F1C80. The send thread's body: drain until asked to stop.
int LogManager::threadSendProc(void* theArg)
{
    (void)theArg;
    while (!mStopRequested)
    {
        threadSendMsg();
        Thread::Sleep(1000);
    }
    return 1;
}

} // namespace TM
