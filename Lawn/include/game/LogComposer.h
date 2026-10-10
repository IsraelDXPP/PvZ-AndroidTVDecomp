// LogComposer -- reconstructed from libGameMain.so.
//
// A plain data record: no vtable (it has no entry in _recon/vtables.json) and no
// base class, so nothing here is virtual and there is no inheritance.
//
// SIZE 0x1C, from the allocation site: LawnApp.cpp:12416 is
// `v12 = (TM::LogComposer *)operator new(0x1Cu); new (v12) TM::LogComposer();`
// (the same pair appears in _recon/ida/LawnApp.cpp:13897). The constructor
// (@0x4F9CB8) assigns exactly seven Sexy::String objects, at +0x00, +0x04, +0x08,
// +0x0C, +0x10, +0x14 and +0x18, i.e. 7 * 4 = 0x1C. The 0x28 the transcription
// wrote was a max-offset guess and is 0x0C too large; no body in either dump
// reaches past +0x18.
//
// The last five members are named after the XML child tags initComposer
// (@0x4F8920) reads them from -- it does
//   getChildNode(child, path, "platform")   -> assign to this + 8
//   getChildNode(child, path, "vendor")     -> assign to this + 12
//   getChildNode(child, path, "product")    -> assign to this + 16
//   getChildNode(child, path, "gamename")   -> assign to this + 20
//   getChildNode(child, path, "gameversion")-> assign to this + 24
// so the names are the tags.
//
// +0x00 is the serial number: the constructor (@0x4F9CB8) calls
// generateSerialNum (@0x4F46D8) and assigns the result to it. +0x04 is still
// unidentified -- the constructor only default-constructs it and nothing in the
// dump ever writes it, so its name is left open.
//
// Every compose* method returns a `Sexy::String` BY VALUE. That is why the
// decompilation showed a destination pointer in r0: ARM passes the hidden
// return-value buffer there, so `this` is r1, not r0, and the transcription
// that read r0 as the receiver was reading the result buffer. Confirmed on
// LawnApp::PostLeaveLevel (@0x2B3220), which loads r1 from mLogComposer
// (LawnApp + 0xB00), r2 from getLevelForPost() and r0 from a stack string, and
// on the callee's own body, which constructs the result at r0 and returns it.
//
// The binary declares the class in namespace TM. The global alias below
// keeps existing callers using the same type.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_LOGCOMPOSER_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_LOGCOMPOSER_H

#include "ReconTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace TM
{

class LogComposer
{
public:
    Sexy::String mSerialNum; // +0x00  generateSerialNum writes this
    Sexy::String mAccount; // +0x04  login and auth logs carry this field
    Sexy::String mPlatform; // +0x08  XML tag "platform"
    Sexy::String mVendor; // +0x0C  XML tag "vendor"
    Sexy::String mProduct; // +0x10  XML tag "product"
    Sexy::String mGameName; // +0x14  XML tag "gamename"
    Sexy::String mGameVersion; // +0x18  XML tag "gameversion"
    // Size 0x1C.

    Sexy::String composeLog(std::vector<Sexy::String> const &theFields);
    Sexy::String getClientTime();
    Sexy::String generateSerialNum();
    Sexy::String composeCrashLog(Sexy::String &theReason);
    Sexy::String composeLoginLog(int theKind);
    Sexy::String composeLoginLog();
    Sexy::String composeLogoutLog();
    Sexy::String composeAssembledLog(int theKind, Sexy::String &theDetail);
    Sexy::String composeAuthLog(int theKind, Sexy::String const &theDetail);
    Sexy::String composeAuthStartLog(Sexy::String const &theDetail);
    Sexy::String composeAuthSucceedLog(Sexy::String const &theDetail);
    Sexy::String composeAuthFailedLog(Sexy::String const &theDetail);
    Sexy::String composeGameDataLog(int theKind, Sexy::String const &theDetail);
    Sexy::String composeGameResultLog(Sexy::String const &theDetail);
    Sexy::String composeGameDataLog(int theKind, int theValue);
    Sexy::String composeScoreLog(int theScore);
    Sexy::String composeTimeLog(int theSeconds);
    Sexy::String composeConsumeLog(int theKind, int theItem, int theCount);
    Sexy::String composeRechargeLog(int theKind, int theAmount, int theCurrency);
    Sexy::String composeRechargeByRMBLog(int theAmount, int theCurrency);
    Sexy::String composeRechargeByDollarLog(int theAmount, int theCurrency);
    Sexy::String composeGameLevelLog(int theKind, int theLevel, int theResult, Sexy::String const &theDetail);
    Sexy::String composeLevelEnterLog(Sexy::String const &theLevel, int theResult);
    Sexy::String composeLevelLeaveLog(int theResult);
    Sexy::String composeLevelSucceedLog(int theResult);
    Sexy::String composeLevelFailedLog(int theResult);
    Sexy::String composePayLog(int theKind, int theAmount, int theCurrency, int theItem);
    Sexy::String composePayByRMBLog(int theAmount);
    Sexy::String composePayByRMBLog(int theAmount, int theCurrency, int theItem);
    Sexy::String composePayByDollarLog(int theAmount);
    Sexy::String composePayByDollarLog(int theAmount, int theCurrency, int theItem);
    void initComposer(Sexy::String const &theXml);
    LogComposer();
};

}

using TM::LogComposer;

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_LOGCOMPOSER_H
