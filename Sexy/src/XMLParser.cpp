// XMLParser -- reconstructed from libGameMain.so.
//
// The framework's XML reader, polymorphic on top of Sexy::EncodingParser (the
// character source). It reads either the binary chunk format NextBinaryElement
// walks or the text format NextElement scans, and fills one XMLElement per call.
//
// Two shapes the dump got wrong and this file spells correctly:
//   * GetErrorText/GetFileName are the sret form (ARM passes the destination
//     String in r0), so their definitions return by value.
//   * The element's character/tag buffers are the wide strings XMLElement owns;
//     this tree models every string as Sexy::String, so they are spelled that
//     way here (see Runtime\SexyString.h).
//
// The reconstruction helpers this file used to call (StringAssign StringAssign,
// StringDispose the destructor, GrowIntString the COW grow, ...) are now the ordinary
// String operations they stand for.

#include "XMLParser.h"
#include "Buffer.h"
#include "EncodingParser.h"
#include "SexyAppBase.h"
#include "XMLElement.h"
#include "ReconHelpers.h"
#include <pugixml.hpp>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstring>
#include <algorithm>
#include <cctype>

// The text walk used to be the 5776-byte character state machine at 0x3B5720.
// Callers only consume the elements that machine emitted: type 1 start, type 2
// end, type 3 text, type 6 character data, with the tag name in mInstruction
// and attributes in the element's map. pugixml parses the document; the cursor
// below is that same sequence. It cannot live in the 0x6C object, so it is
// keyed by the parser pointer and dropped in the destructor.
namespace
{

struct XmlEvent
{
    int mType = 0;
    std::string mInstruction;
    std::string mText;
    std::string mValue;
    std::string mSectionAfter;
    int mLine = 1;
    std::vector<std::pair<std::string, std::string>> mAttrs;
};

struct XmlCursor
{
    std::vector<XmlEvent> mEvents;
    size_t mIndex = 0;
};

std::map<const Sexy::XMLParser*, XmlCursor> gXmlCursors;

bool IsSpaceOnly(const char* theText)
{
    if (theText == nullptr)
        return true;
    for (const char* cursor = theText; *cursor != 0; ++cursor)
    {
        if (isspace((unsigned char)*cursor) == 0)
            return false;
    }
    return true;
}

int LineAt(const std::vector<size_t>& theNewlines, ptrdiff_t theOffset)
{
    if (theOffset <= 0) return 1;
    const auto position = std::lower_bound(theNewlines.begin(), theNewlines.end(), static_cast<size_t>(theOffset));
    return 1 + static_cast<int>(position - theNewlines.begin());
}

void AppendNode(std::vector<XmlEvent>& theEvents, pugi::xml_node theNode,
                std::string& theSection, const std::vector<size_t>& theNewlines)
{
    // A processing instruction, including the "<?xml ...?>" declaration. The
    // manifest reader skips a start tag whose name begins with '?', so the
    // declaration has to reach it. Dropping the node made the parser report
    // the first line as a broken start tag.
    if (theNode.type() == pugi::node_declaration || theNode.type() == pugi::node_pi)
    {
        XmlEvent aEvent;
        aEvent.mType = 1;
        aEvent.mInstruction = std::string("?") + theNode.name();
        aEvent.mValue = theSection;
        aEvent.mSectionAfter = theSection;
        aEvent.mLine = LineAt(theNewlines, theNode.offset_debug());
        for (pugi::xml_attribute aAttr = theNode.first_attribute(); aAttr; aAttr = aAttr.next_attribute())
            aEvent.mAttrs.emplace_back(aAttr.name(), aAttr.value());
        theEvents.push_back(aEvent);
        return;
    }
    if (theNode.type() == pugi::node_pcdata || theNode.type() == pugi::node_cdata)
    {
        if (IsSpaceOnly(theNode.value()))
            return;
        XmlEvent aEvent;
        aEvent.mType = theNode.type() == pugi::node_cdata ? 6 : 3;
        aEvent.mText = theNode.value();
        aEvent.mValue = theSection;
        aEvent.mSectionAfter = theSection;
        aEvent.mLine = LineAt(theNewlines, theNode.offset_debug());
        theEvents.push_back(aEvent);
        return;
    }
    if (theNode.type() != pugi::node_element)
        return;

    XmlEvent aStart;
    aStart.mType = 1;
    aStart.mInstruction = theNode.name();
    aStart.mValue = theSection;
    aStart.mLine = LineAt(theNewlines, theNode.offset_debug());
    for (pugi::xml_attribute aAttr = theNode.first_attribute(); aAttr; aAttr = aAttr.next_attribute())
        aStart.mAttrs.emplace_back(aAttr.name(), aAttr.value());

    if (!theSection.empty())
        theSection += "/";
    theSection += theNode.name();
    aStart.mSectionAfter = theSection;
    int aLine = aStart.mLine;
    theEvents.push_back(std::move(aStart));

    for (pugi::xml_node aChild = theNode.first_child(); aChild; aChild = aChild.next_sibling())
        AppendNode(theEvents, aChild, theSection, theNewlines);

    XmlEvent aEnd;
    aEnd.mType = 2;
    aEnd.mInstruction = theNode.name();
    aEnd.mValue = theSection;
    aEnd.mLine = aLine;
    size_t aSlash = theSection.rfind('/');
    if (aSlash == std::string::npos)
        theSection.clear();
    else
        theSection.erase(aSlash);
    aEnd.mSectionAfter = theSection;
    theEvents.push_back(aEnd);
}

std::string ReadOpenStream(SDL_IOStream* theFile)
{
    std::string aBytes;
    char aChunk[4096];
    for (;;)
    {
        size_t aRead = p_fread(aChunk, 1, sizeof(aChunk), theFile);
        if (aRead == 0)
            break;
        aBytes.append(aChunk, aRead);
    }
    return aBytes;
}

void LoadXmlText(Sexy::XMLParser* theParser, const char* theText, size_t theSize)
{
    XmlCursor& aCursor = gXmlCursors[theParser];
    aCursor.mEvents.clear();
    aCursor.mIndex = 0;
    if (theText == nullptr)
    {
        theText = "";
        theSize = 0;
    }
    pugi::xml_document aDoc;
    pugi::xml_parse_result aResult = aDoc.load_buffer(
        theText, theSize, pugi::parse_default, pugi::encoding_auto);
    if (!aResult)
    {
        fprintf(stderr, "XML parse failed at %d of %u, head:",
                (int)aResult.offset, (unsigned)theSize);
        unsigned aShow = theSize < 24 ? (unsigned)theSize : 24;
        for (unsigned i = 0; i < aShow; ++i)
            fprintf(stderr, " %02X", (unsigned char)theText[i]);
        fprintf(stderr, "\n");
        fflush(stderr);
        theParser->Fail(aResult.description());
        return;
    }
    // Compute line offsets once. Scanning the whole prefix for every node was
    // quadratic for the unified levels.xml and delayed every stage selection.
    std::vector<size_t> newlines;
    for (size_t offset = 0; offset < theSize; ++offset)
        if (theText[offset] == '\n') newlines.push_back(offset);
    std::string aSection;
    for (pugi::xml_node aChild = aDoc.first_child(); aChild; aChild = aChild.next_sibling())
        AppendNode(aCursor.mEvents, aChild, aSection, newlines);
}

} // namespace

