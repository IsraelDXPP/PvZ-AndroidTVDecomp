// AttachmentHolder -- reconstructed from libGameMain.so.
//
// Not polymorphic: there is no vtable symbol and no RTTI entry for it, so the
// class declares no virtual function and the compiler contributes no vptr.
//
// The whole layout is visible in _recon/ida/AttachmentHolder.cpp:
//   ctor             @0x1F56BC  zeroes +0x00..+0x18, stores 1 at +0x14
//   InitializeHolder @0x1F56D0  operator new[](0xC7000); +0x08 = 1024,
//                               +0x14 = 54388 (0xD474), +0x18 = "attachments"
//   AllocAttachment  @0x1F5DAC  element stride 796 (0xC7000 / 1024), +0x0C is
//                               the freelist head, +0x10 is bumped per alloc
//   DisposeHolder    @0x1F5CDC  DataArray<Attachment>::DataArrayDispose(this)
//   dtor             @0x1F5CE8  DisposeHolder() then DataArrayDispose()
// Size 0x1C; the transcribed class body, an opaque 0xC80-byte block, was sized
// from a mis-attributed `this` offset and was 180 times too big.
//
// The seven words are `DataArray<Attachment>`'s, field for field (authoritative
// definition: PvZ\TodLib\Common\DataArray.h:44-50).  The binary's own symbol for
// the teardown is `DataArray<Attachment>::DataArrayDispose` @0x1F5C64 and the
// destructor @0x1F5CE8 calls DisposeHolder first and DataArrayDispose second --
// the second call is the DataArray base destructor, so this class really does
// derive from the template.  The copy of the template in this tree
// (Sexy\ReconHelpers.h:130) is a three-word surrogate whose field meanings do not
// match (`mCount/mCapacity/mData`), so the base cannot be spelled as that type
// without changing every DataArray user; the seven members are expanded here and
// named after the authoritative ones instead.
//
// TODO(recon): make this `class AttachmentHolder : public DataArray<Attachment>`
// once DataArray<T> is the real seven-word template with DataArrayItem<T>.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_ATTACHMENTHOLDER_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_ATTACHMENTHOLDER_H

#include "ReconTypes.h"
// Attachment is only needed by the methods; the include is kept so that other
// files' transitive dependencies do not change.
#include "Attachment.h"

class AttachmentHolder
{
public:
    Attachment *mBlock; // +0x00  block of 1024 (0x400) records of 796 bytes
    unsigned int mMaxUsedCount; // +0x04  highest used index; AllocAttachment bumps it
    unsigned int mMaxSize; // +0x08  1024, set by InitializeHolder
    unsigned int mFreeListHead; // +0x0C  index of the first free record
    unsigned int mSize; // +0x10  live attachment count
    unsigned int mNextKey; // +0x14  1 at construction, 54388 after InitializeHolder
    const char *mName; // +0x18  "attachments"
    // Size 0x1C.

    AttachmentHolder();
    ~AttachmentHolder();

    // Returns 1 after the block is allocated. Callers only check that init ran.
    int InitializeHolder();
    void DisposeHolder();
    Attachment *AllocAttachment();
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_ATTACHMENTHOLDER_H
