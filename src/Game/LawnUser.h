// LawnUser -- reconstructed from libGameMain.so.
//
// A plain data record: no vtable (no entry in _recon/vtables.json) and no base
// class, so nothing here is virtual. Note that the destructor is NOT declared
// below: Game\LawnUser.cpp never defines `LawnUser::~LawnUser`, it only calls
// `((LawnUser *)theObject)->~LawnUser()` from its registration thunk, which the
// implicit destructor satisfies.
//
// SIZE 0x34, and the transcription's 0x28 was 0x0C too small -- the destructor
// (@0x174DFC) reads and writes past it. The three members below are pinned by that
// destructor and by getCurUser (@0x2BB9B8), which builds a function-local static at
// these offsets:
//   +0x00..+0x0F  four Sexy::String objects. getCurUser stores the shared empty-rep
//                 sentinel `Sexy::gEmptyStringData` into +0x04, +0x08 and +0x0C and then assigns
//                 the defaults "007", "shax" and "0"; each is released through its
//                 12-byte libstdc++ rep header (`v3 - 12 != (int *)Sexy::gEmptyStringData` in the
//                 destructor), which is what makes them Sexy::String.
//   +0x10..+0x27  a std::map<int, std::string>. It is a libstdc++ _Rb_tree: the node
//                 base (colour, parent, left, right) sits at +0x14..+0x23 and the
//                 node count at +0x24, with the four bytes of empty
//                 comparator/allocator bases at +0x10. getCurUser initialises it with
//                 the exact _M_reset() pattern (parent = 0 at +0x18, left = right =
//                 &header at +0x1C/+0x20 with the header base at +0x14, count = 0 at
//                 +0x24) and the destructor drains it with
//                 `std::_Rb_tree<...>::_M_erase((char *)this + 16, *((_DWORD *)this + 6))`.
//   +0x28..+0x33  three words zeroed by getCurUser; the destructor frees the first
//                 one with `operator delete(*(void **)(this + 10))`, i.e. it is a
//                 container's storage pointer plus its two bookkeeping words.
//
// TODO(recon): neither the four strings nor the two containers are named anywhere in
// the dumps, so the names are left open rather than guessed. LawnApp.cpp:10761 does
// `atoi(*(const char **)(CurUser + 12))`, i.e. the string at +0x0C holds a decimal
// number, which is the only semantic hint available.
// TODO(recon): this tree has no libstdc++-layout map (StlCompat.h provides only
// libstdcxx::vector), so the map is modelled as a named 6-word block with the exact
// offsets. Adding a `libstdcxx::map` next to `libstdcxx::vector` would let both
// containers be retyped without moving a byte.

#pragma once

#include "ReconTypes.h"
#include <map>

class LawnUser
{
public:
    Sexy::String mAccount;   // +0x00
    Sexy::String mToken;     // +0x04  default "007"
    Sexy::String mChannel;   // +0x08  default "shax"
    Sexy::String mCoinText;  // +0x0C  default "0"; the decimal coin count
    // Prices keyed by shop packet. The binary keeps a 24-byte libstdc++ map
    // here; MSVC's map is the same size, so the record stays 0x34 bytes.
    std::map<int, Sexy::String> mPrices; // +0x10 .. +0x27
    // The coin tiers. Three pointers, the same three words a vector keeps.
    // MSVC's std::vector is larger than 12 bytes, so the record stays three pointers.
    SCoinItem*   mCoinItems;     // +0x28
    SCoinItem*   mCoinItemsEnd;  // +0x2C
    SCoinItem*   mCoinItemsCap;  // +0x30

    int CoinTierCount() const
    {
        if (mCoinItems == nullptr || mCoinItemsEnd == nullptr)
            return 0;
        return (int)(mCoinItemsEnd - mCoinItems);
    }
    // Size 0x34.

    static LawnUser* getCurUser();
};


