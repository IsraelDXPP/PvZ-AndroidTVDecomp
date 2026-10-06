// DescParser turns a descriptor file into a command list. Names in the
// define table are stored upper-case. A value that names a define is
// replaced by that define when a command asks for the value.

#include "DescParser.h"

#include "ReconHelpers.h"
#include "SingleDataElement.h"

#include <cstring>
#include <string>
#include <vector>

namespace Sexy {
int StringToDouble(Sexy::String theText, double* theDest);
}

namespace {

bool SameText(const Sexy::String& theText, const char* theSpelling)
{
    return strcasecmp(theText.c_str(), theSpelling) == 0;
}

Sexy::String UpperKey(const Sexy::String& theText)
{
    Sexy::String key;
    StringToUpper(&key, theText.c_str());
    return key;
}

unsigned int EncodeCodePoint(int theCode, char (&theBytes)[4])
{
    unsigned int code = (unsigned int)theCode;
    if (code < 0x80)
    {
        theBytes[0] = (char)code;
        return 1;
    }
    if (code < 0x800)
    {
        theBytes[0] = (char)(0xC0 | (code >> 6));
        theBytes[1] = (char)(0x80 | (code & 0x3F));
        return 2;
    }
    if (code < 0x10000)
    {
        theBytes[0] = (char)(0xE0 | (code >> 12));
        theBytes[1] = (char)(0x80 | ((code >> 6) & 0x3F));
        theBytes[2] = (char)(0x80 | (code & 0x3F));
        return 3;
    }
    theBytes[0] = (char)(0xF0 | (code >> 18));
    theBytes[1] = (char)(0x80 | ((code >> 12) & 0x3F));
    theBytes[2] = (char)(0x80 | ((code >> 6) & 0x3F));
    theBytes[3] = (char)(0x80 | (code & 0x3F));
    return 4;
}

} // namespace

// One code point becomes the UTF-8 bytes a command name or an ASCII glyph
// already expects. A value above 127 stays as the character FontLayer looks up.
void AppendCodePoint(Sexy::String& theText, int theCode)
{
    char bytes[4];
    unsigned int byteCount = EncodeCodePoint(theCode, bytes);
    theText.Append(bytes, byteCount);
}

int ReadCodePoint(const Sexy::String& theText, unsigned int& theIndex)
{
    if (theIndex >= theText.size())
        return 0;
    unsigned char aLead = (unsigned char)theText[theIndex++];
    if (aLead < 0x80)
        return aLead;

    int aNeed = 0;
    int aCode = 0;
    if ((aLead & 0xE0) == 0xC0)
    {
        aNeed = 1;
        aCode = aLead & 0x1F;
    }
    else if ((aLead & 0xF0) == 0xE0)
    {
        aNeed = 2;
        aCode = aLead & 0x0F;
    }
    else if ((aLead & 0xF8) == 0xF0)
    {
        aNeed = 3;
        aCode = aLead & 0x07;
    }
    else
    {
        return aLead;
    }

    for (int i = 0; i < aNeed; ++i)
    {
        if (theIndex >= theText.size())
            return aLead;
        unsigned char aNext = (unsigned char)theText[theIndex];
        if ((aNext & 0xC0) != 0x80)
            return aLead;
        ++theIndex;
        aCode = (aCode << 6) | (aNext & 0x3F);
    }
    return aCode;
}

Sexy::String CodePointsToUtf8(const std::vector<int>& thePoints)
{
    // Sexy::String owns a copy of its bytes. Build the run in a growable
    // buffer first so drawing text does not copy and free it for every byte.
    std::string text;
    text.reserve(thePoints.size() * 4);
    for (int codePoint : thePoints)
    {
        char bytes[4];
        unsigned int byteCount = EncodeCodePoint(codePoint, bytes);
        text.append(bytes, byteCount);
    }
    return Sexy::String(text);
}

