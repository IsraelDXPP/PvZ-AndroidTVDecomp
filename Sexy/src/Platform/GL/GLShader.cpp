// GLShader.cpp -- shader compilation and the framework's shader set.

#include "GLShader.h"
#include "SexyTypes.h"

#include "GLTexture.h"
#include "Graphics.h"
#include "AndroidShims.h"
namespace Sexy
{

// ---------------------------------------------------------------------------
// Shared vertex shader
// ---------------------------------------------------------------------------
// Positions arrive in pixel space; the transform maps them to clip space. The
// framework's Graphics keeps a 3x3 affine transform, so we take it as a mat3
// and expand to mat4 here rather than doing it per-draw on the CPU.
//
// Texture coordinates are passed through unchanged: the source rect is baked
// into them by the caller, which is what GLTexture::GetTexture did in the
// original (it returned a uv rect for a sub-image inside an atlas).

static const char* kVertexSrc =
LAWNTV_GLSL_VERSION
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec2 aUV;\n"
"layout(location = 2) in vec4 aColor;\n"
"uniform mat3 uTransform;\n"
"uniform vec2 uViewport;\n"
"out vec2 vUV;\n"
"out vec4 vColor;\n"
"void main()\n"
"{\n"
"    // Identity stays identity. A rotated blit is already expanded on the CPU\n"
"    // by GLTexture::BltTransformed, so this uniform is the frame transform.\n"
"    vec3 p = uTransform * vec3(aPos, 1.0);\n"
"    // pixel space -> clip space (+Y down, matching the framework's coords)\n"
"    vec2 ndc = vec2(p.x / uViewport.x * 2.0 - 1.0,\n"
"                    1.0 - p.y / uViewport.y * 2.0);\n"
"    gl_Position = vec4(ndc, 0.0, 1.0);\n"
"    vUV = aUV;\n"
"    // GLGraphics::Flush enables the colour array and then glColor4f(1,1,1,1),\n"
"    // so the vertex colour is the modulate, not a uniform shared by the batch.\n"
"    vColor = aColor;\n"
"}\n";

// ---------------------------------------------------------------------------
// Blit fragment shader
// ---------------------------------------------------------------------------
// Mirrors the software blitter's semantics exactly so that GL and software
// output agree pixel-for-pixel (the self-test relies on this):
//
//   tint        : componentwise multiply of the texel by the draw colour
//   colorize    : replace RGB with the draw colour, keep the texel's alpha
//                 (Graphics::mColorizeImages -- used to draw silhouettes)
//   alpha test  : discard, replacing the fixed-function GL_ALPHA_TEST that
//                 GLES2 had and GL 3.3 core does not

static const char* kBlitFragSrc =
LAWNTV_GLSL_VERSION
"in vec2 vUV;\n"
"in vec4 vColor;\n"
"uniform sampler2D uTexture;\n"
"uniform int  uColorize;\n"
"uniform int  uUseAlphaTest;\n"
"uniform float uAlphaRef;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"    vec4 t = texture(uTexture, vUV);\n"
"    vec4 c;\n"
"    if (uColorize != 0)\n"
"        c = vec4(vColor.rgb, t.a * vColor.a);\n"
"    else\n"
"        c = t * vColor;\n"
"    if (uUseAlphaTest != 0 && c.a < uAlphaRef)\n"
"        discard;\n"
"    fragColor = c;\n"
"}\n";

// ---------------------------------------------------------------------------
// Solid fragment shader
// ---------------------------------------------------------------------------

static const char* kSolidFragSrc =
LAWNTV_GLSL_VERSION
"in vec2 vUV;\n"
"in vec4 vColor;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"    fragColor = vColor;\n"
"}\n";

// ---------------------------------------------------------------------------
// MPEG-1 fragment shader
// ---------------------------------------------------------------------------
// Three R8 planes. MPEG-1 uses BT.601 limited range. The chroma planes are
// half the luma size, and the sampler interpolates them.

static const char* kYuvFragSrc =
LAWNTV_GLSL_VERSION
"in vec2 vUV;\n"
"uniform sampler2D uY;\n"
"uniform sampler2D uCb;\n"
"uniform sampler2D uCr;\n"
"out vec4 fragColor;\n"
"void main()\n"
"{\n"
"    float y = texture(uY, vUV).r;\n"
"    float cb = texture(uCb, vUV).r - 0.5;\n"
"    float cr = texture(uCr, vUV).r - 0.5;\n"
"    y = (y - 0.0627451) * 1.1643836;\n"
"    fragColor = vec4(y + 1.5960268 * cr,\n"
"                     y - 0.3917623 * cb - 0.8129676 * cr,\n"
"                     y + 2.0172321 * cb,\n"
"                     1.0);\n"
"}\n";

// ---------------------------------------------------------------------------
// GLShader
// ---------------------------------------------------------------------------

GLShader::GLShader()
    : mProgram(0), mVertShader(0), mFragShader(0),
      mLocTexture(-1), mLocColor(-1), mLocColorize(-1), mLocTransform(-1),
      mLocAlphaRef(-1), mLocUseAlphaTest(-1), mLocTexCoordRect(-1)
{
}

GLShader::~GLShader()
{
    Destroy();
}

unsigned int GLShader::CompileStage(unsigned int theType, const char* theSrc,
                                    const char* theName)
{
    unsigned int shader = glCreateShader(theType);
    glShaderSource(shader, 1, &theSrc, nullptr);
    glCompileShader(shader);

    int ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        int len = 0;
        glGetShaderInfoLog(shader, sizeof(log) - 1, &len, log);
        log[len > 0 ? len : 0] = '\0';
        TOD_ERROR("GLShader '%s': %s shader compile failed:\n%s",
                  theName,
                  theType == GL_VERTEX_SHADER ? "vertex" : "fragment",
                  log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool GLShader::Create(const char* theName, const char* theVertexSrc,
                      const char* theFragmentSrc)
{
    Destroy();
    mName = theName ? theName : "";

    mVertShader = CompileStage(GL_VERTEX_SHADER, theVertexSrc, theName);
    if (!mVertShader) return false;

    mFragShader = CompileStage(GL_FRAGMENT_SHADER, theFragmentSrc, theName);
    if (!mFragShader)
    {
        glDeleteShader(mVertShader);
        mVertShader = 0;
        return false;
    }

    mProgram = glCreateProgram();
    glAttachShader(mProgram, mVertShader);
    glAttachShader(mProgram, mFragShader);
    glLinkProgram(mProgram);

    int ok = 0;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[2048];
        int len = 0;
        glGetProgramInfoLog(mProgram, sizeof(log) - 1, &len, log);
        log[len > 0 ? len : 0] = '\0';
        TOD_ERROR("GLShader '%s': link failed:\n%s", theName, log);
        Destroy();
        return false;
    }

    // Shaders can be released once linked; the program keeps what it needs.
    glDetachShader(mProgram, mVertShader);
    glDetachShader(mProgram, mFragShader);
    glDeleteShader(mVertShader);
    glDeleteShader(mFragShader);
    mVertShader = mFragShader = 0;

    // Uniform values belong to the program object, but glUniform* only affects
    // the program *currently in use*. Bind before setting anything, or these
    // calls are silently dropped and the shader reads uninitialised uniforms
    // (which is exactly what made every draw land outside the viewport).
    glUseProgram(mProgram);

    mLocTexture       = glGetUniformLocation(mProgram, "uTexture");
    mLocColor         = glGetUniformLocation(mProgram, "uColor");
    mLocColorize      = glGetUniformLocation(mProgram, "uColorize");
    mLocTransform     = glGetUniformLocation(mProgram, "uTransform");
    mLocAlphaRef      = glGetUniformLocation(mProgram, "uAlphaRef");
    mLocUseAlphaTest  = glGetUniformLocation(mProgram, "uUseAlphaTest");

    // Defaults: identity transform, opaque white, no colourise, no alpha test.
    if (mLocTransform >= 0)
    {
        float ident[9] = {1,0,0, 0,1,0, 0,0,1};
        glUniformMatrix3fv(mLocTransform, 1, GL_FALSE, ident);
    }
    if (mLocColor >= 0)
        glUniform4f(mLocColor, 1.0f, 1.0f, 1.0f, 1.0f);
    if (mLocColorize >= 0)
        glUniform1i(mLocColorize, 0);
    if (mLocUseAlphaTest >= 0)
        glUniform1i(mLocUseAlphaTest, 0);
    if (mLocAlphaRef >= 0)
        glUniform1f(mLocAlphaRef, 0.0f);
    if (mLocTexture >= 0)
        glUniform1i(mLocTexture, 0);

    // The viewport uniform is shared by both programs. Capture the real
    // framebuffer size (the caller may not have set the viewport yet, in which
    // case the default framebuffer dimensions are what we want).
    int locViewport = glGetUniformLocation(mProgram, "uViewport");
    if (locViewport >= 0)
    {
        GLint vp[4] = {0, 0, 0, 0};
        glGetIntegerv(GL_VIEWPORT, vp);
        glUniform2f(locViewport, (float)vp[2], (float)vp[3]);
    }

    glUseProgram(0);

    TOD_TRACE("GLShader '%s' created (program %u)", theName, mProgram);
    return true;
}

void GLShader::Destroy()
{
    if (mProgram)
    {
        glDeleteProgram(mProgram);
        mProgram = 0;
    }
    if (mVertShader) { glDeleteShader(mVertShader); mVertShader = 0; }
    if (mFragShader) { glDeleteShader(mFragShader); mFragShader = 0; }
}

void GLShader::Bind()
{
    glUseProgram(mProgram);
}

void GLShader::SetTexture(int theUnit)
{
    if (mLocTexture >= 0)
        glUniform1i(mLocTexture, theUnit);
}

void GLShader::SetColor(const Color& theColor)
{
    if (mLocColor < 0)
        return;

    // GLTexture::Blt @0x471cd4 and BltTransformed @0x4734ac premultiply the
    // colour before GLGraphics::SetColor, then pass the raw alpha byte.
    // glBlendFunc(ONE, ONE_MINUS_SRC_ALPHA) and glBlendFunc(ONE, ONE) both
    // use that premultiplied RGB as the source. A straight colour here adds
    // a white pulse and a red glow at full strength.
    int aAlpha = theColor.mAlpha;
    if (aAlpha < 0)
        aAlpha = 0;
    if (aAlpha > 255)
        aAlpha = 255;
    int aRed = theColor.mRed * aAlpha + 128;
    int aGreen = theColor.mGreen * aAlpha + 128;
    int aBlue = theColor.mBlue * aAlpha + 128;
    aRed = (aRed + (aRed >> 8)) >> 8;
    aGreen = (aGreen + (aGreen >> 8)) >> 8;
    aBlue = (aBlue + (aBlue >> 8)) >> 8;
    glUniform4f(mLocColor,
                aRed / 255.0f, aGreen / 255.0f, aBlue / 255.0f, aAlpha / 255.0f);
}

void GLShader::SetColorize(bool theColorize)
{
    if (mLocColorize >= 0)
        glUniform1i(mLocColorize, theColorize ? 1 : 0);
}

void GLShader::SetTransform(const SexyMatrix3& theMatrix)
{
    if (mLocTransform < 0) return;

    // glUniformMatrix3fv reads nine floats in column-major order. m[] is stored
    // row by row, and operator* is x' = x*m00 + y*m01 + m02, so the uniform
    // is uploaded transposed.
    glUniformMatrix3fv(mLocTransform, 1, GL_TRUE, theMatrix.m);
}

void GLShader::SetAlphaTest(bool theEnable, float theRef)
{
    if (mLocUseAlphaTest >= 0) glUniform1i(mLocUseAlphaTest, theEnable ? 1 : 0);
    if (mLocAlphaRef >= 0)     glUniform1f(mLocAlphaRef, theRef);
}

void GLShader::SetTexCoordRect(float u0, float v0, float u1, float v1)
{
    if (mLocTexCoordRect < 0) return;
    glUniform4f(mLocTexCoordRect, u0, v0, u1, v1);
}

// ---------------------------------------------------------------------------
// GLShaderSet
// ---------------------------------------------------------------------------

bool GLShaderSet::Create()
{
    if (!mBlit.Create("Blit", kVertexSrc, kBlitFragSrc))
        return false;
    if (!mSolid.Create("Solid", kVertexSrc, kSolidFragSrc))
        return false;
    if (!mYuv.Create("Yuv", kVertexSrc, kYuvFragSrc))
        return false;
    int aY = glGetUniformLocation(mYuv.mProgram, "uY");
    int aCb = glGetUniformLocation(mYuv.mProgram, "uCb");
    int aCr = glGetUniformLocation(mYuv.mProgram, "uCr");
    mYuv.Bind();
    if (aY >= 0) glUniform1i(aY, 0);
    if (aCb >= 0) glUniform1i(aCb, 1);
    if (aCr >= 0) glUniform1i(aCr, 2);
    return true;
}

void GLShaderSet::Destroy()
{
    mBlit.Destroy();
    mSolid.Destroy();
    mYuv.Destroy();
}

} // namespace Sexy
