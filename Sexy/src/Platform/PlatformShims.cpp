// PlatformShims.cpp -- the last ten platform imports the linker could not find.
//
// Two unrelated groups, both of them platform code that this port replaces
// rather than reproduces. The project's rule for that layer is to substitute an
// equivalent or a placeholder, so these are placeholders, marked as such.
//
// --- libcurl ---------------------------------------------------------------
//
// DrRpcEngine is the game's RPC client and it URL-encodes its arguments with
// libcurl's curl_escape / curl_unescape / curl_free. libcurl is not a
// dependency of this project (see vcpkg.json) and the RPC path is not needed to
// start the game, so the four entry points are stubbed rather than pulling in a
// network stack.
//
// The declarations in game/DrRpcEngine.cpp are at file scope with C linkage,
// which is why the linker asks for the undecorated `_curl_*` names.
//
// --- Android notification JNI ---------------------------------------------
//
// Native notification services remain unavailable without the original Java bridge.
// RPC and notifications retain the existing unavailable-service behavior.

// ---------------------------------------------------------------------------
// libcurl
// ---------------------------------------------------------------------------

extern "C" int curl_global_init(int theFlags)
{
    (void)theFlags;
    return 0; // CURLE_OK; nothing to initialise
}

// TODO(recon): percent-encodes the argument. Returning null means "failed",
// which every caller already handles -- the transcribed bodies test the result
// before using it.
extern "C" char *curl_escape(const char *theText)
{
    (void)theText;
    return nullptr;
}

extern "C" char *curl_unescape(const char *theText, int theLength)
{
    (void)theText;
    (void)theLength;
    return nullptr;
}

extern "C" void curl_free(void *thePointer)
{
    (void)thePointer;
}

// ---------------------------------------------------------------------------
// Android notification JNI
// ---------------------------------------------------------------------------

extern "C" int NativeNotificationManagerCreate(void *theObject)
{
    (void)theObject;
    return 0; // "not available" -- callers take the no-notification path
}

extern "C" int NativeNotificationManagerGetFeatures(void *theObject)
{
    (void)theObject;
    return 0; // no capabilities, so every feature test reads unsupported
}

extern "C" int NativeNotificationManagerGetName(void *theObject)
{
    (void)theObject;
    return 0;
}

extern "C" int NativeNotificationManagerSetAlias()
{
    return 0; // not set
}

extern "C" int NativeNotificationManagerSetTags()
{
    return 0; // not set
}

// Returns 1 rather than 0: nothing was ever posted, so a cancellation succeeds
// trivially, and a failure code would make the caller retry an operation that
// cannot be performed here.
extern "C" int NativeNotificationManagerCancelAllNotifications(void *theObject)
{
    (void)theObject;
    return 1;
}
