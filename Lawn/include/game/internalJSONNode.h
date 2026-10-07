// internalJSONNode -- reconstructed from libGameMain.so.
//
// No base class and no virtual function: no body in _recon/ida/internalJSONNode.cpp
// stores a vptr, and libGameMain115_rtti.c holds neither a `vtable for' nor a
// `typeinfo for'internalJSONNode entry.
//
// The class is the pimpl behind JSONNode and comes from the JSONNode/jsoncpp-era
// library the game embeds (JSONNode, internalJSONNode, jsonChildren, JSONWorker
// and the jsonSingleton* constants are all in this tree). The field names below
// are that library's own -- `_type`, `_name`, `_name_encoded`, `_string`,
// `_string_encoded`, `_value`, `refcount`, `fetched`, `_comment`, `CHILDREN` --
// and the offsets are this build's, read off the recon bodies.
//
// sizeof is 0x30, and that is measured, not guessed: all four `newInternal`
// helpers allocate exactly it -- the allocation call at 0x00134D88, 0x00134EFC,
// 0x00134F14 and 0x00134F30 passes 0x30u and is immediately followed by an
// internalJSONNode constructor call. The 0x40 the header carried before was a
// transcriber placeholder ("largest offset reached, plus alignment").
//
// Layout, from the three constructors and the destructor:
//   +0x00  _type            1 byte: 0 null, 1 string, 2 number, 3 bool, 4 array, 5 object.
//                                 Every constructor and Set() writes it, Fetch() switches on it.
//   +0x04  _name            the node's name; the (name, value) constructor fills it with
//                                 `JSONWorker::FixString(this + 4, a2, this + 8)`
//   +0x08  _name_encoded    the `bool &` out-parameter of that FixString call
//   +0x0C  _string          the string value; Set(std::string) assigns here and
//                                 FetchNumber() does `strtod(*(char **)(this + 12))`
//   +0x10  _string_encoded  the flag FetchString() receives from FixString and Write()
//                                 passes to JSONWorker::UnfixString
//   +0x18  _value           the value union: the double Set(double/float/...) stores, and
//                                 the one byte Set(bool) writes (`*((_BYTE *)this + 24) = a2`)
//   +0x20  refcount        1 in every constructor and bumped where a node is shared, e.g.
//                                 DrRpcEngine::perform's `++*(_DWORD *)(node + 32)`
//   +0x24  fetched         the "already fetched" flag Fetch() tests before doing any work
//   +0x28  _comment        the node's comment string (destroyed third in ~internalJSONNode)
//   +0x2C  CHILDREN        the jsonChildren*; null unless _type is array (4) or object (5)
// The unreferenced bytes at +0x11..+0x17, +0x21..+0x23 and +0x25..+0x27 are the
// padding the 8-byte alignment of _value needs; no body in the dump touches them.
//
// The destructor at 0x00133558 releases the child nodes and their container.

#pragma once

#include "ReconTypes.h"
// JSONNode.h and JSONWorker.h are deliberately NOT included.
//
// No declaration below needs a complete type from either. JSONNode appears only
// as `JSONNode const&` in two member declarations, and JSONWorker does not
// appear at all. ReconTypes.h already forward-declares both (lines 1384 and
// 1385), so the names resolve without the headers.
//
// jsonChildren.h stays, and the edge is cut on ITS side instead. The two
// transcribed bodies that actually dereference a jsonChildren live in
// Game\internalJSONNode.cpp and Game\JSONWorker.cpp, and neither of those
// translation units includes jsonChildren.h itself -- they reach the complete
// type through this header. Dropping the include here compiled the headers but
// broke both .cpp files with C2027 ("use of undefined type 'jsonChildren'"),
// which is exactly the transitive-dependency trap jsonChildren.h's own comment
// warns about.
//
// Together with the cut inside jsonChildren.h these edges are gone:
//   JSONNode.h -> internalJSONNode.h -> JSONNode.h
//   internalJSONNode.h -> JSONWorker.h -> internalJSONNode.h
//   JSONNode.h -> internalJSONNode.h -> JSONWorker.h -> JSONNode.h
//   internalJSONNode.h -> JSONWorker.h -> jsonChildren.h -> internalJSONNode.h
//   JSONNode.h -> internalJSONNode.h -> JSONWorker.h -> jsonChildren.h -> JSONNode.h
// JSONNode.h keeps its include of this header on purpose: Game\JSONNode.cpp
// includes "JSONNode.h" and nothing else of the four.
#include "jsonChildren.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// jsonChildren.h no longer includes this header, so this declaration is now the
// only thing standing between the CHILDREN member and an unknown name. It was
// already here: ReconTypes.h forward-declares JSONNode, JSONWorker and
// internalJSONNode but not jsonChildren, and a pointer member needs no more.
class jsonChildren;

