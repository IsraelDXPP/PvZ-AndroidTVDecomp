// CritSect -- reconstructed from libGameMain.so.
//
// Not polymorphic: no vtable symbol, no RTTI entry, no `virtual` anywhere.
//
// The binary embeds a bionic pthread_mutex_t here, which is one word -- the
// constructor is literally `pthread_mutex_init(this, NULL)` (@0x2FDE3C) and the
// destructor `pthread_mutex_destroy(this)` (@0x2FDE4C).
//
// TODO(recon): the Windows stand-in is a CRITICAL_SECTION, and on x86 that is 24
// bytes, which does not fit. The tree places two CritSects four bytes apart
// (SexyAppBase +132/+136, InputManager +36/+40, LogManager +0/+4); two in-place
// sections cannot both live there, and in ResourceManager a 24-byte section ran
// four bytes past the end of the enclosing 448-byte block. CritSect.cpp therefore
// heap-allocates the section on first use and keeps only the pointer in this
// word, so the member is `void *` and the object stays 4 bytes.
//
// The transcribed header also declared Enter()/Leave(). Those belong to
// PakLib::CritSect (@0x4441F8), a second class in a different namespace that
// happens to have the same shape; they are declared below rather than here.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class CritSect
{
public:
    void *mSection;   // +0x00  the CRITICAL_SECTION*, created on first use
    // Size 4.

    CritSect();
    ~CritSect();
    bool TryLock();
    int Lock();
    int Unlock();
};

} // namespace Sexy

namespace PakLib {

// PakLib::CritSect -- the archive layer's own mutex, a different class from
// Sexy::CritSect even though the two have the same four-byte layout. Its
// Enter/Leave thunks live at 0x4441F4/0x4441F8, next to PakLib::DriverFactory,
// which is what puts them in this namespace rather than in Sexy's.
class CritSect
{
public:
    void *mSection;   // +0x00  the CRITICAL_SECTION*, created on first use
    // Size 4.

    CritSect();
    ~CritSect();
    bool TryLock();
    int Lock();
    int Unlock();
    int Enter();
    int Leave();
};

} // namespace PakLib
