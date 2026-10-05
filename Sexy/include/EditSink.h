// EditSink -- the interface an edit widget talks to while the keyboard is up.
//
// Pure interface, no base, no data. The seven slots the binary leaves pure have
// no recovered name; they stay as named hooks so the slot order matches.
// MSVC gives the destructor one slot where the binary gives it two, so only
// the order is copied.

#pragma once

#include "ReconTypes.h"

namespace Sexy {

class EditController;

class EditSink
{
public:
    virtual ~EditSink();
    virtual int GetEditWidget();
    virtual void UnknownPureVirtual3();
    virtual void UnknownPureVirtual4();
    virtual void UnknownPureVirtual5();
    virtual void UnknownPureVirtual6();
    virtual void UnknownPureVirtual7();
    virtual int GetMultiLineMode();
    virtual void UnknownPureVirtual9();
    virtual void UnknownPureVirtual10(int theFirst, int theSecond);
    virtual void SetEditMark(int theStart, int theEnd);
    virtual int SetEditState(Sexy::EditState const& theState);
    virtual void KeyboardWasShown(Sexy::TRect<int> const& theRect);
    virtual void KeyboardWillBeHidden();
    virtual int EditFinished();
    virtual void EditCancelled();
    virtual void ShowKeyboard();
    virtual void HideKeyboard();
    virtual EditController* GetEditController();
};

} // namespace Sexy