class internalJSONNode
{
public:
    ~internalJSONNode();
    double FetchNumber();
    int Set(int theValue);
    void Set(char theValue);
    void Set(unsigned char theValue);
    void Set(short theValue);
    void Set(unsigned short theValue);
    void Set(unsigned int theValue);
    void Set(long theValue);
    void Set(unsigned long theValue);
    void Set(long long theValue);
    void Set(unsigned long long theValue);
    int FetchNode();
    int FetchArray();
    internalJSONNode(Sexy::String const& theName);
    internalJSONNode(Sexy::String const& theName, Sexy::String const& theValue);
    void FetchString();
    void Fetch();
    bool AreEqualNoCase(char const* theLeft, char const* theRight);
    void deleteInternal(internalJSONNode *theNode);
    internalJSONNode(internalJSONNode const& theOther);
    internalJSONNode * newInternal(char theType);
    internalJSONNode * newInternal(Sexy::String const& theName);
    internalJSONNode * newInternal(Sexy::String const& theName, Sexy::String const& theValue);
    internalJSONNode * newInternal(internalJSONNode const& theOther);
    void Set(long double theValue);
    void Set(double theValue);
    void Set(float theValue);
    // WriteName(bool theQuoted, bool theSkip, Sexy::String&). The first int is
    // unused; the ARM bound the receiver there.
    int WriteName(int theUnused, int theQuoted, int theSkip, Sexy::String *theOut);
    void DumpRawString(Sexy::String *theOut);
    void WriteComment(unsigned int theIndent, Sexy::String *theOut);
    void WriteChildren(unsigned char *theUnused, int theIndent, Sexy::String *theOut);
    void Write(unsigned int theIndent, int theFormatted, Sexy::String *theOut);
    int Set(bool theValue);
    int Nullify();
    int IsEqualTo(const internalJSONNode *theOther);
    internalJSONNode* push_back(JSONNode const& theChild);
    JSONNode ** push_front(JSONNode const& theChild);
    JSONNode * pop_back(unsigned int theIndex);
    JSONNode ** at(Sexy::String const& theName);
    JSONNode * pop_back(Sexy::String const& theName);
    JSONNode ** at_nocase(Sexy::String const& theName);
    JSONNode * pop_back_nocase(Sexy::String const& theName);
    void preparse();

    unsigned char _type;    // +0x00
    Sexy::String _name;     // +0x04
    bool _name_encoded;     // +0x08
    Sexy::String _string;   // +0x0C
    bool _string_encoded;   // +0x10
    // The library declares this union so that the numeric and the boolean value
    // share the slot; Set(bool) writes one byte of it at +0x18.
    union value_union_t
    {
        double _number;     // +0x18
        bool _bool;         // +0x18
    };
    value_union_t _value;   // +0x18
    unsigned int refcount;  // +0x20
    bool fetched;           // +0x24
    Sexy::String _comment;  // +0x28
    jsonChildren *CHILDREN; // +0x2C
};

// Layout check: no base class, so the whole size is the members above.
