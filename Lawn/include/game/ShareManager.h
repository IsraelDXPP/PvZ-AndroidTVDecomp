// Native::ShareManager -- reconstructed from libGameMain.so.
//
// No base class and no virtual function of its own. Every body forwards to an
// imported symbol out of libnative_code.so (`__imp__ZN6Native12ShareManager...`),
// so there is no layout evidence beyond the allocation size.
//
// SIZE 0x8, from the binary's allocation sites.

#pragma once

#include "ReconTypes.h"

namespace Native {

class ShareManager
{
public:
    // ShareManager::Listener -- a NESTED class in the binary, the same shape as
    // GameCenter::Listener (Game/ShareManager.cpp already spells the __imp_
    // reference as _ZN6Native12ShareManager11setListenerEPNS0_8ListenerE). Its
    // three entry points are real functions:
    //   00537264  2 bytes  Listener::onComplete(ShareManager *, int)
    //   00537268  2 bytes  Listener::onCancel(ShareManager *, int)
    //   0053726C  2 bytes  Listener::onError(ShareManager *, int)
    // As with GameCenter the leading pointer is the SENDER; the receiver is the
    // implicit `Listener *`.
    class Listener
    {
    public:
        virtual void onComplete(ShareManager * theManager, int theCode);
        virtual void onCancel(ShareManager * theManager, int theCode);
        virtual void onError(ShareManager * theManager, int theCode);
    };

    int create();
    int setListener(Listener * theListener);
    int getName();
    int getFeatures();
    int share();
    int share(Native::ShareManager_Post const & thePost);
};

} // namespace Native

// Layout check: two unnamed words.
