// NativeApp -- reconstructed from libGameMain.so.
//
// NO DATA MEMBERS. The authoritative header PvZ\Android\Native\NativeApp.h
// declares this class with a completely empty body -- three inline methods that
// tail-call entries of libnative_code and nothing else:
//
//     class NativeApp {
//     public:
//         jobject getActivity()      { ... }
//         jobject getView()          { ... }
//         char   *getPackageName()   { ... }
//     };
//
// The game never allocates one: there is no `operator new` for it anywhere in
// _recon/ida, no vtable and no RTTI record. Native::BridgeApp::getSingleton()
// hands out a pointer into libnative_code and NativeApp is only ever called
// through, so libGameMain carries the declaration, not the storage.
//
// The transcriber's `mObject[0x8]` was its default placeholder size, not a
// measurement: 0x8 is the same figure it left on PIBlocker, PIDeflector,
// PIValue2D, PixelData and SWHelper, every one of which turned out to have a
// different, recoverable extent. Here the evidence says the extent is "no state
// at all", so the class is left empty and the assertion below pins that.
//
// TODO(recon): the three methods are `const` in the binary -- the mangled names
// are `_ZNK6Native9NativeApp13getGameCenterEv`,
// `_ZNK6Native9NativeApp15getShareManagerEv` and
// `_ZNK6Native9NativeApp22getNotificationManagerEv` -- and the authoritative
// header returns `jobject` / `char *`, not `int`. Neither is reproduced here
// because Game\NativeApp.cpp still defines the transcribed spellings; phase 2
// has to change both sides together. The authoritative header also names
// getActivity / getView / getPackageName, which live in libnative_code and have
// no thunk in libGameMain.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_NATIVEAPP_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_NATIVEAPP_H

#include "ReconTypes.h"

namespace Native
{

class NativeApp
{
public:
    int getGameCenter();
    int getShareManager();
    int getNotificationManager();
};

} // namespace Native

// Layout check: no state, so the class is one byte as every empty class is.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_NATIVEAPP_H
