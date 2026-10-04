// Container and stream shims the game still calls by the recovered names.
// Tree walks keep the red-black result. String clones go through Sexy::String.
// Stream helpers read and write the fields the names describe.

#include "SexyTypes.h"
#include "ReconExterns.h"
#include "ReconTypes.h"
#include "SexyString.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <ios>

using Sexy::RbTree::NodeBase;

namespace
{

enum
{
    kGoodbit = 0,
    kBadbit = 1,
    kEofbit = 2,
    kFailbit = 4
};

enum
{
    kTreeRed = 0,
    kTreeBlack = 1
};

struct ListLink
{
    ListLink* mNext;
    ListLink* mPrev;
};

struct IosFields
{
    int mPrefix[2];
    int mWidth;
    int mFlags;
    int mException;
    int mState;
    int mMiddle[22];
    void* mTie;
    unsigned char mFill;
    unsigned char mFillInit;
    unsigned char mFillPad[2];
    void* mStreambuf;
    void* mCtype;
    void* mNumPut;
    void* mNumGet;
};

struct BufferFields
{
    void* mVtable;
    unsigned char* mGetBegin;
    unsigned char* mGet;
    unsigned char* mGetEnd;
    void* mPutBegin;
    unsigned char* mPut;
    unsigned char* mPutEnd;
};

IosFields* AsIos(void* theIos)
{
    return (IosFields*)theIos;
}

BufferFields* AsBuffer(void* theBuffer)
{
    return (BufferFields*)theBuffer;
}

IosFields* StreamIos(void* theStream)
{
    if (theStream == nullptr)
        return nullptr;
    return AsIos(theStream);
}

bool IsCtypeSpace(int theChar)
{
    return theChar == ' ' || theChar == '\t' || theChar == '\n' ||
           theChar == '\v' || theChar == '\f' || theChar == '\r';
}

bool UncaughtException()
{
    return std::uncaught_exceptions() > 0;
}

typedef int (*BufferIntFn)(void* theBuffer);
typedef int (*BufferCharFn)(void* theBuffer, int theChar);
typedef int (*BufferRangeFn)(void* theBuffer, const char* theData, int theCount);

void* SlotFn(void* theObject, int theSlot)
{
    if (theObject == nullptr)
        return nullptr;
    void** aVtable = *(void***)theObject;
    if (aVtable == nullptr)
        return nullptr;
    return aVtable[theSlot];
}

int StreamUnderflow(void* theBuffer)
{
    BufferIntFn aFn = (BufferIntFn)SlotFn(theBuffer, 8);
    if (aFn == nullptr)
        return -1;
    return aFn(theBuffer);
}

int StreamUflow(void* theBuffer)
{
    BufferIntFn aFn = (BufferIntFn)SlotFn(theBuffer, 9);
    if (aFn == nullptr)
        return -1;
    return aFn(theBuffer);
}

int StreamSbumpc(void* theBuffer)
{
    BufferFields* aBuffer = AsBuffer(theBuffer);
    if (aBuffer == nullptr)
        return -1;
    if (aBuffer->mGet != nullptr && aBuffer->mGet < aBuffer->mGetEnd)
    {
        unsigned char aChar = *aBuffer->mGet;
        ++aBuffer->mGet;
        return aChar;
    }
    return StreamUflow(theBuffer);
}

int StreamSgetc(void* theBuffer)
{
    BufferFields* aBuffer = AsBuffer(theBuffer);
    if (aBuffer == nullptr)
        return -1;
    if (aBuffer->mGet != nullptr && aBuffer->mGet < aBuffer->mGetEnd)
        return *aBuffer->mGet;
    return StreamUnderflow(theBuffer);
}

int StreamSputc(void* theBuffer, unsigned char theChar)
{
    BufferFields* aBuffer = AsBuffer(theBuffer);
    if (aBuffer == nullptr)
        return -1;
    if (aBuffer->mPut != nullptr && aBuffer->mPut < aBuffer->mPutEnd)
    {
        *aBuffer->mPut = theChar;
        ++aBuffer->mPut;
        return theChar;
    }
    BufferCharFn aOverflow = (BufferCharFn)SlotFn(theBuffer, 12);
    if (aOverflow == nullptr)
        return -1;
    return aOverflow(theBuffer, theChar);
}

bool OutputSentry(IosFields* theIos)
{
    if (theIos == nullptr)
        return false;

    if (theIos->mTie != nullptr && theIos->mState == 0)
        OStreamFlush(theIos->mTie);

    if (theIos->mState != 0)
    {
        IosSetState(theIos, (unsigned int)theIos->mState | kFailbit);
        return false;
    }
    return true;
}

bool InputSentry(IosFields* theIos, bool theSkipWs)
{
    if (theIos == nullptr || theIos->mState != 0)
        return false;

    if (theIos->mStreambuf == nullptr)
    {
        IosSetState(theIos, kBadbit);
        return false;
    }
    if (!theSkipWs)
        return true;

    for (;;)
    {
        int aChar = StreamSgetc(theIos->mStreambuf);
        if (aChar == -1)
        {
            IosSetState(theIos, kEofbit | kFailbit);
            return false;
        }
        if (!IsCtypeSpace(aChar))
            return true;
        StreamSbumpc(theIos->mStreambuf);
    }
}

void OutputEpilogue(IosFields* theIos)
{
    if (theIos == nullptr)
        return;
    if (((unsigned int)theIos->mFlags & 0x2000u) == 0)
        return;
    if (UncaughtException())
        return;

    BufferIntFn aSync = (BufferIntFn)SlotFn(theIos->mStreambuf, 6);
    if (aSync == nullptr)
        return;
    if (aSync(theIos->mStreambuf) == -1)
        IosSetState(theIos, (unsigned int)theIos->mState | kBadbit);
}

char StreamFill(IosFields* theIos)
{
    if (theIos->mFillInit != 0)
        return (char)theIos->mFill;

    char aFill = ' ';
    unsigned char* aFacet = (unsigned char*)theIos->mCtype;
    if (aFacet != nullptr)
    {
        if (aFacet[28] != 0)
        {
            aFill = (char)aFacet[61];
        }
        else
        {
            InitCtypeWiden(aFacet);
            typedef int (*WidenFn)(void*, int);
            WidenFn aWiden = (WidenFn)SlotFn(aFacet, 6);
            if (aWiden != nullptr)
                aFill = (char)aWiden(aFacet, ' ');
        }
    }
    theIos->mFill = (unsigned char)aFill;
    theIos->mFillInit = 1;
    return aFill;
}

void StreamFillN(IosFields* theIos, void* theBuffer, char theChar, int theCount)
{
    if (theBuffer == nullptr)
    {
        IosSetState(theIos, (unsigned int)theIos->mState | kBadbit);
        return;
    }
    for (int i = 0; i < theCount; ++i)
    {
        if (StreamSputc(theBuffer, (unsigned char)theChar) == -1)
        {
            IosSetState(theIos, (unsigned int)theIos->mState | kBadbit);
            return;
        }
    }
}

void StreamPutN(IosFields* theIos, void* theBuffer, const char* theData, int theCount)
{
    if (theBuffer == nullptr)
    {
        IosSetState(theIos, (unsigned int)theIos->mState | kBadbit);
        return;
    }
    BufferRangeFn aPut = (BufferRangeFn)SlotFn(theBuffer, 12);
    if (aPut == nullptr || aPut(theBuffer, theData, theCount) != theCount)
        IosSetState(theIos, (unsigned int)theIos->mState | kBadbit);
}

void* ExtractString(void* theStream, Sexy::String* theString,
                    bool theSkipWs, bool theHasDelim, int theDelim)
{
    if (theStream == nullptr || theString == nullptr)
        return theStream;

    IosFields* aIos = StreamIos(theStream);
    if (aIos == nullptr)
        return theStream;

    unsigned int aError = kGoodbit;
    if (InputSentry(aIos, theSkipWs))
    {
        int aMax = aIos->mWidth;
        if (aMax <= 0)
            aMax = 1073741820;

        theString->clear();
        void* aBuffer = aIos->mStreambuf;
        int aCount = 0;
        bool aDone = false;
        while (!aDone && aCount < aMax)
        {
            int aChar = StreamSbumpc(aBuffer);
            if (aChar == -1)
            {
                aError = kEofbit;
                aDone = true;
            }
            else if (theHasDelim && aChar == theDelim)
            {
                aDone = true;
            }
            else if (!theHasDelim && IsCtypeSpace(aChar))
            {
                aDone = true;
            }
            else
            {
                char aByte = (char)aChar;
                theString->reserve((unsigned int)(aCount + 1));
                theString->Append(&aByte, 1);
                ++aCount;
            }
        }
        if (aCount == 0)
            aError |= kFailbit;
    }

    aIos->mWidth = 0;
    if (aError != kGoodbit)
        IosSetState(aIos, aError | (unsigned int)aIos->mState);
    return theStream;
}

struct ExitEntry
{
    void (*mFunc)(void*);
    void* mArg;
};

const int kMaxExitEntries = 512;
ExitEntry gExitEntries[kMaxExitEntries];
int gExitCount = 0;
bool gExitHooked = false;

void RunExitEntries()
{
    for (int i = gExitCount - 1; i >= 0; --i)
    {
        if (gExitEntries[i].mFunc != nullptr)
            gExitEntries[i].mFunc(gExitEntries[i].mArg);
    }
}

NodeBase* TreeIncrement(NodeBase* theNode)
{
    if (theNode->mRight != nullptr)
    {
        NodeBase* aRight = theNode->mRight;
        while (aRight->mLeft != nullptr)
            aRight = aRight->mLeft;
        return aRight;
    }

    NodeBase* aUp = theNode->mParent;
    if (theNode != aUp->mRight)
        return theNode->mParent;

    for (;;)
    {
        NodeBase* aGrand = aUp->mParent;
        if (aGrand->mRight != aUp)
        {
            if (aGrand == aUp->mRight)
                return aUp;
            return aGrand;
        }
        aUp = aUp->mParent;
    }
}

NodeBase* Leftmost(NodeBase* theNode)
{
    while (theNode != nullptr && theNode->mLeft != nullptr)
        theNode = theNode->mLeft;
    return theNode;
}

NodeBase* Rightmost(NodeBase* theNode)
{
    while (theNode != nullptr && theNode->mRight != nullptr)
        theNode = theNode->mRight;
    return theNode;
}

void RelinkChild(NodeBase* theHeader, NodeBase* theNode, NodeBase* theChild)
{
    if (theHeader->mParent == theNode)
        theHeader->mParent = theChild;
    else if (theNode->mParent->mLeft == theNode)
        theNode->mParent->mLeft = theChild;
    else
        theNode->mParent->mRight = theChild;
}

void FixLeftmost(NodeBase* theHeader, NodeBase* theNode, NodeBase* theChild)
{
    if (theHeader->mLeft != theNode)
        return;
    if (theNode->mRight != nullptr)
        theHeader->mLeft = Leftmost(theChild);
    else
        theHeader->mLeft = theNode->mParent;
}

void FixRightmost(NodeBase* theHeader, NodeBase* theNode, NodeBase* theChild)
{
    if (theHeader->mRight != theNode)
        return;
    if (theNode->mLeft != nullptr)
        theHeader->mRight = Rightmost(theChild);
    else
        theHeader->mRight = theNode->mParent;
}

struct CharRep
{
    unsigned int mLength;
    unsigned int mCapacity;
    int mRefCount;
    char mChars[1];
};

char* CloneRep(const char* theOldRep, int theExtra)
{
    void* aOldStorage = (void*)theOldRep;
    CharRep* aOld = static_cast<CharRep*>(aOldStorage);
    if (aOld == nullptr)
        return nullptr;

    unsigned int aLength = aOld->mLength;
    unsigned int aWant = aLength + (unsigned int)theExtra;
    if (aWant < aLength)
        aWant = aLength;

    Sexy::String aClone(aOld->mChars, aLength);
    if (aWant > aClone.capacity())
        aClone.reserve(aWant);
    char* aData = aClone.data();
    aClone.mData = Sexy::gEmptyStringData;
    return aData;
}

} // namespace

