// TypingCheck matches a typed cheat phrase. The TV object holds only the phrase
// (one Sexy::String). Check compares the typed text against the start of that
// phrase, case-insensitive, and an empty phrase never matches.

#pragma once

#include "ReconTypes.h"

class TypingCheck
{
public:
    Sexy::String mPhrase;

    TypingCheck();
    explicit TypingCheck(const Sexy::String& thePhrase);

    void SetPhrase(const Sexy::String& thePhrase);
    void AddKeyCode(Sexy::KeyCode theKeyCode);
    void AddChar(char theChar);
    bool Check(const Sexy::String& theTyped);
};
