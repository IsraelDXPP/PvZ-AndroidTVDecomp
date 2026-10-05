#include "ResourceManagerException.h"

Sexy::ResourceManagerException::ResourceManagerException(Sexy::String const& text)
    : mText(text)
{
}

Sexy::ResourceManagerException::~ResourceManagerException()
{
}
