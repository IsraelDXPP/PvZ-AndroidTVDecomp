// XMLWriter -- reconstructed from libGameMain.so.
//
// A polymorphic class with a virtual destructor and nothing else: the binary's
// _ZTVN4Sexy9XMLWriterE holds only the two Itanium destructor slots (D2/D0), and
// it has no base with virtuals of its own (the entries after the table belong to
// the next symbol, EncodingParser). It is therefore declared here as a root class
// with `virtual ~XMLWriter()`.
//
// Layout recovered from the binary's own bodies:
//   +0x04 / +0x08 / +0x18
//                  three libstdc++ COW strings (Sexy::String here). The
//                  constructor @0x3B9488 stores the empty rep (Sexy::gEmptyStringData) into
//                  all three; GetFileName @0x3B7E14 returns a copy of +0x04 and
//                  GetErrorText @0x3B7E00 a copy of +0x08 (both are the sret form
//                  -- r0 is the destination, r1 the object); Fail @0x3B7D80
//                  assigns its message to +0x08; OpenFile assigns the file name to
//                  +0x04; everything the writer emits is appended to +0x18 and
//                  written to the file by CloseFile. So +0x04 is mFileName,
//                  +0x08 mErrorText, +0x18 mOutputText.
//   +0x0C          a counter set to 1 by Init and incremented by WriteHeader (line number)
//   +0x10          a file stream: opened by OpenFile, written/closed by CloseFile
//   +0x14          a bool cleared by Init and set by Fail -- the failure flag
//   +0x15          a bool the constructor clears and nothing else touches
//   +0x16          a bool StartElement sets and StopElement clears; WriteAttribute
//                  refuses to run unless it is set, i.e. the attributes section of
//                  the element being written is still open
//   +0x17          a bool OpenMemoryFile sets and CheckFileOpen accepts in place of
//                  mFile -- "writing into mOutputText rather than a real file"
//   +0x1C          0x400 bytes, memset to 0 by Init -- the in-object text buffer
//   +0x41C / +0x444
//                  the two std::deque<Sexy::String> stacks that the bodies drive by
//                  hand (element names and pending comments), 0x28 bytes each. See
//                  StringDeque below for the word layout.
// The largest offset any body reaches is +0x468; the class is padded to 0x478, the
// size the old placeholder carried.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_XMLWRITER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_XMLWRITER_H

#include "ReconTypes.h"
#include <SDL3/SDL_iostream.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>

namespace Sexy
{

class XMLElement;

class XMLWriter
{
public:
public:
    virtual ~XMLWriter();

    // Not vtable slots; the signatures are the ones the transcribed bodies define.
    int Fail(Sexy::String const &theMessage);
    void *Init();
    // Append returns the output string; OpenMemoryFile tail-calls this.
    Sexy::String *WriteHeader();
    Sexy::String *OpenMemoryFile();
    int HasFailed();
    // Both return the string by value. The binary hands the destination in r0 and
    // the object in r1, which the transcription mistook for a `return this`.
    Sexy::String GetErrorText();
    int GetCurrentLineNum();
    Sexy::String GetFileName();
    int ValidateElementNodeName(Sexy::String const &theName);
    int CheckFileOpen();
    int StopElement();
    int CloseFile();
    int OpenFile(Sexy::String const &theFileName);
    // Returns the comment just stored at the back of mComments.
    Sexy::String *Comment(Sexy::String const &theComment);
    // The message is the second argument of the binary's Warn: it is what
    // StringConcatCStr prefixes "WARNING: " to before the result is pushed on mComments.
    void Warn(Sexy::String const &theWarning);
    int WriteAttribute(Sexy::String const &theName, Sexy::String const &theValue);
    int WriteAttribute(Sexy::String const &theName, int const &theValue);
    int WriteAttribute(Sexy::String const &theName, float const &theValue);
    int StartElement(Sexy::String const &theName);
    int StartElement(Sexy::XMLElement *theElement);
    int AddAttribute(Sexy::XMLElement *theElement, Sexy::String const &theName, Sexy::String const &theValue);
    XMLWriter();

    // -----------------------------------------------------------------------
    // Members. Offsets are absolute byte offsets from the start of the object.
    // -----------------------------------------------------------------------
    Sexy::String mFileName; // +0x04
    Sexy::String mErrorText; // +0x08
    int mLineNum; // +0x0C, set to 1 by Init and incremented by WriteHeader
    SDL_IOStream *mFile; // +0x10, the stream OpenFile/CloseFile work with
    bool mHasFailed; // cleared by Init and set by Fail
    bool mSpareFlag; // cleared by Init; no reader in this tree
    bool mAttributesOpen; // the attributes section is still open
    bool mWritingToMemory; // no stream, the output goes to mOutputText
    Sexy::String mOutputText; // +0x18, everything the writer emits is appended here
    char mBuffer[0x400]; // +0x1C .. +0x41B, memset to 0 by Init
    std::deque<Sexy::String> mElements; // +0x41C .. +0x443, the open-element stack
    std::deque<Sexy::String> mComments; // the pending-comment stack
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::XMLWriter;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_XMLWRITER_H