Sexy::DescParser::DescParser()
    : mCmdSep(CMDSEP_SEMICOLON)
    , mCurrentLineNum(0)
{
}

Sexy::DescParser::~DescParser()
{
    for (auto& entry : mDefineMap)
        delete entry.second;
    mDefineMap.clear();
}

bool Sexy::DescParser::Error(const Sexy::String& theError)
{
    if (mError.empty())
        mError = theError;
    return false;
}

Sexy::DataElement* Sexy::DescParser::Dereference(const Sexy::String& theString)
{
    auto found = mDefineMap.find(UpperKey(theString));
    if (found == mDefineMap.end())
        return nullptr;
    return found->second;
}

int Sexy::DescParser::IsImmediate(Sexy::String const& theText)
{
    if (theText.empty())
        return 0;
    const char first = theText[0];
    if ((first >= '0' && first <= '9') || first == '-' || first == '+'
        || first == '\'' || first == '"')
        return 1;
    return 0;
}

Sexy::String Sexy::DescParser::Unquote(Sexy::String const& theText)
{
    if (theText.length() < 2)
        return theText;
    const char quote = theText[0];
    if (quote != '\'' && quote != '"')
        return theText;

    Sexy::String literal;
    bool lastWasQuote = true;
    bool lastWasSlash = false;
    const unsigned int last = theText.length() - 1;
    for (unsigned int i = 1; i < last; ++i)
    {
        const char ch = theText[i];
        if (lastWasSlash)
        {
            literal += ch;
            lastWasSlash = false;
            continue;
        }
        if (ch == quote)
        {
            if (lastWasQuote)
                literal += quote;
            lastWasQuote = true;
        }
        else if (ch == '\\')
        {
            lastWasSlash = true;
            lastWasQuote = false;
        }
        else
        {
            literal += ch;
            lastWasQuote = false;
        }
    }
    return literal;
}

Sexy::String Sexy::DescParser::DataElementToString(const Sexy::DataElement* theElement, bool theNested)
{
    if (theElement == nullptr)
        return Sexy::String();

    if (theElement->mElementKind != 0)
    {
        const ListDataElement* list = (const ListDataElement*)theElement;
        Sexy::String text;
        if (theNested)
            text += "(";
        for (unsigned int i = 0; i < list->mDataElements.size(); ++i)
        {
            if (i != 0)
                text += theNested ? ", " : " ";
            text += DataElementToString(list->mDataElements[i], true);
        }
        if (theNested)
            text += ")";
        return text;
    }

    const SingleDataElement* single = (const SingleDataElement*)theElement;
    if (single->mNext != nullptr)
        return single->mData + "=" + DataElementToString(single->mNext, true);
    return single->mData;
}

int Sexy::DescParser::DataToString(Sexy::DataElement* theElement, Sexy::String* theResult)
{
    if (theResult == nullptr)
        return 0;
    theResult->clear();
    if (theElement == nullptr || theElement->mElementKind != 0)
        return 0;

    SingleDataElement* single = (SingleDataElement*)theElement;
    if (single->mNext != nullptr)
        return 0;

    DataElement* resolved = Dereference(single->mData);
    if (resolved != nullptr)
    {
        if (resolved->mElementKind != 0)
            return 0;
        *theResult = Unquote(((SingleDataElement*)resolved)->mData);
    }
    else
    {
        *theResult = Unquote(single->mData);
    }
    return 1;
}

int Sexy::DescParser::DataToInt(Sexy::DataElement* theElement, int* theResult)
{
    if (theResult == nullptr)
        return 0;
    *theResult = 0;
    Sexy::String text;
    if (DataToString(theElement, &text) == 0)
        return 0;
    return StringToInt(text, theResult);
}

int Sexy::DescParser::DataToDouble(Sexy::DataElement* theElement, double* theResult)
{
    if (theResult == nullptr)
        return 0;
    *theResult = 0.0;
    Sexy::String text;
    if (DataToString(theElement, &text) == 0)
        return 0;
    return Sexy::StringToDouble(text, theResult);
}