void* IosSetState(void* theStream, int theState)
{
    IosFields* aIos = AsIos(theStream);
    if (aIos == nullptr)
        return theStream;

    unsigned int aState = (unsigned int)theState;
    if (aIos->mStreambuf == nullptr)
        aState |= kBadbit;
    aIos->mState = (int)aState;
    if ((aState & (unsigned int)aIos->mException) != 0)
        throw std::ios_base::failure("basic_ios::clear");
    return theStream;
}

void* ListHook(void* theNode, void* thePos)
{
    ListLink* aNode = (ListLink*)theNode;
    ListLink* aPos = (ListLink*)thePos;
    if (aNode == nullptr || aPos == nullptr)
        return theNode;

    aNode->mNext = aPos;
    aNode->mPrev = aPos->mPrev;
    if (aPos->mPrev != nullptr)
        aPos->mPrev->mNext = aNode;
    aPos->mPrev = aNode;
    return theNode;
}

void* ListUnhook(void* theNode)
{
    ListLink* aNode = (ListLink*)theNode;
    if (aNode == nullptr)
        return theNode;

    if (aNode->mNext != nullptr)
        aNode->mNext->mPrev = aNode->mPrev;
    if (aNode->mPrev != nullptr)
        aNode->mPrev->mNext = aNode->mNext;
    return theNode;
}

