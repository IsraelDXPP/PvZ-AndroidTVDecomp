// PIEffect plays a loaded particle definition. It owns the file reader, the
// shared definition, the live emitters and the particles they spawn. Three
// virtuals sit at the front, in the binary's order: GetImage, SetImageOpts,
// WriteImage. There is no virtual destructor.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PIEFFECT_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PIEFFECT_H

#include "ReconTypes.h"
#include "Bezier.h"
#include "Buffer.h"
#include "CritSect.h"
#include "Graphics.h"
#include "Image.h"
#include "MTRand.h"
#include "PIBlocker.h"
#include "PIDeflector.h"
#include "PIEmitter.h"
#include "PIEmitterInstance.h"
#include "PIEmitterInstanceDef.h"
#include "PIForce.h"
#include "PIInterpolator.h"
#include "PILayer.h"
#include "PILayerDef.h"
#include "PITexture.h"
#include "PIValue.h"
#include "PIValue2D.h"
#include "SexyTransform2D.h"
#include "SharedImageRef.h"
#include "Transform.h"
#include <SDL3/SDL_iostream.h>
#include <map>
#include <vector>

namespace Sexy
{

class PIParticleDef;

struct PIParticle;
struct PIParticleType;

// One loaded texture set. LoadEffect builds one per CMultiTexture record.
// mRefs holds the uploaded frames; mImage is the atlas the wide format binds.
struct PITextureData
{
    Sexy::String mName;
    std::vector<Sexy::SharedImageRef> mRefs;
    Sexy::SharedImageRef mImage;
    int mImageCount;
    bool mAtlas;

    PITextureData() :
        mName(),
        mRefs(),
        mImage(),
        mImageCount(0),
        mAtlas(false)
    {
    }
};

// The group record UpdateParticleGroup and CalcParticleTransform receive.
// mHead and mTail are the particle chain, mCount its length. mSkipAttach
// selects the free-emitter path; mUseEmitter selects the emitter's curves.
struct PIParticleGroup
{
    PIParticle *mHead;
    PIParticle *mTail;
    int mCount;
    unsigned char mSkipAttach;
    unsigned char mUseEmitter;

    PIParticleGroup() :
        mHead(nullptr),
        mTail(nullptr),
        mCount(0),
        mSkipAttach(0),
        mUseEmitter(0)
    {
    }
};

// One live particle. The chain words sit at the front, the definition and
// emitter next, then position, velocity and the variation samples the spawn
// path writes. mTransform is the draw matrix. mSpawnRecords is the per-child
// accumulator the particle owns.
struct PIParticle
{
    PIParticle *mPrev;
    PIParticle *mNext;
    PIParticleType *mType;
    PIEmitter *mEmitter;
    int mSpawnIndex;
    PIParticle *mParent;
    float mPosX;
    float mPosY;
    float mLaunchPosX;
    float mLaunchPosY;
    float mScaleX;
    float mScaleY;
    float mVelX;
    float mVelY;
    float mSpinX;
    float mSpinY;
    float mSize;
    float mVarLife;
    float mVarSpin;
    float mVarSpin2;
    float mVarLaunch;
    float mVarGravity;
    float mVarSpinRate;
    float mVarJitter;
    float mVarBounce;
    float mVarUnused;
    float mAngle;
    float mSpinScaleX;
    float mSpinScaleY;
    float mKeyParam;
    float mLaunchAngle;
    int mFrame;
    Sexy::Transform mTransform;
    float mArea;
    int mFrameIndex;
    float mEmitterTime;
    float mLife;
    float mLifePct;
    float mAge;
    unsigned char mDead;
    unsigned char mPadDD[3];
    unsigned int mColor;
    std::vector<PIParticleDefInstance> mSpawnRecords;
    PIParticle *mChildren;
    PIParticle *mChildTail;
    int mChildCount;
    unsigned char mFlagFC;
    unsigned char mFlagFD;

