#include "EditSink.h"

int Sexy::EditSink::GetMultiLineMode()
{
    return 0;
}

void Sexy::EditSink::SetEditMark(int theStart, int theEnd)
{
    (void)theStart;
    (void)theEnd;
}

Sexy::EditSink::~EditSink()
{
}

int Sexy::EditSink::GetEditWidget()
{
    return 0;
}

void Sexy::EditSink::UnknownPureVirtual3() {}
void Sexy::EditSink::UnknownPureVirtual4() {}
void Sexy::EditSink::UnknownPureVirtual5() {}
void Sexy::EditSink::UnknownPureVirtual6() {}
void Sexy::EditSink::UnknownPureVirtual7() {}
void Sexy::EditSink::UnknownPureVirtual9() {}
void Sexy::EditSink::UnknownPureVirtual10(int theFirst, int theSecond)
{
    (void)theFirst;
    (void)theSecond;
}

// Pushes the five words of the state through the edit interface: slot 9 with
// no arguments, slot 10 with words 1 and 2, SetEditMark with words 3 and 4.
// Word 0 is never loaded. EditState is only forward-declared, so the words
// are read through a same-sized view.
int Sexy::EditSink::SetEditState(Sexy::EditState const& theState)
{
    struct EditStateWords
    {
        int mWord0;
        int mWord1;
        int mWord2;
        int mWord3;
        int mWord4;
    };
    const EditStateWords* theWords = static_cast<const EditStateWords*>(static_cast<const void*>(&theState));

    UnknownPureVirtual9();
    UnknownPureVirtual10(theWords->mWord1, theWords->mWord2);
    SetEditMark(theWords->mWord3, theWords->mWord4);
    return 0;
}

void Sexy::EditSink::KeyboardWasShown(Sexy::TRect<int> const& theRect)
{
    (void)theRect;
}

void Sexy::EditSink::KeyboardWillBeHidden()
{
}

int Sexy::EditSink::EditFinished()
{
    return 1;
}

void Sexy::EditSink::EditCancelled()
{
}

void Sexy::EditSink::ShowKeyboard()
{
}

void Sexy::EditSink::HideKeyboard()
{
}

Sexy::EditController* Sexy::EditSink::GetEditController()
{
    return nullptr;
}
