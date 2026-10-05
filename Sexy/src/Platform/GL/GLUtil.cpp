// GLUtil.cpp -- GL error reporting helpers (see GLHeaders.h).

#include "GLHeaders.h"
#include "SexyTypes.h"
#include "AndroidShims.h"
namespace Sexy
{

const char* GLErrorString(unsigned int theError)
{
    switch (theError)
    {
    case GL_NO_ERROR:                      return "GL_NO_ERROR";
    case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
    default:                               return "GL_UNKNOWN_ERROR";
    }
}

void GLCheckError(const char* theContext)
{
    // Drain the queue so a stale error from an earlier call is not attributed
    // to the current one, but report everything found.
    for (int guard = 0; guard < 16; ++guard)
    {
        unsigned int err = glGetError();
        if (err == GL_NO_ERROR) break;
        TOD_ERROR("GL error in %s: %s (0x%04X)",
                  theContext ? theContext : "?", GLErrorString(err), err);
    }
}

} // namespace Sexy
