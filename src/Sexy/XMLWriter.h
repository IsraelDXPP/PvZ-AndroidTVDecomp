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
//                  fputs'd to the file by CloseFile. So +0x04 is mFileName,
//                  +0x08 mErrorText, +0x18 mOutputText.
//   +0x0C          a counter set to 1 by Init and incremented by WriteHeader (line number)
//   +0x10          a FILE*: fopen's result in OpenFile, fputs/fclose'd in CloseFile
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

#pragma once

#include "ReconTypes.h"
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy {

class XMLElement;

class XMLWriter
{
public:
    // A std::deque<Sexy::String> as libstdc++ lays one out: the node map, the map
    // size, then the _M_start and _M_finish _Deque_iterators, each of which is
    // { _M_cur, _M_first, _M_last, _M_node }. sizeof(Sexy::String) == 4, so a node
    // buffer holds 512/4 = 128 elements and the bodies' 512 / 508 / 4 byte
    // distances are 128 elements, the last element and one element.
    //
    // The iterators are Sexy::String pointers. A node holds 128 of them.
    struct StringDeque
    {
        Sexy::String **mMap;        // +0x00  _M_map
        unsigned int mMapSize;       // +0x04
        Sexy::String *mStartCur;     // +0x08  _M_start._M_cur
        Sexy::String *mStartFirst;   // +0x0C  _M_start._M_first
        Sexy::String *mStartLast;    // +0x10  _M_start._M_last
        Sexy::String **mStartNode;   // +0x14  _M_start._M_node
        Sexy::String *mFinishCur;    // +0x18  _M_finish._M_cur
        Sexy::String *mFinishFirst;  // +0x1C  _M_finish._M_first
        Sexy::String *mFinishLast;   // +0x20  _M_finish._M_last
        Sexy::String **mFinishNode;  // +0x24  _M_finish._M_node

        // libstdc++ deque<String>::size: 128 strings per 512-byte node.
        int Size() const
        {
            return (int)(mFinishCur - mFinishFirst)
                + (((int)(mFinishNode - mStartNode) - 1) << 7)
                + (int)(mStartLast - mStartCur);
        }

        // The string just before _M_finish, wrapping to the previous node
        // when the cursor sits on the first slot of this node.
        Sexy::String *Back() const
        {
            if (mFinishCur == mFinishFirst)
                return *(mFinishNode - 1) + 127;
            return mFinishCur - 1;
        }

        // Destroy that string and step _M_finish back. At a node boundary the
        // emptied node buffer is freed and the iterator moves to the previous
        // node's last string. Matches StopElement @0x3B7F2C.
        void PopBack()
        {
            if (mFinishCur == mFinishFirst)
            {
                operator delete(mFinishCur);
                Sexy::String **aNode = mFinishNode - 1;
                mFinishNode = aNode;
                Sexy::String *aFirst = *aNode;
                mFinishFirst = aFirst;
                mFinishLast = aFirst + 128;
                mFinishCur = aFirst + 127;
                (aFirst + 127)->~String();
            }
            else
            {
                --mFinishCur;
                mFinishCur->~String();
            }
        }
    };

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
    Sexy::String mFileName;  // +0x04
    Sexy::String mErrorText; // +0x08
    int mLineNum;            // +0x0C, set to 1 by Init and incremented by WriteHeader
    FILE *mFile;             // +0x10, the FILE* OpenFile/CloseFile work with
    bool mHasFailed;         // cleared by Init and set by Fail
    bool mSpareFlag;         // cleared by Init; no reader in this tree
    bool mAttributesOpen;    // the attributes section is still open
    bool mWritingToMemory;   // no FILE*, the output goes to mOutputText
    Sexy::String mOutputText; // +0x18, everything the writer emits is appended here
    char mBuffer[0x400];     // +0x1C .. +0x41B, memset to 0 by Init
    StringDeque mElements;   // +0x41C .. +0x443, the open-element stack
    StringDeque mComments;   // the pending-comment stack
};

// std::_Deque_base<string>::_M_initialize_map @0x3B8980. The body is in
// CxxRuntime.cpp; the declaration lives here because StringDeque is a member
// of XMLWriter and is not complete until this class closes.
namespace StlInternals {
void _M_initialize_map(XMLWriter::StringDeque* theDeque, size_t theCount);
}

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::XMLWriter;
