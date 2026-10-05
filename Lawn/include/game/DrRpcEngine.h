// DrRpcEngine -- reconstructed from libGameMain.so.
//
// Non-polymorphic: libGameMain115_rtti.c has no `vtable for'DrRpcEngine (and no
// `vtable for'DrCurlEngine either), so there is no vptr to declare.
//
// Base class: DrCurlEngine, the libcurl transport. Evidence, all from
// _recon/ida/DrRpcEngine.cpp:
//   * the constructor's first statement is `DrCurlEngine::DrCurlEngine(this)`;
//   * the destructor's last statement is `DrCurlEngine::~DrCurlEngine(this)`;
//   * getRpcEngine calls the base's method through the object --
//     `((DrCurlEngine *)&unk_69F9D0)->init(&unk_69F9D0)` -- and perform() calls
//     `DrCurlEngine::perform(...)`.
// DrCurlEngine is declared in Sexy/ReconFreeFunctions.h (which ReconTypes.h
// includes at its end, hence the explicit include below); that declaration is
// still a stub of static members with no data, so it contributes 1 byte here.
// TODO(recon): give DrCurlEngine its real body and the offsets below line up.
// The binary's DrCurlEngine occupies +0x00..+0x203 -- 0x204 bytes, which is
// exactly where the first member DrRpcEngine's own constructor writes. Only two
// base fields are visible anywhere: the CURL* that curl_easy_init returns at
// +0x00 and the CURLOPT_ERRORBUFFER byte buffer at +0x04 whose first byte is the
// "an error was recorded" flag (1.15 dump, DrCurlEngine::DrCurlEngine and
// ::init). That accounts for 0x104 bytes; nothing read or written by any
// DrRpcEngine body in the dump, nor by any DrCurlEngine body in the 1.15 dump,
// falls in +0x104..+0x203. Which part of that hole is the tail of the base and
// which is DrRpcEngine state no transcribed body touches is unresolved.
//
// Total size 0x214: the Meyers singleton `unk_69F9D0` that getRpcEngine returns
// sits in .bss immediately before the guard variable `dword_69FBE4` that
// getRpcEngine also reads, and 0x69FBE4 - 0x69F9D0 == 0x214. The next four
// four-byte .bss slots (dword_69FBE8, DrRpcEngine::kNotDone at 0x69FBEC,
// dword_69FBF0, dword_69FBF4, dword_69FBF8) follow it without padding, so a
// larger object would have swallowed them.
//
// Layout of the derived part, from the constructor and the destructor:
//   +0x204  mDefaultUrl      the request URL; seeded with the LAN debug address
//                            "http://192.168.1.9/MobileGameServer/server/index.php"
//                            and every call_* copies it into the request
//   +0x208  mDataEncoder     the DrDataEncoder built from the hard-coded key
//                            "97ac4289fcfc1bbea07bccc16f5fa35f"
//   +0x20C  mDataEncryption  the bool setDataEncryption writes and
//                            decodeResponseString/encodeRequesArgs test
//   +0x210  mOemTarget       a string the requests carry as the "sOemTarget"
//                            field (DrCurlRequest::setField(request, "sOemTarget",
//                            this + 528)), empty by default
// Name evidence for the last one is the field name only; the getter/setter pair
// the other three have does not exist for it.

#pragma once

#include "ReconTypes.h"
// Declares the DrCurlEngine base class and the DrDataEncoder member type.
#include "ReconFreeFunctions.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "JSONNode.h"
#include "JSONWorker.h"
#include "internalJSONNode.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ---------------------------------------------------------------------------
// The result records.
//
// Every `call_*` and `parseResponse` returns a struct BY VALUE, so ARM passes
// its destination in r0 and the decompiler's `this` inside those bodies is the
// CALLER'S RECORD, not the engine -- the engine arrives as the second argument.
// That is what the `+0x204`..`+0x210` offsets those bodies spell against `this`
// really address: the engine's own mDefaultUrl, mDataEncoder, mDataEncryption
// and mOemTarget, reached through the engine argument. Modelling the records is
// therefore not a convenience; without them every one of those stores lands on
// the record's own words and the engine's members are unreachable.
//
// The layouts come from the binary's destructors, which name each word:
//   DrRpcResCommon::~DrRpcResCommon           @0x1ED8CC  releases +0x04 (String)
//   DrRpcEngine::DrRpcRes::~DrRpcRes          @0x1ED964  releases +0x08 (String)
//                                                        and +0x0C (JSON node)
//   DrRpcResOfCoinList::~DrRpcResOfCoinList   @0x1ED9C0  deletes +0x08 as an
//                                                        array, releases +0x04
//   DrRpcResOfAllItems::~DrRpcResOfAllItems   @0x1F1604  erases the tree at
//                                                        +0x08, releases +0x04
//   DrRpcResOfLoggin::~DrRpcResOfLoggin       @0x1EDC30  releases +0x04..+0x10
//   DrRpcResOf{DebugAddMoney,UseTool,QueryCoin,GetKeyValue}::~...
//                                             release +0x04 and +0x08
// The `error()` singletons (DrRpcResOfLoggin::error @0x1EDD34,
// DrRpcResOfCoinList::error @0x1EDE08, DrRpcResOfAllItems::error @0x1F1D98) fill
// those same words, which is what fixes each record's extent.
// ---------------------------------------------------------------------------