int RegisterAtExit(void* theArg, void (*theFunc)(void*), void* theDso)
{
    (void)theDso;
    if (theFunc == nullptr)
        return 0;

    if (gExitCount < kMaxExitEntries)
    {
        gExitEntries[gExitCount].mFunc = theFunc;
        gExitEntries[gExitCount].mArg = theArg;
        ++gExitCount;
    }
    if (!gExitHooked)
    {
        gExitHooked = true;
        std::atexit(RunExitEntries);
    }
    return 0;
}

char* CloneBufferStringRep(const char* theRep, int theExtra)
{
    return CloneRep(theRep, theExtra);
}

char* CloneImageStringRep(const char* theRep, int theExtra)
{
    return CloneRep(theRep, theExtra);
}

char* CloneWideStringRep(const char* theRep, int theExtra)
{
    return CloneRep(theRep, theExtra);
}

void ReleaseClonedStringRep(unsigned int* theRep)
{
    void* aStorage = theRep;
    CharRep* aRep = static_cast<CharRep*>(aStorage);
    if (aRep == nullptr || aRep == (CharRep*)&Sexy::gEmptyStringRep || aRep->mRefCount < 0)
        return;

    Sexy::String aOwned(aRep->mChars);
    aOwned.clear();
}

void* RbTreeNext(void* theNode)
{
    if (theNode == nullptr)
        return theNode;
    return TreeIncrement((NodeBase*)theNode);
}

