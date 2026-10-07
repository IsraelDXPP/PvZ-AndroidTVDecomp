// JSONNode -- the jsoncpp node handle used by the TV build.
//
// A handle is one pointer. Copying shares the internal node and bumps its
// refcount; the last release deletes it. Writes unshare first so one handle
// cannot change every other handle of the same node.

#include "JSONNode.h"
#include "jsonChildren.h"
#include "ReconExterns.h"
#include "ReconHelpers.h"

#include <cstdarg>
#include <stdexcept>
#include <stdio.h>

namespace
{

enum JsonType
{
    JSON_NULL = 0,
    JSON_STRING = 1,
    JSON_NUMBER = 2,
    JSON_BOOL = 3,
    JSON_ARRAY = 4,
    JSON_OBJECT = 5
};

bool IsContainer(unsigned char theType)
{
    return theType == JSON_ARRAY || theType == JSON_OBJECT;
}

void Retain(internalJSONNode *theNode)
{
    if (theNode != nullptr)
        theNode->refcount += 1;
}

void ReleaseNode(internalJSONNode *theNode)
{
    if (theNode == nullptr)
        return;
    if (theNode->refcount > 0)
        theNode->refcount -= 1;
    if (theNode->refcount == 0)
        theNode->deleteInternal(theNode);
}

internalJSONNode *Unshare(internalJSONNode *theNode)
{
    if (theNode == nullptr || theNode->refcount <= 1)
        return theNode;
    theNode->refcount -= 1;
    return theNode->newInternal(*theNode);
}

void AssignName(internalJSONNode *theNode, Sexy::String const& theName)
{
    theNode->_name = theName;
    theNode->_name_encoded = true;
}

template <typename T>
internalJSONNode *MakeValued(Sexy::String const& theName, T theValue)
{
    internalJSONNode *aNode = new internalJSONNode(theName);
    aNode->Set(theValue);
    AssignName(aNode, theName);
    return aNode;
}

double ReadNumber(internalJSONNode *theNode)
{
    theNode->Fetch();
    unsigned char aType = theNode->_type;
    if (aType == JSON_STRING)
    {
        theNode->FetchNumber();
        return theNode->_value._number;
    }
    if (aType == JSON_NULL)
        return 0.0;
    if (aType == JSON_BOOL)
        return theNode->_value._bool ? 1.0 : 0.0;
    return theNode->_value._number;
}

bool ReadBool(internalJSONNode *theNode)
{
    theNode->Fetch();
    if (theNode->_type == JSON_NULL)
        return false;
    if (theNode->_type != JSON_NUMBER)
        return theNode->_value._bool;
    double aNumber = theNode->_value._number;
    bool aNearZero = aNumber > 0.0 ? aNumber < 0.00001 : aNumber > -0.00001;
    return !aNearZero;
}

void ClearName(internalJSONNode *theNode)
{
    theNode->_name = Sexy::String();
}

}

JSONNode::JSONNode(JSONNode const& theOther)
    : mInternal(theOther.mInternal)
{
    Retain(mInternal);
}

JSONNode & JSONNode::operator=(JSONNode const& theOther)
{
    if (mInternal == theOther.mInternal)
        return *this;
    internalJSONNode *aIncoming = theOther.mInternal;
    Retain(aIncoming);
    ReleaseNode(mInternal);
    mInternal = aIncoming;
    return *this;
}

JSONNode::~JSONNode()
{
    ReleaseNode(mInternal);
    mInternal = nullptr;
}

