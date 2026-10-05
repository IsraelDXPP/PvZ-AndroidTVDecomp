// DataSync -- reconstructed from libGameMain.so.
//
// Real polymorphic class: it has a virtual destructor and no base with virtuals
// of its own, so it is a root class. _ZTV8DataSync holds only the two Itanium
// destructor slots.
//
// The member names come from the PC reference source
// (LawnProject\src\Lawn\DataSync.h), which declares the same five fields in the
// same order; the offsets are the raw `this`-relative ones DataSync.cpp uses.
// The two pointer maps are modelled with Sexy\ReconTypes.h's MapStorage, which
// is word for word the old libstdc++ std::map layout DataSync.cpp walks.

#pragma once

#include "ReconTypes.h"
#include "DataReader.h"
#include "DataWriter.h"
#include <vector>

class DataSync
{
public:
    virtual ~DataSync();

    int SyncBytes(void *theData, unsigned long theDataLen);
    int SyncLongLong(int &theNum);
    unsigned int SyncLongLong(unsigned long &theNum);
    int SyncLongLong(unsigned short &theNum);
    int SyncLongLong(unsigned char &theNum);
    int SyncLongLong(long long &theNum);
    int SyncLongLong(long &theNum);
    int SyncLongLong(short &theNum);
    int SyncLongLong(char &theNum);
    int SyncSLongLong(int &theNum);
    unsigned int SyncSLongLong(unsigned long &theNum);
    int SyncSLongLong(unsigned short &theNum);
    int SyncSLongLong(unsigned char &theNum);
    int SyncSLongLong(long &theNum);
    int SyncSLongLong(short &theNum);
    int SyncSLongLong(char &theNum);
    int SyncLong(int &theNum);
    int SyncLong(unsigned long &theNum);
    int SyncLong(unsigned short &theNum);
    int SyncLong(unsigned char &theNum);
    int SyncLong(long long &theNum);
    int SyncLong(long &theNum);
    int SyncLong(short &theNum);
    int SyncLong(char &theNum);
    int SyncSLong(int &theNum);
    int SyncSLong(unsigned long &theNum);
    int SyncSLong(unsigned short &theNum);
    int SyncSLong(unsigned char &theNum);
    int SyncSLong(long &theNum);
    int SyncSLong(short &theNum);
    int SyncSLong(char &theNum);
    int SyncShort(int &theNum);
    int SyncShort(unsigned long &theNum);
    int SyncShort(unsigned short &theNum);
    int SyncShort(unsigned char &theNum);
    int SyncShort(long long &theNum);
    int SyncShort(long &theNum);
    int SyncShort(short &theNum);
    int SyncShort(char &theNum);
    int SyncSShort(int &theNum);
    int SyncSShort(unsigned long &theNum);
    int SyncSShort(unsigned short &theNum);
    int SyncSShort(unsigned char &theNum);
    int SyncSShort(long &theNum);
    int SyncSShort(short &theNum);
    int SyncSShort(char &theNum);
    int SyncByte(int &theNum);
    int SyncByte(unsigned long &theNum);
    int SyncByte(unsigned short &theNum);
    int SyncByte(unsigned char &theNum);
    int SyncByte(long long &theNum);
    int SyncByte(long &theNum);
    int SyncByte(short &theNum);
    int SyncByte(char &theNum);
    int SyncSByte(int &theNum);
    int SyncSByte(unsigned long &theNum);
    int SyncSByte(unsigned short &theNum);
    int SyncSByte(unsigned char &theNum);
    int SyncSByte(long &theNum);
    int SyncSByte(short &theNum);
    int SyncSByte(char &theNum);
    int SyncBool(bool &theValue);
    int SyncFloat(float &theValue);
    int SyncDouble(double &theValue);
    int SyncString(Sexy::String &theString);
    int SyncPointer(void **theSlot);
    int ResetPointerTable();
    int Reset();
    DataSync();
    DataSync(DataReader &theReader);
    DataSync(DataWriter &theWriter);
    int SetReader(DataReader &theReader);
    int SetWriter(DataWriter &theWriter);
    int SyncPointers();
    int RegisterPointer(void *thePointer);

    // -----------------------------------------------------------------------
    // Members. Offsets are absolute byte offsets from the start of the object.
    // -----------------------------------------------------------------------
    DataReader *mReader;      // +0x04, PC: DataReader* mReader
    DataWriter *mWriter;      // +0x08, PC: DataWriter* mWriter
    int mVersion;             // +0x0C, PC: int mVersion
    // The two pointer maps, in libstdc++'s layout: the stateless comparator
    // first, then the _Rb_tree_header (whose mParent is the root and whose own
    // address is end()) and the node count. MapStorage is the same 24-byte
    // block Sexy\ReconTypes.h models for every other tree in this build, and
    // the two of them together are the 0x30 bytes the transcription left as a
    // bare array. The helpers DataSync.cpp calls carry the instantiation in
    // their mangled names, which fixes which is which:
    //   +0x10  std::map<void *, int>  _Rb_tree<void *, pair<void * const, int>>
    //   +0x28  std::map<int, void *>  _Rb_tree<int, pair<int const, void *>>
    Sexy::RbTree::MapStorage mPointerToIntMap;   // +0x10, PC: mPointerToIntMap
    Sexy::RbTree::MapStorage mIntToPointerMap;   // +0x28, PC: mIntToPointerMap

    // A node of either tree. libstdc++'s _Rb_tree_node is the 16-byte link
    // block followed by the pair the tree stores, so the key is at +0x10 and
    // the value at +0x14 whichever way round the map is.
    struct PointerToIntNode : Sexy::RbTree::NodeBase
    {
        void *mKey;      // +0x10
        int   mValue;    // +0x14
    };
    struct IntToPointerNode : Sexy::RbTree::NodeBase
    {
        int   mKey;      // +0x10
        void *mValue;    // +0x14
    };

    // Address of each pointer slot SyncPointer was handed. SyncPointers writes
    // the resolved pointer back through that address.
    std::vector<void **> mPointerSyncList;
    int mCurPointerIndex;
};