int Sexy::DescParser::DataToBoolean(Sexy::DataElement* theElement, bool* theResult)
{
    if (theResult == nullptr)
        return 0;
    *theResult = false;
    Sexy::String text;
    if (DataToString(theElement, &text) == 0)
        return 0;
    if (SameText(text, "false") || SameText(text, "no") || SameText(text, "0"))
        return 1;
    if (SameText(text, "true") || SameText(text, "yes") || SameText(text, "1"))
    {
        *theResult = true;
        return 1;
    }
    return 0;
}

int Sexy::DescParser::GetValues(Sexy::ListDataElement* theSource, Sexy::ListDataElement* theResult)
{
    if (theSource == nullptr || theResult == nullptr)
        return 0;

    for (unsigned int i = 0; i < theResult->mDataElements.size(); ++i)
        delete theResult->mDataElements[i];
    theResult->mDataElements.clear();

    for (unsigned int i = 0; i < theSource->mDataElements.size(); ++i)
    {
        DataElement* child = theSource->mDataElements[i];
        if (child == nullptr)
            continue;
        if (child->mElementKind != 0)
        {
            ListDataElement* nested = new ListDataElement();
            theResult->mDataElements.push_back(nested);
            if (GetValues((ListDataElement*)child, nested) == 0)
                return 0;
            continue;
        }

        SingleDataElement* single = (SingleDataElement*)child;
        if (single->mData.empty())
            continue;

        const char first = single->mData[0];
        if (first == '\'' || first == '"')
        {
            SingleDataElement* value = new SingleDataElement();
            value->mData = Unquote(single->mData);
            theResult->mDataElements.push_back(value);
        }
        else if (IsImmediate(single->mData) != 0)
        {
            SingleDataElement* value = new SingleDataElement();
            value->mData = single->mData;
            theResult->mDataElements.push_back(value);
        }
        else
        {
            auto found = mDefineMap.find(UpperKey(single->mData));
            if (found == mDefineMap.end() || found->second == nullptr)
            {
                Error(Sexy::String("Unable to Dereference \"") + single->mData + "\"");
                return 0;
            }
            theResult->mDataElements.push_back(found->second->Duplicate());
        }
    }
    return 1;
}

int Sexy::DescParser::DataToList(Sexy::DataElement* theElement, Sexy::ListDataElement* theResult)
{
    if (theElement == nullptr || theResult == nullptr)
        return 0;
    if (theElement->mElementKind != 0)
        return GetValues((ListDataElement*)theElement, theResult);

    DataElement* resolved = Dereference(((SingleDataElement*)theElement)->mData);
    if (resolved == nullptr || resolved->mElementKind == 0)
        return 0;

    for (unsigned int i = 0; i < theResult->mDataElements.size(); ++i)
        delete theResult->mDataElements[i];
    theResult->mDataElements.clear();
    ListDataElement* list = (ListDataElement*)resolved;
    for (unsigned int i = 0; i < list->mDataElements.size(); ++i)
        theResult->mDataElements.push_back(list->mDataElements[i]->Duplicate());
    return 1;
}

int Sexy::DescParser::DataToStringVector(Sexy::DataElement* theElement, std::vector<Sexy::String>* theResult)
{
    if (theElement == nullptr || theResult == nullptr)
        return 0;
    theResult->clear();

    ListDataElement held;
    ListDataElement* values = nullptr;
    if (theElement->mElementKind != 0)
    {
        if (GetValues((ListDataElement*)theElement, &held) == 0)
            return 0;
        values = &held;
    }
    else
    {
        DataElement* resolved = Dereference(((SingleDataElement*)theElement)->mData);
        if (resolved == nullptr)
        {
            Error(Sexy::String("Unable to Dereference \"") + ((SingleDataElement*)theElement)->mData + "\"");
            return 0;
        }
        if (resolved->mElementKind == 0)
            return 0;
        values = (ListDataElement*)resolved;
    }

    for (unsigned int i = 0; i < values->mDataElements.size(); ++i)
    {
        if (values->mDataElements[i] == nullptr || values->mDataElements[i]->mElementKind != 0)
        {
            theResult->clear();
            return 0;
        }
        theResult->push_back(((SingleDataElement*)values->mDataElements[i])->mData);
    }
    return 1;
}

