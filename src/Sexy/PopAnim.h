// PopAnim is a Widget that loads a .pam popanim and draws its sprites.
//
// Virtuals, in declaration order:
//   ~PopAnim, Update, UpdateF, Draw,
//   DrawSprite, DrawSpriteMirrored,
//   Load_Init, Load_SetModPamFile, Load_AddRemap, Load_LoadPam, Load_LoadMod,
//   Load_GetImageHook, Load_GetImage, Load_PostImageLoadHook.
// A mod build overrides the load hooks, so Draw reaches DrawSprite through the
// virtual rather than a direct call.

#pragma once

#include "Buffer.h"
#include "DescParser.h"
#include "Graphics.h"
#include "Image.h"
#include "ImageLib.h"
#include "MTRand.h"
#include "PIEffect.h"
#include "ResourceManager.h"
#include "ResourceRef.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "SharedImageRef.h"
#include "Widget.h"
#include <cmath>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Sexy {

class PopAnim;
class PopAnimListener;
class PASpriteDef;
class PASpriteInst;

// One image named by the pam. Several files can attach to the same name.
class PAImage
{
public:
    std::vector<SharedImageRef> mRefs;
    int mWidth;
    int mHeight;
    int mNumCols;
    int mNumRows;
    String mName;
    int mDrawMode;
    SexyMatrix3 mTransform;
    float mTransX;
    float mTransY;

    PAImage();
};

// One named object inside a sprite. A sprite object points at a nested def.
class PAObjectDef
{
public:
    const char* mName;
    int mIndex;
    bool mIsSprite;
    bool mAdditive;
    int mImageIndex;
    int mAdvanceCount;
    float mRateScale;
    bool mUseSrcRect;
    TRect<int> mSrcRect;
    PASpriteDef* mNestedDef;

    PAObjectDef();
};

// One sampled pose of an object: a matrix, a packed colour, and the object it belongs to.
class PAObjectPos
{
public:
    SexyMatrix3 mMatrix;
    int mPackedColor;
    bool mInterpolate;
    PAObjectDef* mDef;

    PAObjectPos();
};

// A frame command: a name and its argument text.
class PACommand
{
public:
    String mName;
    String mArgs;
};

// One frame of a sprite: the live poses, a hold flag, and the commands to run.
class PAFrame
{
public:
    std::vector<PAObjectPos*> mObjects;
    bool mHold;
    std::vector<PACommand> mCommands;

    PAFrame();
};

// A sprite definition: frames, labels, objects, and the rate it plays at.
class PASpriteDef
{
public:
    const char* mName;
    std::vector<PAFrame> mFrames;
    std::map<String, int> mLabels;
    int mFirstFrame;
    int mLastFrame;
    bool mPingPong;
    std::vector<PAObjectDef> mObjects;
    float mRate;

    PASpriteDef();
    int GetLabelFrame(String const& theName) const;
};

// A particle spawned by an addparticleeffect command.
class PAParticleEffect
{
public:
    ResourceManager_BaseRes* mBaseRes;
    PIEffect* mEffect;
    String mName;
    int mUpdateStamp;
    bool mDrawAdditive;
    bool mFollowSprite;
    bool mActive;
    double mOffsetX;
    double mOffsetY;

    PAParticleEffect();
    PAParticleEffect(PAParticleEffect const& theOther);
    PAParticleEffect& operator=(PAParticleEffect const& theOther);
    ~PAParticleEffect();
};

// One object of a live sprite instance.
class PAObjectInst
{
public:
    const char* mName;
    PASpriteInst* mInst;
    SexyTransform2D mPrevTransform;
    int mPrevRed;
    int mPrevGreen;
    int mPrevBlue;
    int mPrevAlpha;
    bool mBlending;
    SexyTransform2D mTransform;
    Color mColor;
    bool mQueryDraw;
    bool mUseDrawHelper;
    bool mQueryDrawn;

