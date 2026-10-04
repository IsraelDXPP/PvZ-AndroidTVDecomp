// GLStateManager.cpp -- cached GL state setters.

#include "GLStateManager.h"
#include "SexyTypes.h"
namespace Sexy
{

GLStateManager::GLStateManager()
{
    Invalidate();
    mStateChanges = 0;
    mRedundantCallsAvoided = 0;
}

GLStateManager::~GLStateManager()
{
}

void GLStateManager::Invalidate()
{
    mBlendEnabled   = false;
    mSrcBlend       = BLEND_ONE;
    mDstBlend       = BLEND_ZERO;

    mDepthTestEnabled  = false;
    mDepthWriteEnabled = false;
    mDepthFunc         = CMP_LESS;

    mCullEnabled   = false;
    mCullFrontFace = false;

    mScissorEnabled = false;
    mScissorRect    = Rect(0, 0, 0, 0);

    mAlphaTestEnabled = false;
    mAlphaFunc        = CMP_ALWAYS;
    mAlphaRef         = 0.0f;

    for (int i = 0; i < 4; ++i)
    {
        mTextureEnabled[i] = false;
        mBoundTexture[i]   = 0;
    }

    mClearColor = Color(0, 0, 0, 255);
    mBoundProgram = 0;
    mActiveTextureUnit = -1;   // unknown until the first bind queries it
    mBoundVBO     = 0;

    mViewportX = mViewportY = 0;
    mViewportW = mViewportH = 0;
}

void GLStateManager::Init()
{
    // Start from a fully unknown state so the first setter of each kind
    // actually reaches the driver.
    Invalidate();
    mStateChanges = 0;
    mRedundantCallsAvoided = 0;
}

unsigned int GLStateManager::ToGLBlend(BlendFactor theFactor)
{
    switch (theFactor)
    {
    case BLEND_ZERO:            return GL_ZERO;
    case BLEND_ONE:             return GL_ONE;
    case BLEND_SRC_ALPHA:       return GL_SRC_ALPHA;
    case BLEND_INV_SRC_ALPHA:   return GL_ONE_MINUS_SRC_ALPHA;
    case BLEND_DST_ALPHA:       return GL_DST_ALPHA;
    case BLEND_INV_DST_ALPHA:   return GL_ONE_MINUS_DST_ALPHA;
    case BLEND_SRC_COLOR:       return GL_SRC_COLOR;
    case BLEND_INV_SRC_COLOR:   return GL_ONE_MINUS_SRC_COLOR;
    }
    return GL_ONE;
}

unsigned int GLStateManager::ToGLCmp(CmpFunc theFunc)
{
    switch (theFunc)
    {
    case CMP_NEVER:    return GL_NEVER;
    case CMP_LESS:     return GL_LESS;
    case CMP_EQUAL:    return GL_EQUAL;
    case CMP_LEQUAL:   return GL_LEQUAL;
    case CMP_GREATER:  return GL_GREATER;
    case CMP_NOTEQUAL: return GL_NOTEQUAL;
    case CMP_GEQUAL:   return GL_GEQUAL;
    case CMP_ALWAYS:   return GL_ALWAYS;
    }
    return GL_ALWAYS;
}

// ---------------------------------------------------------------------------
// blend
// ---------------------------------------------------------------------------

void GLStateManager::EnableBlend(bool theEnable)
{
    if (mBlendEnabled == theEnable) { ++mRedundantCallsAvoided; return; }
    mBlendEnabled = theEnable;
    ++mStateChanges;
    if (theEnable) glEnable(GL_BLEND);
    else           glDisable(GL_BLEND);
}

void GLStateManager::SetBlend(BlendFactor theSrc, BlendFactor theDst)
{
    if (mSrcBlend == theSrc && mDstBlend == theDst) { ++mRedundantCallsAvoided; return; }
    mSrcBlend = theSrc;
    mDstBlend = theDst;
    ++mStateChanges;
    glBlendFunc(ToGLBlend(theSrc), ToGLBlend(theDst));
}

// ---------------------------------------------------------------------------
// depth
// ---------------------------------------------------------------------------

void GLStateManager::EnableDepthTest(bool theEnable)
{
    if (mDepthTestEnabled == theEnable) { ++mRedundantCallsAvoided; return; }
    mDepthTestEnabled = theEnable;
    ++mStateChanges;
    if (theEnable) glEnable(GL_DEPTH_TEST);
    else           glDisable(GL_DEPTH_TEST);
}

void GLStateManager::SetDepthWrite(bool theEnable)
{
    if (mDepthWriteEnabled == theEnable) { ++mRedundantCallsAvoided; return; }
    mDepthWriteEnabled = theEnable;
    ++mStateChanges;
    glDepthMask(theEnable ? GL_TRUE : GL_FALSE);
}

void GLStateManager::SetDepthFunc(CmpFunc theFunc)
{
    if (mDepthFunc == theFunc) { ++mRedundantCallsAvoided; return; }
    mDepthFunc = theFunc;
    ++mStateChanges;
    glDepthFunc(ToGLCmp(theFunc));
}

// ---------------------------------------------------------------------------
// culling
// ---------------------------------------------------------------------------

void GLStateManager::EnableCull(bool theEnable, bool theFrontFace)
{
    if (mCullEnabled == theEnable && mCullFrontFace == theFrontFace)
    {
        ++mRedundantCallsAvoided;
        return;
    }
    ++mStateChanges;

    if (mCullEnabled != theEnable)
    {
        mCullEnabled = theEnable;
        if (theEnable) glEnable(GL_CULL_FACE);
        else           glDisable(GL_CULL_FACE);
    }
    if (mCullFrontFace != theFrontFace)
    {
        mCullFrontFace = theFrontFace;
        // The framework culls back faces by default; the flag selects which
        // winding is considered front-facing.
        glFrontFace(theFrontFace ? GL_CW : GL_CCW);
    }
}

// ---------------------------------------------------------------------------
// scissor
// ---------------------------------------------------------------------------

void GLStateManager::EnableScissor(bool theEnable)
{
    if (mScissorEnabled == theEnable) { ++mRedundantCallsAvoided; return; }
    mScissorEnabled = theEnable;
    ++mStateChanges;
    if (theEnable) glEnable(GL_SCISSOR_TEST);
    else           glDisable(GL_SCISSOR_TEST);
}

void GLStateManager::SetScissorRect(const Rect& theRect)
{
    if (mScissorRect == theRect) { ++mRedundantCallsAvoided; return; }
    mScissorRect = theRect;
    ++mStateChanges;
    glScissor(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

// ---------------------------------------------------------------------------
// alpha test
// ---------------------------------------------------------------------------
// GL 3.3 core has no fixed-function alpha test (GLES2 did). The equivalent is a
// discard in the fragment shader; we keep the state here so the shader can read
// it as a uniform, and record whether it is active.

void GLStateManager::EnableAlphaTest(bool theEnable)
{
    if (mAlphaTestEnabled == theEnable) { ++mRedundantCallsAvoided; return; }
    mAlphaTestEnabled = theEnable;
    ++mStateChanges;
}

void GLStateManager::SetAlphaFunc(CmpFunc theFunc, float theRef)
{
    if (mAlphaFunc == theFunc && mAlphaRef == theRef) { ++mRedundantCallsAvoided; return; }
    mAlphaFunc = theFunc;
    mAlphaRef  = theRef;
    ++mStateChanges;
}

// ---------------------------------------------------------------------------
// textures / buffers
// ---------------------------------------------------------------------------

void GLStateManager::EnableTexture(int theStage, bool theEnable)
{
    if (theStage < 0 || theStage > 3) return;
    if (mTextureEnabled[theStage] == theEnable) { ++mRedundantCallsAvoided; return; }
    mTextureEnabled[theStage] = theEnable;
    ++mStateChanges;
}

void GLStateManager::BindTexture(int theStage, unsigned int theTexture)
{
    if (theStage < 0 || theStage > 3) return;
    if (mBoundTexture[theStage] == theTexture && mActiveTextureUnit == theStage)
    {
        ++mRedundantCallsAvoided;
        return;
    }
    mBoundTexture[theStage] = theTexture;
    ++mStateChanges;

    // Select the stage, bind, then put the previously active unit back. The
    // uploads bind through glBindTexture on whatever unit is active, so leaving
    // a different one selected writes their pixels onto this stage.
    GLint aPreviousUnit = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &aPreviousUnit);
    glActiveTexture(GL_TEXTURE0 + theStage);
    glBindTexture(GL_TEXTURE_2D, theTexture);
    glActiveTexture((unsigned int)aPreviousUnit);
    mActiveTextureUnit = aPreviousUnit - GL_TEXTURE0;
}

void GLStateManager::BindProgram(unsigned int theProgram)
{
    if (mBoundProgram == theProgram) { ++mRedundantCallsAvoided; return; }
    mBoundProgram = theProgram;
    ++mStateChanges;
    glUseProgram(theProgram);
}

void GLStateManager::BindVBO(unsigned int theVBO)
{
    if (mBoundVBO == theVBO) { ++mRedundantCallsAvoided; return; }
    mBoundVBO = theVBO;
    ++mStateChanges;
    glBindBuffer(GL_ARRAY_BUFFER, theVBO);
}

// ---------------------------------------------------------------------------
// misc
// ---------------------------------------------------------------------------

void GLStateManager::SetClearColor(const Color& theColor)
{
    if (mClearColor == theColor) { ++mRedundantCallsAvoided; return; }
    mClearColor = theColor;
    ++mStateChanges;
    glClearColor(theColor.mRed / 255.0f, theColor.mGreen / 255.0f,
                 theColor.mBlue / 255.0f, theColor.mAlpha / 255.0f);
}

void GLStateManager::SetViewport(int x, int y, int w, int h)
{
    if (mViewportX == x && mViewportY == y && mViewportW == w && mViewportH == h)
    {
        ++mRedundantCallsAvoided;
        return;
    }
    mViewportX = x; mViewportY = y; mViewportW = w; mViewportH = h;
    ++mStateChanges;
    glViewport(x, y, w, h);
}

// The state every frame's 2D pass begins from. Matches what the Android build
// set up in AndroidGraphicsDriver before drawing the widget tree.
void GLStateManager::ApplyDefaultState()
{
    EnableDepthTest(false);
    EnableCull(false);
    EnableScissor(false);
    EnableAlphaTest(false);
    EnableBlend(true);
    // Standard source-over compositing, which is what DRAWMODE_NORMAL means.
    SetBlend(BLEND_SRC_ALPHA, BLEND_INV_SRC_ALPHA);
}

} // namespace Sexy
