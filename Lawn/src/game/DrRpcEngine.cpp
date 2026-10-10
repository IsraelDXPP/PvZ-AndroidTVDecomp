// DrRpcEngine -- reconstructed from libGameMain.so.
//
// Bodies rewritten from the IDA decompilation. Two shapes need explaining:
//
// * Hidden result records. Every call_* and parseResponse returns its result
//   through the ARM sret pointer, so inside those bodies the dump's `this` is
//   the CALLER'S record and the engine arrives as a separate argument (see
//   parseResponseBody below). The tree declares none of the DrRpcRes* records,
//   so those bodies alias the receiver (`_BYTE *aResult = (_BYTE *)this`) and
//   name each field in a comment rather than inventing a layout.
//
// * Member offsets. The engine's own members start at +0x204 (the binary's
//   DrCurlEngine base is 0x204 bytes); the dump spells them
//   `*((_DWORD *)this + 129)` .. `+132`, which is mDefaultUrl, mDataEncoder,
//   mDataEncryption and mOemTarget.

#include "DrRpcEngine.h"
#include "SexyString.h"
#include "JSONNode.h"
#include "JSONWorker.h"
#include "ReconFreeFunctions.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <stdexcept>

// Function-local static guards. Each is one int: the low byte is
// "initialised" and the next is "in progress", which is what the
// transcribed tests and CxaGuardAcquire/CxaGuardRelease use.
static int gGuard_error_4 = 0;
// Every failed RPC reports 255. The binary stored it in a once-guarded static.
static const int kRpcFailed = 255;
static Sexy::String gEmptyRpcString;
static int gGuard_single_3 = 0;

// Linker symbols IDA printed verbatim; each becomes a stub whose
// address is what the original code compares or reads.
static int gGuardVar_1 = 0;
static int gGuardVar_2 = 0;
static int gGuardVar_3 = 0;
static int gGuardVar_4 = 0;
static int gGuardVar_5 = 0;
static int gGuardVar_6 = 0;
static int gGuardVar_7 = 0;
static int gGuardVar_8 = 0;
static int gGuardVar_9 = 0;

// ---------------------------------------------------------------------------
// Symbols the RPC engine reaches that no header declares yet.
// ---------------------------------------------------------------------------

// TODO(recon): the engine drives libcurl directly and libcurl is not among the
// tree's dependencies (vcpkg_installed carries no curl headers). These are the
// four entry points the dump calls, with the shapes it prints:
// `curl_global_init(3)` (CURL_GLOBAL_ALL), `curl_escape(text)` -- libcurl's own
// signature also takes the length, which this call site does not pass --
// `curl_unescape(text, length)` and `curl_free(ptr)`. Their definitions belong
// with game/DrCurlEngine or in whatever curl target the build links.
extern "C"
{
    int curl_global_init(int theFlags);
    char *curl_escape(const char *theText);
    char *curl_unescape(const char *theText, int theLength);
    void curl_free(void *thePointer);
}

namespace logger
{
struct Clog
{
};
static Clog debug;
}

static logger::Clog *logStream(logger::Clog *theLogger, const char *theText)
{
    if (theText != nullptr)
        logfi("%s", theText);
    return theLogger;
}

static logger::Clog *logStream(logger::Clog *theLogger, const void *theObject)
{
    if (theObject != nullptr)
        logfi("%p", theObject);
    return theLogger;
}

// TODO(recon): `RegisterAtExit` registers the destructor of the function-local
// DrRpcEngine static with the runtime, and C++ cannot take the address of a
// destructor; the registration goes through this inert thunk, the same shape
// game/JSONNode.cpp and game/JSONWorker.cpp use for their JSON singletons.
static void drRpcEngineDtorThunk(DrRpcEngine *theObject)
{
    theObject->~DrRpcEngine();
}

// TODO(recon): every call_* body registers the destructor of its own `DrRpcRes*`
// function-local static (`DrRpcResCommon::error(void)::error` and siblings) with
// RegisterAtExit, and no header declares those classes. The registrations go through
// this inert thunk; a header should carry the DrRpcRes* classes so the real
// destructors can be named.
static void drRpcResDtorThunk(void *theObject)
{
    (void)theObject;
}

// The dump calls parseResponse with the result record first and the response
// text last (`DrRpcEngine::parseResponse(&result, this, &text)`), which is
// exactly the ARM sret shape the header now models: r0 is the DrRpcRes the
// function returns by value and r1 is the engine. No forwarder is needed.

// TODO(recon): the dump's `internalJSONNode::operator long long(word)` is the
// node's numeric conversion and internalJSONNode.h declares no conversion
// operator. Only the low word of the result is ever stored (the result struct's
// second field), so this returns the word sign-extended; the header should carry
// the conversion.
static long long internalJSONNode_QWORD(int theWord)
{
    return (long long)theWord;
}

// ==========================================================================
// DrRpcEngine::DrRpcEngine(void)
// @001EDEEC  146 bytes
// ==========================================================================
DrRpcRes DrRpcEngine::gFailedResult;

