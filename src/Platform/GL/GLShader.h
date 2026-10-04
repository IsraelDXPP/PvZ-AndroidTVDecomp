// GLShader.h -- shader program management.
//
// Mirrors the binary's render-effect layer (Sexy::RenderEffect,
// Sexy::AndroidGL20RenderEffect, Sexy::RenderEffectDefinition). The Android build
// compiled GLSL ES 1.00 at runtime from effect definitions; we do the same with
// GLSL 3.30 core, keeping one shader per blend/tint combination that the 2D
// pipeline actually needs.
//
// The two programs cover every 2D draw the game performs:
//   * Blit      -- textured quad with a colour tint and a colourise flag.
//                  Covers Graphics::DrawImage / DrawImageMirror / DrawImageCel
//                  and the Font glyph path.
//   * Solid     -- untextured quad, for FillRect / ClearRect / DrawRect.
//
// DRAWMODE_ADDITIVE is a blend-state change, not a different shader, which is
// exactly how the original handled it (the draw mode maps to a blend mode).

#ifndef __GLSHADER_H__
#define __GLSHADER_H__

#include "GLHeaders.h"
#include "CoreTypes.h"
#include "SexyString.h"
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class GLShader
{
public:
    unsigned int    mProgram;
    unsigned int    mVertShader;
    unsigned int    mFragShader;

    // Uniform locations, resolved once after linking.
    int             mLocTexture;
    int             mLocColor;
    int             mLocColorize;
    int             mLocTransform;
    int             mLocAlphaRef;
    int             mLocUseAlphaTest;
    int             mLocTexCoordRect;

    Sexy::String     mName;

public:
    GLShader();
    ~GLShader();

    // Compile + link from source. Returns false and logs on failure.
    bool    Create(const char* theName, const char* theVertexSrc, const char* theFragmentSrc);
    void    Destroy();

    void    Bind();
    void    SetTexture(int theUnit);
    void    SetColor(const Color& theColor);
    void    SetColorize(bool theColorize);
    void    SetTransform(const SexyMatrix3& theMatrix);
    void    SetAlphaTest(bool theEnable, float theRef);
    void    SetTexCoordRect(float u0, float v0, float u1, float v1);

    bool    IsValid() const { return mProgram != 0; }

private:
    unsigned int CompileStage(unsigned int theType, const char* theSrc, const char* theName);
};

// ---------------------------------------------------------------------------
// The framework's shader set.
// ---------------------------------------------------------------------------
class GLShaderSet
{
public:
    GLShader    mBlit;      // textured + tint + optional colourise
    GLShader    mSolid;     // untextured fill

    bool    Create();
    void    Destroy();
};

} // namespace Sexy



#endif // __GLSHADER_H__