void* RbTreeNextConst(void* theNode)
{
    return RbTreeNext(theNode);
}

void* RbTreePrev(void* theNode)
{
    if (theNode == nullptr)
        return theNode;
    return Sexy::RbTree::_M_decrement((NodeBase*)theNode);
}

void* RbTreePrevConst(void* theNode)
{
    return RbTreePrev(theNode);
}

int RbTreeInsert(int theInsertLeft, void* theNode, void* theParent, void* theHeader)
{
    NodeBase* aNode = (NodeBase*)theNode;
    NodeBase* aParent = (NodeBase*)theParent;
    NodeBase* aHeader = (NodeBase*)theHeader;
    if (aNode == nullptr || aParent == nullptr || aHeader == nullptr)
        return theInsertLeft;

    Sexy::RbTree::_Rb_tree_insert_and_rebalance(theInsertLeft != 0, aNode, aParent, *aHeader);
    return theInsertLeft;
}

void* RbTreeErase(void* theNode, void* theHeader)
{
    NodeBase* aNode = (NodeBase*)theNode;
    NodeBase* aHeader = (NodeBase*)theHeader;
    if (aNode == nullptr || aHeader == nullptr)
        return theNode;

    NodeBase* aChild = nullptr;
    NodeBase* aParent = nullptr;
    int aRemovedColor = aNode->mColor;

    if (aNode->mLeft == nullptr || aNode->mRight == nullptr)
    {
        aChild = (aNode->mLeft == nullptr) ? aNode->mRight : aNode->mLeft;
        aParent = aNode->mParent;
        if (aChild != nullptr)
            aChild->mParent = aParent;
        RelinkChild(aHeader, aNode, aChild);
        FixLeftmost(aHeader, aNode, aChild);
        FixRightmost(aHeader, aNode, aChild);
    }
    else
    {
        NodeBase* aSuccessor = aNode->mRight;
        while (aSuccessor->mLeft != nullptr)
            aSuccessor = aSuccessor->mLeft;

        aChild = aSuccessor->mRight;
        aNode->mLeft->mParent = aSuccessor;
        aSuccessor->mLeft = aNode->mLeft;
        if (aSuccessor != aNode->mRight)
        {
            aParent = aSuccessor->mParent;
            if (aChild != nullptr)
                aChild->mParent = aSuccessor->mParent;
            aSuccessor->mParent->mLeft = aChild;
            aSuccessor->mRight = aNode->mRight;
            aNode->mRight->mParent = aSuccessor;
        }
        else
        {
            aParent = aSuccessor;
        }

        RelinkChild(aHeader, aNode, aSuccessor);
        aSuccessor->mParent = aNode->mParent;
        aRemovedColor = aSuccessor->mColor;
        aSuccessor->mColor = aNode->mColor;
    }

    if (aRemovedColor != kTreeBlack)
        return theNode;

    NodeBase*& aRoot = aHeader->mParent;
    while (aChild != aRoot && (aChild == nullptr || aChild->mColor == kTreeBlack))
    {
        bool aOnLeft = (aChild == aParent->mLeft);
        NodeBase* aSibling = aOnLeft ? aParent->mRight : aParent->mLeft;
        if (aSibling == nullptr)
            break;

        if (aSibling->mColor == kTreeRed)
        {
            aSibling->mColor = kTreeBlack;
            aParent->mColor = kTreeRed;
            if (aOnLeft)
                Sexy::RbTree::_Rb_tree_rotate_left(aParent, aRoot);
            else
                Sexy::RbTree::_Rb_tree_rotate_right(aParent, aRoot);
            aSibling = aOnLeft ? aParent->mRight : aParent->mLeft;
        }
        if (aSibling == nullptr)
            break;

        NodeBase* aNear = aOnLeft ? aSibling->mLeft : aSibling->mRight;
        NodeBase* aFar = aOnLeft ? aSibling->mRight : aSibling->mLeft;
        bool aNearBlack = (aNear == nullptr || aNear->mColor == kTreeBlack);
        bool aFarBlack = (aFar == nullptr || aFar->mColor == kTreeBlack);
        if (aNearBlack && aFarBlack)
        {
            aSibling->mColor = kTreeRed;
            aChild = aParent;
            aParent = aParent->mParent;
            continue;
        }

        if (aFarBlack)
        {
            if (aNear != nullptr)
                aNear->mColor = kTreeBlack;
            aSibling->mColor = kTreeRed;
            if (aOnLeft)
                Sexy::RbTree::_Rb_tree_rotate_right(aSibling, aRoot);
            else
                Sexy::RbTree::_Rb_tree_rotate_left(aSibling, aRoot);
            aSibling = aOnLeft ? aParent->mRight : aParent->mLeft;
            aFar = (aSibling != nullptr) ? (aOnLeft ? aSibling->mRight : aSibling->mLeft) : nullptr;
        }
        if (aSibling == nullptr)
            break;

        aSibling->mColor = aParent->mColor;
        aParent->mColor = kTreeBlack;
        if (aFar != nullptr)
            aFar->mColor = kTreeBlack;
        if (aOnLeft)
            Sexy::RbTree::_Rb_tree_rotate_left(aParent, aRoot);
        else
            Sexy::RbTree::_Rb_tree_rotate_right(aParent, aRoot);
        break;
    }

    if (aChild != nullptr)
        aChild->mColor = kTreeBlack;
    return theNode;
}

