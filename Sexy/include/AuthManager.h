// AuthManager -- the framework's authentication object.
//
// Polymorphic with no base class. The derived CommonAuthManager overrides the
// destructor and DoAuthenticate and adds no data of its own; it is not declared
// here.
//
// Virtual order is the binary's, with the two Itanium destructor slots collapsed
// into the one MSVC emits:
//   destructor, HandleCmdLineParam, DoAuthenticate, Start, Handled.
// AuthorizeProc is the thread body Start hands to the thread, not a virtual.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_AUTHMANAGER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_AUTHMANAGER_H

#include "ReconTypes.h"
#include "Thread.h"

namespace Sexy
{

class AuthManager
{
public:
    virtual ~AuthManager();
    virtual int HandleCmdLineParam(Sexy::String const &theName, Sexy::String const &theValue);
    virtual int DoAuthenticate();
    virtual void Start();
    virtual int Handled();

    int AuthorizeProc(void *theArg);
    AuthManager(Sexy::SexyAppBase *theApp);
    Sexy::String *GetName();
    int IsAuthenticaled();
    int SetAuthId(int theId);
    int SetAuthAction(int theAction);
    int GetAuthAction();
    int SetAuthMode(int theMode);
    int GetAuthMode();
    int SetAuthState(int theState);
    int GetAuthState();
    int SetAuthSerialNumber(Sexy::String const &theSerial);
    // Returned by value: the binary builds the string into the caller's buffer.
    Sexy::String GetAuthSerialNumber();

    Sexy::SexyAppBase *mApp;
    Sexy::String mName;
    bool mAuthenticated;
    int mAuthId;
    Sexy::String mAuthSerialNumber;
    int mAuthAction;
    int mAuthMode;
    int mAuthState;
    TM::Thread mThread;
};

} // namespace Sexy

using Sexy::AuthManager;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_AUTHMANAGER_H
