// BridgeApp -- reconstructed from libGameMain.so.
//
// Layout from the authoritative header PvZ\Android\Native\BridgeApp.h, and from
// the one body that exists for it (_recon\ida\BridgeApp.cpp): a single thunk,
// `Native::BridgeApp::getSingleton` @0x12E8E0, which ADRLs the GOT slot of the
// imported symbol `_ZN6Native9BridgeApp12getSingletonEv` and tail-calls it.
// Because nothing is passed but the GOT slot, getSingleton takes no receiver --
// it is a static member, which is how the authoritative header declares it.
//
// No vtable and no RTTI record exist for Native::BridgeApp in the binary, so
// the class is NOT polymorphic here and its first word is not a C++ vptr. The
// authoritative header names that word `int *mVtable`, which is where the
// Android side's own dispatch table would sit; it is modelled as one opaque
// pointer because this reconstruction cannot type it.
// TODO(recon): `mVtable` is a name inherited from the hook project, not a C++
// vptr in this build. Confirm what writes it before phase 2 relies on it.

#pragma once

#include "ReconTypes.h"

namespace Native {

class NativeApp;

class BridgeApp
{
public:
    void *mVtable;          // +0x00
    NativeApp *mNativeApp;  // +0x04
    // size 0x08

    static BridgeApp *getSingleton();
};

} // namespace Native

// Layout check: two pointers.
