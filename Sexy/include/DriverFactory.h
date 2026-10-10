// Sexy::DriverFactory -- reconstructed from libGameMain.so.
//
// The framework's own driver registry, and a genuinely DIFFERENT class from
// `PakLib::DriverFactory` in Game\DriverFactory.h: the two have their own
// bodies at 0x409D80 and 0x4442A4 and their own method sets. They only look
// alike because both hold a libstdc++ `std::set` of driver pointers:
//
//   std::_Rb_tree<Sexy::Driver *, Sexy::Driver *, std::_Identity<Sexy::Driver *>,
//                 Sexy::DriverCompare, std::allocator<Sexy::Driver *>>
//
// which is why every descent compares `Driver::mPriority`.
//
// SIZE 0x1C. The constructor `_ZN4Sexy13DriverFactoryC2Ev` @0x409D80 is 26
// bytes and writes exactly these fields:
//     +0x00                   never written -- the stateless comparator's word
//     +0x04 = 0               the tree header's colour and its parent/root
//     +0x08 = 0
//     +0x0C = this + 0x04     the header's left link, its own end sentinel
//     +0x10 = this + 0x04     the header's right link
//     +0x14 = 0               the element count
//     +0x18 = 1               the byte flag the destructor clears again
// The header's left/right links point at `this + 0x04`, i.e. at the header
// itself, which is what puts the tree header at +0x04 and the comparator word
// in front of it. `Sexy\ReconTypes.h`'s RbTree::MapStorage is exactly that
// six-word footprint, so it is reused verbatim here.
//
// The destructor `_ZN4Sexy13DriverFactoryD2Ev` @0x409EA4 is 18 bytes:
//     mDeleteDrivers = false;  _M_erase(mDriverMap.mHeader.mParent);
// so it frees the tree's nodes and deliberately does NOT delete the drivers.
//
// `Sexy::InputDriverFactory` derives from this class and adds nothing: its own
// constructor at 0x40A2AC is a 12-byte tail call into this one, and its
// destructor at 0x40A2B8 is the matching call into the destructor above.
//
// This header exists because Game\DriverFactory.cpp, Sexy\InputDriverFactory.cpp
// and Sexy\InputManager.cpp each carried their own file-local copy of the class
// (and the first two disagreed about its size -- one said 0x68). They now all
// include this one.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_DRIVERFACTORY_H
#define LAWNPROJECTTV_SEXY_INCLUDE_DRIVERFACTORY_H

#include "ReconTypes.h"
#include <vector>

namespace Sexy
{

class Driver;

class DriverFactory
{
public:
    // +0x00 .. +0x17: the std::set<Driver *, DriverCompare>, word by word.
    RbTree::MapStorage mDriverMap;
    bool mDeleteDrivers; // +0x18, set to 1 by the constructor
    // size 0x1C

    DriverFactory();
    ~DriverFactory();
    int AddDriver(Driver *theDriver);
    Driver *Find(String theName);
    void GetDrivers();
    std::vector<Driver *> Drivers();
    void RemoveDriver(Driver *theDriver);
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::DriverFactory;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_DRIVERFACTORY_H