    PIParticle() :
        mPrev(nullptr),
        mNext(nullptr),
        mType(nullptr),
        mEmitter(nullptr),
        mSpawnIndex(0),
        mParent(nullptr),
        mPosX(0.0f),
        mPosY(0.0f),
        mLaunchPosX(0.0f),
        mLaunchPosY(0.0f),
        mScaleX(0.0f),
        mScaleY(0.0f),
        mVelX(0.0f),
        mVelY(0.0f),
        mSpinX(0.0f),
        mSpinY(0.0f),
        mSize(0.0f),
        mVarLife(0.0f),
        mVarSpin(0.0f),
        mVarSpin2(0.0f),
        mVarLaunch(0.0f),
        mVarGravity(0.0f),
        mVarSpinRate(0.0f),
        mVarJitter(0.0f),
        mVarBounce(0.0f),
        mVarUnused(0.0f),
        mAngle(0.0f),
        mSpinScaleX(0.0f),
        mSpinScaleY(0.0f),
        mKeyParam(0.0f),
        mLaunchAngle(0.0f),
        mFrame(0),
        mTransform(),
        mArea(1.0f),
        mFrameIndex(0),
        mEmitterTime(0.0f),
        mLife(0.0f),
        mLifePct(0.0f),
        mAge(0.0f),
        mDead(0),
        mColor(0xFFFFFFFFu),
        mSpawnRecords(),
        mChildren(nullptr),
        mChildTail(nullptr),
        mChildCount(0),
        mFlagFC(0),
        mFlagFD(0)
    {
        mPadDD[0] = 0;
        mPadDD[1] = 0;
        mPadDD[2] = 0;
    }
};

// The pool element. Free-list stride 228, dead-list stride 256. This is the
// live particle, not a one-int enum.
using PIParticleInstance = PIParticle;

// One particle type the file stores. The shared emitter still publishes a
// PIParticleDef for callers that only need the pointer; the player reads the
// curves and flags from this record.
struct PIParticleType
{
    int mUnusedWord;
    Sexy::String mName;
    int mImageIndex;
    Sexy::PIValue mValues[23];
    Sexy::PIValue mAngle;
    Sexy::PIValue mSpread;
    Sexy::PIValue mSpinY;
    Sexy::PIValue mSpinVarY;
    Sexy::PIValue mSizeYOverLife;
    float mPivotX;
    float mPivotY;
    bool mLockSpin;
    bool mAdditive;
    bool mSuperEmitter;
    bool mAdditiveSecond;
    bool mAttachToParent;
    int mAnimLength;
    bool mRandomFrame;
    float mAttachWeight;
    bool mFlipX;
    bool mFlipY;
    int mColorRepeat;
    int mAlphaRepeat;
    bool mUseColorKeys;
    bool mUseSpawnColor;
    bool mSampleColor;
    bool mSampleAlpha;
    bool mKeepAlpha;
    bool mForceAlpha;
    int mColorStride;
    bool mAlignToPath;
    bool mAlignToVelocity;
    bool mUseOwnAngle;
    int mAngleOffset;
    int mFixedAngle;
    int mAngleSpread;
    int mAngleSpreadRange;
    Sexy::PIInterpolator mColorKeys;
    Sexy::PIInterpolator mAlphaKeys;
    Sexy::PIParticleDef *mPublished;

    PIParticleType() :
        mUnusedWord(0),
        mName(),
        mImageIndex(0),
        mPivotX(0.0f),
        mPivotY(0.0f),
        mLockSpin(false),
        mAdditive(false),
        mSuperEmitter(false),
        mAdditiveSecond(false),
        mAttachToParent(false),
        mAnimLength(0),
        mRandomFrame(false),
        mAttachWeight(0.0f),
        mFlipX(false),
        mFlipY(false),
        mColorRepeat(0),
        mAlphaRepeat(0),
        mUseColorKeys(false),
        mUseSpawnColor(false),
        mSampleColor(false),
        mSampleAlpha(false),
        mKeepAlpha(false),
        mForceAlpha(false),
        mColorStride(0),
        mAlignToPath(false),
        mAlignToVelocity(false),
        mUseOwnAngle(false),
        mAngleOffset(0),
        mFixedAngle(0),
        mAngleSpread(0),
        mAngleSpreadRange(0),
        mColorKeys(),
        mAlphaKeys(),
        mPublished(nullptr)
    {
    }
};

// One placed emitter. The definition supplies the curves; this record holds
// the live flags, the spawn accumulators and the two particle chains. The
// shape words are copied out of the definition at load so the player can name
// them for what they drive.
struct PILiveEmitter
{
    Sexy::PIEmitterInstanceDef *mDef;
    Sexy::PIEmitter *mEmitter;
    Sexy::String mName;
    int mLifeBias;
    int mEmitterType;
    int mShape;
    bool mHeadingLock;
    bool mHeadingRandom;
    int mCount;
    int mRows;
    bool mFreeEmitters;
    bool mMaskCompare;
    bool mEmitting;
    bool mKeepSpawn;
    bool mActive;
    float mOffsetX;
    float mOffsetY;
    float mRateScale;
    Sexy::Color mTint;
    Sexy::SharedImageRef mImage;
    Sexy::SexyTransform2D mTransform;
    std::vector<Sexy::PIParticleDefInstance> mFreeSpawns;
    std::vector<Sexy::PIParticleDefInstance> mDefSpawns;
    PIParticleGroup mFree;
    PIParticleGroup mTrail;

