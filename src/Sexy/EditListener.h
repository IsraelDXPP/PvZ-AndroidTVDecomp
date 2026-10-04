// EditListener -- the callback an edit widget reports text changes through.
//
// Pure interface. The binary table holds eight slots and no destructor, so
// this class has eight virtual functions and no virtual destructor: adding
// one would shift every derived class.
//
// Each narrow slot has a wide overload next to it. UserDialog is one consumer.

#pragma once

#include "ReconTypes.h"
#include <string>

namespace Sexy {

class EditListener
{
public:
    virtual void EditWidgetText(int theId, Sexy::String const& theText);
    virtual void EditWidgetText(int theId, std::wstring const& theText);
    virtual int AllowKey(int theId, Sexy::KeyCode theKey);
    virtual int AllowChar(int theId, char theChar);
    virtual int AllowChar(int theId, int theChar);
    virtual int AllowText(int theId, Sexy::String const& theText);
    virtual int AllowText(int theId, std::wstring const& theText);
    virtual void TextChanged(int theId, Sexy::String const& theText);
};

} // namespace Sexy

using Sexy::EditListener;
