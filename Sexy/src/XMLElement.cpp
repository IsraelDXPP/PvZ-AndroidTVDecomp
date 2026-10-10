// XMLElement looks attributes up by name. A missing name keeps the caller's
// default. "true" and "1" are true; "false" and "0" are false; an empty value
// is true, matching the resource flags the manifests already use.

#include "XMLElement.h"
#include <cstring>

namespace
{

bool IsTrueText(const char *theText)
{
    if (theText == nullptr || theText[0] == 0)
        return true;
    if (strcasecmp(theText, "true") == 0 || strcasecmp(theText, "1") == 0)
        return true;
    return false;
}

bool IsFalseText(const char *theText)
{
    if (theText == nullptr)
        return false;
    return strcasecmp(theText, "false") == 0 || strcasecmp(theText, "0") == 0;
}

} // namespace

Sexy::XMLElement::XMLElement() :
    mType(0)
{
}

int Sexy::XMLElement::GetAttributeBool(Sexy::String const &theName, bool theDefault)
{
    std::map<Sexy::String, Sexy::String>::const_iterator found = mAttributes.find(theName);
    if (found == mAttributes.end())
        return theDefault ? 1 : 0;

    const char *text = found->second.c_str();
    if (IsTrueText(text))
        return 1;
    if (IsFalseText(text))
        return 0;
    return theDefault ? 1 : 0;
}

Sexy::String Sexy::XMLElement::GetAttribute(Sexy::String const &theName) const
{
    std::map<Sexy::String, Sexy::String>::const_iterator found = mAttributes.find(theName);
    if (found == mAttributes.end())
        return Sexy::String();
    return found->second;
}

bool Sexy::XMLElement::HasAttribute(Sexy::String const &theName) const
{
    return mAttributes.find(theName) != mAttributes.end();
}
