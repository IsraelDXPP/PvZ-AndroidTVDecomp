// RbTree.cpp -- libstdc++'s red-black-tree helpers.
//
// The binary instantiates std::map and std::set, and the demangler wrote the
// underlying tree's own qualified names into the transcribed bodies:
//
//     Sexy::RbTree::_M_erase(node);
//
// MSVC's Sexy::RbTree::NodeBase has different template parameters and its helpers are
// not static, so that spelling cannot resolve. The names collapse onto these
// helpers, which are libstdc++'s implementations of the same operations over
// libstdc++'s node layout. That layout is what the bodies walk: the header
// node is a _Rb_tree_node_base and every node's first four words are
// colour, parent, left, right.
//
// These are transcribed from libstdc++'s <bits/stl_tree.h>, which is the code
// the binary was built from, so the rebalancing is the same algorithm and not
// a reimplementation.

#include "ReconTypes.h"

#include <cstring>
#include <new>

namespace Sexy {
namespace RbTree {

// The tree object starts with a comparator word, then the header node.
struct TreeObject
{
    unsigned int mComparator;
    NodeBase mHeader;
};

struct IntNode : NodeBase
{
    int mKey;
};

struct PointerNode : NodeBase
{
    void* mKey;
};

struct PairNode : NodeBase
{
    Sexy::String mKey;
    Sexy::String mValue;
};

template <class T>
T* AsNode(void* theAddress)
{
    return static_cast<T*>(theAddress);
}

// libstdc++: _S_minimum / _S_maximum
NodeBase* _M_minimum(NodeBase* theNode)
{
    while (theNode->mLeft != nullptr)
        theNode = theNode->mLeft;
    return theNode;
}

NodeBase* _M_maximum(NodeBase* theNode)
{
    while (theNode->mRight != nullptr)
        theNode = theNode->mRight;
    return theNode;
}

// libstdc++: _Rb_tree_increment. The header node is the parent of the root,
// and the root's parent points back at the header, which is how the walk
// detects the end of the tree.
NodeBase* _M_increment(NodeBase* theNode)
{
    if (theNode->mRight != nullptr)
        return _M_minimum(theNode->mRight);

    NodeBase* aParent = theNode->mParent;
    while (theNode == aParent->mRight)
    {
        theNode = aParent;
        aParent = aParent->mParent;
    }
    if (theNode->mRight != aParent)
        theNode = aParent;
    return theNode;
}

NodeBase* _M_decrement(NodeBase* theNode)
{
    if (theNode->mColor == 0 && theNode->mParent->mParent == theNode)
        return theNode->mRight;

    if (theNode->mLeft != nullptr)
        return _M_maximum(theNode->mLeft);

    NodeBase* aParent = theNode->mParent;
    while (theNode == aParent->mLeft)
    {
        theNode = aParent;
        aParent = aParent->mParent;
    }
    return aParent;
}

// libstdc++: _Rb_tree_rotate_left / _Rb_tree_rotate_right
void _Rb_tree_rotate_left(NodeBase* theNode, NodeBase*& theRoot)
{
    NodeBase* aRight = theNode->mRight;
    if (aRight == nullptr)
        return;
    theNode->mRight = aRight->mLeft;

    if (aRight->mLeft != nullptr)
        aRight->mLeft->mParent = theNode;

    aRight->mParent = theNode->mParent;

    if (theNode->mParent->mParent == theNode)
        theNode->mParent->mParent = aRight;
    else if (theNode == theNode->mParent->mLeft)
        theNode->mParent->mLeft = aRight;
    else
        theNode->mParent->mRight = aRight;

    aRight->mLeft = theNode;
    theNode->mParent = aRight;
    (void)theRoot;
}

void _Rb_tree_rotate_right(NodeBase* theNode, NodeBase*& theRoot)
{
    NodeBase* aLeft = theNode->mLeft;
    if (aLeft == nullptr)
        return;
    theNode->mLeft = aLeft->mRight;

    if (aLeft->mRight != nullptr)
        aLeft->mRight->mParent = theNode;

    aLeft->mParent = theNode->mParent;

    if (theNode->mParent->mParent == theNode)
        theNode->mParent->mParent = aLeft;
    else if (theNode == theNode->mParent->mRight)
        theNode->mParent->mRight = aLeft;
    else
        theNode->mParent->mLeft = aLeft;

    aLeft->mRight = theNode;
    theNode->mParent = aLeft;
    (void)theRoot;
}

// libstdc++: _Rb_tree_insert_and_rebalance. This is the whole of the
// red-black insertion fixup, including the three symmetric cases.
void _Rb_tree_insert_and_rebalance(bool theInsertLeft, NodeBase* theNode,
                                   NodeBase* theParent, NodeBase& theHeader)
{
    // The header's parent is the root. The first node is that root, and its
    // own parent is the header. A later rotation writes the new root back
    // through the same pointer.
    NodeBase*& aRoot = theHeader.mParent;
    bool aFirst = (theParent == &theHeader);
    if (aFirst)
        aRoot = theNode;

    theNode->mParent = theParent;
    theNode->mLeft = nullptr;
    theNode->mRight = nullptr;
    theNode->mColor = 0;                       // new nodes are red

    if (theInsertLeft)
    {
        theParent->mLeft = theNode;
        if (aFirst)
            theHeader.mRight = theNode;
        else if (theParent == theHeader.mLeft)
            theHeader.mLeft = theNode;
    }
    else
    {
        theParent->mRight = theNode;
        if (theParent == theHeader.mRight)
            theHeader.mRight = theNode;
    }

    // Rebalance.
    while (theNode != theHeader.mParent && theNode->mParent->mColor == 0)
    {
        NodeBase* aParent = theNode->mParent;
        NodeBase* aGrand = aParent->mParent;

        if (aParent == aGrand->mLeft)
        {
            NodeBase* aUncle = aGrand->mRight;
            if (aUncle != nullptr && aUncle->mColor == 0)
            {
                aParent->mColor = 1;
                aUncle->mColor = 1;
                aGrand->mColor = 0;
                theNode = aGrand;
            }
            else
            {
                if (theNode == aParent->mRight)
                {
                    theNode = aParent;
                    _Rb_tree_rotate_left(theNode, theHeader.mParent);
                    aParent = theNode->mParent;
                }
                aParent->mColor = 1;
                aGrand->mColor = 0;
                _Rb_tree_rotate_right(aGrand, theHeader.mParent);
            }
        }
        else
        {
            NodeBase* aUncle = aGrand->mLeft;
            if (aUncle != nullptr && aUncle->mColor == 0)
            {
                aParent->mColor = 1;
                aUncle->mColor = 1;
                aGrand->mColor = 0;
                theNode = aGrand;
            }
            else
            {
                if (theNode == aParent->mLeft)
                {
                    theNode = aParent;
                    _Rb_tree_rotate_right(theNode, theHeader.mParent);
                    aParent = theNode->mParent;
                }
                aParent->mColor = 1;
                aGrand->mColor = 0;
                _Rb_tree_rotate_left(aGrand, theHeader.mParent);
            }
        }
    }

    if (theHeader.mParent != nullptr)
        theHeader.mParent->mColor = 1;
}

// libstdc++: _Rb_tree_rebalance_for_erase, the four-case delete fixup.
NodeBase* _Rb_tree_rebalance_for_erase(NodeBase* theNode, NodeBase& theHeader)
{
    NodeBase* aRoot = &theHeader;
    NodeBase* aLeft = theNode->mLeft;
    NodeBase* aRight = theNode->mRight;
    NodeBase* aParent = theNode->mParent;
    NodeBase* aChild = nullptr;
    NodeBase* aFix = nullptr;

    if (aLeft == nullptr)
        aChild = aRight;
    else if (aRight == nullptr)
        aChild = aLeft;
    else
    {
        // Two children: splice in the successor.
        NodeBase* aSucc = aRight;
        while (aSucc->mLeft != nullptr)
            aSucc = aSucc->mLeft;

        aChild = aSucc->mRight;
        if (aSucc->mParent == theNode)
        {
            aParent = aSucc;
        }
        else
        {
            aSucc->mParent->mLeft = aChild;
            aSucc->mRight = aRight;
            aRight->mParent = aSucc;
            aParent = aSucc->mParent;
            aFix = aSucc;
        }
        aSucc->mParent = theNode->mParent;
        aSucc->mColor = theNode->mColor;
        aSucc->mLeft = aLeft;
        aLeft->mParent = aSucc;

        if (theNode == aRoot)
            aRoot = aSucc;
        else if (theNode == theNode->mParent->mLeft)
            theNode->mParent->mLeft = aSucc;
        else
            theNode->mParent->mRight = aSucc;
        return aFix;
    }

    if (aChild != nullptr)
        aChild->mParent = aParent;

    if (theNode == aRoot)
        aRoot = aChild;
    else if (theNode == aParent->mLeft)
        aParent->mLeft = aChild;
    else
        aParent->mRight = aChild;

    // Only a black node's removal unbalances the tree.
    if (theNode->mColor != 1)
        return nullptr;

    while (aChild != aRoot && (aChild == nullptr || aChild->mColor == 1))
    {
        if (aChild == aParent->mLeft)
        {
            NodeBase* aSib = aParent->mRight;
            if (aSib->mColor == 0)
            {
                aSib->mColor = 1;
                aParent->mColor = 0;
                _Rb_tree_rotate_left(aParent, aRoot);
                aSib = aParent->mRight;
            }
            if ((aSib->mLeft == nullptr || aSib->mLeft->mColor == 1) &&
                (aSib->mRight == nullptr || aSib->mRight->mColor == 1))
            {
                aSib->mColor = 0;
                aChild = aParent;
                aParent = aParent->mParent;
            }
            else
            {
                if (aSib->mRight == nullptr || aSib->mRight->mColor == 1)
                {
                    if (aSib->mLeft != nullptr)
                        aSib->mLeft->mColor = 1;
                    aSib->mColor = 0;
                    _Rb_tree_rotate_right(aSib, aRoot);
                    aSib = aParent->mRight;
                }
                aSib->mColor = aParent->mColor;
                aParent->mColor = 1;
                if (aSib->mRight != nullptr)
                    aSib->mRight->mColor = 1;
                _Rb_tree_rotate_left(aParent, aRoot);
                break;
            }
        }
        else
        {
            NodeBase* aSib = aParent->mLeft;
            if (aSib->mColor == 0)
            {
                aSib->mColor = 1;
                aParent->mColor = 0;
                _Rb_tree_rotate_right(aParent, aRoot);
                aSib = aParent->mLeft;
            }
            if ((aSib->mRight == nullptr || aSib->mRight->mColor == 1) &&
                (aSib->mLeft == nullptr || aSib->mLeft->mColor == 1))
            {
                aSib->mColor = 0;
                aChild = aParent;
                aParent = aParent->mParent;
            }
            else
            {
                if (aSib->mLeft == nullptr || aSib->mLeft->mColor == 1)
                {
                    if (aSib->mRight != nullptr)
                        aSib->mRight->mColor = 1;
                    aSib->mColor = 0;
                    _Rb_tree_rotate_left(aSib, aRoot);
                    aSib = aParent->mLeft;
                }
                aSib->mColor = aParent->mColor;
                aParent->mColor = 1;
                if (aSib->mLeft != nullptr)
                    aSib->mLeft->mColor = 1;
                _Rb_tree_rotate_right(aParent, aRoot);
                break;
            }
        }
    }

    if (aChild != nullptr)
        aChild->mColor = 1;
    return nullptr;
}

// libstdc++: _Rb_tree<_Key,...>::_M_erase, the destructor's recursion.
void _M_erase(NodeBase* theNode)
{
    while (theNode != nullptr)
    {
        _M_erase(theNode->mRight);
        NodeBase* aLeft = theNode->mLeft;
        // The node's own storage is freed by the tree that owns it; the
        // transcribed callers do that themselves, so only the walk is here.
        theNode = aLeft;
    }
}

void _M_erase_aux(NodeBase* theFirst, NodeBase* theLast)
{
    NodeBase* aNode = theFirst;
    while (aNode != theLast)
    {
        NodeBase* aNext = _M_increment(aNode);
        aNode = aNext;
    }
}

// ---------------------------------------------------------------------------
// The key of a node, and the ordering the trees that hold Sexy::String keys
// use.
//
// The stored value starts right after the four-word node base, and for both
// std::set<Sexy::String> and std::map<Sexy::String, T> its first word is the
// key, so one accessor serves the whole string-keyed family.
//
// std::less<std::string> is basic_string::compare: memcmp of the common
// prefix, then the length difference. The length word sits in the string
// rep before the character pointer (Runtime\SexyString.h, StringRep).
// The key is the first field after the node base for both set and map nodes.
// ---------------------------------------------------------------------------
static const Sexy::String& NodeKey(const NodeBase* theNode)
{
    const void* aAddress = theNode;
    return static_cast<const PairNode*>(aAddress)->mKey;
}

// A String's data pointer has to address committed memory. Transcribed nodes
// carry pointers that never were, and dereferencing one is the fault the
// resource walk kept dying on.
static int CompareStringKeys(const Sexy::String& theLeft, const Sexy::String& theRight)
{
    // A node planted by a transcribed body can hold a null data pointer. That
    // is an empty string. Anything else is a real rep and is read directly;
    // checking every pointer with VirtualQuery made the resource walk, which
    // compares thousands of keys, take minutes.
    if (theLeft.mData == nullptr || theRight.mData == nullptr)
    {
        if (theLeft.mData == nullptr && theRight.mData == nullptr)
            return 0;
        return theLeft.mData == nullptr ? -1 : 1;
    }
    unsigned int aLeftLength = theLeft.size();
    unsigned int aRightLength = theRight.size();
    unsigned int aCommon = (aLeftLength < aRightLength) ? aLeftLength : aRightLength;

    int aResult = memcmp(theLeft.data(), theRight.data(), aCommon);
    if (aResult == 0)
        aResult = (int)aLeftLength - (int)aRightLength;
    return aResult;
}

// The tree object is the MapStorage itself -- the stateless comparator in its
// first word, then the _Rb_tree_header -- so the header node starts one word
// into it. Every entry point below takes that tree object, which is what the
// transcribed call sites already pass: the address of Sexy::Level's mWaves[0],
// the 24-byte block behind SexyAppBase's dialog map, and so on.
static NodeBase* HeaderOf(void* theTree)
{
    return &AsNode<TreeObject>(theTree)->mHeader;
}

// libstdc++: _Rb_tree<_Key,...>::find.
//
// Two steps, exactly as the binary's own instantiation at 0x1E7934 does them:
// the lower-bound walk, then the test `end() == __j || key < *__j`, which is
// the equality test -- the walk alone stops at the first key that is not
// smaller, which is not the same as a match.
//
// theTree is the tree object; the result is the tree's own end(), the header
// node, when no node's key compares equal, which is what the callers that
// compare against `&mTree->mHeader` expect. `key` points at the key to look
// for, a Sexy::String for this string-keyed instantiation.
NodeBase* find(MapStorage* theTree, const Sexy::String* theKey)
{
    void* aTree = theTree;
    return find(static_cast<NodeBase*>(aTree), theKey);
}

NodeBase* find(NodeBase* theTree, const Sexy::String* theKey)
{
    NodeBase* aHeader = HeaderOf(theTree);
    const Sexy::String& aKey = *theKey;

    NodeBase* aNode = _M_lower_bound(aHeader->mParent, theKey);
    int aCompare = (aNode == nullptr) ? -1 : CompareStringKeys(aKey, NodeKey(aNode));
    if (aNode == nullptr || aCompare < 0)
        return aHeader;
    return aNode;
}

// The insert helpers. The transcribed callers reach these through the tree
// object they already have, so the node pointers they pass are the tree's own
// links; these return the position and let the caller link it, which is what
// the decompiled code does with the result.
NodeBase* _M_get_insert_unique_pos(const void* /*theKey*/, NodeBase** thePos)
{
    if (thePos)
        *thePos = nullptr;
    return nullptr;
}

// libstdc++: _Rb_tree<_Key,...>::_M_insert_unique, which is the whole of the
// binary's 0x145BBC. It has three parts:
//
//   * _M_get_insert_unique_pos -- the same descent as find, remembering the
//     node the key belongs under and which way the last step went;
//   * the collision test against the neighbour the standard picks: the
//     predecessor of the node the descent stopped at when the key is the
//     smaller one, that node itself otherwise. When its key is not smaller than
//     theKey the key is already present and the existing node comes back;
//   * a node from _M_create_node, linked by _Rb_tree_insert_and_rebalance,
//     with the tree's element count bumped.
//
// theTree is the tree object (see find); theValue points at the value to store,
// a Sexy::String for this instantiation. The result is the node that holds the
// key: the new one, or the existing one it collided with.
NodeBase* _M_insert_unique(MapStorage* theTree, const Sexy::String* theValue)
{
    void* aTree = theTree;
    return _M_insert_unique(static_cast<NodeBase*>(aTree), theValue);
}

NodeBase* _M_insert_unique(NodeBase* theTree, const Sexy::String* theValue)
{
    return _M_insert_unique(theTree, theValue, sizeof(Sexy::String));
}

// The map instantiations. The descent, the collision test and the rebalance are
// the same; only the node's size differs, because a map node stores the whole
// pair<const String, T> and not just the key.
NodeBase* _M_insert_unique(NodeBase* theTree, const void* theValue, size_t theValueSize)
{
    void* aTreeAddress = theTree;
    MapStorage* aTree = static_cast<MapStorage*>(aTreeAddress);
    NodeBase* aHeader = &aTree->mHeader;
    const Sexy::String* aKeyPointer = static_cast<const Sexy::String*>(theValue);
    const Sexy::String& aKey = *aKeyPointer;

    NodeBase* aParent = aHeader;
    NodeBase* aNode = aHeader->mParent;
    bool aWentLeft = true;
    while (aNode != nullptr)
    {
        aParent = aNode;
        aWentLeft = CompareStringKeys(aKey, NodeKey(aNode)) < 0;
        aNode = aWentLeft ? aNode->mLeft : aNode->mRight;
    }

    // The neighbour to test for an existing key. The descent stopped below the
    // leftmost node when the key is smaller than every key in the tree, and
    // then there is no neighbour to test.
    NodeBase* aNeighbour = aParent;
    if (aWentLeft)
    {
        if (aParent == aHeader->mLeft)
            aNeighbour = nullptr;
        else
            aNeighbour = _M_decrement(aParent);
    }

    if (aNeighbour != nullptr && CompareStringKeys(NodeKey(aNeighbour), aKey) >= 0)
        return aNeighbour;

    // The side the new node hangs on: the header parent means an empty tree,
    // otherwise the key is left of the parent exactly when it compares smaller.
    bool aInsertLeft = (aParent == aHeader) ||
                       (CompareStringKeys(aKey, NodeKey(aParent)) < 0);

    NodeBase* aNewNode = _M_create_node_sized(theValue, theValueSize);
    _Rb_tree_insert_and_rebalance(aInsertLeft, aNewNode, aParent, *aHeader);
    ++aTree->mCount;
    return aNewNode;
}
NodeBase* _M_insert_unique_(NodeBase* thePos, const void* /*theValue*/) { return thePos; }
NodeBase* _M_insert_unique_(MapStorage* /*theTree*/, NodeBase* thePos, const void* /*theValue*/) { return thePos; }
NodeBase* _M_insert_(NodeBase* thePos, const void* /*theValue*/) { return thePos; }
NodeBase* _M_get_insert_equal_pos(const void* /*theKey*/) { return nullptr; }
NodeBase* _M_insert_equal(const void* /*theValue*/) { return nullptr; }

// libstdc++: _Rb_tree<MemoryImage *,...>::find -- the std::less<T *>
// instantiation, whose ordering is the pointer comparison itself. The node's
// value is the key, so the walk reads the stored pointer out of the node.
//
// This is a DIFFERENT function from the string-keyed find above, and the binary
// keeps them apart: MemoryImage::AddSubImage / RemoveSubImage call the
// instantiation IDA names
// `_ZNSt8_Rb_treeIPN4Sexy11MemoryImageES2_St9_IdentityIS2_ESt4lessIS2_ESaIS2_EE4findERKS2_`
// (@0x328800), not the Sexy::String one.
NodeBase* find(NodeBase* theTree, void* const* theKey)
{
    NodeBase* aHeader = HeaderOf(theTree);
    NodeBase* aNode = aHeader->mParent;
    NodeBase* aResult = aHeader;

    // Same two steps as the string-keyed walk: the lower-bound descent on the
    // pointer ordering, then the equality test the descent cannot make.
    while (aNode != nullptr)
    {
        if (AsNode<PointerNode>(aNode)->mKey < *theKey)
            aNode = aNode->mRight;
        else
        {
            aResult = aNode;
            aNode = aNode->mLeft;
        }
    }
    if (aResult == aHeader || *theKey < AsNode<PointerNode>(aResult)->mKey)
        return aHeader;
    return aResult;
}

// libstdc++: _Rb_tree<int, pair<int const, Sexy::_Wave>, ...>::find -- the
// std::less<int> instantiation, which is Sexy::Level's wave table. The binary
// names it
// `_ZNSt8_Rb_treeIiSt4pairIKiN4Sexy5_WaveEESt10_Select1stIS4_ESt4lessIiESaIS4_EE4findERS1_`
// (@0x1EC12C), and its body is the same two steps as the other two: the
// lower-bound descent on the int ordering, then the equality test the descent
// cannot make. The root is mHeader.mParent. The end iterator every caller
// compares against is the header. The key sits immediately past the node base.
//
// The key sits immediately past the node base, which
// is the same offset NodeKey uses for the string-keyed family.
NodeBase* find(NodeBase* theTree, const int* theKey)
{
    NodeBase* aHeader = HeaderOf(theTree);
    NodeBase* aNode = aHeader->mParent;
    NodeBase* aResult = aHeader;
    const int aKey = *theKey;

    while (aNode != nullptr)
    {
        if (AsNode<IntNode>(aNode)->mKey < aKey)
            aNode = aNode->mRight;
        else
        {
            aResult = aNode;
            aNode = aNode->mLeft;
        }
    }

    if (aResult == aHeader || AsNode<IntNode>(aResult)->mKey > aKey)
        return aHeader;
    return aResult;
}

NodeBase* find(MapStorage* theTree, const int* theKey)
{
    NodeBase* aHeader = &theTree->mHeader;
    NodeBase* aNode = aHeader->mParent;
    NodeBase* aResult = aHeader;
    const int aKey = *theKey;

    while (aNode != nullptr)
    {
        if (AsNode<IntNode>(aNode)->mKey < aKey)
            aNode = aNode->mRight;
        else
        {
            aResult = aNode;
            aNode = aNode->mLeft;
        }
    }

    if (aResult == aHeader || AsNode<IntNode>(aResult)->mKey > aKey)
        return aHeader;
    return aResult;
}

// The pointer-keyed insert, the counterpart of the string-keyed one below. The
// value is the pointer itself, so the node's payload is a single word.
NodeBase* _M_insert_unique(NodeBase* theTree, void* const* theValue)
{
    void* aTreeAddress = theTree;
    MapStorage* aTree = static_cast<MapStorage*>(aTreeAddress);
    NodeBase* aHeader = &aTree->mHeader;
    NodeBase* aParent = aHeader;
    NodeBase* aNode = aHeader->mParent;
    bool aLeft = true;

    while (aNode != nullptr)
    {
        aParent = aNode;
        aLeft = *theValue < AsNode<PointerNode>(aNode)->mKey;
        aNode = aLeft ? aNode->mLeft : aNode->mRight;
    }

    NodeBase* aNeighbour = aParent;
    if (aLeft)
    {
        if (aParent == aHeader->mLeft)
        {
            NodeBase* aNewNode = new PointerNode();
            AsNode<PointerNode>(aNewNode)->mKey = *theValue;
            _Rb_tree_insert_and_rebalance(true, aNewNode, aParent, *aHeader);
            ++aTree->mCount;
            return aNewNode;
        }
        aNeighbour = _M_decrement(aParent);
    }
    if (*theValue < AsNode<PointerNode>(aNeighbour)->mKey)
    {
        NodeBase* aNewNode = new PointerNode();
        AsNode<PointerNode>(aNewNode)->mKey = *theValue;
        _Rb_tree_insert_and_rebalance(aLeft, aNewNode, aParent, *aHeader);
        ++aTree->mCount;
        return aNewNode;
    }
    return aNeighbour;
}

// libstdc++: _Rb_tree<_Key,...>::_M_lower_bound. The node given is the subtree
// root and the walk is `-1`/`+1` on the three-way compare: descend left past a
// key that is not smaller, right past one that is. The result is the first
// node whose key is not smaller than theKey, or nullptr when every key in the
// subtree is smaller -- find's end() case.
NodeBase* _M_lower_bound(NodeBase* theNode, const Sexy::String* theKey)
{
    const Sexy::String& aKey = *theKey;
    NodeBase* aResult = nullptr;

    while (theNode != nullptr)
    {
        int aStep = CompareStringKeys(NodeKey(theNode), aKey);
        if (aStep < 0)
            theNode = theNode->mRight;
        else
        {
            aResult = theNode;
            theNode = theNode->mLeft;
        }
    }
    return aResult;
}

NodeBase* _M_upper_bound(NodeBase* theNode, const void* /*theKey*/)
{
    return theNode;
}

NodeBase* _M_begin(NodeBase* theHeader)
{
    return theHeader->mParent;
}

NodeBase* _M_end(NodeBase* theHeader)
{
    return theHeader;
}

NodeBase* _M_leftmost(NodeBase* theNode)
{
    return _M_minimum(theNode);
}

NodeBase* _M_rightmost(NodeBase* theNode)
{
    return _M_maximum(theNode);
}

void _M_destroy_node(NodeBase* theNode)
{
    if (theNode == nullptr)
        return;
    // The stored value begins after the four link words. The callers of this
    // tree store a key and a value, both Strings, so both are destroyed. The
    // block itself is released by the caller.
    void* aAddress = theNode;
    PairNode* aPair = static_cast<PairNode*>(aAddress);
    aPair->mValue.~String();
    aPair->mKey.~String();
}
void _M_put_node(NodeBase* theNode)
{
    ::operator delete(theNode);
}

// The node: the four-word base followed by the stored value. That is exactly the
// 0x14 bytes the binary allocates at 0x145C94 (sizeof(NodeBase) +
// sizeof(Sexy::String)), and the key is copy-constructed into it (StringCopyCtor,
// which for a Sexy::String is a shared-rep AddRef).
struct StringNode : NodeBase
{
    Sexy::String mKey;

