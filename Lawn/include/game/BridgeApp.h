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
// Android header names that word as a dispatch-table pointer. This build keeps
// the word, under the name of that platform table, and does not dispatch through it.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_BRIDGEAPP_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_BRIDGEAPP_H

#include "ReconTypes.h"

namespace Native
{

class NativeApp;

class BridgeApp
{
public:
    void *mPlatformDispatch; // +0x00  Android dispatch table, unused as a C++ vptr
    NativeApp *mNativeApp; // +0x04
    // size 0x08

    static BridgeApp *getSingleton();
};

} // namespace Native

// Layout check: two pointers.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_BRIDGEAPP_H
