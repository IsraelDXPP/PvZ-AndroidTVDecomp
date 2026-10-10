// ParseWidget (the binary's `Sexy::MenuParser::ParseWidget`) -- reconstructed from
// libGameMain.so.
//
// `class ParseWidget : public Sexy::RefCount`.
//   RTTI: `_ZTIN4Sexy10MenuParser11ParseWidgetE` @0x67F7C8 records
//   `_ZTIN4Sexy8RefCountE` @0x67F7C0 as its base, and the subclass record
//   `_ZTIN14LawnMenuParser15LawnParseWidgetE` @0x67F7D4 records this class -- which
//   is why Game\LawnParseWidget.h can derive from it.
//
// SIZE = sizeof(Sexy::RefCount); this class adds NO data members.
//   The class has exactly one symbol pair in the binary (its D0/D1 destructors at
//   0x149068/0x1490b8) and no body touches `this` at all, so the transcriber's 0x8
//   was a bare placeholder. The extent is whatever RefCount is; Game\ParseWidget.h
//   deliberately does not restate it, because RefCount is reconstructed in
//   Sexy\RefCount.h and the two must not drift.
//
// SCOPE. The real class is nested (`Sexy::MenuParser::ParseWidget`). It is declared
// at global scope here because Game\LawnParseWidget.h and the transcribed .cpp
// bodies already spell it unqualified; moving it into Sexy::MenuParser would change
// every one of those sites and that is not a header-only change.
// TODO(recon): nest it under Sexy::MenuParser in phase 2.
//
// VTABLE -- two slots, both the destructor:
//   [  0] 0x1490b8  ParseWidget::~ParseWidget (deleting)
// RefCount already declares a virtual destructor, so `virtual ~ParseWidget()` is the
// only declaration needed. MSVC x86 gives a virtual destructor one slot while the
// Itanium table has a D0/D1 pair.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_PARSEWIDGET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_PARSEWIDGET_H

#include "ReconTypes.h"
// The base must be complete: RefCount supplies the vtable ParseWidget reuses.
#include "RefCount.h"

class ParseWidget : public Sexy::RefCount
{
public:
    virtual ~ParseWidget();

    // ---- no own data members ----
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_PARSEWIDGET_H
