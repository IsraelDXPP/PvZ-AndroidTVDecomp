// DataWriter -- reconstructed from libGameMain.so.
//
// Polymorphic class with no base class: the vtable at 0x682978 holds only the
// two destructor slots, so the class declares nothing but a virtual destructor.
//
// vtable order -- this is what the binary actually holds, and `virtual` must be
// declared in this order:
//   [  0] 0x1924e4  DataWriter::~DataWriter   (complete object destructor)
//   [  1] 0x192504  DataWriter::~DataWriter   (deleting destructor)
// Itanium/ARM gives the destructor two slots; MSVC x86 gives it one, so the
// generated absolute slot numbers are one lower than the comments above.
//
// Member layout comes from the constructor at 0x192498, which writes exactly
// five words, and from Close()/EnsureCapacity()/SetLong():
//     +0x00 vptr      = &off_682978
//     +0x04 mFile     is a file stream (SDL_IOStream* in this port)
//     +0x08 mBuffer   is a uchar*  (operator new[] / operator delete[])
//     +0x0C mBufferPos   byte count written so far (this+12)
//     +0x10 mBufferSize  allocated capacity, doubled by EnsureCapacity (this+16)

#pragma once

#include "ReconTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <SDL3/SDL_iostream.h>

class DataWriter
{
public:
    virtual ~DataWriter();

    DataWriter();
    void Close();
    bool OpenFile(Sexy::String const &theFileName);
    int OpenMemory(unsigned long theCapacity);
    bool WriteToFile(Sexy::String const &theFileName);
    void EnsureCapacity(unsigned long theNeeded);
    // DataSync::SyncBytes reads this result as an int, not a file pointer.
    int WriteBytes(void const *theBytes, unsigned long theCount);
    int WriteLongLong(unsigned long long theValue);
    int WriteLong(unsigned long theValue);
    int WriteShort(unsigned short theValue);
    int WriteByte(unsigned char theValue);
    int WriteBool(bool theValue);
    int WriteFloat(float theValue);
    int WriteDouble(double theValue);
    int WriteString(Sexy::String const &theValue);
    unsigned int SetLongLong(unsigned long long theValue, unsigned long theOffset);
    unsigned int SetLong(unsigned long theValue, unsigned long theOffset);
    unsigned int SetShort(unsigned short theValue, unsigned long theOffset);
    int SetByte(unsigned char theValue, unsigned long theOffset);

    // The constructor writes the vptr and these four words. The three unread
    // padding words that used to follow them are gone.
    SDL_IOStream* mFile;     // +0x04
    unsigned char *mBuffer;  // +0x08
    unsigned long mBufferPos;  // +0x0C
    unsigned long mBufferSize; // +0x10
};