int InitCtypeWiden(void* theFacet)
{
    unsigned char* aFacet = (unsigned char*)theFacet;
    if (aFacet == nullptr)
        return 0;

    unsigned char aTable[256];
    for (int i = 0; i != 256; ++i)
        aTable[i] = (unsigned char)i;

    typedef void (*WidenRangeFn)(void*, unsigned char*, unsigned char*, unsigned char*);
    WidenRangeFn aWiden = (WidenRangeFn)SlotFn(aFacet, 7);
    if (aWiden != nullptr)
        aWiden(aFacet, aTable, aTable + 256, aFacet + 29);

    aFacet[28] = 1;
    int aDifference = memcmp(aTable, aFacet + 29, 256);
    if (aDifference != 0)
        aFacet[28] = 2;
    return aDifference;
}

void* FileBufFinish(void* theFileBuf)
{
    void** aSlot = (void**)theFileBuf;
    if (aSlot == nullptr || aSlot[0] == nullptr)
        return theFileBuf;
    if (*((unsigned char*)aSlot + 4) == 0)
        return theFileBuf;

    errno = 0;
    int aResult = 0;
    do
    {
        aResult = fclose((FILE*)aSlot[0]);
    }
    while (aResult != 0 && errno == EINTR);
    aSlot[0] = nullptr;
    return theFileBuf;
}