DrRpcEngine::DrRpcEngine() :
    mDataEncoder(Sexy::String("97ac4289fcfc1bbea07bccc16f5fa35f"))
{
    // The DrCurlEngine base and the DrDataEncoder member are built by the
    // compiler now, so the vptr store and the placement-new the dump shows are
    // gone.
    mDefaultUrl = "http://192.168.1.9/MobileGameServer/server/index.php";
    mOemTarget = "";
    mDataEncryption = false;
}

// ==========================================================================
// DrRpcEngine::getRpcEngine(void)
// @001EDFD4  186 bytes
// ==========================================================================
void *DrRpcEngine::getRpcEngine()
{
    // Meyers singleton. The binary keeps the object at 0x69F9D0, the
    // once-guard at 0x69FBE4 and the init result at 0x69FBE8.
    static DrRpcEngine gEngine;
    static int gInitState = 0;

    if (gInitState == 0)
    {
        logger::Clog *aLog = logStream(&logger::debug, "Initializing RPC engine...");
        logStream(aLog, (const char *)nullptr);

        curl_global_init(3);

        // 1 when the transport came up, -1 when it did not.
        if (gEngine.init() != 0)
            gInitState = 1;
        else
            gInitState = -1;

        aLog = logStream(&logger::debug, "RPC engine initialized ");
        const char *aOutcome = "successfully";
        if (gInitState == 0)
            aOutcome = "with failures";
        aLog = logStream(aLog, aOutcome);
        aLog = logStream(aLog, ".");
        logStream(aLog, (const char *)nullptr);
    }

    if (gInitState <= 0)
        return nullptr;
    return &gEngine;
}

// ==========================================================================
// DrRpcEngine::setDefaultUrl(Sexy::String const&)
// @001EE0DC  8 bytes
// ==========================================================================
// `ADD.W R0, R0, #0x204 / B.W AssignStringThunk` -- the body is the String assignment
// inlined into a tail call, so the int result is the transcriber's.
int DrRpcEngine::setDefaultUrl(Sexy::String const &theUrl)
{
    AssignStringThunk(&mDefaultUrl, &theUrl);
    return 1;
}

// ==========================================================================
// DrRpcEngine::setDataEncryption(bool)
// @001EE0E4  6 bytes
// ==========================================================================
// `STRB.W R1, [R0,#0x20C] / BX LR` -- a plain void store, so the dump's
// "return this" is a scratch word and is dropped.
void DrRpcEngine::setDataEncryption(bool theEnabled)
{
    mDataEncryption = theEnabled;
}