// Both element walks start each round by emptying the element they fill.
static void ResetElement(Sexy::XMLElement* theElement, const Sexy::String& theSection)
{
    theElement->mType = 0;
    theElement->mValue = theSection;
    theElement->mValueEncoded.clear();
    theElement->mInstruction.clear();
    theElement->mText.clear();
    theElement->mExtra.clear();
    theElement->mComment.clear();
    theElement->mAttributes.clear();
    theElement->mAttributesEncoded.clear();
}

// Sexy::Trim @0x2FA90C (`_ZN4Sexy4TrimERKSs`) has no home in this tree; it
// strips the leading and trailing whitespace, which is all the text walk needs
// it for.
static Sexy::String TrimString(const Sexy::String & theText)
{
    unsigned int aStart = 0;
    unsigned int anEnd = theText.size();
    while (aStart < anEnd && isspace((unsigned char)theText[aStart]))
        ++aStart;
    while (anEnd > aStart && isspace((unsigned char)theText[anEnd - 1]))
        --anEnd;
    return theText.substr(aStart, anEnd - aStart);
}

// StringRFindChar(&theText, '/', npos) -- the offset of the LAST '/' in the text, or
// -1. The section path uses it to cut itself back one component.
static int FindLastSlash(const Sexy::String & theText)
{
    for (int i = (int)theText.size() - 1; i >= 0; --i)
    {
        if (theText[i] == '/')
            return i;
    }
    return -1;
}

