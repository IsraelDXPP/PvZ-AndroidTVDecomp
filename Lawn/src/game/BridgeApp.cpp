// BridgeApp -- transcribed from libGameMain.so
//
// Bodies are the IDA decompilation with syntactic changes only;
// see the tool's docstring for the complete, justified list.

#include "BridgeApp.h"

// ==========================================================================
// The import behind Native::BridgeApp::getSingleton
// ==========================================================================
// The binary's thunk at 0x12E8E0 ADRLs the GOT slot of
// `_ZN6Native9BridgeApp12getSingletonEv` and tail-calls it. That symbol is
// resolved at load time out of libnative_code.so -- the Java side of the TV
// build -- so it is an IMPORT, not code in libGameMain.so: there is no ARM body
// for it anywhere in the image.
//
// On Windows there is no such library and no equivalent object to return. The
// definition below is therefore the honest one for a platform that has no
// bridge: it answers null, which is the value every caller of a singleton
// getter has to handle anyway. It is deliberately NOT a plausible-looking fake
// -- inventing a BridgeApp would make the bridge appear to work when nothing
// backs it.
//
// Shape: the thunk passes nothing in r0 and the authoritative
// PvZ\Android\Native\BridgeApp.h declares `static BridgeApp *getSingleton()`, so
// the import takes no arguments and returns the pointer. (A stub taking a
// receiver and returning int would mangle to a different symbol entirely -- that
// mistake is what left this undefined for several rounds; see the note in
// Sexy\ReconFuncs.cpp.)
Native::BridgeApp *__imp__ZN6Native9BridgeApp12getSingletonEv()
{
  return nullptr;
}

// ==========================================================================
// Native::BridgeApp::getSingleton(void)
// @0012E8E0  12 bytes
// ==========================================================================
Native::BridgeApp *Native::BridgeApp::getSingleton()
{
  return __imp__ZN6Native9BridgeApp12getSingletonEv();
}
