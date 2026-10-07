// DataSync -- reconstructed from libGameMain.so.
//
// Real polymorphic class: it has a virtual destructor and no base with virtuals
// of its own, so it is a root class. _ZTV8DataSync holds only the two Itanium
// destructor slots.
//
// The member names come from the PC reference source
// (LawnProject\src\Lawn\DataSync.h), which declares the same five fields in the
// same order; the offsets are the raw `this`-relative ones DataSync.cpp uses.
// Pointer indices are stored in ordinary maps using native pointer keys.

#pragma once

#include "ReconTypes.h"
#include "DataReader.h"
#include "DataWriter.h"
#include <vector>
#include <map>

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
    void SyncFloat(float &theValue);
    void SyncDouble(double &theValue);
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
    // Save files refer to registered pointers through fixed-width integer IDs.
    std::map<void *, int> mPointerToIntMap;
    std::map<int, void *> mIntToPointerMap;

    // Address of each pointer slot SyncPointer was handed. SyncPointers writes
    // the resolved pointer back through that address.
    std::vector<void **> mPointerSyncList;
    int mCurPointerIndex;
};