JSONNode::JSONNode(Sexy::String const& theName, char theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, unsigned char theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, short theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, unsigned short theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, int theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, unsigned int theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, float theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, double theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, long long theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, unsigned long long theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, long double theValue)
    : mInternal(MakeValued(theName, (double)theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, Sexy::String const& theValue)
    : mInternal(new internalJSONNode(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, bool theValue)
    : mInternal(MakeValued(theName, theValue))
{
}

JSONNode::JSONNode(Sexy::String const& theName, char const* theValue)
    : mInternal(new internalJSONNode(theName, Sexy::String(theValue)))
{
}

JSONNode * JSONNode::Share()
{
    JSONNode *aHandle = new JSONNode();
    Retain(mInternal);
    aHandle->mInternal = mInternal;
    return aHandle;
}

JSONNode * JSONNode::Adopt(internalJSONNode *theNode)
{
    JSONNode *aHandle = new JSONNode();
    aHandle->mInternal = theNode;
    return aHandle;
}

void JSONNode::Bind(JSONNode &theHandle, internalJSONNode *theNode)
{
    theHandle.mInternal = theNode;
}

JSONNode * JSONNode::Steal()
{
    JSONNode *aHandle = new JSONNode();
    aHandle->mInternal = mInternal;
    mInternal = nullptr;
    return aHandle;
}

void JSONNode::Release()
{
    if (this == nullptr)
        return;
    ReleaseNode(mInternal);
    mInternal = nullptr;
    delete this;
}

void JSONNode::MakeUnique()
{
    mInternal = Unshare(mInternal);
}

void JSONNode::ThrowOutOfRange()
{
    Sexy::String aMessage("JSONNode::at");
    void *aException = CxaAllocateException(8u);
    ConstructOutOfRangeCopy(aException, &aMessage);
    CxaThrow(aException, nullptr, (void (*)(void *))DestroyOutOfRange);
}

JSONNode * JSONNode::merge(JSONNode *theOther)
{
    if (theOther == nullptr || mInternal == theOther->mInternal)
        return this;
    JSONNode *aKeeper = this;
    JSONNode *aDonor = theOther;
    if (mInternal != nullptr
        && theOther->mInternal != nullptr
        && mInternal->refcount >= theOther->mInternal->refcount)
    {
        aKeeper = theOther;
        aDonor = this;
    }
    *aKeeper = *aDonor;
    return aKeeper;
}

JSONNode * JSONNode::merge(unsigned int theCount, ...)
{
    va_list aArgs;
    va_start(aArgs, theCount);
    JSONNode *aResult = this;
    for (unsigned int aIndex = 0; aIndex < theCount; ++aIndex)
    {
        JSONNode *aNext = va_arg(aArgs, JSONNode *);
        aResult = aResult->merge(aNext);
    }
    va_end(aArgs);
    return aResult;
}

Sexy::String JSONNode::write_formatted(unsigned int theReserve) const
{
    if (mInternal != nullptr && IsContainer(mInternal->_type))
    {
        Sexy::String aText;
        aText.reserve(theReserve);
        mInternal->Write(0, 1, &aText);
        return aText;
    }
    return Sexy::String();
}

JSONNode JSONNode::duplicate() const
{
    JSONNode aCopy;
    if (mInternal == nullptr)
        return aCopy;
    if (mInternal->refcount > 1)
        aCopy.mInternal = mInternal->newInternal(*mInternal);
    else
    {
        aCopy.mInternal = mInternal;
        Retain(mInternal);
    }
    return aCopy;
}

JSONNode JSONNode::as_node() const
{
    if (mInternal != nullptr && mInternal->_type == JSON_OBJECT)
    {
        JSONNode aShared;
        Retain(mInternal);
        aShared.mInternal = mInternal;
        return aShared;
    }
    if (mInternal != nullptr && mInternal->_type == JSON_ARRAY)
    {
        JSONNode aObject = duplicate();
        aObject.mInternal->_type = JSON_OBJECT;
        return aObject;
    }
    JSONNode aFresh;
    aFresh.mInternal = new internalJSONNode(Sexy::String());
    aFresh.mInternal->_type = JSON_OBJECT;
    aFresh.mInternal->CHILDREN = new jsonChildren();
    return aFresh;
}

JSONNode JSONNode::as_array() const
{
    if (mInternal != nullptr && mInternal->_type == JSON_ARRAY)
    {
        JSONNode aShared;
        Retain(mInternal);
        aShared.mInternal = mInternal;
        return aShared;
    }
    if (mInternal != nullptr && mInternal->_type == JSON_OBJECT)
    {
        JSONNode aArray = duplicate();
        aArray.mInternal->_type = JSON_ARRAY;
        jsonChildren *aChildren = aArray.mInternal->CHILDREN;
        if (aChildren != nullptr)
        {
            for (unsigned int aIndex = 0; aIndex < aChildren->mChildren.size(); ++aIndex)
            {
                JSONNode *aChild = aChildren->mChildren[aIndex];
                if (aChild == nullptr || aChild->mInternal == nullptr)
                    continue;
                aChild->mInternal = Unshare(aChild->mInternal);
                ClearName(aChild->mInternal);
            }
        }
        return aArray;
    }
    JSONNode aFresh;
    aFresh.mInternal = new internalJSONNode(Sexy::String());
    aFresh.mInternal->_type = JSON_ARRAY;
    aFresh.mInternal->CHILDREN = new jsonChildren();
    return aFresh;
}

void JSONNode::cast(char theType)
{
    if (mInternal == nullptr || (char)mInternal->_type == theType)
        return;
    switch (theType)
    {
    case JSON_NULL:
        MakeUnique();
        mInternal->Nullify();
        return;
    case JSON_STRING:
    {
        mInternal->Fetch();
        Sexy::String aText = mInternal->_string;
        MakeUnique();
        mInternal->_type = JSON_STRING;
        mInternal->_string = aText;
        mInternal->_string_encoded = true;
        mInternal->fetched = true;
        return;
    }
    case JSON_NUMBER:
        MakeUnique();
        mInternal->Set(ReadNumber(mInternal));
        return;
    case JSON_BOOL:
        MakeUnique();
        mInternal->Set(ReadBool(mInternal));
        return;
    case JSON_ARRAY:
        *this = as_array();
        return;
    case JSON_OBJECT:
        *this = as_node();
        return;
    default:
        return;
    }
}

JSONNode * JSONNode::at(unsigned int theIndex)
{
    MakeUnique();
    JSONNode const *aReader = this;
    return aReader->at(theIndex);
}

JSONNode * JSONNode::at(Sexy::String const& theName)
{
    MakeUnique();
    JSONNode const *aReader = this;
    return aReader->at(theName);
}

JSONNode * JSONNode::at_nocase(Sexy::String const& theName)
{
    MakeUnique();
    JSONNode const *aReader = this;
    return aReader->at_nocase(theName);
}

JSONNode * JSONNode::at(unsigned int theIndex) const
{
    if (mInternal == nullptr || !IsContainer(mInternal->_type))
        ThrowOutOfRange();
    mInternal->Fetch();
    jsonChildren *aChildren = mInternal->CHILDREN;
    if (aChildren == nullptr || theIndex >= aChildren->mChildren.size())
        ThrowOutOfRange();
    return aChildren->mChildren[theIndex];
}

JSONNode * JSONNode::at(Sexy::String const& theName) const
{
    if (mInternal == nullptr)
        ThrowOutOfRange();
    JSONNode **aSlot = (JSONNode **)mInternal->at(theName);
    if (aSlot == nullptr || *aSlot == nullptr)
        ThrowOutOfRange();
    return *aSlot;
}

JSONNode * JSONNode::at_nocase(Sexy::String const& theName) const
{
    if (mInternal == nullptr)
        ThrowOutOfRange();
    JSONNode **aSlot = (JSONNode **)mInternal->at_nocase(theName);
    if (aSlot == nullptr || *aSlot == nullptr)
        ThrowOutOfRange();
    return *aSlot;
}

JSONNode JSONNode::pop_back(unsigned int theIndex)
{
    if (mInternal == nullptr || !IsContainer(mInternal->_type))
        ThrowOutOfRange();
    mInternal->Fetch();
    jsonChildren *aChildren = mInternal->CHILDREN;
    if (aChildren == nullptr || theIndex >= aChildren->mChildren.size())
        ThrowOutOfRange();
    MakeUnique();
    JSONNode *aRemoved = (JSONNode *)mInternal->pop_back(theIndex);
    if (aRemoved == nullptr)
        ThrowOutOfRange();
    JSONNode aTaken(*aRemoved);
    aRemoved->Release();
    return aTaken;
}

JSONNode JSONNode::pop_back(Sexy::String const& theName)
{
    if (mInternal == nullptr)
        ThrowOutOfRange();
    JSONNode *aRemoved = (JSONNode *)mInternal->pop_back(theName);
    if (aRemoved == nullptr)
        ThrowOutOfRange();
    JSONNode aTaken(*aRemoved);
    aRemoved->Release();
    return aTaken;
}

JSONNode JSONNode::pop_back_nocase(Sexy::String const& theName)
{
    if (mInternal == nullptr)
        ThrowOutOfRange();
    JSONNode *aRemoved = (JSONNode *)mInternal->pop_back_nocase(theName);
    if (aRemoved == nullptr)
        ThrowOutOfRange();
    JSONNode aTaken(*aRemoved);
    aRemoved->Release();
    return aTaken;
}