// ==========================================================================
// Sexy::XMLParser::Fail(Sexy::String const&)
// @003B459C  10 bytes
// ==========================================================================
// Fail records the message and the flag; the binary hands the assignment's
// result register back, which is what the int return type is.
int Sexy::XMLParser::Fail(Sexy::String const& theErrorText)
{
    mHasFailed = true;
    mErrorText = theErrorText;
    // The binary returns the assignment's result register, which happens to
    // hold the error text. Callers only test it for zero, so any non-zero
    // value is the same answer.
    return 1;
}

// ==========================================================================
// Sexy::XMLParser::OpenFile(Sexy::String const&)
// @003B6F50
// ==========================================================================
// The override reads the file into the character buffer and resets the parser,
// which is what a later NextElement walks. Nothing on the loading path calls
// it; the manifest comes through SetStringSource.
int Sexy::XMLParser::OpenFile(Sexy::String const& thePath)
{
    mFileName = thePath;
    Init();
    int aOpened = Sexy::EncodingParser::OpenFile(thePath);
    if (aOpened == 0)
        return 0;
    std::string aBytes = ReadOpenStream(mFile);
    LoadXmlText(this, aBytes.data(), aBytes.size());
    return aOpened;
}

// ==========================================================================
// Sexy::XMLParser::Init(void)
// @003B45A8  140 bytes
// ==========================================================================
void Sexy::XMLParser::Init()
{
    mSection = "";
    mLineNum = 1;
    mHasFailed = false;
    mErrorText = "";
    mStringList.clear();
    mElementCount = 0;
    mCurrentElementIndex = 0;
    mIsBinary = false;
    gXmlCursors.erase(this);
}

// ==========================================================================
// Sexy::XMLParser::SetStringSource(Sexy::String const&)
// @003B4640  50 bytes
// ==========================================================================
// Widens the text and hands it to the virtual slot 6. The binary's slot 6 holds
// the wide-string overload of SetStringSource (XMLParser's own @0x3B7034, which
// is the base's implementation); this tree spells both overloads with
// Sexy::String, so the call is written qualified to reach the base rather than
// itself.
void Sexy::XMLParser::SetStringSource(Sexy::String const& theString)
{
    // The dump's slot-6 override @0x3B7034 is two calls:
    //
    //     Sexy::XMLParser::Init(this);
    //     Sexy::EncodingParser::SetStringSource(this, theString);
    //
    // The Init is not decoration. ResourceManager::ParseResourcesFile tries the
    // compiled `.bxml` first and, when that open fails, OpenBinaryFile calls
    // Fail() -- which sets mHasFailed. This override is what clears it again on
    // the text fallback. Skipping it left the flag set, so the parse walk bailed
    // on its very first HasFailed() check with the manifest text sitting in the
    // parser intact, and every resource group stayed unloaded.
    Init();
    Sexy::String aWide;
    StringToWString(&aWide, theString.c_str());
    Sexy::EncodingParser::SetStringSource(aWide);
    LoadXmlText(this, theString.c_str(), theString.size());
}

