// DescParser reads a descriptor file into commands. A line is a command name
// followed by values, lists, and name=value pairs. DEFINE names live in
// mDefineMap and are looked up case-insensitively.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_DESCPARSER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_DESCPARSER_H

#include "EncodingParser.h"
#include "ListDataElement.h"

#include <map>
#include <vector>

namespace Sexy
{

class DescParser : public EncodingParser
{
public:
    enum
    {
        CMDSEP_SEMICOLON = 1,
        CMDSEP_NO_INDENT = 2
    };

    virtual bool Error(const Sexy::String &theError);
    virtual Sexy::DataElement *Dereference(const Sexy::String &theString);
    virtual bool HandleCommand(const Sexy::ListDataElement &theParams) = 0;
    virtual bool LoadDescriptor(const Sexy::String &theFileName);
    virtual ~DescParser();

    DescParser();
    Sexy::String DataElementToString(const Sexy::DataElement *theElement, bool theNested);
    int DataToString(Sexy::DataElement *theElement, Sexy::String *theResult);
    int DataToInt(Sexy::DataElement *theElement, int *theResult);
    int DataToDouble(Sexy::DataElement *theElement, double *theResult);
    int DataToBoolean(Sexy::DataElement *theElement, bool *theResult);
    int GetValues(Sexy::ListDataElement *theSource, Sexy::ListDataElement *theResult);
    int DataToList(Sexy::DataElement *theElement, Sexy::ListDataElement *theResult);
    int DataToStringVector(Sexy::DataElement *theElement, std::vector<Sexy::String> *theResult);
    int DataToIntVector(Sexy::DataElement *theElement, std::vector<int> *theResult);
    int DataToDoubleVector(Sexy::DataElement *theElement, std::vector<double> *theResult);
    int IsImmediate(Sexy::String const &theText);
    int ParseDescriptorLine(const std::vector<int> &theLine);
    Sexy::String Unquote(Sexy::String const &theText);

    int mCmdSep;
    Sexy::String mError;
    int mCurrentLineNum;
    // A descriptor line is one code point per element, the same layout
    // LoadDescriptor writes into basic_string<int>. A UTF-8 byte string would
    // split a quoted CJK character and close the quote on a continuation byte.
    std::vector<int> mCurrentLine;
    std::map<Sexy::String, Sexy::DataElement *> mDefineMap;

protected:
    int ParseToList(
        const std::vector<int> &theText, Sexy::ListDataElement *theList, bool theExpectListEnd, int *thePos);
};

} // namespace Sexy

using Sexy::DescParser;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_DESCPARSER_H