// ==========================================================================
// DrRpcEngine::escapeUrl(Sexy::String const&)
// @001EE0EC  128 bytes
// ==========================================================================
// Returns a Sexy::String BY VALUE, so ARM passes its destination in r0 and the
// argument in r1 -- which is why the dump reads its only argument as `R1` and
// never touches a receiver. The body builds the escaped text in a local and
// moves it into the result with the String move-assignment.
Sexy::String DrRpcEngine::escapeUrl(Sexy::String const &theUrl)
{
    // curl_escape returns a malloc'd buffer that curl_free releases.
    char *aEscaped = curl_escape(theUrl.c_str());
    Sexy::String aResult;
    if (aEscaped != nullptr)
    {
        aResult = aEscaped;
        curl_free(aEscaped);
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::unescapeUrl(Sexy::String const&)
// @001EE184  128 bytes
// ==========================================================================
// The same by-value return shape as escapeUrl. curl_unescape's length argument
// is the string rep's length word at data - 12.
Sexy::String DrRpcEngine::unescapeUrl(Sexy::String const &theUrl)
{
    char *aUnescaped = curl_unescape(theUrl.c_str(), static_cast<int>(theUrl.size()));
    Sexy::String aResult;
    if (aUnescaped != nullptr)
    {
        aResult = aUnescaped;
        curl_free(aUnescaped);
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::parseResponse(Sexy::String const&)
// @001EE21C  686 bytes
// ==========================================================================
// Returns a DrRpcRes BY VALUE: r0 is the caller's record.
DrRpcRes DrRpcEngine::parseResponse(Sexy::String const &theBody)
{
    // parse() writes the node into the receiver's first word. The slot is a
    // JSONNode; that is the only word the rest of this function reads back.
    JSONWorker aRoot;
    aRoot.parse(theBody);

    logger::Clog *aLog = logStream(&logger::debug, "\n");

    {
        Sexy::String aDumped;
        // _type 4 is an array and 5 an object. (type - 4) <= 1 is that pair.
        if ((unsigned char)(aRoot.mNode.mInternal->_type - 4) <= 1u)
        {
            Sexy::String aFormatted;
            aFormatted.reserve(2048);
            aRoot.mNode.mInternal->Write(0, 1, &aFormatted);
            aDumped = aFormatted;
        }
        else
        {
            // A scalar response has no formatted body. The dump uses the process-wide
            // empty JSON string; this translation unit already has that empty string.
            aDumped = gEmptyRpcString;
        }
        aLog = logStream(aLog, aDumped.c_str());
        logStream(aLog, static_cast<const char *>(nullptr));
    }

    // at() is declared to return int and to take the name as an internal node.
    // The binary returns the child JSONNode* (mInternal at +0) and reads the
    // name as a String. The engine argument is unused.
    JSONNode *aIRetNode;
    {
        Sexy::String aKey("iRet");
        aIRetNode = aRoot.mNode.at(aKey);
    }
    JSONNode *aMsgNode;
    {
        Sexy::String aKey("sRetMsg");
        aMsgNode = aRoot.mNode.at(aKey);
    }
    JSONNode *aListNode;
    {
        Sexy::String aKey("list");
        aListNode = aRoot.mNode.at(aKey);
    }

    // operator long long @0x52A218. Only the low word (R0) is stored.
    internalJSONNode *aIRet = aIRetNode->mInternal;
    aIRet->Fetch();
    long long aStatusWide;
    unsigned char aType = aIRet->_type;
    if (aType == 0)
        aStatusWide = 0;
    else if (aType == 3)
        aStatusWide = aIRet->_value._bool ? 1 : 0;
    else
    {
        if (aType == 1)
            aIRet->FetchNumber();
        aStatusWide = (long long)aIRet->_value._number;
    }

    internalJSONNode *aMsg = aMsgNode->mInternal;
    aMsg->Fetch();

    DrRpcRes aResult;
    {
        Sexy::String aRetMsg = aMsg->_string;
        JSONNode aList;
        aList = aListNode->as_array();

        aResult.mRet = 1;
        aResult.mStatus = (int)aStatusWide;
        aResult.mRetMsg = aRetMsg;
        aResult.mList = aList.mInternal;
        // This handle does not own a counted reference. Delete only when the
        // node was never shared; the pointer is stored either way.
        if (aResult.mList->refcount == 0)
            aResult.mList->deleteInternal(nullptr);
    }

    if (aRoot.mNode.mInternal != nullptr)
    {
        if (--aRoot.mNode.mInternal->refcount == 0)
            aRoot.mNode.mInternal->deleteInternal(nullptr);
        aRoot.mNode.mInternal = nullptr;
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::decodeResponseString(Sexy::String &)
// @001EE650  104 bytes
// ==========================================================================
// A real member: r0 is the engine and r1 the destination String. The result is
// whether the decoded text is non-empty, which is the string rep's length word.
bool DrRpcEngine::decodeResponseString(Sexy::String &theBody)
{
    if (mDataEncryption)
    {
        theBody = mDataEncoder.decode(theBody);
    }
    return !theBody.empty();
}

// ==========================================================================
// DrRpcEngine::encodeRequesArgs(DrCurlRequest const&)
// @001EE6D0  798 bytes
// ==========================================================================
// Returns a Sexy::String BY VALUE, so ARM passes its destination in r0 and the
// engine in r1 -- the dump's `this` is the RESULT and its `a2` is the engine,
// which is why the encryption flag and the encoder are read at +0x20C and +0x208
// off `a2`. Verified against the disasm: the request is r2. Encryption on
// stores the sorted fields, logs that text, then replaces the result with
// "sig=" + encode(those) and then "action=" + getField("action") + "&" + that
// text. Encryption off stores the post fields.
Sexy::String DrRpcEngine::encodeRequesArgs(DrCurlRequest const &theRequest)
{
    // Seeded with the shared empty rep (Sexy::gEmptyStringData) before the branch.
    Sexy::String aResult;

    if (mDataEncryption)
    {
        {
            aResult = theRequest.getSortedFields();
        }

        // operator<<(char const*) then operator<<(string const&) of the result
        // (r4 holds the sorted fields, not the engine), then endl.
        logger::Clog *aLog = logStream(&logger::debug, "args: ");
        aLog = logStream(aLog, aResult.c_str());
        const char *aEndl = nullptr;
        logStream(aLog, aEndl);

        {
            Sexy::String aSigPrefix("sig=");
            Sexy::String aEncoded = mDataEncoder.encode(aResult);
            aResult = aSigPrefix + aEncoded;
        }

        {
            Sexy::String aActionEq("action=");
            Sexy::String aActionKey("action");
            Sexy::String aField = theRequest.getField(aActionKey);
            // operator+(string, "&") is the copy plus append("&", 1).
            aResult = aActionEq + aField + "&" + aResult;
        }
    }
    else
    {
        aResult = theRequest.getPostFields();
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::perform(DrCurlRequest const&)
// @001EEAC0  750 bytes
// ==========================================================================
// Returns a DrRpcRes BY VALUE, so r0 is the caller's record and r1 the engine.
// The dump's `this` is that record: words 0..3 are mRet, mStatus, mRetMsg and
// mList. The engine is what `encodeRequesArgs` and `decodeResponseString` are
// called on. The retry loop runs the transport twice. Giving up, and a body
// with no '{', both copy the failure record that begins at kNotDone: STRB of
// its low byte into mRet, the next word (the failure status) into mStatus, the
// string at +8, and the list node at +12 with its refcount bumped.
DrRpcRes DrRpcEngine::perform(DrCurlRequest const &theRequest)
{
    DrRpcRes aResult;
    Sexy::String aResponse;
    int aTries = 2;
    for (;;)
    {
        // Each attempt seeds the response with the shared empty rep, then
        // releases the url and the post body before testing the transport.
        aResponse = Sexy::String();
        int aTransport;
        {
            Sexy::String aUrl = theRequest.getUrl();
            Sexy::String aArgs = encodeRequesArgs(theRequest);
            // Static in this tree: live strings, method 0 (HTTP_METHOD_GET).
            // The binary passes the engine in r0; the stub has no receiver.
            aTransport = DrCurlEngine::perform(aUrl, aArgs, aResponse, DrCurlRequest::HTTP_METHOD_GET);
            if (aTransport != 0)
                aTransport = decodeResponseString(aResponse);
        }
        if (aTransport != 0)
            break;
        if (--aTries == 0)
        {
            aResult.mRet = gFailedResult.mRet;
            aResult.mStatus = gFailedResult.mStatus;
            aResult.mRetMsg = gFailedResult.mRetMsg;
            if (gFailedResult.mList != nullptr)
                ++gFailedResult.mList->refcount;
            aResult.mList = gFailedResult.mList;
            return aResult;
        }
    }

    // basic_string::find('{', 0). npos is -1; a hit past the length cannot
    // happen, but substr still throws on that check rather than clamping.
    unsigned int aBrace = static_cast<unsigned int>(-1);
    unsigned int aLength = aResponse.size();
    if (aLength != 0)
    {
        const void *aHit = std::memchr(aResponse.c_str(), '{', aLength);
        if (aHit != nullptr)
        {
            const char *aHitChars = static_cast<const char *>(static_cast<const void *>(aHit));
            aBrace = static_cast<unsigned int>(aHitChars - aResponse.c_str());
        }
    }
    if (aBrace == static_cast<unsigned int>(-1))
    {
        aResult.mRet = gFailedResult.mRet;
        aResult.mStatus = gFailedResult.mStatus;
        aResult.mRetMsg = gFailedResult.mRetMsg;
        if (gFailedResult.mList != nullptr)
            ++gFailedResult.mList->refcount;
        aResult.mList = gFailedResult.mList;
    }
    else
    {
        if (aBrace > aResponse.size())
            throw std::out_of_range("basic_string::substr");
        {
            Sexy::String aSlice = aResponse.substr(aBrace);
            Sexy::String aFiltered = UTF8::filterUnicode(aSlice);
            aResponse = aFiltered;
        }
        // Only the low byte of mRet is stored (STRB). An unshared list node
        // is deleted without a refcount decrement; the pointer is kept either way.
        DrRpcRes aParsed = parseResponse(aResponse);
        aResult.mRet = static_cast<unsigned char>(aParsed.mRet);
        aResult.mStatus = aParsed.mStatus;
        aResult.mRetMsg = aParsed.mRetMsg;
        aResult.mList = aParsed.mList;
        if (aResult.mList->refcount == 0)
            aResult.mList->deleteInternal(nullptr);
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_Logout(Sexy::String const&)
// @001EEE14  624 bytes
// ==========================================================================
// Returns a DrRpcResCommon BY VALUE, so r0 is the record, r1 the engine and r2
// the uid. The request is the URL copied from mDefaultUrl (+0x204) plus the
// string map setField indexes at +4. LDRB of perform's mRet picks the branch:
// a non-zero low byte stores mStatus into the result, a zero low byte copies
// the inlined DrRpcResCommon::error() static.
DrRpcResCommon DrRpcEngine::call_Logout(Sexy::String const &theToken)
{
    // The original request contains a URL and an ordered string map.
    DrRpcResCommon aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        // Keys die before perform, in reverse construction order.
        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1001");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
        }

        {
            DrRpcRes aPerformed = perform(aRequest);
            // CBZ on the low byte. A non-zero byte stores the status word.
            if (static_cast<unsigned char>(aPerformed.mRet) != 0)
            {
                if (aPerformed.mStatus == 0 && static_cast<unsigned char>(aPerformed.mList->_type - 4) <= 1u)
                    aPerformed.mList->Fetch();
                aResult.mRet = aPerformed.mStatus;
                aResult.mRetMsg = aPerformed.mRetMsg;
            }
            else
            {
                // LSLS/BPL: bit 0 clear means the static is not initialised.
                // Acquire returning 0 means another thread won; still copy.
                aResult.mRet = kRpcFailed;
                aResult.mRetMsg = Sexy::String();
            }
            if (aPerformed.mList != nullptr)
            {
                if (--aPerformed.mList->refcount == 0)
                    aPerformed.mList->deleteInternal(nullptr);
            }
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_QueryCoin(Sexy::String const&)
// @001EF0FC  926 bytes
// ==========================================================================
// Returns a DrRpcResWithText BY VALUE, so r0 is the record, r1 the engine and r2
// the uid. Its three words are mRet, mRetMsg and mText.
DrRpcResWithText DrRpcEngine::call_QueryCoin(Sexy::String const &theToken)
{
    // Same request shape as call_Logout: the default URL plus a string map.
    DrRpcResWithText aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1008");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
        }

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet != 0)
        {
            // mText is the coin total. A missing list leaves it 0.
            aResult.mText = 0;
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
            {
                aPerformed.mList->Fetch();
                if (aPerformed.mList->CHILDREN != nullptr && aPerformed.mList->CHILDREN->mChildren.size() != 0)
                {
                    JSONNode aRow;
                    JSONNode::Bind(aRow, aPerformed.mList);
                    Sexy::String aKey("iCoin");
                    JSONNode *aCoin = aRow.at(aKey);
                    if (aCoin != nullptr && aCoin->mInternal != nullptr)
                    {
                        aCoin->mInternal->Fetch();
                        aResult.mText = (int)aCoin->mInternal->_value._number;
                    }
                    aRow.mInternal = nullptr;
                }
            }
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
            aResult.mText = 0;
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_GetKeyValue(Sexy::String const&,Sexy::String const&)
// @001EF618  920 bytes
// ==========================================================================
// Returns a DrRpcResWithText BY VALUE: r0 the record, r1 the engine, r2 the uid
// and r3 the key. Its third word mText carries the "sValue" number.
DrRpcResWithText DrRpcEngine::call_GetKeyValue(Sexy::String const &theToken, Sexy::String const &theKey)
{
    DrRpcResWithText aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1028");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
            // The server stores every saved value under a "pvz" prefix.
            Sexy::String aKeyName("sKey");
            Sexy::String aPrefixed("pvz");
            aPrefixed += theKey;
            aRequest.setField(aKeyName, aPrefixed);
        }

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet != 0)
        {
            aResult.mText = 0;
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
            {
                aPerformed.mList->Fetch();
                if (aPerformed.mList->CHILDREN != nullptr && aPerformed.mList->CHILDREN->mChildren.size() != 0)
                {
                    JSONNode aRow;
                    JSONNode::Bind(aRow, aPerformed.mList);
                    Sexy::String aKey("sValue");
                    JSONNode *aValue = aRow.at(aKey);
                    if (aValue != nullptr && aValue->mInternal != nullptr)
                    {
                        aValue->mInternal->Fetch();
                        aResult.mText = (int)aValue->mInternal->_value._number;
                    }
                    aRow.mInternal = nullptr;
                }
            }
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
            aResult.mText = 0;
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_Recharge(Sexy::String const&,int)
// @001EFB0C  838 bytes
// ==========================================================================
// Returns a DrRpcResCommon BY VALUE: r0 the record, r1 the engine, r2 the uid,
// r3 the amount. Its two words are mRet and mRetMsg.
DrRpcResCommon DrRpcEngine::call_Recharge(Sexy::String const &theToken, int theAmount)
{
    DrRpcResCommon aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1006");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
            Sexy::String aAmountKey("iAmount");
            Sexy::String aAmountText(std::to_string(theAmount).c_str());
            aRequest.setField(aAmountKey, aAmountText);
            Sexy::String aSerialKey("sSerialNum");
            Sexy::String aSerial;
            LawnSerialNumber::GetNewOne(&aSerial);
            aRequest.setField(aSerialKey, aSerial);
        }

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet != 0)
        {
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
                aPerformed.mList->Fetch();
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_SetKeyValue(Sexy::String const&,Sexy::String const&,int)
// @001EFF00  890 bytes
// ==========================================================================
// Returns a DrRpcResCommon BY VALUE: r0 the record, r1 the engine, r2 the uid,
// r3 the key and the value on the stack. A status other than kNotDone or ok
// is reported as a failed call.
DrRpcResCommon DrRpcEngine::call_SetKeyValue(Sexy::String const &theToken, Sexy::String const &theKey, int theValue)
{
    DrRpcResCommon aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1029");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
            Sexy::String aKeyName("sKey");
            Sexy::String aPrefixed("pvz");
            aPrefixed += theKey;
            aRequest.setField(aKeyName, aPrefixed);
            Sexy::String aValueKey("sValue");
            Sexy::String aValueText(std::to_string(theValue).c_str());
            aRequest.setField(aValueKey, aValueText);
        }

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet == 0)
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
        }
        else if (aPerformed.mStatus != kNotDone && aPerformed.mStatus != 0)
        {
            // Anything other than "not done" or "ok" is a failed save.
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
        }
        else
        {
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_DebugAddMoney(Sexy::String const&,int)
// @001F0330  1164 bytes
// ==========================================================================
// Returns a DrRpcResWithString BY VALUE. mText carries the "iCoin" text.
DrRpcResWithString DrRpcEngine::call_DebugAddMoney(Sexy::String const &theToken, int theAmount)
{
    DrRpcResWithString aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1007");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
            Sexy::String aAmountKey("iAmount");
            Sexy::String aAmountText(std::to_string(theAmount).c_str());
            aRequest.setField(aAmountKey, aAmountText);
            Sexy::String aSerialKey("sSerialNum");
            Sexy::String aSerial;
            LawnSerialNumber::GetNewOne(&aSerial);
            aRequest.setField(aSerialKey, aSerial);
        }

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet != 0)
        {
            aResult.mText = Sexy::String();
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
            {
                aPerformed.mList->Fetch();
                if (aPerformed.mList->CHILDREN != nullptr && aPerformed.mList->CHILDREN->mChildren.size() != 0)
                {
                    JSONNode aRow;
                    JSONNode::Bind(aRow, aPerformed.mList);
                    Sexy::String aKey("iCoin");
                    JSONNode *aCoin = aRow.at(aKey);
                    if (aCoin != nullptr && aCoin->mInternal != nullptr)
                    {
                        aCoin->mInternal->Fetch();
                        aResult.mText = aCoin->mInternal->_string;
                    }
                    aRow.mInternal = nullptr;
                }
            }
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
            aResult.mText = Sexy::String();
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_UseTool(Sexy::String const&,int)
// @001F0970  1028 bytes
// ==========================================================================
// Returns a DrRpcResWithString BY VALUE. The tool id is sent as id+1000, and
// the count is always 1. mText stays empty: this action only confirms the use.
DrRpcResWithString DrRpcEngine::call_UseTool(Sexy::String const &theToken, int theTool)
{
    DrRpcResWithString aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1009");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
            Sexy::String aToolKey("iToolId");
            Sexy::String aToolText(std::to_string(theTool + 1000).c_str());
            aRequest.setField(aToolKey, aToolText);
            Sexy::String aCountKey("iToolCount");
            Sexy::String aCountText("1");
            aRequest.setField(aCountKey, aCountText);
            Sexy::String aSerialKey("sSerialNum");
            Sexy::String aSerial;
            LawnSerialNumber::GetNewOne(&aSerial);
            aRequest.setField(aSerialKey, aSerial);
        }

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet != 0)
        {
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
                aPerformed.mList->Fetch();
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
            aResult.mText = Sexy::String();
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
            aResult.mText = Sexy::String();
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_Loggin(Sexy::String const&,Sexy::String const&,Sexy::String const&)
// @001F0E4C  1558 bytes
// ==========================================================================
// Returns a DrRpcResOfLoggin BY VALUE. The five words are mRet, mRetMsg and
// the uid, role name and coin text the response carries. The password argument
// is the oem target the engine remembers for later shop queries.
DrRpcResOfLoggin DrRpcEngine::call_Loggin(
    Sexy::String const &theAccount, Sexy::String const &thePassword, Sexy::String const &theExtra)
{
    DrRpcResOfLoggin aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1000");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUserKey("strUsername");
            aRequest.setField(aUserKey, theAccount);
            Sexy::String aOemKey("sOemTarget");
            aRequest.setField(aOemKey, thePassword);
            Sexy::String aNameKey("spUserName");
            aRequest.setField(aNameKey, theExtra);
        }
        mOemTarget = thePassword;

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet != 0)
        {
            aResult.mText1 = Sexy::String();
            aResult.mText2 = Sexy::String();
            aResult.mText3 = Sexy::String();
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
            {
                aPerformed.mList->Fetch();
                if (aPerformed.mList->CHILDREN != nullptr && aPerformed.mList->CHILDREN->mChildren.size() != 0)
                {
                    JSONNode aRow;
                    JSONNode::Bind(aRow, aPerformed.mList);
                    Sexy::String aUidKey("iUid");
                    JSONNode *aUid = aRow.at(aUidKey);
                    if (aUid != nullptr && aUid->mInternal != nullptr)
                    {
                        aUid->mInternal->Fetch();
                        aResult.mText1 = aUid->mInternal->_string;
                    }
                    Sexy::String aRoleKey("sRoleName");
                    JSONNode *aRole = aRow.at(aRoleKey);
                    if (aRole != nullptr && aRole->mInternal != nullptr)
                    {
                        aRole->mInternal->Fetch();
                        aResult.mText2 = aRole->mInternal->_string;
                    }
                    Sexy::String aCoinKey("iCoin");
                    JSONNode *aCoin = aRow.at(aCoinKey);
                    if (aCoin != nullptr && aCoin->mInternal != nullptr)
                    {
                        aCoin->mInternal->Fetch();
                        aResult.mText3 = aCoin->mInternal->_string;
                    }
                    aRow.mInternal = nullptr;
                }
            }
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
            aResult.mText1 = Sexy::String();
            aResult.mText2 = Sexy::String();
            aResult.mText3 = Sexy::String();
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// ==========================================================================
// DrRpcEngine::call_QueryCoinList(void)
// @001F1744  1342 bytes
// ==========================================================================
// Asks the shop for the coin tiers of the remembered oem target. Each list
// entry becomes one SCoinItem { value, getcoin }.
DrRpcResOfCoinList DrRpcEngine::call_QueryCoinList()
{
    DrRpcResOfCoinList aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1013");
            aRequest.setField(aAction, aActionId);
            Sexy::String aOemKey("sOemTarget");
            aRequest.setField(aOemKey, mOemTarget);
        }

        DrRpcRes aPerformed = perform(aRequest);
        aResult.mList.clear();
        if (aPerformed.mRet != 0)
        {
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
            {
                aPerformed.mList->Fetch();
                jsonChildren *aChildren = aPerformed.mList->CHILDREN;
                if (aChildren != nullptr)
                {
                    unsigned int aIndex = 0;
                    while (aIndex < aChildren->mChildren.size())
                    {
                        aPerformed.mList->Fetch();
                        JSONNode aList;
                        JSONNode::Bind(aList, aPerformed.mList);
                        JSONNode *aChild = aList.at(aIndex);
                        aList.mInternal = nullptr;
                        if (aChild == nullptr || aChild->mInternal == nullptr)
                        {
                            ++aIndex;
                            continue;
                        }

                        SCoinItem anItem;
                        anItem.mValue = 0;
                        anItem.mGetCoin = 0;

                        JSONNode aEntry;
                        JSONNode::Bind(aEntry, aChild->mInternal);
                        Sexy::String aValueKey("value");
                        JSONNode *aValueNode = aEntry.at(aValueKey);
                        if (aValueNode != nullptr && aValueNode->mInternal != nullptr)
                        {
                            aValueNode->mInternal->Fetch();
                            anItem.mValue = (int)aValueNode->mInternal->_value._number;
                        }
                        aEntry.mInternal = nullptr;

                        JSONNode aCoinEntry;
                        JSONNode::Bind(aCoinEntry, aChild->mInternal);
                        Sexy::String aGetCoinKey("getcoin");
                        JSONNode *aGetCoin = aCoinEntry.at(aGetCoinKey);
                        if (aGetCoin != nullptr && aGetCoin->mInternal != nullptr)
                        {
                            aGetCoin->mInternal->Fetch();
                            anItem.mGetCoin = (int)aGetCoin->mInternal->_value._number;
                        }
                        aCoinEntry.mInternal = nullptr;

                        aResult.mList.push_back(anItem);
                        ++aIndex;
                    }
                }
            }
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
            aResult.mList.clear();
        }
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}

// A shop-price node: item id (already shifted by -1000) then the price text.
// MapStorage nodes are the four-word base followed by this pair.
struct ShopPrice
{
    int mItemId;
    Sexy::String mPrice;
};

static void InitItemMap(Sexy::RbTree::MapStorage *theMap)
{
    theMap->mComparator = 0;
    theMap->mHeader.mColor = 0;
    theMap->mHeader.mParent = nullptr;
    theMap->mHeader.mLeft = &theMap->mHeader;
    theMap->mHeader.mRight = &theMap->mHeader;
    theMap->mCount = 0;
}

// Insert or replace one price. The key is the item id stored at node +16.
static void PutShopPrice(Sexy::RbTree::MapStorage *theMap, int theItemId, Sexy::String const &thePrice)
{
    struct PriceNode : Sexy::RbTree::NodeBase
    {
        int mItemId;
        Sexy::String mPrice;
    };

    Sexy::RbTree::NodeBase *aParent = &theMap->mHeader;
    Sexy::RbTree::NodeBase *aCursor = theMap->mHeader.mParent;
    bool aWentLeft = true;
    while (aCursor != nullptr)
    {
        aParent = aCursor;
        PriceNode *aNode = static_cast<PriceNode *>(aCursor);
        aWentLeft = theItemId < aNode->mItemId;
        aCursor = aWentLeft ? aCursor->mLeft : aCursor->mRight;
    }

    Sexy::RbTree::NodeBase *aNeighbour = aParent;
    if (aWentLeft)
    {
        if (aParent == theMap->mHeader.mLeft)
            aNeighbour = nullptr;
        else
            aNeighbour = Sexy::RbTree::_M_decrement(aParent);
    }
    if (aNeighbour != nullptr && aNeighbour != &theMap->mHeader)
    {
        PriceNode *aExisting = static_cast<PriceNode *>(aNeighbour);
        if (aExisting->mItemId == theItemId)
        {
            aExisting->mPrice = thePrice;
            return;
        }
    }

    PriceNode *aNew = new PriceNode();
    aNew->mColor = 0;
    aNew->mParent = nullptr;
    aNew->mLeft = nullptr;
    aNew->mRight = nullptr;
    aNew->mItemId = theItemId;
    aNew->mPrice = thePrice;
    bool aInsertLeft = (aParent == &theMap->mHeader) || aWentLeft;
    Sexy::RbTree::_Rb_tree_insert_and_rebalance(aInsertLeft, aNew, aParent, theMap->mHeader);
    ++theMap->mCount;
}

static void CopyItemMap(Sexy::RbTree::MapStorage *theDest, Sexy::RbTree::MapStorage const *theSource)
{
    InitItemMap(theDest);
    if (theSource->mHeader.mParent == nullptr)
        return;

    struct Frame
    {
        Sexy::RbTree::NodeBase *mSource;
        Sexy::RbTree::NodeBase *mParent;
        bool mIsLeft;
    };
    struct PriceNode : Sexy::RbTree::NodeBase
    {
        int mItemId;
        Sexy::String mPrice;
    };

    Frame aStack[64];
    int aDepth = 0;
    aStack[aDepth].mSource = theSource->mHeader.mParent;
    aStack[aDepth].mParent = &theDest->mHeader;
    aStack[aDepth].mIsLeft = false;
    ++aDepth;

    Sexy::RbTree::NodeBase *aLeftmost = nullptr;
    Sexy::RbTree::NodeBase *aRightmost = nullptr;
    while (aDepth > 0)
    {
        --aDepth;
        Frame aFrame = aStack[aDepth];
        PriceNode *aSourceNode = static_cast<PriceNode *>(aFrame.mSource);
        PriceNode *aCopy = new PriceNode();
        aCopy->mColor = aSourceNode->mColor;
        aCopy->mParent = aFrame.mParent;
        aCopy->mLeft = nullptr;
        aCopy->mRight = nullptr;
        aCopy->mItemId = aSourceNode->mItemId;
        aCopy->mPrice = aSourceNode->mPrice;
        if (aFrame.mParent == &theDest->mHeader)
            theDest->mHeader.mParent = aCopy;
        else if (aFrame.mIsLeft)
            aFrame.mParent->mLeft = aCopy;
        else
            aFrame.mParent->mRight = aCopy;
        if (aLeftmost == nullptr || aCopy->mItemId < static_cast<PriceNode *>(aLeftmost)->mItemId)
            aLeftmost = aCopy;
        if (aRightmost == nullptr || aCopy->mItemId > static_cast<PriceNode *>(aRightmost)->mItemId)
            aRightmost = aCopy;
        ++theDest->mCount;
        if (aSourceNode->mRight != nullptr && aDepth < 64)
        {
            aStack[aDepth].mSource = aSourceNode->mRight;
            aStack[aDepth].mParent = aCopy;
            aStack[aDepth].mIsLeft = false;
            ++aDepth;
        }
        if (aSourceNode->mLeft != nullptr && aDepth < 64)
        {
            aStack[aDepth].mSource = aSourceNode->mLeft;
            aStack[aDepth].mParent = aCopy;
            aStack[aDepth].mIsLeft = true;
            ++aDepth;
        }
    }
    if (aLeftmost != nullptr)
        theDest->mHeader.mLeft = aLeftmost;
    if (aRightmost != nullptr)
        theDest->mHeader.mRight = aRightmost;
}

static void EraseItemMap(Sexy::RbTree::NodeBase *theNode)
{
    struct PriceNode : Sexy::RbTree::NodeBase
    {
        int mItemId;
        Sexy::String mPrice;
    };

    while (theNode != nullptr)
    {
        PriceNode *aNode = static_cast<PriceNode *>(theNode);
        EraseItemMap(aNode->mRight);
        PriceNode *aLeft = static_cast<PriceNode *>(aNode->mLeft);
        delete aNode;
        theNode = aLeft;
    }
}

// ==========================================================================
// DrRpcEngine::call_QueryAllItems(Sexy::String const&)
// @001F1FD0  1556 bytes
// ==========================================================================
// Asks for the shop catalogue. Each list entry is keyed by iItemId-1000 and
// stores the iPrice text.
DrRpcResOfAllItems DrRpcEngine::call_QueryAllItems(Sexy::String const &theToken)
{
    DrRpcResOfAllItems aResult;
    {
        DrCurlRequest aRequest;
        aRequest.mUrl = mDefaultUrl;

        {
            Sexy::String aAction("action");
            Sexy::String aActionId("A1012");
            aRequest.setField(aAction, aActionId);
            Sexy::String aUidKey("iUid");
            aRequest.setField(aUidKey, theToken);
        }

        Sexy::RbTree::MapStorage aPrices;
        InitItemMap(&aPrices);

        DrRpcRes aPerformed = perform(aRequest);
        if (aPerformed.mRet != 0)
        {
            if (aPerformed.mStatus == 0 && aPerformed.mList != nullptr &&
                (unsigned char)(aPerformed.mList->_type - 4) <= 1u)
            {
                aPerformed.mList->Fetch();
                jsonChildren *aChildren = aPerformed.mList->CHILDREN;
                if (aChildren != nullptr)
                {
                    unsigned int aIndex = 0;
                    while (aIndex < aChildren->mChildren.size())
                    {
                        aPerformed.mList->Fetch();
                        JSONNode aList;
                        JSONNode::Bind(aList, aPerformed.mList);
                        JSONNode *aChild = aList.at(aIndex);
                        aList.mInternal = nullptr;
                        if (aChild == nullptr || aChild->mInternal == nullptr)
                        {
                            ++aIndex;
                            continue;
                        }

                        int aItemId = 0;
                        JSONNode aEntry;
                        JSONNode::Bind(aEntry, aChild->mInternal);
                        Sexy::String aItemKey("iItemId");
                        JSONNode *aItem = aEntry.at(aItemKey);
                        if (aItem != nullptr && aItem->mInternal != nullptr)
                        {
                            aItem->mInternal->Fetch();
                            aItemId = (int)aItem->mInternal->_value._number - 1000;
                        }
                        aEntry.mInternal = nullptr;

                        Sexy::String aPriceText;
                        JSONNode aPriceEntry;
                        JSONNode::Bind(aPriceEntry, aChild->mInternal);
                        Sexy::String aPriceKey("iPrice");
                        JSONNode *aPrice = aPriceEntry.at(aPriceKey);
                        if (aPrice != nullptr && aPrice->mInternal != nullptr)
                        {
                            aPrice->mInternal->Fetch();
                            aPriceText = aPrice->mInternal->_string;
                        }
                        aPriceEntry.mInternal = nullptr;

                        PutShopPrice(&aPrices, aItemId, aPriceText);
                        ++aIndex;
                    }
                }
            }
            aResult.mRet = aPerformed.mStatus;
            aResult.mRetMsg = aPerformed.mRetMsg;
            CopyItemMap(&aResult.mList, &aPrices);
        }
        else
        {
            aResult.mRet = kRpcFailed;
            aResult.mRetMsg = Sexy::String();
            InitItemMap(&aResult.mList);
        }
        EraseItemMap(aPrices.mHeader.mParent);
        if (aPerformed.mList != nullptr)
        {
            if (--aPerformed.mList->refcount == 0)
                aPerformed.mList->deleteInternal(nullptr);
        }
    }
    return aResult;
}
