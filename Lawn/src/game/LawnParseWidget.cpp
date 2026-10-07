// LawnParseWidget -- restored from libGameMain.so.
//
// Rewritten by hand in stage 2: the stage-1 body was the IDA decompilation with
// syntactic changes only. Every raw offset, hand-written vtable call and vptr
// store is gone; the members and the virtuals come from LawnParseWidget.h.

#include "LawnParseWidget.h"

#include "LawnMenuParser.h"
// ==========================================================================
// LawnMenuParser::LawnParseWidget::~LawnParseWidget()
// @00149080  14 bytes
// ==========================================================================
// The binary's mangled name is LawnMenuParser::LawnParseWidget, but the header
// declares the class at global scope (see LawnParseWidget.h), so the out-of-line
// definition is written with the spelling that names that same class. Same
// arrangement as game/LogComposer.cpp.
//
// The binary's body is the 14-byte vptr store alone, so with `virtual` declared
// on the header the body is empty and the compiler emits the store itself.
// ==========================================================================
LawnParseWidget::~LawnParseWidget()
{
}
