#pragma once

#include <cstring>

// The pool the per-track arrays, particle-list nodes and cached-image nodes
// are carved out of. Five words: a free list threaded through the first word
// of each free item, a block list whose first word links the previous block,
// then the grow count, the live count and the item size.
class TodAllocator
{
public:
    void *mFreeList;   // +0x00
    void *mBlockList;  // +0x04
    int   mGrowCount;  // +0x08
    int   mTotalItems; // +0x0C
    int   mItemSize;   // +0x10
};


inline void *TodAllocatorReadLink(void *theItem)
{
    void *aLink = nullptr;
    std::memcpy(&aLink, theItem, sizeof(aLink));
    return aLink;
}

inline void TodAllocatorWriteLink(void *theItem, void *theLink)
{
    std::memcpy(theItem, &theLink, sizeof(theLink));
}

// One grow, the body inlined in FindGlobalAllocator and in Alloc. The new
// block's first word keeps the previous block, and every item in the block is
// pushed onto the free list, leaving the last item at the head.
inline void TodAllocatorGrow(TodAllocator *theAllocator)
{
    char *aBlock = new char[theAllocator->mGrowCount * theAllocator->mItemSize + 4];
    TodAllocatorWriteLink(aBlock, theAllocator->mBlockList);
    theAllocator->mBlockList = aBlock;

    void *aFree = theAllocator->mFreeList;
    if (theAllocator->mGrowCount > 0)
    {
        char *anItem = aBlock + 4;
        int anIndex = 0;
        do
        {
            ++anIndex;
            TodAllocatorWriteLink(anItem, aFree);
            aFree = anItem;
            anItem += theAllocator->mItemSize;
        }
        while (anIndex != theAllocator->mGrowCount);
        aFree = aBlock + theAllocator->mItemSize * (anIndex - 1) + 4;
    }
    theAllocator->mFreeList = aFree;
}

// Pop one item. An empty free list grows one block first. The live count is
// bumped before the pop, matching the inlined alloc at ReanimationInitialize.
inline void *TodAllocatorAlloc(TodAllocator *theAllocator)
{
    void *aNode = theAllocator->mFreeList;
    ++theAllocator->mTotalItems;
    if (aNode == nullptr)
    {
        TodAllocatorGrow(theAllocator);
        aNode = theAllocator->mFreeList;
    }
    theAllocator->mFreeList = TodAllocatorReadLink(aNode);
    return aNode;
}