// ==========================================================================
// Sexy::XMLParser::GetString(unsigned short)const
// @003B4690  20 bytes
// ==========================================================================
Sexy::String const * Sexy::XMLParser::GetString(unsigned int theIdx) const
{
    if (theIdx >= mStringList.size())
        return nullptr;
    return &mStringList[theIdx];
}

// ==========================================================================
// Sexy::XMLParser::HasFailed(void)
// @003B46A4  6 bytes
// ==========================================================================
int Sexy::XMLParser::HasFailed()
{
    return mHasFailed;
}

// ==========================================================================
// Sexy::XMLParser::GetErrorText(void)
// @003B46AC  14 bytes
// ==========================================================================
// The destination String arrives in r0, so the parameter-less declaration this
// tree carried was the sret destination mistaken for a receiver.
Sexy::String Sexy::XMLParser::GetErrorText()
{
    return mErrorText;
}

// ==========================================================================
// Sexy::XMLParser::GetCurrentLineNum(void)
// @003B46BC  4 bytes
// ==========================================================================
int Sexy::XMLParser::GetCurrentLineNum()
{
    return mLineNum;
}

// ==========================================================================
// Sexy::XMLParser::GetFileName(void)
// @003B46C0  14 bytes
// ==========================================================================
// Same sret form as GetErrorText: the binary copies mFileName into the
// destination the caller passes in r0.
Sexy::String Sexy::XMLParser::GetFileName()
{
    return mFileName;
}

// ==========================================================================
// Sexy::XMLParser::XMLParser(void)
// @003B4754  74 bytes
// ==========================================================================
Sexy::XMLParser::~XMLParser()
{
    gXmlCursors.erase(this);
}

Sexy::XMLParser::XMLParser()
{
    mFileName = "";
    mErrorText = "";
    mSection = "";

    mLineNum = 0;
    mLinePad = 0;
    mInQuote = false;
    mElementCount = 0;
    mIsBinary = false;
}

// ==========================================================================
// Sexy::XMLParser::OpenBinaryFile(Sexy::String const&)
// @003B494C  888 bytes
// ==========================================================================
int Sexy::XMLParser::OpenBinaryFile(Sexy::String const& theFileName)
{
    if (gSexyAppBase == 0
        || ((Sexy::SexyAppBase *)gSexyAppBase)->ReadBufferFromFile(theFileName, &mBuffer, false) == 0
        || mBuffer.mDataBitSize <= 120)
    {
        mLineNum = 0;
        // The message is the address of a String, not its text.
        Sexy::String aText("Unable to open file ");
        aText += theFileName;
        Sexy::String aMessage;
        StringToSexyString(&aMessage, aText.c_str());
        Fail(aMessage);
        return 0;
    }

    // The chunk file opens with the magic "BXML".
    unsigned char aMagic[4];
    aMagic[0] = 0;
    aMagic[1] = 0;
    aMagic[2] = 0;
    aMagic[3] = 0;
    mBuffer.ReadBytes(aMagic, 4);
    if (aMagic[0] != 'B' || aMagic[1] != 'X' || aMagic[2] != 'M' || aMagic[3] != 'L')
    {
        Sexy::String aText("Bad magic in ");
        aText += theFileName;
        Sexy::String aMessage;
        StringToSexyString(&aMessage, aText.c_str());
        Fail(aMessage);
        return 0;
    }

    if (mBuffer.ReadInt32() != 1)
    {
        Sexy::String aMessage;
        StringToSexyString(&aMessage, "Unsupported version");
        Fail(aMessage);
        return 0;
    }

    mFileName = theFileName;
    Init();
    mIsBinary = true;

    // The string table: each entry is a length word followed by an index, and
    // the strings themselves follow, sized by those indexes.
    unsigned int aCount = mBuffer.ReadInt32();

    std::vector<int> aStringSizes;
    if (aCount != 0)
    {
        if (aCount >= 0x40000000)
            ThrowLengthError("vector::reserve");
        aStringSizes.reserve(aCount);
        for (unsigned int i = 0; i < aCount; ++i)
        {
            mBuffer.ReadInt32();
            aStringSizes.push_back(mBuffer.ReadInt16());
        }
    }

    mStringList.reserve(aCount);
    for (unsigned int i = 0; i < aCount; ++i)
    {
        Sexy::String aText;
        mStringList.push_back(aText);

        Sexy::String & aString = mStringList.back();
        aString.Assign("", 0);
        mBuffer.ReadBytes((unsigned __int8 *)aString.data(), aStringSizes[i]);
    }

    mElementCount = mBuffer.ReadInt32();
    return 1;
}

