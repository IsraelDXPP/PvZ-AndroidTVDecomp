// AttachmentHolder -- reconstructed from libGameMain.so.
//
// The bodies below are the transcribed IDA decompilation, rewritten by hand into
// real C++ against the reconstructed header.  Each body was re-read from the
// binary, because two of them carried transcription defects:
//
//   * AllocAttachment's free-list branch read its base through a bogus
//     this-relative word at byte offset 3184.  The disassembly at
//     0x1F5DBC-0x1F5DD0 is `LDRNE R3,[R0]` (the block pointer) / `MLA R5,R5,R6,R3`
//     (block + 796 * index), so both branches index the block identically.
//   * DisposeHolder's `DataArray<Attachment>::DataArrayDispose` went through the
//     three-word DataArray surrogate in Sexy\ReconHelpers.h, whose fields are
//     {mCount, mCapacity, mData} -- on this seven-word layout that reads
//     mMaxUsedCount as a count and mMaxSize (1024) as a pointer, and the
//     `operator delete` would free address 0x400.  The real teardown is spelled
//     out below from DataArray.h's own DataArrayDispose.
//
// The whole layout is visible in _recon/ida/AttachmentHolder.cpp:
//   ctor             @0x1F56BC  zeroes +0x00..+0x18, stores 1 at +0x14
//   InitializeHolder @0x1F56D0  operator new[](0xC7000); +0x08 = 1024,
//                               +0x14 = 54388 (0xD474), +0x18 = "attachments"
//   AllocAttachment  @0x1F5DAC  record stride 796 (0xC7000 / 1024), +0x0C is the
//                               free-list head, +0x10 is bumped per allocation
//   DisposeHolder    @0x1F5CDC  DataArray<Attachment>::DataArrayDispose(this)
//   dtor             @0x1F5CE8  DisposeHolder() then DataArrayDispose()

#include "AttachmentHolder.h"
#include "Attachment.h"
#include "RawObject.h"
#include <cstring>

// Use the reconstructed object size, including the standard library members.
static const unsigned int kAttachmentCapacity = 1024;

// ==========================================================================
// AttachmentHolder::AttachmentHolder()
// @001F56BC  20 bytes
// ==========================================================================
AttachmentHolder::AttachmentHolder()
{
    mNextKey = 1;
    mBlock = nullptr;
    mMaxUsedCount = 0;
    mMaxSize = 0;
    mFreeListHead = 0;
    mSize = 0;
    mName = nullptr;
}

// ==========================================================================
// AttachmentHolder::InitializeHolder()
// @001F56D0  34 bytes
// ==========================================================================
// The block is raw storage: the records are NOT constructed here, AllocAttachment
// placement-constructs the one it hands out.  The 54388 in mNextKey is the
// binary's own seed (PvZ\TodLib\Common\DataArray.h uses 1001 in its source form;
// the shipped ARM build was compiled with a different constant).
int AttachmentHolder::InitializeHolder()
{
    mBlock = static_cast<Attachment *>(::operator new[](kAttachmentCapacity * sizeof(Attachment)));
    mMaxSize = kAttachmentCapacity;
    mNextKey = 54388;
    mName = "attachments";
    return 1;
}

// ==========================================================================
// AttachmentHolder::DisposeHolder()
// @001F5CDC  10 bytes
// ==========================================================================
// `LDR R3,[R0] / CBZ R3,ret / B DataArray<Attachment>::DataArrayDispose` -- a
// null test and a tail call.  DataArrayDispose is DataArrayFreeAll() followed by
// the block release and the field reset (PvZ\TodLib\Common\DataArray.h:66-77).
void AttachmentHolder::DisposeHolder()
{
    if (mBlock == nullptr)
        return;

    // DataArrayFreeAll: walk the used prefix and destruct every record whose id
    // still carries a key (DataArray.h:89-95, :79-87).
    for (unsigned int i = 0; i < mMaxUsedCount; ++i)
    {
        Attachment *aRecord = &mBlock[i];
        if (aRecord->mAttachmentID & 0xFFFF0000u)
            aRecord->~Attachment();
    }

    ::operator delete[](mBlock);

    mBlock = nullptr;
    mMaxUsedCount = 0;
    mMaxSize = 0;
    mFreeListHead = 0;
    mSize = 0;
    mName = nullptr;
}

// ==========================================================================
// AttachmentHolder::~AttachmentHolder()
// @001F5CE8  18 bytes
// ==========================================================================
// `BL DisposeHolder / MOV R0,R4 / BL DataArray<Attachment>::DataArrayDispose`:
// the second call is the DataArray base destructor, which runs after the derived
// destructor body.  With no base in this reconstruction it is spelled out; the
// null test in DisposeHolder makes the second pass a no-op.
AttachmentHolder::~AttachmentHolder()
{
    DisposeHolder();
    DisposeHolder();
}

// ==========================================================================
// AttachmentHolder::AllocAttachment()
// @001F5DAC  102 bytes
// ==========================================================================
// The inlined DataArray<Attachment>::DataArrayAlloc (DataArray.h:120-140):
//
//     LDR R3,[R0,#4]      mMaxUsedCount
//     LDR R6,[R0,#0xC]    mFreeListHead
//     CMP R6,R3           mFreeListHead == mMaxUsedCount ?
//     ... MLA R5,R5,R6,R3 with R5 = 796 and R3 = [R0] (the block) in BOTH arms
//     NE: R3 = [R5,#0x318] -> mFreeListHead = that record's id
//     EQ: R3 = R6 + 1      -> mMaxUsedCount = R3
//
// Both branches advance the free-list head. Index the typed records so their
// standard library members can use the native compiler's layout.
Attachment *AttachmentHolder::AllocAttachment()
{
    unsigned int anIndex = mFreeListHead;
    Attachment *aRecord;

    if (anIndex == mMaxUsedCount)
    {
        mMaxUsedCount = anIndex + 1;
        mFreeListHead = mMaxUsedCount;
        aRecord = &mBlock[anIndex];
    }
    else
    {
        aRecord = &mBlock[anIndex];
        mFreeListHead = (unsigned int)aRecord->mAttachmentID;
    }

    // This slot is raw storage; construct it once before assigning its id.
    memset(aRecord, 0, sizeof(Attachment));
    ConstructRaw<Attachment>(aRecord);

    unsigned int aKey = mNextKey;
    aRecord->mAttachmentID = (int)(anIndex | (aKey << 16));

    // DataArray.h:133-136 -- the key restarts at 1 once it reaches 0x10000.
    ++aKey;
    mNextKey = (aKey >= 0x10000u) ? 1u : aKey;
    ++mSize;

    return aRecord;
}
