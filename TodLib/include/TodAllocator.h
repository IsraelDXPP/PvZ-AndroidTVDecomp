#ifndef LAWNPROJECTTV_TODLIB_INCLUDE_TODALLOCATOR_H
#define LAWNPROJECTTV_TODLIB_INCLUDE_TODALLOCATOR_H

#include <cstring>
#include <cstddef>
#include <limits>
#include <new>

// The pool the per-track arrays, particle-list nodes and cached-image nodes
// are carved out of. Five words: a free list threaded through the first word
// of each free item, a block list whose first word links the previous block,
// then the grow count, the live count and the item size.
class TodAllocator
{
public:
    void *mFreeList; // +0x00
    void *mBlockList; // +0x04
    int mGrowCount; // +0x08
    int mTotalItems; // +0x0C
    size_t mItemSize;
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
    const size_t aAlignment = alignof(std::max_align_t);
    const size_t aHeaderSize = aAlignment;
    const size_t aMinimumSize = theAllocator->mItemSize < sizeof(void *) ? sizeof(void *) : theAllocator->mItemSize;
    if (aMinimumSize > (std::numeric_limits<size_t>::max)() - aAlignment + 1)
        throw std::bad_alloc();
    const size_t anItemStride = (aMinimumSize + aAlignment - 1) / aAlignment * aAlignment;
    const size_t aGrowCount = theAllocator->mGrowCount > 0 ? theAllocator->mGrowCount : 0;
    if (aGrowCount == 0 || aGrowCount > ((std::numeric_limits<size_t>::max)() - aHeaderSize) / anItemStride)
        throw std::bad_alloc();
    char *aBlock = new char[aGrowCount * anItemStride + aHeaderSize];
    TodAllocatorWriteLink(aBlock, theAllocator->mBlockList);
    theAllocator->mBlockList = aBlock;

    void *aFree = theAllocator->mFreeList;
    if (theAllocator->mGrowCount > 0)
    {
        char *anItem = aBlock + aHeaderSize;
        int anIndex = 0;
        do
        {
            ++anIndex;
            TodAllocatorWriteLink(anItem, aFree);
            aFree = anItem;
            anItem += anItemStride;
        } while (anIndex != theAllocator->mGrowCount);
        aFree = aBlock + anItemStride * (anIndex - 1) + aHeaderSize;
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

#endif // LAWNPROJECTTV_TODLIB_INCLUDE_TODALLOCATOR_H
