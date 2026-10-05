// Sexy::InputDriver -- reconstructed from libGameMain.so.
//
// The abstract base every platform input driver derives from (AndroidInputDriver,
// UdpInputDriver, CISInputDriver, LinuxInputDriver, ...). It derives from
// Sexy::Driver, whose two-slot vtable (the Itanium D1/D0 destructor pair) it
// extends, and it is the type `InputManager::Cleanup` walks the driver list with:
// the call at 0x3B7112 reads the driver's vptr+16, which is slot 4 of this table
// -- `OnStop()`.
//
// vtable order -- _ZTVN4Sexy11InputDriverE @0x68D848, five slots after the
// Itanium header. MSVC x86 gives a virtual destructor ONE slot where Itanium
// gives two, so the declared order below is one entry shorter on purpose and the
// RELATIVE order of everything else is unchanged:
//   [ 0] 0x409d10  InputDriver::~InputDriver
//   [ 1] 0x409d30  InputDriver::~InputDriver        (the D0 half)
//   [ 2] __cxa_pure_virtual                          pure: the per-platform Start
//   [ 3] 0x409d50  InputDriver::OnStart(SexyAppBase *, InputManager *)
//   [ 4] 0x409d70  InputDriver::OnStop()
// Slot 2 is pure in the binary, so this class cannot be instantiated -- which is
// correct, nothing ever does.
//
// Size 0x10: the base Driver is 0x10 (vptr + a one-word String + two words, see
// Driver.h) and this class adds no member of its own.

#pragma once

#include "ReconTypes.h"
#include "Driver.h"

namespace Sexy {

class SexyAppBase;
class InputManager;

class InputDriver : public Driver
{
public:
    InputDriver(Sexy::String theName, int thePriority);
    virtual ~InputDriver();

    // The per-platform hook the concrete drivers implement. Init builds the
    // IInputDriver the manager then owns; a null result skips the driver.
    virtual IInputDriver *Init() = 0;                           // [ 2] pure
    virtual int OnStart(Sexy::SexyAppBase *theApp,
                        Sexy::InputManager *theManager);        // [ 3] 0x409d50
    virtual int OnStop();                                       // [ 4] 0x409d70
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::InputDriver;