int Sexy::XMLParser::AddAttribute(Sexy::XMLElement* theElement, Sexy::String const& theName, Sexy::String const& theValue)
{
    if (theElement == nullptr)
        return 0;
    const bool inserted = theElement->mAttributes.find(theName) == theElement->mAttributes.end();
    theElement->mAttributes[theName] = theValue;
    return inserted ? 1 : 0;
}

int Sexy::XMLParser::AddAttributeEncoded(Sexy::XMLElement* theElement, Sexy::String const& theName, Sexy::String const& theValue)
{
    if (theElement == nullptr)
        return 0;
    const bool inserted = theElement->mAttributesEncoded.find(theName) == theElement->mAttributesEncoded.end();
    theElement->mAttributesEncoded[theName] = theValue;
    return inserted ? 1 : 0;
}

// ==========================================================================
// Sexy::XMLParser::NextBinaryElement(Sexy::XMLElement *)
// @003B50B8  1522 bytes
// ==========================================================================
// Walks one chunk of the binary format. Each chunk is a type byte, a string
// index and an attribute count, optionally followed by that many attribute
// index pairs.
int Sexy::XMLParser::NextBinaryElement(Sexy::XMLElement * theElement)
{
    while (mCurrentElementIndex < mElementCount)
    {
        if (mBuffer.mDataBitSize <= mBuffer.mReadBitPos)
            return 0;

        ResetElement(theElement, mSection);

        int aType = mBuffer.ReadInt8();
        unsigned short aStringIdx = (unsigned short)mBuffer.ReadInt16();
        int anAttributeCount = mBuffer.ReadInt16();

        if (anAttributeCount > 0)
        {
            // The chunk carries one name/value index pair per attribute. A
            // corrupt pair ends the walk.
            int anAttribute = 0;
            bool aCorrupt = false;
            for (;;)
            {
                unsigned short aNameIdx = (unsigned short)mBuffer.ReadInt16();
                unsigned short aValueIdx = (unsigned short)mBuffer.ReadInt16();
                Sexy::String const * aName = GetString(aNameIdx);
                Sexy::String const * aValue = GetString(aValueIdx);
                if (aName == nullptr || aValue == nullptr)
                {
                    aCorrupt = true;
                    break;
                }

                if (!aName->empty())
                {
                    ++anAttribute;
                    AddAttribute(theElement, *aName, *aValue);
                    if (anAttributeCount <= anAttribute)
                        break;
                }
                else if (anAttributeCount <= ++anAttribute)
                {
                    break;
                }
            }

            if (aCorrupt)
            {
                Fail("The file is corrupted.");
                return 0;
            }
        }

        ++mCurrentElementIndex;

        switch (aType)
        {
        case 1:
            // Character data.
            theElement->mType = 4;
            {
                Sexy::String const * aText = GetString(aStringIdx);
                if (aText == nullptr)
                {
                    Fail("The file is corrupted.");
                    return 0;
                }
                theElement->mInstruction = aText->c_str();
            }
            break;

        case 2:
            // A comment: the instruction is the marker, mComment the body.
            theElement->mType = 5;
            {
                Sexy::String const * aText = GetString(aStringIdx);
                if (aText == nullptr)
                {
                    Fail("The file is corrupted.");
                    return 0;
                }
                theElement->mInstruction = "!--";
                theElement->mComment = aText->c_str();
            }
            break;

        case 3:
            // A start tag: extend the section path by this element's name.
            theElement->mType = 1;
            {
                Sexy::String const * aName = GetString(aStringIdx);
                if (aName == nullptr)
                {
                    Fail("The file is corrupted.");
                    return 0;
                }
                theElement->mInstruction = aName->c_str();
                theElement->mValue = mSection;

                if (!mSection.empty())
                {
                    Sexy::String aSeparator("/");
                    aSeparator += theElement->mInstruction;
                    mSection += aSeparator;
                }
                else
                {
                    mSection += theElement->mInstruction;
                }
            }
            break;

        case 4:
            // An end tag: drop one component off the section path.
            theElement->mType = 2;
            {
                Sexy::String const * aName = GetString(aStringIdx);
                if (aName == nullptr)
                {
                    Fail("The file is corrupted.");
                    return 0;
                }
                theElement->mInstruction = aName->c_str();
                theElement->mValue = mSection;

                int aSlash = FindLastSlash(mSection);
                if (aSlash == -1)
                    mSection.clear();
                else
                    mSection = mSection.substr((unsigned int)aSlash);
            }
            break;

        case 5:
            theElement->mType = 6;
            {
                Sexy::String const * aText = GetString(aStringIdx);
                if (aText == nullptr)
                {
                    Fail("The file is corrupted.");
                    return 0;
                }
                theElement->mInstruction = aText->c_str();
                theElement->mValue = mSection;
            }
            break;

        case 6:
            // A processing instruction; an empty one restarts the walk.
            theElement->mType = 3;
            {
                Sexy::String const * aText = GetString(aStringIdx);
                if (aText == nullptr)
                {
                    Fail("The file is corrupted.");
                    return 0;
                }
                theElement->mInstruction = aText->c_str();
                theElement->mValue = mSection;

                if (TrimString(theElement->mInstruction).empty())
                    continue;
            }
            break;

        default:
            break;
        }

        if (theElement->mType != 5)
            return 1;
        if (mInQuote)
            return mInQuote;
        if (mCurrentElementIndex >= mElementCount)
            return 0;
    }

    return 0;
}

