// JSONNode -- the jsoncpp node handle used by the TV build.
//
// One pointer to a heap internalJSONNode. Sharing is the internal node's
// refcount: a copy bumps it, and the last handle deletes the node.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONNODE_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONNODE_H

#include "ReconTypes.h"
#include "internalJSONNode.h"

class JSONNode
{
public:
    internalJSONNode *mInternal;

    JSONNode() :
        mInternal(nullptr)
    {
    }
    JSONNode(JSONNode const &theOther);
    JSONNode &operator=(JSONNode const &theOther);
    ~JSONNode();

    JSONNode(Sexy::String const &theName, char theValue);
    JSONNode(Sexy::String const &theName, unsigned char theValue);
    JSONNode(Sexy::String const &theName, short theValue);
    JSONNode(Sexy::String const &theName, unsigned short theValue);
    JSONNode(Sexy::String const &theName, int theValue);
    JSONNode(Sexy::String const &theName, unsigned int theValue);
    JSONNode(Sexy::String const &theName, float theValue);
    JSONNode(Sexy::String const &theName, double theValue);
    JSONNode(Sexy::String const &theName, long long theValue);
    JSONNode(Sexy::String const &theName, unsigned long long theValue);
    JSONNode(Sexy::String const &theName, long double theValue);
    JSONNode(Sexy::String const &theName, Sexy::String const &theValue);
    JSONNode(Sexy::String const &theName, bool theValue);
    JSONNode(Sexy::String const &theName, char const *theValue);

    // Share this node's internal pointer. The new handle is heap-allocated
    // because the child arrays store JSONNode*.
    JSONNode *Share();
    static JSONNode *Adopt(internalJSONNode *theNode);
    // Fill a stack handle without taking a reference. The caller still owns the node,
    // and must clear mInternal before the handle goes out of scope.
    static void Bind(JSONNode &theHandle, internalJSONNode *theNode);
    // Steal this node's internal pointer and leave this handle empty.
    JSONNode *Steal();

    // Drop one reference and free this heap handle. A null handle is a no-op.
    void Release();

    // Keep the node with the higher refcount and assign the other into it.
    JSONNode *merge(JSONNode *theOther);
    JSONNode *merge(unsigned int theCount, ...);

    // Pretty-print an array or object. A scalar yields the empty JSON string.
    Sexy::String write_formatted(unsigned int theReserve) const;

    // Return a node of the requested type. An array becomes an object, and an
    // object becomes an array with every child name cleared. Anything else is
    // a fresh empty node of that type.
    JSONNode as_node() const;
    JSONNode as_array() const;

    // Deep copy when the node is shared. A unique node is returned as-is.
    JSONNode duplicate() const;
    void cast(char theType);

    // Child lookup. A missing child throws std::out_of_range. The writing forms
    // unshare the parent first. The const forms only read.
    JSONNode *at(unsigned int theIndex);
    JSONNode *at(Sexy::String const &theName);
    JSONNode *at_nocase(Sexy::String const &theName);
    JSONNode *at(unsigned int theIndex) const;
    JSONNode *at(Sexy::String const &theName) const;
    JSONNode *at_nocase(Sexy::String const &theName) const;

    // Remove the child and return a handle that owns it.
    JSONNode pop_back(unsigned int theIndex);
    JSONNode pop_back(Sexy::String const &theName);
    JSONNode pop_back_nocase(Sexy::String const &theName);

private:
    // Unshare so a later write does not change every handle of this node.
    void MakeUnique();
    static void ThrowOutOfRange();
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONNODE_H