    PILiveEmitter() :
        mDef(nullptr),
        mEmitter(nullptr),
        mName(),
        mLifeBias(0),
        mEmitterType(0),
        mShape(0),
        mHeadingLock(false),
        mHeadingRandom(false),
        mCount(0),
        mRows(0),
        mFreeEmitters(false),
        mMaskCompare(false),
        mEmitting(false),
        mKeepSpawn(false),
        mActive(true),
        mOffsetX(0.0f),
        mOffsetY(0.0f),
        mRateScale(1.0f),
        mTint(Sexy::Color::White),
        mImage(),
        mTransform(),
        mFreeSpawns(),
        mDefSpawns(),
        mFree(),
        mTrail()
    {
        mFree.mSkipAttach = 1;
        mTrail.mSkipAttach = 0;
    }
};

// One playing layer: the definition it was built from and the emitters on it.
// mPublic is the layer GetLayer returns, so Update and Draw step the same
// object callers hold.
struct PILiveLayer
{
    Sexy::PILayerDef *mDef;
    Sexy::PILayer *mPublic;
    std::vector<PILiveEmitter> mEmitters;

    PILiveLayer() :
        mDef(nullptr),
        mPublic(nullptr),
        mEmitters()
    {
    }
};

// The shared, reference-counted definition. Duplicate shares the pointer and
// bumps the count; Deref destroys it when the count reaches zero.
struct PIEffectDef
{
    int mRefCount;
    std::vector<Sexy::PIEmitter *> mEmitters;
    std::vector<PIParticleType *> mTypes;
    std::vector<PITextureData *> mImages;
    std::vector<Sexy::PILayerDef> mLayerDefs;
    std::map<int, int> mEmitterIndex;

    PIEffectDef() :
        mRefCount(1),
        mEmitters(),
        mTypes(),
        mImages(),
        mLayerDefs(),
        mEmitterIndex()
    {
    }
};

class PIEffect
{
public:
    virtual Sexy::SharedImageRef GetImage(Sexy::String const &unusedName, Sexy::String const &theImageName);
    virtual void SetImageOpts(Sexy::DeviceImage *theImage);
    virtual int WriteImage(
        Sexy::String const &unusedResult, int theFrame, Sexy::DeviceImage *theImage, bool *theResult);

