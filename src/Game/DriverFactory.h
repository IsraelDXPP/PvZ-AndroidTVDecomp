// DriverFactory -- reconstructed from libGameMain.so.
//
// Plain data class: no vtable in the RTTI export, no base class.
//
// The constructors build the object and their stores fix the layout:
//   @0x4442A4 PakLib::DriverFactory::DriverFactory, @0x409D80 Sexy::DriverFactory
//     +0x04 = 0 and +0x08 = 0            the tree header's colour and parent
//     +0x0C = this+0x04, +0x10 = this+0x04   header left/right, self-sentinel
//     +0x14 = 0                          the node count
//     +0x18 = 1                          the "delete the drivers" flag
// The left/right sentinel is `this+0x04`, so the tree header starts at +0x04 and
// the map's first word is at +0x00 -- six words in total, which is the old
// libstdc++ `std::map` footprint (the tree header sits behind the comparator).
// Nothing in this build reads or writes +0x00; it is left as map storage.
//
// SIZE 0x1C, corroborated twice:
//   * the constructor stores the flag at +0x18, so the object is 0x19 bytes at
//     least and 0x1C after 4-byte alignment;
//   * PakLib::FileSystemManager::FileSystemManager @0x4001A8 constructs a
//     FileSystemDriverFactory at `this+0x14` and then starts its own map at
//     +0x30 with the same prefix-then-header pattern, leaving exactly 0x1C bytes
//     (+0x14 .. +0x2F) for the factory.

#pragma once

#include "ReconTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace PakLib {

class DriverFactory
{
public:
    // +0x00..+0x17: the PC source's `std::set<Driver *, DriverCompare> mDriverMap`.
    // The demangled helper names the instantiation in full --
    //   std::_Rb_tree<PakLib::Driver *, PakLib::Driver *, std::_Identity<PakLib::Driver *>,
    //                 PakLib::DriverCompare, std::allocator<PakLib::Driver *>>
    // -- so the storage is the tree, word by word: the stateless comparator first,
    // then the _Rb_tree_header, then the element count. See
    // Sexy\ReconTypes.h's RbTree::MapStorage.
    Sexy::RbTree::MapStorage mDriverMap; // +0x00 .. +0x17
    bool mDeleteDrivers;    // +0x18, set to 1 by the constructor
    // size 0x1C

    DriverFactory();
    ~DriverFactory();
    int AddDriver(Sexy::Driver *theDriver);
    int Find(Sexy::String theName);
    void GetDrivers();
    void RemoveDriver(Sexy::Driver *theDriver);
    int AddDriver(PakLib::Driver *theDriver);
    int Clear();
    PakLib::Driver *FindNext(PakLib::Driver *theDriver);
    void RemoveDriver(PakLib::Driver *theDriver, bool theDeleteDriver);
};

} // namespace PakLib

