#include <SDL3_net/SDL_net.h>
// LogManager -- reconstructed from libGameMain.so.
//
// This header carries TWO classes that the transcription had merged into one
// because they share a name. They are not related and do not share a layout:
//
//   Sexy::LogManager  0x40 bytes, the framework's listener registry.
//                     _ZN4Sexy10LogManagerC2Ev @0x3C4554 is its only
//                     constructor and it writes exactly +0x08, +0x0C, +0x10,
//                     +0x14, +0x18, +0x1C, +0x20, +0x24, +0x2C, +0x30, +0x34,
//                     +0x38 and +0x3C. NOT polymorphic: no vtable and no RTTI
//                     symbol exists for it, so nothing here is `virtual`.
//
//   TM::LogManager   0x48 bytes, the network log transport. Unit 705 of
//                     LawnApp (`MLogManager`) holds one: LawnApp::HardwareInit
//                     @0x2C26C8 does `operator new(0x48)` +
//                     `TM::LogManager::LogManager(String const&, String)`.
//                     It derives from TM::Thread (its constructor calls
//                     TM::Thread::Thread::Thread(this) and its destructor calls
//                     ~Thread(this) last), and it owns two TM::Thread::Mutex.
//
// The merge was possible only while every member was an opaque integer, which
// is why the fields below are now named individually. Everything left is
// derived from the two constructors, the two destructors and the IDA bodies of
// the members themselves.

#pragma once

#include "ReconTypes.h"
// CritSect is a member, so its complete type is needed here.
#include "CritSect.h"
// TM::LogManager derives from TM::Thread and holds two TM::Thread::Mutex.
#include "Thread.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <atomic>

namespace Sexy {

// libstdc++'s std::list: one node base, two pointers. Same stand-in as
// SexyAppBase.h's ListStorage, spelled separately because this header cannot
// include SexyAppBase.h (that header includes this one).
struct LogListStorage
{
    void *mNext;    // +0x00
    void *mPrev;    // +0x04
};

class LogManager
{
public:
    // The binary keeps two one-word CritSects at the head of the object; every
    // lock in the class takes the second one (`this + 4`).
    CritSect mCritSect1;            // +0x00
    CritSect mCritSect2;            // +0x04
    LogListener *mListener;         // +0x08  the active listener
    // The fan-out listener. Its concrete type lives in LogManager.cpp; the
    // header only ever holds the pointer.
    LogListener *mListenerRoot;     // +0x0C
    int mListenerNesting;           // +0x10  bumped while a default listener is installed
    int mVerboseLevel;              // +0x14
    Sexy::String mDefaultTag;       // +0x18  getDefaultTag/setDefaultTag
    int mDefaultFormat;             // +0x1C
    Sexy::String mListenerSpec;     // +0x20  the SEXY_LOG value, Split() on ';'
    Sexy::String mName;             // +0x24  getName/setName
    int mUnusedWord;                // never written and never read
    // The per-tag level table. The binary keeps it as an _Rb_tree of
    // (name, level) pairs and walks it by hand in getVerboseLevel(String);
    // a map is that table.
    std::map<std::string, int> mLevelByTag;
    // Size 0x40 in the binary. The map above is the standard container the
    // tree asks for, so this object is larger than the binary's; nothing in
    // the port places one by offset.

    // Statics the binary keeps in this class's own storage; the transcribed
    // bodies name them through the qualified spelling.
    // The binary's msLogManager is the object itself, 0x80 bytes of bss at
    // 0x762A60, not a pointer to one. getInstance returns its address.
    static Sexy::LogManager msLogManager;

    static void * getInstance();
    LogListener * createListener(Sexy::String const& theSpec);
    int setListener(Sexy::LogListener *theListener);
    int resetListener();
    int resetToPlatformListener();
    int release();
    int silent();
    int setVerboseLevel(int theLevel);
    int getVerboseLevel();
    // Both are sret in the binary: r0 is the return slot and the receiver
    // arrives in r1 (_ZN4Sexy10LogManager13getDefaultTagEv @0x3C3C24 copies
    // `this + 0x18`, getName @0x3C3C3C copies `this + 0x24`).
    Sexy::String getDefaultTag();
    void setDefaultTag(Sexy::String const& theTag);
    Sexy::String getName();
    void setName(Sexy::String const& theName);
    int getDefaultFormat();
    int setDefaultFormat(int theFormat);
    int getVerboseLevel(Sexy::String const& theTag);
    void setupDefaultListener();
    int log(int theLevel, Sexy::String const& theTag, Sexy::String const& theMessage);
    int close();
    LogManager();

};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::LogManager;

namespace TM {

class LogManager : public Thread
{
public:
    // The four queues are std::list<std::string> in the binary, walked through
    // a sentinel node. The standard list is that container; the port never
    // places one of these objects by offset, so the larger node is harmless.
    std::list<std::string> mSendQueue;     // entries waiting for the send thread
    std::list<std::string> mRetryQueue;    // entries the server rejected
    std::list<std::string> mBackupQueue;   // entries to hand to the local file
    std::list<std::string> mIncomingQueue; // entries read back from the file
    NET_StreamSocket* mSocket;
    int          mLastSendTime;     // milliseconds, from gettimeofday
    Sexy::String mServerAddr;       // host or host:port
    std::atomic<bool> mStopRequested; // set by destroy(), polled by both threads
    Sexy::String mLocalLogDir;      // the directory setLocalLogPath was given
    Sexy::String mLocalLogPath;     // mLocalLogDir + "/" + "trans_log"
    Sexy::String mRemoteLogPath;    // the path the server reported back
    Thread::Mutex mSendMutex;
    Thread::Mutex mRecvMutex;
    // The binary's object is 0x48, from the operator new(0x48) in
    // LawnApp::HardwareInit @0x2C26C8. The lists above are the standard
    // containers the tree asks for, so this object is larger.

    LogManager();
    LogManager(Sexy::String const& theServer, Sexy::String thePath);
    ~LogManager();

    int initialize();
    // sret, like getDefaultTag above: the destination arrives in r0 and the
    // receiver in r1 (_ZN2TM10LogManager11getServerIpERKSs @0x4EF0D8 fills the
    // buffer r0 points at, not the object).
    Sexy::String getServerIp(Sexy::String const& theHost);
    void setServerAddr(Sexy::String const& theAddr);
    void setLocalLogPath(Sexy::String const& thePath);
    int myClose(NET_StreamSocket*& theSocket);
    int sendLog(Sexy::String const& theLine);
    int appendLogToLocalFile(Sexy::String const& theLine);
    void sendLogToServer(Sexy::String const& theLine);
    int connectToServer();
    int batchAppendLogToLocalFile(std::list<std::string> const& theLines);
    int destroy();
    int readLogFromLocalFile();
    int threadSendMsg();
    int threadSendProc(void *theSelf);

    // The plain function Thread::Create starts. It recovers the receiver from
    // its argument and runs threadSendProc.
    static void SendThreadMain(void* theSelf);
};

} // namespace TM
