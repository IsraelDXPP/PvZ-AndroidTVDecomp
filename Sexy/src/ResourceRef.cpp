// ResourceRef holds one resource record and keeps its reference count.
// Copy and assign add a count. Release asks the manager to drop it.

#include "ResourceRef.h"
#include "ResourceManager.h"

Sexy::ResourceRef::ResourceRef()
    : mBaseRes(nullptr)
    , mRefPad(0)
{
}

Sexy::ResourceRef::ResourceRef(const ResourceRef& theOther)
    : mBaseRes(theOther.mBaseRes)
    , mRefPad(0)
{
    if (mBaseRes != nullptr)
        ++mBaseRes->mRefCount;
}

Sexy::ResourceRef& Sexy::ResourceRef::operator=(const ResourceRef& theOther)
{
    if (this == &theOther)
        return *this;
    Release();
    mBaseRes = theOther.mBaseRes;
    if (mBaseRes != nullptr)
        ++mBaseRes->mRefCount;
    return *this;
}

bool Sexy::ResourceRef::HasResource()
{
    return mBaseRes != nullptr;
}

Sexy::String Sexy::ResourceRef::GetId()
{
    if (mBaseRes == nullptr)
        return Sexy::String();
    return mBaseRes->mId;
}

void Sexy::ResourceRef::Release()
{
    ResourceManager_BaseRes* record = mBaseRes;
    mBaseRes = nullptr;
    if (record != nullptr && record->mManager != nullptr)
        record->mManager->Deref(record);
}

Sexy::ResourceRef::~ResourceRef()
{
    Release();
}