    PAObjectInst();
    int GetObjectInst(String const& theName);
};

// A live sprite: the def it plays, the frame clock, and its objects.
class PASpriteInst
{
public:
    PASpriteInst* mParent;
    int mHoldFrames;
    float mFrameTime;
    int mLoopCount;
    bool mFrameAdvanced;
    int mLastStamp;
    SexyTransform2D mTransform;
    Color mColor;
    std::vector<PAObjectInst> mObjects;
    std::vector<PAParticleEffect> mParticles;
    PASpriteDef* mDef;

    PASpriteInst();
    ~PASpriteInst();
    int GetObjectInst(String const& theName);
};

// The shared table a loaded pam owns: pose pools, nested defs, and the main def.
class PASpriteTable
{
public:
    std::vector<PAObjectPos*> mTransformBlocks;
    PAObjectPos* mTransformBlock;
    int mTransformSlot;
    std::vector<PAObjectDef*> mDataBlocks;
    PAObjectDef* mDataBlock;
    int mDataSlot;
    PASpriteDef* mMainDef;
    std::vector<PASpriteDef> mNestedDefs;
    std::list<String> mNames;
    int mRefCount;

    PASpriteTable();
    ~PASpriteTable();

    PAObjectPos* AllocTransform();
    PAObjectDef* AllocData();
    const char* Intern(String const& theName);
};

// The receiver a PopAnim reports playback to. The interface lives outside the
// library; the slot order is the one the call sites use.
class PopAnimListener
{
public:
    virtual void PlaySample(String const& theSample, int thePan,
                            double theVolume, double theSteps);
    virtual PIEffect* GetParticleEffect(String const& theName);
    virtual bool ShouldDrawObject(int theId, Graphics* theGraphics,
                                  PASpriteInst* theSprite, PAObjectInst* theObject,
                                  PATransform* theTransform, Color const& theColor);
    virtual bool DidDrawObject(int theId, Graphics* theGraphics,
                               PASpriteInst* theSprite, PAObjectInst* theObject,
                               PATransform* theTransform, Color const& theColor);
    virtual int DrawObjectHelper(int theId, PASpriteInst* theSprite,
                                 PAObjectInst* theObject, float* theTransform,
                                 Image* theImage, Graphics* theGraphics,
                                 float theScale);
    virtual bool OnSpriteInstFrameEnd(int theId);
    virtual bool OnSpriteInstLoop(int theId, int theFrame);
    virtual void UnusedSlot();
    virtual bool OnCommand(int theId, PASpriteInst* theSprite,
                           String const& theName, String const& theArgs);
};

// A 3x3 pose plus the identity constructor the loader and the draw path share.
class PATransform
{
public:
    SexyMatrix3 mMatrix;

    PATransform();
    PATransform& TransformSrc(PATransform const& theSource, float const* theValues);
    PATransform& InterpolateTo(PATransform const& theFrom, float const* theTo, float theFraction);
};

class PopAnim : public Widget
{
public:
    virtual ~PopAnim();
    virtual void Update();
    virtual void UpdateF(float theFrac);
    virtual void Draw(Graphics* theGraphics);

    virtual int DrawSprite(Graphics* theGraphics, PASpriteInst* theSprite,
                           PATransform* theParent, Color const& theColor,
                           bool theAdditive, bool theNoInterp);
    virtual int DrawSpriteMirrored(Graphics* theGraphics, PASpriteInst* theSprite,
                                   PATransform* theParent, Color const& theColor,
                                   bool theAdditive, bool theNoInterp);
    virtual int Load_Init();
    virtual int Load_SetModPamFile(String const& theFile);
    virtual void Load_AddRemap(String const& theFrom, String const& theTo);
    virtual int Load_LoadPam(String const& theFile);
    virtual int Load_LoadMod(String const& theFile);
    virtual SharedImageRef Load_GetImageHook(String const& theDir, String const& theName,
                                             String const& theFile);
    virtual int Load_GetImage(PAImage* theImage, String const& theDir,
                              String const& theName, String const& theFile);
    virtual void Load_PostImageLoadHook(PAImage* theImage);

