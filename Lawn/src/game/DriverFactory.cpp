// DriverFactory -- the driver registries.
//
// Two classes share this translation unit because the binary puts them next to
// each other and both bodies walk the same libstdc++ red-black tree:
//
//   * Sexy::DriverFactory   @0x409D80..0x409F72 -- the framework's own registry,
//     the base Sexy::InputDriverFactory placement-constructs at offset 0;
//   * PakLib::DriverFactory @0x4442A4..0x44451E -- the archive driver registry,
//     declared in DriverFactory.h.
//
// The demangled helper name gives the container away:
//
//   std::_Rb_tree<Sexy::Driver *, Sexy::Driver *, std::_Identity<Sexy::Driver *>,
//                 Sexy::DriverCompare, std::allocator<Sexy::Driver *>>
//
// -- a std::set of driver pointers ordered by DriverCompare, which is why every
// descent below compares `Driver::mPriority` and why a lookup by name has to walk
// the tree in order.

#include "DriverFactory.h"
#include "game/DriverFactory.h"
#include "Driver.h"
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

// libstdc++'s _Rb_tree_node<Driver *>: the four-word node base followed by the
// one stored pointer. Both AddDriver bodies allocate exactly sizeof(DriverNode)
// = 0x14 bytes for it and put the driver in the value slot at +0x10.
template <class T> struct DriverNode : RbTree::NodeBase
{
    T *mDriver; // +0x10
};

} // namespace Sexy

namespace
{

// DriverCompare orders the set by driver priority, so every descent in this file
// reads the node's driver and then its mPriority.
inline int PriorityOf(Sexy::RbTree::NodeBase *theNode)
{
    return static_cast<Sexy::DriverNode<Sexy::Driver> *>(theNode)->mDriver->mPriority;
}

} // namespace

// ==========================================================================
// Sexy::DriverFactory::DriverFactory()
// @00409D80  26 bytes
// ==========================================================================
Sexy::DriverFactory::DriverFactory()
{
    // `_Rb_tree_header::_M_header`'s initialisation: red, no parent, both links
    // pointing back at the header, which is the set's end sentinel. The
    // comparator word at +0x00 is the empty std::less slot and is never written.
    mDriverMap.mHeader.mColor = 0;
    mDriverMap.mHeader.mParent = nullptr;
    mDriverMap.mHeader.mLeft = &mDriverMap.mHeader;
    mDriverMap.mHeader.mRight = &mDriverMap.mHeader;
    mDriverMap.mCount = 0;

    mDeleteDrivers = true;
}

// ==========================================================================
// Sexy::DriverFactory::AddDriver(Sexy::Driver *)
// @00409D9C  106 bytes
// ==========================================================================
int Sexy::DriverFactory::AddDriver(Sexy::Driver *theDriver)
{
    if (mDeleteDrivers == 0)
        return 0;

    RbTree::NodeBase *aEnd = &mDriverMap.mHeader;
    RbTree::NodeBase *aParent = mDriverMap.mHeader.mParent;
    bool aInsertLeft;

    if (aParent != nullptr)
    {
        // libstdc++'s _M_get_insert_unique_pos: step left while the new priority
        // is the smaller one, right otherwise.
        const int aNewPriority = theDriver->mPriority;
        int aParentPriority = 0;

        while (true)
        {
            aParentPriority = PriorityOf(aParent);
            RbTree::NodeBase *aChild = (aParentPriority <= aNewPriority) ? aParent->mRight : aParent->mLeft;
            if (aChild == nullptr)
                break;
            aParent = aChild;
        }
        aInsertLeft = (aEnd == aParent) || (aNewPriority < aParentPriority);
    }
    else
    {
        aParent = aEnd;
        aInsertLeft = true;
    }

    DriverNode<Sexy::Driver> *aNewNode = new DriverNode<Sexy::Driver>;
    aNewNode->mDriver = theDriver;

    RbTree::_Rb_tree_insert_and_rebalance(aInsertLeft, aNewNode, aParent, mDriverMap.mHeader);
    ++mDriverMap.mCount;
    return 0;
}

// ==========================================================================
// Sexy::DriverFactory::Find(Sexy::String)
// @00409E08  110 bytes
// ==========================================================================
Sexy::Driver *Sexy::DriverFactory::Find(Sexy::String theName)
{
    if (mDeleteDrivers == 0)
        return nullptr;

    if (theName.compare("auto") == 0)
    {
        // The literal name "auto" resolves to the last driver in the set, which
        // is where decrementing the end sentinel lands.
        if (mDriverMap.mCount != 0)
            return static_cast<DriverNode<Sexy::Driver> *>(RbTree::_M_decrement(&mDriverMap.mHeader))->mDriver;
        return nullptr;
    }

    // The set is ordered by priority, not by name, so a name lookup is an
    // in-order walk over the whole tree.
    RbTree::NodeBase *aEnd = &mDriverMap.mHeader;
    RbTree::NodeBase *aNode = mDriverMap.mHeader.mLeft;
    if (aNode == aEnd)
        return nullptr;

    const char *aName = theName.data();
    const unsigned int aLength = theName.size();

    while (true)
    {
        Sexy::Driver *aDriver = static_cast<DriverNode<Sexy::Driver> *>(aNode)->mDriver;
        if (aLength == aDriver->mName.size() && std::memcmp(aDriver->mName.data(), aName, aLength) == 0)
            return aDriver;

        aNode = RbTree::_M_increment(aNode);
        if (aNode == aEnd)
            return nullptr;
    }
}

