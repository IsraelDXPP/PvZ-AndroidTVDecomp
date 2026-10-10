// Thrown when a resource group fails to load. The text is the message the
// constructor stores; what() stays the base implementation.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_RESOURCEMANAGEREXCEPTION_H
#define LAWNPROJECTTV_SEXY_INCLUDE_RESOURCEMANAGEREXCEPTION_H

#include "ReconTypes.h"
#include <exception>

namespace Sexy
{

class ResourceManagerException : public std::exception
{
public:
    virtual ~ResourceManagerException();

    ResourceManagerException(Sexy::String const &text);

    Sexy::String mText;
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_RESOURCEMANAGEREXCEPTION_H
