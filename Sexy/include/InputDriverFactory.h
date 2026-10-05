// InputDriverFactory -- reconstructed from libGameMain.so.
//
// Polymorphic? No -- neither this class nor its base has a vtable, and it has no
// virtual function.
//
// Real chain: `class InputDriverFactory : public Sexy::DriverFactory`. The
// constructor at 0x40a2ac is 12 bytes and is nothing but a tail call into
// `_ZN4Sexy13DriverFactoryC2Ev` at 0x409d80, and the destructor at 0x40a2b8 is
// the matching `_ZN4Sexy13DriverFactoryD2Ev` call. The class adds no member of
// its own: GetInputDriverFactory (0x40a2e4) allocates it with
// a 0x1C-byte allocation (0x40a2f8), which is exactly the base's size, so the
// whole object is the base sub-object.
//
// The previous placeholder was the opaque storage array of the right size, which
// named nothing.
//
// The chain is `class InputDriverFactory : public Sexy::DriverFactory`, and the
// class adds no member of its own -- the whole object IS the base sub-object.
// `Sexy\DriverFactory.h` now declares the base, so the chain is spelled out
// below; the seven raw words this header used to carry were the base's fields.

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "DriverFactory.h"

namespace Sexy {

class InputDriverFactory : public DriverFactory
{
public:
    InputDriverFactory();
    ~InputDriverFactory();
    // STATIC in the binary: the mangled name carries no receiver and the body
    // never reads one -- it allocates the singleton on first use and returns it.
    static InputDriverFactory* GetInputDriverFactory();
    void Load();
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::InputDriverFactory;
