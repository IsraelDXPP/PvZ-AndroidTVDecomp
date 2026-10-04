// XMLElement is one node from an XML parse: its kind, the text fields the
// parser fills, and the attributes keyed by name.

#pragma once

#include "ReconTypes.h"
#include <map>

namespace Sexy {

class XMLElement
{
public:
    int mType;
    Sexy::String mSection;
    Sexy::String mValue;
    Sexy::String mValueEncoded;
    Sexy::String mInstruction;
    Sexy::String mText;
    Sexy::String mExtra;
    Sexy::String mComment;
    std::map<Sexy::String, Sexy::String> mAttributes;
    std::map<Sexy::String, Sexy::String> mAttributesEncoded;

    XMLElement();

    int GetAttributeBool(Sexy::String const& theName, bool theDefault);
    Sexy::String GetAttribute(Sexy::String const& theName) const;
    bool HasAttribute(Sexy::String const& theName) const;
};

} // namespace Sexy

using Sexy::XMLElement;
