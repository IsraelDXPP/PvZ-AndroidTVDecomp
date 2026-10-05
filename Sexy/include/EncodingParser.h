// EncodingParser reads a file or an in-memory string and hands back one
// character at a time. The encoding is selected by a member function that
// GetChar calls; the constructor starts that function on UTF-8.
//
// The virtuals, in order: destructor, SetEncodingType, OpenFile, CloseFile,
// EndOfFile, SetStringSource, GetChar, PutChar, PutString. CloseFile and
// EndOfFile keep the return types their overrides already use.
//
// A convenience SetStringSource that widens a narrow string and re-enters the
// virtual is folded into the one signature below, because Sexy::String already
// holds the characters this port stores.
//
// Layout, after the compiler's vptr:
//   mFile                  the stdio stream OpenFile opened
//   mBufferedText          characters PutChar / SetStringSource pushed ahead
//                          of the file
//   mGetCharFunc           which decoder GetChar calls
//   mGetCharFuncAdj        the unused second word of that member pointer
//   mForcedEncodingType    set when the caller picked an encoding by hand
//   mFirstChar             the first character of a UTF-16 stream
//   mByteSwap              set when the byte order does not match the host

#pragma once

#include "ReconTypes.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy {

class EncodingParser
{
public:
    virtual ~EncodingParser() = default;

    virtual int SetEncodingType(int theEncoding);
    virtual int OpenFile(Sexy::String const& thePath);
    virtual int CloseFile();
    virtual bool EndOfFile();
    virtual void SetStringSource(Sexy::String const& theSource);
    virtual int GetChar(wchar_t *theChar);
    virtual int PutChar(wchar_t const& theChar);
    virtual int PutString(Sexy::String const& theText);

    FILE* mFile;
    std::vector<wchar_t> mBufferedText;
    int (Sexy::EncodingParser::*mGetCharFunc)(wchar_t *, bool *);
    int mGetCharFuncAdj;
    bool mForcedEncodingType;
    bool mFirstChar;
    bool mByteSwap;

    int GetAsciiChar(wchar_t *theChar, bool *theFirstChar);
    int GetUTF8Char(wchar_t *theChar, bool *theFirstChar);
    int GetUTF16Char(wchar_t *theChar, bool *theFirstChar);
    int GetUTF16LEChar(wchar_t *theChar, bool *theFirstChar);
    int GetUTF16BEChar(wchar_t *theChar, bool *theFirstChar);

    EncodingParser();
};

} // namespace Sexy
