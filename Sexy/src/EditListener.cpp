// EditListener is the base callback an edit widget reports through.
// Text hooks do nothing. Allow hooks accept every key, character, and string.

#include "EditListener.h"

void Sexy::EditListener::EditWidgetText(int theId, Sexy::String const& theText)
{
    (void)theId;
    (void)theText;
}

int Sexy::EditListener::AllowKey(int theId, Sexy::KeyCode theKey)
{
    (void)theId;
    (void)theKey;
    return 1;
}

int Sexy::EditListener::AllowChar(int theId, char theChar)
{
    (void)theId;
    (void)theChar;
    return 1;
}

int Sexy::EditListener::AllowChar(int theId, int theChar)
{
    (void)theId;
    (void)theChar;
    return 1;
}

int Sexy::EditListener::AllowText(int theId, Sexy::String const& theText)
{
    (void)theId;
    (void)theText;
    return 1;
}

void Sexy::EditListener::EditWidgetText(int theId, std::wstring const& theText)
{
    (void)theId;
    (void)theText;
}

int Sexy::EditListener::AllowText(int theId, std::wstring const& theText)
{
    (void)theId;
    (void)theText;
    return 1;
}

void Sexy::EditListener::TextChanged(int theId, Sexy::String const& theText)
{
    (void)theId;
    (void)theText;
}
