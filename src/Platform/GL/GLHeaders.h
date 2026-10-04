// GLHeaders.h -- single point of contact with the OpenGL loader.
//
// The Android build shipped its own GLES2 entry points (Sexy::AndroidGL20RenderEffect,
// Sexy::AndroidRenderDeviceES20, Sexy::AndroidGL20StateManager). On desktop we use
// glad for the same reason: the platform's GL symbols are not linkable at build time.
//
// We target desktop OpenGL 3.3 *core*, which is the closest widely-available
// profile to GLES2-with-extensions:
//   * GLES2 shaders are GLSL ES 1.00; GL 3.3 core uses GLSL 3.30 core, so the
//     shader sources differ in their #version line only (see GLShader.cpp).
//   * GLES2 has no fixed-function pipeline, and neither does GL 3.3 core, so the
//     "everything is a shader" structure of the original carries over unchanged.
//   * VAOs are mandatory in core profile; GLES2 used client-side vertex arrays.
//     GLVertexBuffer.h hides that difference.
//
// Defining LAWNTV_USE_GLES2 switches the loader and shader version back to GLES2,
// which is what an Android rebuild would want.

#ifndef __GLHEADERS_H__
#define __GLHEADERS_H__

#include "CoreTypes.h"

#ifdef LAWNTV_USE_GLES2
    #include <glad/gles2.h>
    #define LAWNTV_GLSL_VERSION "#version 100\n"
    #define LAWNTV_GLSL_ES      1
#else
    // glad 0.1.36 generates a single glad/glad.h for the configured API
    // (this build is gl=3.3, see vcpkg.json's gl-api-33 feature).
    #include <glad/glad.h>
    #define LAWNTV_GLSL_VERSION "#version 330 core\n"
    #define LAWNTV_GLSL_ES      0
#endif

namespace Sexy
{

// ---------------------------------------------------------------------------
// GL error reporting. The binary's AndroidGL20StateManager tracked a shadow copy
// of GL state to avoid redundant driver calls; we keep that idea in
// GLStateManager, and this helper for surfacing failures.
// ---------------------------------------------------------------------------

const char* GLErrorString(unsigned int theError);
void        GLCheckError(const char* theContext);

#ifdef _DEBUG
    #define GL_CHECK(ctx) ::Sexy::GLCheckError(ctx)
#else
    #define GL_CHECK(ctx) ((void)0)
#endif

} // namespace Sexy

#endif // __GLHEADERS_H__