// ==========================================================================
// Sexy::XMLParser::NextElement(Sexy::XMLElement *)
// @003B5720  5776 bytes
// ==========================================================================
// One element from the document OpenFile or SetStringSource parsed. The
// sequence is the one the callers test: 1 start, 2 end, 3 text, 6 character
// data. The tag name is mInstruction. A start tag's mValue is the parent
// path; the matching end tag's mValue still includes the tag.
int Sexy::XMLParser::NextElement(Sexy::XMLElement * theElement)
{
    if (mIsBinary)
        return NextBinaryElement(theElement);

    std::map<const Sexy::XMLParser*, XmlCursor>::iterator aFound = gXmlCursors.find(this);
    if (aFound == gXmlCursors.end() || aFound->second.mIndex >= aFound->second.mEvents.size())
        return 0;

    const XmlEvent& aEvent = aFound->second.mEvents[aFound->second.mIndex];
    ++aFound->second.mIndex;

    ResetElement(theElement, Sexy::String(aEvent.mValue.c_str()));
    theElement->mType = aEvent.mType;
    theElement->mInstruction.assign(aEvent.mInstruction.c_str(), aEvent.mInstruction.size());
    theElement->mText.assign(aEvent.mText.c_str(), aEvent.mText.size());
    mSection.assign(aEvent.mSectionAfter.c_str(), aEvent.mSectionAfter.size());
    mLineNum = aEvent.mLine;

    for (size_t aIndex = 0; aIndex < aEvent.mAttrs.size(); ++aIndex)
    {
        Sexy::String aName(aEvent.mAttrs[aIndex].first.c_str());
        Sexy::String aValue(aEvent.mAttrs[aIndex].second.c_str());
        AddAttribute(theElement, aName, aValue);
        AddAttributeEncoded(theElement, aName, aValue);
    }
    return 1;
}