// ==========================================================================
// Sexy::DriverFactory::GetDrivers()
// @00409E7C  2 bytes
//
// Two bytes, `BX LR`: an empty body. Its callers rely on the callee leaving r0
// alone and read the receiver back out of it.
// ==========================================================================
void Sexy::DriverFactory::GetDrivers() {}

std::vector<Sexy::Driver *> Sexy::DriverFactory::Drivers()
{
    std::vector<Sexy::Driver *> theDrivers;
    RbTree::NodeBase *theEnd = &mDriverMap.mHeader;
    for (RbTree::NodeBase *theNode = mDriverMap.mHeader.mLeft; theNode != theEnd;
        theNode = RbTree::_M_increment(theNode))
    {
        theDrivers.push_back(static_cast<DriverNode<Sexy::Driver> *>(theNode)->mDriver);
    }
    return theDrivers;
}

// ==========================================================================
// Sexy::DriverFactory::~DriverFactory()
// @00409EA4  18 bytes
// ==========================================================================
Sexy::DriverFactory::~DriverFactory()
{
    mDeleteDrivers = false;
    RbTree::_M_erase(mDriverMap.mHeader.mParent);
}

// ==========================================================================
// Sexy::DriverFactory::RemoveDriver(Sexy::Driver *)
// @00409EB8  186 bytes
//
// std::set<Driver *, DriverCompare>::erase(key): locate the run of nodes whose
// priority equals the key's, then drop exactly that run.
// ==========================================================================
void Sexy::DriverFactory::RemoveDriver(Sexy::Driver *theDriver)
{
    if (mDeleteDrivers == 0)
        return;

    RbTree::NodeBase *aEnd = &mDriverMap.mHeader;
    RbTree::NodeBase *aNode = mDriverMap.mHeader.mParent;
    RbTree::NodeBase *aUpperBound = aEnd;
    const int aPriority = theDriver->mPriority;

    // _M_lower_bound plus the equality confirmation set::find adds: stop on an
    // equal priority, otherwise remember the last node the search passed.
    while (aNode != nullptr)
    {
        const int aNodePriority = PriorityOf(aNode);
        if (aPriority <= aNodePriority)
        {
            if (aPriority >= aNodePriority)
                break;
            aUpperBound = aNode;
            aNode = aNode->mLeft;
        }
        else
        {
            aNode = aNode->mRight;
        }
    }

    const bool aFound = (aNode != nullptr);
    if (!aFound)
        aNode = aUpperBound;

    if (aFound)
    {
        // Widen to the whole run of equal keys: [aNode, aUpperBound).
        RbTree::NodeBase *aLeft = aNode->mLeft;
        RbTree::NodeBase *aRight = aNode->mRight;
        while (aLeft != nullptr)
        {
            if (aPriority > PriorityOf(aLeft))
            {
                aLeft = aLeft->mRight;
            }
            else
            {
                aNode = aLeft;
                aLeft = aLeft->mLeft;
            }
        }
        while (aRight != nullptr)
        {
            if (aPriority >= PriorityOf(aRight))
            {
                aRight = aRight->mRight;
            }
            else
            {
                aUpperBound = aRight;
                aRight = aRight->mLeft;
            }
        }
    }

    // _M_erase_aux(first, last).
    if (mDriverMap.mHeader.mLeft == aNode && aEnd == aUpperBound)
    {
        // The run is the whole tree: drop it in one walk and re-initialise the
        // header. The colour word stays red, which is what _M_initialize leaves.
        RbTree::_M_erase(mDriverMap.mHeader.mParent);
        mDriverMap.mHeader.mLeft = aEnd;
        mDriverMap.mHeader.mRight = aEnd;
        mDriverMap.mHeader.mParent = nullptr;
        mDriverMap.mCount = 0;
    }
    else if (aUpperBound != aNode)
    {
        RbTree::NodeBase *aNext;
        do
        {
            aNext = RbTree::_M_increment(aNode);
            DriverNode<Sexy::Driver> *aKilled = static_cast<DriverNode<Sexy::Driver> *>(aNode);
            aNode = aNext;
            delete static_cast<DriverNode<Sexy::Driver> *>(
                RbTree::_Rb_tree_rebalance_for_erase(aKilled, mDriverMap.mHeader));
            --mDriverMap.mCount;
        } while (aNext != aUpperBound);
    }
}