// The two-word base every result shares: the "iRet" status word and the
// "sRetMsg" text.
struct DrRpcResCommon
{
    int          mRet;      // +0x00  "iRet"; its low byte is the ok flag
    Sexy::String mRetMsg;   // +0x04  "sRetMsg"
};

// The three-word results: an extra word at +0x08. DrRpcResOfQueryCoin,
// DrRpcResOfDebugAddMoney, DrRpcResOfUseTool and DrRpcResOfGetKeyValue all have
// this shape; they differ only in which field the caller reads. The third word
// is NOT a String: call_GetKeyValue stores the "sValue" number straight into it
// (`v49 = internalJSONNode_QWORD(*v19)`, then `((_DWORD *)this)[2] = v49`), and
// call_QueryCoin fills it from the "iCoin" node the same way.
struct DrRpcResWithText
{
    int          mRet;      // +0x00
    Sexy::String mRetMsg;   // +0x04
    int          mText;     // +0x08
};

// call_DebugAddMoney and call_UseTool store a string in the third word.
struct DrRpcResWithString
{
    int          mRet;
    Sexy::String mRetMsg;
    Sexy::String mText;
};

// DrRpcEngine::DrRpcRes -- perform()'s result and the widest of the family:
//   word 0  the ok flag parseResponse writes first
//   word 1  the "iRet" value parseResponse read
//   word 2  the "sRetMsg" text
//   word 3  the parsed "list" node
struct DrRpcRes
{
    int               mRet;      // +0x00
    int               mStatus;   // +0x04
    Sexy::String      mRetMsg;   // +0x08
    internalJSONNode *mList;     // +0x0C
};

// call_Loggin's result: five words, the last three Strings.
struct DrRpcResOfLoggin
{
    int          mRet;      // +0x00
    Sexy::String mRetMsg;   // +0x04
    Sexy::String mText1;    // +0x08
    Sexy::String mText2;    // +0x0C
    Sexy::String mText3;    // +0x10
};

// call_QueryCoinList's result. The coin tiers are an ordinary vector.
struct DrRpcResOfCoinList
{
    int                       mRet;     // +0x00
    Sexy::String              mRetMsg;  // +0x04
    std::vector<SCoinItem>    mList;    // +0x08
};

// call_QueryAllItems' result: an int-keyed red-black tree at +0x08, whose
// MapStorage is the comparator word, the header node and the element count.
struct DrRpcResOfAllItems
{
    int                 mRet;     // +0x00
    Sexy::String        mRetMsg;  // +0x04
    Sexy::RbTree::MapStorage mList; // +0x08
};

class DrRpcEngine : public DrCurlEngine
{
public:
    DrRpcEngine();
    static void * getRpcEngine();
    int setDefaultUrl(Sexy::String const& theUrl);
    // `STRB.W R1, [R0,#0x20C] / BX LR` @0x1ee0e4 -- a plain store, no result, and
    // the only call site (LawnApp.cpp:6383) discards it.
    void setDataEncryption(bool theEnabled);
    Sexy::String escapeUrl(Sexy::String const& theUrl);
    Sexy::String unescapeUrl(Sexy::String const& theUrl);
    DrRpcRes parseResponse(Sexy::String const& theBody);
    bool decodeResponseString(Sexy::String & theBody);
    Sexy::String encodeRequesArgs(DrCurlRequest const& theRequest);
    DrRpcRes perform(DrCurlRequest const& theRequest);
    DrRpcResCommon call_Logout(Sexy::String const& theToken);
    DrRpcResWithText call_QueryCoin(Sexy::String const& theToken);
    DrRpcResWithText call_GetKeyValue(Sexy::String const& theToken, Sexy::String const& theKey);
    DrRpcResCommon call_Recharge(Sexy::String const& theToken, int theAmount);
    DrRpcResCommon call_SetKeyValue(Sexy::String const& theToken, Sexy::String const& theKey, int theValue);
    DrRpcResWithString call_DebugAddMoney(Sexy::String const& theToken, int theAmount);
    DrRpcResWithString call_UseTool(Sexy::String const& theToken, int theTool);
    DrRpcResOfLoggin call_Loggin(Sexy::String const& theAccount, Sexy::String const& thePassword, Sexy::String const& theExtra);
    DrRpcResOfCoinList call_QueryCoinList();
    DrRpcResOfAllItems call_QueryAllItems(Sexy::String const& theToken);

    Sexy::String mDefaultUrl;    // +0x204
    DrDataEncoder mDataEncoder;  // +0x208 -- one Sexy::String (the key) in the binary;
                                 //           this tree's DrDataEncoder stub is empty,
                                 //           see the TODO above
    bool mDataEncryption;        // +0x20C
    Sexy::String mOemTarget;     // +0x210

    // DrRpcEngine::kNotDone -- a class-scope result sentinel
    // (_ZN11DrRpcEngine8kNotDoneE @0x69fbec, data is all 0xFF), i.e. -1.
    static const int kNotDone = -1;

    // The failure record perform() copies when the transport gives up or the
    // body has no '{'. It begins at kNotDone (0x69FBEC).
    static DrRpcRes gFailedResult;
};

// No static_assert: the class has a base class, and the stub DrCurlEngine of
// ReconFreeFunctions.h is 1 byte where the binary's is 0x204, so sizeof here is
// not the binary's 0x214 yet.