void* DestroyLocale(void* theLocale)
{
    void** aLocale = (void**)theLocale;
    if (aLocale == nullptr || aLocale[0] == nullptr)
        return theLocale;

    delete (char*)aLocale[0];
    aLocale[0] = nullptr;
    return theLocale;
}

void* IosInit(void* theStream, void* theStreambuf)
{
    IosFields* aIos = AsIos(theStream);
    if (aIos == nullptr)
        return theStream;

    aIos->mStreambuf = theStreambuf;
    aIos->mState = (theStreambuf == nullptr) ? kBadbit : kGoodbit;
    aIos->mTie = nullptr;
    aIos->mException = 0;
    aIos->mFill = 0;
    aIos->mFillInit = 0;
    aIos->mCtype = nullptr;
    aIos->mNumPut = nullptr;
    aIos->mNumGet = nullptr;
    return theStream;
}

void* OStreamFlush(void* theStream)
{
    IosFields* aIos = StreamIos(theStream);
    if (aIos == nullptr)
        return theStream;

    BufferIntFn aSync = (BufferIntFn)SlotFn(aIos->mStreambuf, 6);
    if (aSync != nullptr && aSync(aIos->mStreambuf) == -1)
        IosSetState(aIos, (unsigned int)aIos->mState | kBadbit);
    return theStream;
}

void* OStreamPut(void* theStream, char theFill)
{
    IosFields* aIos = StreamIos(theStream);
    if (aIos == nullptr)
        return theStream;

    if (OutputSentry(aIos))
    {
        if (StreamSputc(aIos->mStreambuf, (unsigned char)theFill) == -1)
            IosSetState(aIos, (unsigned int)aIos->mState | kBadbit);
    }
    OutputEpilogue(aIos);
    return theStream;
}

void* OStreamWrite(void* theStream, int theValue, int theBase)
{
    IosFields* aIos = StreamIos(theStream);
    if (aIos == nullptr)
        return theStream;

    if (OutputSentry(aIos))
        StreamPutN(aIos, aIos->mStreambuf, (const char*)theValue, theBase);
    OutputEpilogue(aIos);
    return theStream;
}

int* OStreamInsert(int* theStream, int theValue, int theBase)
{
    IosFields* aIos = StreamIos(theStream);
    if (aIos == nullptr)
        return theStream;

    if (!OutputSentry(aIos))
    {
        OutputEpilogue(aIos);
        return theStream;
    }

    int aCount = theBase;
    const char* aText = (const char*)theValue;
    if (aIos->mWidth <= aCount)
    {
        StreamPutN(aIos, aIos->mStreambuf, aText, aCount);
    }
    else
    {
        int aPad = aIos->mWidth - aCount;
        bool aLeft = ((unsigned int)aIos->mFlags & 0xB0u) == 0x20u;
        char aFill = StreamFill(aIos);
        if (!aLeft)
            StreamFillN(aIos, aIos->mStreambuf, aFill, aPad);
        if (aIos->mState == 0)
            StreamPutN(aIos, aIos->mStreambuf, aText, aCount);
        if (aLeft && aIos->mState == 0)
            StreamFillN(aIos, aIos->mStreambuf, aFill, aPad);
    }

    aIos->mWidth = 0;
    OutputEpilogue(aIos);
    return theStream;
}

void* OStreamWriteNum(void* theStream, int theValue)
{
    return OStreamWriteInt(theStream, theValue);
}

void* OStreamWriteInt(void* theStream, int theValue)
{
    IosFields* aIos = StreamIos(theStream);
    if (aIos == nullptr)
        return theStream;

    if (OutputSentry(aIos))
    {
        char aDigits[16];
        int aCount = sprintf(aDigits, "%d", theValue);
        if (aCount < 0)
            IosSetState(aIos, (unsigned int)aIos->mState | kBadbit);
        else
            StreamPutN(aIos, aIos->mStreambuf, aDigits, aCount);
    }
    OutputEpilogue(aIos);
    return theStream;
}

void* StreamExtractString(void* theStream, Sexy::String* theString)
{
    return ExtractString(theStream, theString, true, false, 0);
}

void* StreamGetLine(void* theStream, Sexy::String* theString, int theDelimiter)
{
    return ExtractString(theStream, theString, false, true, theDelimiter);
}
