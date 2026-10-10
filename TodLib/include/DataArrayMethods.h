#ifndef LAWNPROJECTTV_TODLIB_INCLUDE_DATAARRAYMETHODS_H
#define LAWNPROJECTTV_TODLIB_INCLUDE_DATAARRAYMETHODS_H

// Method bodies for DataArray<T, Slot>. Include this from every translation
// unit that calls a method, after T and Slot are both complete. The bodies
// are inline so each unit can instantiate the pair it uses.

#include <cstring>
#include <new>
#include "DataArray.h"
#include "RawObject.h"

template <typename T, typename Slot>
inline void DataArray<T, Slot>::DataArrayInitialize(unsigned int theMaxSize, const char *theName)
{
    mBlock = static_cast<Slot *>(::operator new[](sizeof(Slot) * theMaxSize));
    mMaxSize = theMaxSize;
    unsigned char aNameByte1 = (unsigned char)theName[1];
    unsigned char aNameByte2 = (unsigned char)theName[2];
    mNextKey = (((aNameByte1 & 0xF) << 8) | aNameByte2) + 0xD000;
    mName = theName;
}

template <typename T, typename Slot> inline void DataArray<T, Slot>::DataArrayDispose()
{
    if (mBlock == nullptr)
        return;

    DataArrayFreeAll();
    ::operator delete[](mBlock);
    mBlock = nullptr;
    mMaxUsedCount = 0;
    mMaxSize = 0;
    mFreeListHead = 0;
    mSize = 0;
    mName = nullptr;
}

template <typename T, typename Slot> inline void DataArray<T, Slot>::DataArrayFree(T *theItem)
{
    Slot *aSlot = nullptr;
    for (unsigned int anIndex = 0; anIndex < mMaxUsedCount; ++anIndex)
    {
        if (&mBlock[anIndex].mItem == theItem)
        {
            aSlot = &mBlock[anIndex];
            break;
        }
    }

    theItem->~T();
    unsigned int anIndex = aSlot->mID & DATA_ARRAY_INDEX_MASK;
    aSlot->mID = mFreeListHead;
    mFreeListHead = anIndex;
    --mSize;
}

template <typename T, typename Slot> inline void DataArray<T, Slot>::DataArrayFreeAll()
{
    T *aItem = nullptr;
    while (IterateNext(aItem))
        DataArrayFree(aItem);

    mFreeListHead = 0;
    mMaxUsedCount = 0;
}

template <typename T, typename Slot> inline unsigned int DataArray<T, Slot>::DataArrayGetID(T *theItem)
{
    for (unsigned int anIndex = 0; anIndex < mMaxSize; ++anIndex)
    {
        if (&mBlock[anIndex].mItem == theItem)
            return mBlock[anIndex].mID;
    }
    return 0;
}

template <typename T, typename Slot> inline bool DataArray<T, Slot>::IterateNext(T *&theItem)
{
    Slot *aSlot = &mBlock[0];
    if (theItem != nullptr)
    {
        for (unsigned int anIndex = 0; anIndex < mMaxUsedCount; ++anIndex)
        {
            if (&mBlock[anIndex].mItem == theItem)
            {
                aSlot = &mBlock[anIndex + 1];
                break;
            }
        }
    }

    Slot *aLast = &mBlock[mMaxUsedCount];
    while (aSlot < aLast)
    {
        if (aSlot->mID & DATA_ARRAY_KEY_MASK)
        {
            theItem = &aSlot->mItem;
            return true;
        }
        ++aSlot;
    }

    theItem = nullptr;
    return false;
}

template <typename T, typename Slot> inline T *DataArray<T, Slot>::DataArrayAlloc()
{
    unsigned int anIndex = mMaxUsedCount;
    if (mFreeListHead == mMaxUsedCount)
        mFreeListHead = ++mMaxUsedCount;
    else
    {
        anIndex = mFreeListHead;
        mFreeListHead = mBlock[anIndex].mID;
    }

    Slot *aSlot = &mBlock[anIndex];
    std::memset(aSlot, 0, sizeof(Slot));
    aSlot->mID = (mNextKey << DATA_ARRAY_KEY_SHIFT) | anIndex;
    if (++mNextKey >= DATA_ARRAY_MAX_SIZE)
        mNextKey = DATA_ARRAY_KEY_FIRST;
    ++mSize;
    return ConstructRaw<T>(&aSlot->mItem);
}

template <typename T, typename Slot> inline T *DataArray<T, Slot>::DataArrayTryToGet(unsigned int theId)
{
    if (theId == 0 || (theId & DATA_ARRAY_INDEX_MASK) >= mMaxSize)
        return nullptr;

    Slot *aSlot = &mBlock[theId & DATA_ARRAY_INDEX_MASK];
    return aSlot->mID == theId ? &aSlot->mItem : nullptr;
}

template <typename T, typename Slot> inline T *DataArray<T, Slot>::DataArrayGet(unsigned int theId)
{
    return &mBlock[theId & DATA_ARRAY_INDEX_MASK].mItem;
}

#endif
