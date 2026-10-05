// jsonChildren -- the child array behind an internalJSONNode.
//
// A plain owner of a malloc'd `JSONNode *` array: append, reserve, erase and
// delete-all. It has no base class and no virtual function.

#include "jsonChildren.h"
#include "JSONNode.h"
#include <cstdlib>
#include <cstring>

jsonChildren::jsonChildren()
    : mChildren(nullptr), mSize(0), mCapacity(0)
{
}

// ==========================================================================
// jsonChildren::deleteAll()
// @00135548  28 bytes
// ==========================================================================
void jsonChildren::deleteAll()
{
    // The second argument of deleteJSONNode is vestigial -- it is only forwarded
    // to internalJSONNode::deleteInternal, which never reads it -- so the null
    // below is what the binary effectively passes.
    for (JSONNode **it = mChildren; it != mChildren + mSize; ++it)
        (*it)->Release();
}

// ==========================================================================
// jsonChildren::inc()
// @0052A34C  46 bytes
//
// Grow by doubling when the array is full. The empty case allocates the initial
// eight entries.
// ==========================================================================
JSONNode **jsonChildren::inc()
{
    if (mSize == mCapacity)
    {
        if (mCapacity == 0)
        {
            mChildren = static_cast<JSONNode **>(std::malloc(8 * sizeof(JSONNode *)));
            mCapacity = 8;
        }
        else
        {
            mCapacity = 2 * mSize;
            mChildren = static_cast<JSONNode **>(
                std::realloc(mChildren, 2 * mSize * sizeof(JSONNode *)));
        }
    }
    return mChildren;
}

// ==========================================================================
// jsonChildren::inc(unsigned int)
// @0052A37C  62 bytes
//
// Make room for a2 more entries, doubling the capacity until it fits.
// ==========================================================================
JSONNode **jsonChildren::inc(unsigned int theExtra)
{
    if (theExtra != 0)
    {
        const unsigned int aWanted = mSize + theExtra;
        if (aWanted >= mCapacity)
        {
            if (mCapacity != 0)
            {
                while (aWanted > mCapacity)
                    mCapacity *= 2;
                mChildren = static_cast<JSONNode **>(
                    std::realloc(mChildren, mCapacity * sizeof(JSONNode *)));
            }
            else
            {
                mCapacity = (theExtra < 8) ? 8 : theExtra;
                mChildren = static_cast<JSONNode **>(
                    std::malloc(mCapacity * sizeof(JSONNode *)));
            }
        }
    }
    return mChildren;
}

// ==========================================================================
// jsonChildren::reserve2(jsonChildren*&,unsigned int)
// @0052A3BC  40 bytes
// ==========================================================================
JSONNode **jsonChildren::reserve2(jsonChildren *&theChildren, unsigned int theCapacity)
{
    if (theChildren->mChildren == nullptr)
    {
        theChildren->mChildren = static_cast<JSONNode **>(
            std::malloc(theCapacity * sizeof(JSONNode *)));
        theChildren->mCapacity = theCapacity;
    }
    else if (theChildren->mCapacity < theCapacity)
    {
        return theChildren->inc(theCapacity - theChildren->mCapacity);
    }
    return theChildren->mChildren;
}

// ==========================================================================
// jsonChildren::doerase(JSONNode **,unsigned int)
// @0052A3E4  56 bytes
// ==========================================================================
void jsonChildren::doerase(JSONNode **theStart, unsigned int theCount)
{
    JSONNode **aEnd = mChildren + mSize;
    JSONNode **aFrom = theStart + theCount;

    if (aFrom < aEnd)
    {
        const int aKeep = (int)mSize - (int)theCount - (int)(theStart - mChildren);
        std::memmove(theStart, aFrom, 4 * (size_t)aKeep);
        mSize -= theCount;
    }
    else
    {
        mSize = (unsigned int)(theStart - mChildren);
    }
}