    int mId;
    PopAnimListener* mListener;
    std::vector<String> mSearchPaths;
    int mPamVersion;
    Buffer mPamBuffer;
    float mScaleX;
    float mScaleY;
    bool mPamLoaded;
    int mPamCursor;
    String mModPamFile;
    String mPamFileName;
    std::list<std::pair<String, String> > mRemaps;
    int mMotion;
    int mOriginX;
    int mOriginY;
    int mAnimWidth;
    int mAnimHeight;
    String mErrorString;
    String mAnimName;
    std::vector<PAImage> mImages;
    PASpriteInst* mSpriteInst;
    PASpriteTable* mSpriteDef;
    float mBlendDuration;
    float mBlendTime;
    float mDelay;
    MTRand mRandom;
    bool mSaveRandomState;
    float mOffsetX;
    float mOffsetY;
    SexyTransform2D mTransform;
    Color mColor;
    bool mAdditive;
    bool mTransformsDirty;
    bool mActive;
    bool mDrawMirrored;
    bool mInterpolatePoses;
    bool mImageIsNew;
    bool mParticlesDisabled;
    bool mDebugDraw;
    bool mLoopNotified;

    int Fail(String const& theMessage);
    int LoadFile(String const& theFile, bool theMirrored);
    int ResetAnimHelper(PASpriteInst* theSprite);
    float DrawParticleEffects(Graphics* theGraphics, PASpriteInst* theSprite,
                              PATransform* theParent, Color const& theColor, bool theAdditive);
    int GetToFirstFrame();
    PopAnim* SaveStateSpriteInst(Buffer& theBuffer, PASpriteInst* theSprite);
    int SetColor(Color const& theColor);
    int CalcObjectPos(PASpriteInst* theSprite, int theIndex, bool theNoInterp,
                      PATransform* theTransform, Color* theColor);
    int UpdateTransforms(PASpriteInst* theSprite, PATransform* theParent,
                         Color const& theColor, bool theNoInterp);
    int HasParticles(PASpriteInst* theSprite) const;
    int IsActive() const;
    int IncSpriteInstFrame(PASpriteInst* theSprite, PAObjectPos* thePose, float theStep);
    int GetLabelFrame(String const& theName);
    const char** FindSpriteDef(char const* theName);
    const String& Remap(String const& theName);
    int UpdateParticles(PASpriteInst* theSprite, PAObjectPos* thePose);
    PopAnim* CleanParticles(PASpriteInst* theSprite, bool theAll);
    int ResetAnim();
    int Clear();
    int InitSpriteInst(PASpriteInst* theSprite, PASpriteDef* theDef);
    int SetupSpriteInst(String const& theName);
    int SaveState(Buffer& theBuffer);
    int GetObjectInst(String const& theName);
    int* FrameHit(PASpriteInst* theSprite, PAFrame* theFrame, PAObjectPos* thePose);
    int* DoFramesHit(PASpriteInst* theSprite, PAObjectPos* thePose);
    int Play(int theFrame, bool theReset);
    int Play(String const& theName, bool theReset);
    int BlendTo(String const& theName, int theDuration, int theDelay);
    void PrepSpriteInstFrame(PASpriteInst* theSprite, PAObjectPos* thePose);
    void AnimUpdate(float theStep);
    void LoadStateSpriteInst(Buffer const& theBuffer, PASpriteInst* theSprite);
    int LoadState(Buffer const& theBuffer);
    int LoadSpriteDef(Buffer* theBuffer, PASpriteDef* theDef);
    PopAnim(int theId, PopAnimListener* theListener);
    PopAnim(PopAnim const& theOther);
    PopAnim* Duplicate();
};

} // namespace Sexy

using Sexy::PopAnim;