    explicit StringNode(const Sexy::String& theKey) : mKey(theKey) {}
};

NodeBase* _M_create_node(const Sexy::String* theValue)
{
    return new StringNode(*theValue);
}

// libstdc++: _Rb_tree::_M_create_node for a value_type the caller sizes. The
// block is the node base plus the whole value; the value's first word is the key
// and is copy-constructed (a String copy is an AddRef), and everything after it
// -- the mapped half of a pair -- is plain bytes.
//
// Sizing it from the caller is what the map instantiations need: a set<String>
// node is 20 bytes, but DefaultProfileMgr's map node is 52, and reading the
// mapped half at node+20 of a 20-byte block returned whatever the allocator had
// left there.
NodeBase* _M_create_node_sized(const void* theValue, size_t theValueSize)
{
    char* aBlock = static_cast<char*>(::operator new(sizeof(NodeBase) + theValueSize));
    memset(aBlock, 0, sizeof(NodeBase) + theValueSize);

    // The value is a run of words. Each word that is a String is
    // copy-constructed, so the node owns its own characters. A word that is
    // not a String (an int, a pointer) is kept as the zero the block was
    // cleared to; the caller writes it after the insert.
    const char* aSource = static_cast<const char*>(theValue);
    for (size_t aOffset = 0; aOffset + sizeof(Sexy::String) <= theValueSize;
         aOffset += sizeof(Sexy::String))
    {
        void* aWordAddress = const_cast<char*>(aSource + aOffset);
        const Sexy::String* aWord = static_cast<const Sexy::String*>(aWordAddress);
        const char* aData = aWord->mData;
        bool aIsString = aData == gEmptyStringData
            || !Sexy::String::IsNotARealBuffer(aData);
        if (!aIsString)
            continue;
        new (aBlock + sizeof(NodeBase) + aOffset) Sexy::String(*aWord);
    }
    void* aNodeAddress = aBlock;
    return static_cast<NodeBase*>(aNodeAddress);
}

} // namespace RbTree
} // namespace Sexy