int Sexy::DescParser::DataToIntVector(Sexy::DataElement* theElement, std::vector<int>* theResult)
{
    if (theResult == nullptr)
        return 0;
    theResult->clear();
    std::vector<Sexy::String> text;
    if (DataToStringVector(theElement, &text) == 0)
        return 0;
    for (unsigned int i = 0; i < text.size(); ++i)
    {
        int value = 0;
        if (StringToInt(text[i], &value) == 0)
            return 0;
        theResult->push_back(value);
    }
    return 1;
}

int Sexy::DescParser::DataToDoubleVector(Sexy::DataElement* theElement, std::vector<double>* theResult)
{
    if (theResult == nullptr)
        return 0;
    theResult->clear();
    std::vector<Sexy::String> text;
    if (DataToStringVector(theElement, &text) == 0)
        return 0;
    for (unsigned int i = 0; i < text.size(); ++i)
    {
        double value = 0.0;
        if (Sexy::StringToDouble(text[i], &value) == 0)
            return 0;
        theResult->push_back(value);
    }
    return 1;
}

int Sexy::DescParser::ParseToList(const std::vector<int>& theText, Sexy::ListDataElement* theList,
                                  bool theExpectListEnd, int* thePos)
{
    if (theList == nullptr)
        return 0;

    bool inSingle = false;
    bool inDouble = false;
    bool escaped = false;
    bool endKey = false;
    SingleDataElement* key = nullptr;
    SingleDataElement* current = nullptr;
    int localPos = 0;
    if (thePos == nullptr)
        thePos = &localPos;

    while (*thePos < (int)theText.size())
    {
        bool addChar = false;
        const int ch = theText[(unsigned int)((*thePos)++)];
        const bool separator = ch == ' ' || ch == '\t' || ch == '\n' || ch == ',';

        if (escaped)
        {
            addChar = true;
        }
        else
        {
            if (ch == '\'' && !inDouble)
                inSingle = !inSingle;
            else if (ch == '"' && !inSingle)
                inDouble = !inDouble;

            if (ch == '\\')
            {
                escaped = true;
            }
            else if (!inSingle && !inDouble)
            {
                if (ch == ')')
                {
                    if (theExpectListEnd)
                        return 1;
                    Error("Unexpected List End");
                    return 0;
                }
                if (ch == '(')
                {
                    if (endKey)
                    {
                        current = nullptr;
                        endKey = false;
                    }
                    if (current != nullptr)
                    {
                        Error("Unexpected List Start");
                        return 0;
                    }
                    ListDataElement* child = new ListDataElement();
                    if (ParseToList(theText, child, true, thePos) == 0)
                        return 0;
                    if (key != nullptr)
                    {
                        key->mNext = child;
                        key = nullptr;
                    }
                    else
                    {
                        theList->mDataElements.push_back(child);
                    }
                }
                else if (ch == '=')
                {
                    key = current;
                    endKey = true;
                }
                else if (separator)
                {
                    if (current != nullptr && !current->mData.empty())
                        current = nullptr;
                }
                else
                {
                    if (endKey)
                    {
                        current = nullptr;
                        endKey = false;
                    }
                    addChar = true;
                }
            }
            else
            {
                if (endKey)
                {
                    current = nullptr;
                    endKey = false;
                }
                addChar = true;
            }
        }

        if (!addChar)
            continue;

        if (current == nullptr)
        {
            current = new SingleDataElement();
            if (key != nullptr)
            {
                key->mNext = current;
                key = nullptr;
            }
            else
            {
                theList->mDataElements.push_back(current);
            }
        }
        if (escaped)
        {
            AppendCodePoint(current->mData, '\\');
            escaped = false;
        }
        AppendCodePoint(current->mData, ch);
    }

    if (inSingle)
    {
        Error("Unterminated Single Quotes");
        return 0;
    }
    if (inDouble)
    {
        Error("Unterminated Double Quotes");
        return 0;
    }
    if (theExpectListEnd)
    {
        Error("Unterminated List");
        return 0;
    }
    return 1;
}

