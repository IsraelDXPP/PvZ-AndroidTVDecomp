#include "TypingCheck.h"

#include <cctype>
#include <cstring>

TypingCheck::TypingCheck()
{
}

TypingCheck::TypingCheck(const Sexy::String& thePhrase)
{
    SetPhrase(thePhrase);
}

void TypingCheck::SetPhrase(const Sexy::String& thePhrase)
{
    mPhrase = thePhrase;
}

void TypingCheck::AddKeyCode(Sexy::KeyCode theKeyCode)
{
    mPhrase += (char)theKeyCode;
}

void TypingCheck::AddChar(char theChar)
{
    // The TV body lowercases through glibc's tolower table, then appends the
    // original byte. The phrase LawnApp builds is already lowercase, so the
    // stored character is the one that was typed.
    mPhrase += theChar;
}

bool TypingCheck::Check(const Sexy::String& theTyped)
{
    // The phrase must be at least as long as what was typed, and the typed text
    // must match the start of the phrase. An empty phrase never matches.
    if (mPhrase.empty())
        return false;
    if (mPhrase.length() < theTyped.length())
        return false;
    return strcasecmp(theTyped.c_str(), mPhrase.c_str()) == 0;
}
