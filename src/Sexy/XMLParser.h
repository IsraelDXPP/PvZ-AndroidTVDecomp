// XMLParser reads an XML file or string into elements.
//
// It is an EncodingParser. The vtable replaces OpenFile and SetStringSource.
// The derived area holds the file name, the error text, the line number, the
// section path, the interned string list and the binary buffer.

#pragma once

#include "ReconTypes.h"
#include "Buffer.h"
#include "EncodingParser.h"
#include "SexyAppBase.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy {

class XMLParser : public Sexy::EncodingParser
{
public:
    virtual ~XMLParser();

    virtual int OpenFile(Sexy::String const &theFileName);

    int Fail(Sexy::String const &theError);
    void Init();
    virtual void SetStringSource(Sexy::String const &theSource);
    Sexy::String const * GetString(unsigned int theIdx) const;
    int HasFailed();
    Sexy::String GetErrorText();
    int GetCurrentLineNum();
    Sexy::String GetFileName();
    XMLParser();
    int OpenBinaryFile(Sexy::String const &theFileName);
    int AddAttribute(Sexy::XMLElement *theElement, Sexy::String const &theName, Sexy::String const &theValue);
    int AddAttributeEncoded(Sexy::XMLElement *theElement, Sexy::String const &theName, Sexy::String const &theValue);
    int NextBinaryElement(Sexy::XMLElement *theElement);
    int NextElement(Sexy::XMLElement *theElement);

    Sexy::String mFileName;            // +0x20
    Sexy::String mErrorText;           // +0x24
    int mLineNum;                      // +0x28
    int mLinePad;                      // +0x2C
    bool mHasFailed;                   // +0x30
    bool mInQuote;                     // +0x31
    unsigned char mQuotePad[2];        // +0x32 ~ +0x33
    Sexy::String mSection;             // +0x34
    std::vector<Sexy::String> mStringList;
    int mElementCount;                 // +0x44
    int mCurrentElementIndex;          // +0x48
    bool mIsBinary;                    // +0x4C
    unsigned char mBinaryPad[3];       // +0x4D ~ +0x4F
    Sexy::Buffer mBuffer;              // +0x50
};

} // namespace Sexy

using Sexy::XMLParser;