int Sexy::DescParser::ParseDescriptorLine(const std::vector<int>& theLine)
{
    ListDataElement params;
    if (ParseToList(theLine, &params, false, nullptr) == 0)
        return 0;
    if (params.mDataElements.empty())
        return 1;
    if (params.mDataElements[0]->mElementKind != 0)
    {
        Error("Missing Command");
        return 0;
    }
    if (!HandleCommand(params))
        return 0;
    return 1;
}

bool Sexy::DescParser::LoadDescriptor(const Sexy::String& theFileName)
{
    mCurrentLineNum = 0;
    mError.clear();
    mCurrentLine.clear();
    int lineCount = 0;

    if (OpenFile(theFileName) == 0)
        return Error(Sexy::String("Unable to open file: ") + theFileName);

    while (!EndOfFile())
    {
        bool skipLine = false;
        bool atLineStart = true;
        bool inSingle = false;
        bool inDouble = false;
        bool escaped = false;
        bool indented = false;

        for (;;)
        {
            wchar_t ch = 0;
            const int got = GetChar(&ch);
            if (got == 2)
                break;
            if (got == 1)
                return Error("Invalid Character");
            if (got != 0)
                return Error("Internal Error");
            if (ch == L'\r')
                continue;
            if (ch == L'\n')
                ++lineCount;

            if ((ch == L' ' || ch == L'\t') && atLineStart)
                indented = true;

            if (atLineStart && (ch == L' ' || ch == L'\t' || ch == L'\n'))
            {
                if (ch == L'\n')
                {
                    indented = false;
                    atLineStart = true;
                }
                continue;
            }

            if (atLineStart)
            {
                if ((mCmdSep & CMDSEP_NO_INDENT) != 0 && !indented && !mCurrentLine.empty())
                {
                    PutChar(ch);
                    break;
                }
                if (ch == L'#')
                    skipLine = true;
                atLineStart = false;
            }

            if (ch == L'\n')
            {
                indented = false;
                atLineStart = true;
            }

            if (ch == L'\n' && skipLine)
            {
                skipLine = false;
                continue;
            }
            if (skipLine)
                continue;

            if (ch == L'\\' && (inSingle || inDouble) && !escaped)
            {
                escaped = true;
                continue;
            }
            if (ch == L'\'' && !inDouble && !escaped)
                inSingle = !inSingle;
            if (ch == L'"' && !inSingle && !escaped)
                inDouble = !inDouble;
            if (ch == L';' && (mCmdSep & CMDSEP_SEMICOLON) != 0 && !inSingle && !inDouble)
                break;

            if (escaped)
            {
                mCurrentLine.push_back('\\');
                escaped = false;
            }
            if (mCurrentLine.empty())
                mCurrentLineNum = lineCount + 1;
            // Keep the code point. Casting to char splits a quoted CJK
            // character and the next byte is read as a new quote.
            mCurrentLine.push_back((int)ch);
        }

        if (!mCurrentLine.empty())
        {
            if (ParseDescriptorLine(mCurrentLine) == 0)
            {
                mCurrentLine.clear();
                mCurrentLineNum = 0;
                CloseFile();
                return false;
            }
            mCurrentLine.clear();
        }
    }

    mCurrentLine.clear();
    mCurrentLineNum = 0;
    CloseFile();
    return true;
}