    int Update();
    int Fail(Sexy::String const &theMessage);
    float GetRandFloat();
    float GetRandFloatU();
    float GetRandSign();
    float GetVariationScalar();
    float GetVariationScalarU();
    Sexy::String ReadString();
    size_t WriteByte(char theByte);
    size_t WriteInt(int theValue);
    size_t WriteShort(short theValue);
    size_t WriteFloat(float theValue);
    size_t WriteBool(bool theValue);
    const void *WriteString(const Sexy::String *theText);
    size_t WriteValue2D(Sexy::PIValue2D *theCurve);
    size_t WriteEPoint(Sexy::PIValue2D *theCurve);
    size_t WriteValue(Sexy::PIValue *theCurve);
    size_t WriteEmitterType(Sexy::PIEmitter *theEmitter);
    int SaveParticleDefInstance(Sexy::Buffer &theBuffer, Sexy::PIParticleDefInstance *theRecord);
    int SaveParticle(Sexy::Buffer &theBuffer, Sexy::PILayer *theLayer, PIParticle *theParticle);
    int LoadParticleDefInstance(Sexy::Buffer const &theBuffer, Sexy::PIParticleDefInstance *theRecord);
    int LoadParticle(Sexy::Buffer const &theBuffer, Sexy::PILayer *theLayer, PIParticle *theParticle);
    int ResetAnim();
    Sexy::PILayer *GetLayer(int theIndex);
    Sexy::PILayer *GetLayer(Sexy::String const &theName);
    Sexy::TPoint<float> GetEmitterPos(Sexy::PIEmitterInstance *theInstance, bool theTransform);
    int CountParticles(PIParticle *theHead);
    bool HasTimelineExpired();
    int IsActive();
    Sexy::String GetNotesParam(Sexy::String const &theKey, Sexy::String const &theDefault);
    int CheckCache();
    int SetCacheUpToDate();
    void WriteToCache();
    void Deref();
    int Clear();
    PIEffect();
    ~PIEffect();
    Sexy::String ReadStringS();
    int ExpectCmd(Sexy::String const &theExpected);
    void ReadValue2D(Sexy::PIValue2D *theCurve);
    int ReadEPoint(Sexy::PIValue2D *theCurve);
    Sexy::TPoint<float> GetGeomPos(
        Sexy::PIEmitterInstance *theInstance, PIParticle *theParticle, float *theAngleOut, bool *theMask);
    int CalcParticleTransform(Sexy::PILayer *theLayer, Sexy::PIEmitterInstance *theInstance,
        Sexy::PIEmitter *theEmitter, Sexy::PIParticleDef *theDef, PIParticleGroup *theGroup, PIParticle *theParticle);
    void UpdateParticleGroup(Sexy::PILayer *theLayer, Sexy::PIEmitterInstance *theInstance, PIParticleGroup *theGroup);
    Color *DrawParticleGroup(Sexy::Graphics *theGraphics, Sexy::PILayer *theLayer, Sexy::PIEmitterInstance *theInstance,
        PIParticleGroup *theGroup, unsigned char theAdditivePass);
    int DrawLayer(Sexy::Graphics *theGraphics, Sexy::PILayer *theLayer);
    float Draw(Sexy::Graphics *theGraphics);
    PIParticleDefInstance *UpdateParticleDef(Sexy::PILayer *theLayer, Sexy::PIEmitter *theEmitter,
        Sexy::PIEmitterInstance *theInstance, Sexy::PIParticleDef *theDef, PIParticleDefInstance *theAccum,
        PIParticleGroup *theGroup, PIParticle *theParent);
    void ReadValue(Sexy::PIValue *theCurve);
    int SaveState(Sexy::Buffer &theBuffer, bool theCompact);
    int ReadEmitterType(Sexy::PIEmitter *theEmitter);
    PIEffect(Sexy::PIEffect const &theOther);
    Sexy::PIEffect *Duplicate();
    int LoadEffect(Sexy::String const &thePath);
    int LoadState(Sexy::Buffer const &theBuffer, bool theCompact);

    SDL_IOStream *mReadFile;
    SDL_IOStream *mWriteFile;
    unsigned int mChecksum;
    bool mWideBooleans;
    bool mLoadedFlag;
    int mFileVersion;
    Sexy::String mFilePath;
    Sexy::String mWritePath;
    Sexy::MTRand mRand;
    Sexy::Buffer mBuffer;
    union
    {
        unsigned int mReadWords[256];
        unsigned char mReadBuf[1024];
    };
    int mReadValue;
    int mReadPos;
    Sexy::String mNotesText;
    int mNotesCursor;
    std::vector<Sexy::String> mStrings;
    int mSettingA;
    int mSettingB;
    Sexy::Color mLoadColor;
    int mParticleScale;
    int mTimelineStart;
    int mTimelineEnd;
    int mUnusedE40;
    std::map<Sexy::String, Sexy::String> mNotes;
    PIEffectDef *mDef;
    std::vector<Sexy::PILayer> mLayers;
    std::vector<PILiveLayer> mLive;
    Sexy::String mError;
    bool mLoaded;
    int mFrameTick;
    float mTime;
    bool mTimeCrossed;
    Sexy::ObjectPool<Sexy::PIParticleInstance> mFreeList;
    int mFreeStride;
    Sexy::ObjectPool<Sexy::PIParticleInstance> mDeadList;
    int mDeadStride;
    bool mEmitEnabled;
    bool mEmitAfter;
    bool mLoopFlag;
    int mUpdateCountA;
    int mUpdateCountB;
    int mPixelCount;
    float mTimeScale;
    Sexy::Color mColor;
    bool mDebugDraw;
    bool mVisibleFlag;
    bool mPausedFlag;
    std::vector<int> mRandSeeds;
    int mRandSeedCursor;
    int mUnusedED8;
    bool mNeedsSeed;
    SexyTransform2D mTransform;
    SexyTransform2D mWorldTransform;

    PILiveEmitter *FindLive(Sexy::PIEmitterInstance *theInstance);
    PIParticleType *FindType(Sexy::PIParticleDef *theDef);
};

} // namespace Sexy

using Sexy::PIEffect;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PIEFFECT_H
