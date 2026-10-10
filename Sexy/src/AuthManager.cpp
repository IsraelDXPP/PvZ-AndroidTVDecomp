// AuthManager accepts a serial from the command line and marks the session
// authenticated. Start either authenticates on this thread or joins a worker
// that does the same. The setters store the value callers already passed.

#include "AuthManager.h"
#include "SexyAppBase.h"

namespace
{

void AuthorizeProcThunk(void *theArg)
{
    static_cast<Sexy::AuthManager *>(theArg)->AuthorizeProc(theArg);
}

} // namespace

int Sexy::AuthManager::DoAuthenticate()
{
    mAuthState = 2;
    mAuthenticated = true;
    return 1;
}

int Sexy::AuthManager::AuthorizeProc(void *theArg)
{
    (void)theArg;
    return DoAuthenticate();
}

int Sexy::AuthManager::Handled()
{
    if (mAuthState != 3)
        mAuthState = 3;
    return 1;
}

int Sexy::AuthManager::HandleCmdLineParam(Sexy::String const &theName, Sexy::String const &theValue)
{
    if (theName.compare("-sn") != 0)
        return 0;
    mAuthSerialNumber = theValue;
    return 1;
}

void Sexy::AuthManager::Start()
{
    if (mAuthState > 0)
        return;

    if (mAuthMode != 0)
    {
        mAuthState = 1;
        TM::Thread::Create(&mThread, AuthorizeProcThunk, this);
        return;
    }

    DoAuthenticate();
    mAuthState = 2;
}

Sexy::AuthManager::AuthManager(Sexy::SexyAppBase *theApp) :
    mApp(theApp),
    mAuthenticated(true),
    mAuthId(-1),
    mAuthAction(2),
    mAuthMode(1),
    mAuthState(0)
{
    mName = "Default";
}

Sexy::AuthManager::~AuthManager()
{
    mThread.Join();
}

Sexy::String *Sexy::AuthManager::GetName()
{
    return &mName;
}

int Sexy::AuthManager::IsAuthenticaled()
{
    return mAuthenticated ? 1 : 0;
}

int Sexy::AuthManager::SetAuthId(int theId)
{
    mAuthId = theId;
    return theId;
}

int Sexy::AuthManager::SetAuthAction(int theAction)
{
    mAuthAction = theAction;
    return theAction;
}

int Sexy::AuthManager::GetAuthAction()
{
    return mAuthAction;
}

int Sexy::AuthManager::SetAuthMode(int theMode)
{
    mAuthMode = theMode;
    return theMode;
}

int Sexy::AuthManager::GetAuthMode()
{
    return mAuthMode;
}

int Sexy::AuthManager::SetAuthState(int theState)
{
    mAuthState = theState;
    return theState;
}

int Sexy::AuthManager::GetAuthState()
{
    return mAuthState;
}

int Sexy::AuthManager::SetAuthSerialNumber(Sexy::String const &theSerial)
{
    mAuthSerialNumber = theSerial;
    return 1;
}

Sexy::String Sexy::AuthManager::GetAuthSerialNumber()
{
    return mAuthSerialNumber;
}
