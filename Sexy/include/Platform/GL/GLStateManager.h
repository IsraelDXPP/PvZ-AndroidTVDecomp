// GLStateManager.h -- shadowed GL state, mirroring Sexy::AndroidGL20StateManager.
//
// The binary's AndroidGL20StateManager (16 methods) exists to avoid redundant
// driver calls: every setter compares against a cached value and returns early
// when nothing changes. The game issues a *lot* of redundant state changes
// (every Graphics::SetColor / SetDrawMode reaches the device), so this cache is
// a real optimisation, not a micro-optimisation.
//
// We reproduce the same structure: one manager per context, with the cached
// values in plain members and the setters doing the comparison.

#ifndef __GLSTATEMANAGER_H__
#define __GLSTATEMANAGER_H__

#include "GLHeaders.h"
#include "CoreTypes.h"

namespace Sexy
{

class GLStateManager
{
public:
    // Blend factors, matching the framework's blend modes.
    enum BlendFactor
    {
        BLEND_ZERO = 0,
        BLEND_ONE,
        BLEND_SRC_ALPHA,
        BLEND_INV_SRC_ALPHA,
        BLEND_DST_ALPHA,
        BLEND_INV_DST_ALPHA,
        BLEND_SRC_COLOR,
        BLEND_INV_SRC_COLOR
    };

    enum CmpFunc
    {
        CMP_NEVER = 0,
        CMP_LESS,
        CMP_EQUAL,
        CMP_LEQUAL,
        CMP_GREATER,
        CMP_NOTEQUAL,
        CMP_GEQUAL,
        CMP_ALWAYS
    };

private:
    bool        mBlendEnabled;
    BlendFactor mSrcBlend;
    BlendFactor mDstBlend;

    bool        mDepthTestEnabled;
    bool        mDepthWriteEnabled;
    CmpFunc     mDepthFunc;

    bool        mCullEnabled;
    bool        mCullFrontFace;

    bool        mScissorEnabled;
    Rect        mScissorRect;

    bool        mAlphaTestEnabled;
    CmpFunc     mAlphaFunc;
    float       mAlphaRef;

    bool        mTextureEnabled[4];

    Color       mClearColor;

    unsigned int mBoundProgram;
    unsigned int mBoundTexture[4];
    int          mActiveTextureUnit;   // the unit glBindTexture writes to
    unsigned int mBoundVBO;

    int         mViewportX, mViewportY, mViewportW, mViewportH;

    // Counters so the benefit of the cache is measurable.
    unsigned int mStateChanges;
    unsigned int mRedundantCallsAvoided;

public:
    GLStateManager();
    ~GLStateManager();

    // Reset to a known state and apply it to the driver.
    void    Init();
    void    Invalidate();       // forget cached values (after external GL calls)

    void    EnableBlend(bool theEnable);
    void    SetBlend(BlendFactor theSrc, BlendFactor theDst);

    void    EnableDepthTest(bool theEnable);
    void    SetDepthWrite(bool theEnable);
    void    SetDepthFunc(CmpFunc theFunc);

    void    EnableCull(bool theEnable, bool theFrontFace = false);

    void    EnableScissor(bool theEnable);
    void    SetScissorRect(const Rect& theRect);

    void    EnableAlphaTest(bool theEnable);
    void    SetAlphaFunc(CmpFunc theFunc, float theRef);

    void    EnableTexture(int theStage, bool theEnable);
    void    BindTexture(int theStage, unsigned int theTexture);
    void    BindProgram(unsigned int theProgram);
    void    BindVBO(unsigned int theVBO);

    void    SetClearColor(const Color& theColor);
    void    SetViewport(int x, int y, int w, int h);

    void    ApplyDefaultState();    // what every frame starts from

    unsigned int GetStateChanges() const { return mStateChanges; }
    unsigned int GetAvoidedCalls() const { return mRedundantCallsAvoided; }

    static unsigned int ToGLBlend(BlendFactor theFactor);
    static unsigned int ToGLCmp(CmpFunc theFunc);
};

} // namespace Sexy



#endif // __GLSTATEMANAGER_H__