// ==========================================================================
// PakLib::DriverFactory::AddDriver(PakLib::Driver *)
// @004442B4  98 bytes
// ==========================================================================
int PakLib::DriverFactory::AddDriver(PakLib::Driver *theDriver)
{
    Sexy::RbTree::NodeBase *aEnd = &mDriverMap.mHeader;
    Sexy::RbTree::NodeBase *aParent = mDriverMap.mHeader.mParent;
    bool aInsertLeft;

    if (aParent != nullptr)
    {
        const int aNewPriority = theDriver->mPriority;
        int aParentPriority = 0;

        while (true)
        {
            aParentPriority = static_cast<Sexy::DriverNode<PakLib::Driver> *>(aParent)->mDriver->mPriority;
            Sexy::RbTree::NodeBase *aChild = (aParentPriority <= aNewPriority) ? aParent->mRight : aParent->mLeft;
            if (aChild == nullptr)
                break;
            aParent = aChild;
        }
        aInsertLeft = (aEnd == aParent) || (aNewPriority < aParentPriority);
    }
    else
    {
        aParent = aEnd;
        aInsertLeft = true;
    }

    Sexy::DriverNode<PakLib::Driver> *aNewNode = new Sexy::DriverNode<PakLib::Driver>;
    aNewNode->mDriver = theDriver;

    Sexy::RbTree::_Rb_tree_insert_and_rebalance(aInsertLeft, aNewNode, aParent, mDriverMap.mHeader);
    ++mDriverMap.mCount;
    return 0;
}

// ==========================================================================
// PakLib::DriverFactory::Clear()
// @004443B4  54 bytes
// ==========================================================================
int PakLib::DriverFactory::Clear()
{
    Sexy::RbTree::NodeBase *aEnd = &mDriverMap.mHeader;
    Sexy::RbTree::NodeBase *aNode = mDriverMap.mHeader.mLeft;

    while (aNode != aEnd)
    {
        PakLib::Driver *aDriver = static_cast<Sexy::DriverNode<PakLib::Driver> *>(aNode)->mDriver;
        // The binary calls vtable slot 1, the deleting destructor, so this is a
        // virtual delete of the driver.
        if (aDriver != nullptr)
            delete aDriver;

        aNode = Sexy::RbTree::_M_increment(aNode);
    }

    Sexy::RbTree::_M_erase(mDriverMap.mHeader.mParent);
    mDriverMap.mHeader.mLeft = aEnd;
    mDriverMap.mHeader.mRight = aEnd;
    mDriverMap.mHeader.mParent = nullptr;
    mDriverMap.mCount = 0;
    return 0;
}

// ==========================================================================
// PakLib::DriverFactory::FindNext(PakLib::Driver *)
// @00444448  126 bytes
// ==========================================================================
PakLib::Driver *PakLib::DriverFactory::FindNext(PakLib::Driver *theDriver)
{
    // A null driver means "auto": the last driver in priority order.
    if (theDriver == nullptr)
    {
        if (mDriverMap.mCount == 0)
            return nullptr;
        return static_cast<Sexy::DriverNode<PakLib::Driver> *>(Sexy::RbTree::_M_decrement(&mDriverMap.mHeader))
            ->mDriver;
    }

    Sexy::RbTree::NodeBase *aEnd = &mDriverMap.mHeader;
    void *aKey = theDriver;
    Sexy::RbTree::NodeBase *aFound = Sexy::RbTree::find(aEnd, &aKey);
    if (aFound == aEnd)
        return nullptr;

    Sexy::RbTree::NodeBase *aNext = Sexy::RbTree::_M_increment(aFound);
    if (aNext == aEnd)
        return nullptr;
    return static_cast<Sexy::DriverNode<PakLib::Driver> *>(aNext)->mDriver;
}

// ==========================================================================
// PakLib::DriverFactory::RemoveDriver(PakLib::Driver *,bool)
// @004444E0  62 bytes
// ==========================================================================
void PakLib::DriverFactory::RemoveDriver(PakLib::Driver *theDriver, bool theDeleteDriver)
{
    Sexy::RbTree::NodeBase *aEnd = &mDriverMap.mHeader;
    void *aKey = theDriver;
    Sexy::RbTree::NodeBase *aFound = Sexy::RbTree::find(aEnd, &aKey);
    if (aFound == aEnd)
        return;

    if (theDeleteDriver)
    {
        PakLib::Driver *aDriver = static_cast<Sexy::DriverNode<PakLib::Driver> *>(aFound)->mDriver;
        delete aDriver;
    }

    delete static_cast<Sexy::DriverNode<PakLib::Driver> *>(
        Sexy::RbTree::_Rb_tree_rebalance_for_erase(aFound, mDriverMap.mHeader));
    --mDriverMap.mCount;
}
