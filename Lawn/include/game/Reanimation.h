// Reanimation -- reconstructed from libGameMain.so.
//
// Polymorphic class: `class Reanimation : public SyncObject`. The base occupies
// +0x00..+0x0F (vptr + the 12-byte sync-block vector), so the derived members
// start at +0x10. Member names, order and offsets are from the authoritative
// header PvZ\TodLib\Effect\Reanimator.h:165, and the transcribed bodies confirm
// every anchor:
//   * ColorsMultiply(.., this + 88)  -> mColorOverride at +0x58;
//   * AttachmentPropogateColor(.., this + 124, .., this + 144)
//     -> mExtraAdditiveColor at +0x7C and mExtraOverlayColor at +0x90;
//   * the constructor writes the three flags at bytes 116, 140, 160 and 208.
// Size 0xD8 (54 words), which the authoritative header states explicitly at the
// end of its member table ("54 integers").
//
// vtable order -- from the RTTI export _ZTV11ReanimationE, which holds nothing
// but the two Itanium destructor slots. MSVC x86 emits ONE slot for
// `virtual ~Reanimation()`, so a single virtual destructor reproduces it:
//   [  0] 0x1fac10  Reanimation::~Reanimation   (D2)
//   [  1] 0x1fac60  Reanimation::~Reanimation   (D0)
// Update/Draw and the whole track API are NOT virtual in the binary.
//
// The transcribed header used a 0xE8-byte opaque placeholder here, taken
// from the largest `this`-relative offset in the IDA dump.

#pragma once

#include "ReconTypes.h"
#include "SyncObject.h"
// The transcribed bodies call methods on these, so the includes stay.
#include "Graphics.h"
#include "Image.h"
#include "ReanimationHolder.h"
#include "ReanimatorDefinition.h"
#include "ReanimatorTransform.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"

class AttachEffect;
class TodAllocator;
class TodParticleSystem;

// The 100-byte per-track runtime state. The member table is
// PvZ\TodLib\Effect\Reanimator.h:147, and the instance constructor at 0x1F9584
// writes the same fields. mTrackColor is the 16-byte Color at +0x50: ZombiesWon
// @0x15B374 adds 0x50 to the pointer GetTrackInstanceByName returns and stores
// four words there.
class ReanimatorTrackInstance
{
public:
    int                  mBlendCounter;               // +0x00
    int                  mBlendTime;                  // +0x04
    ReanimatorTransform  mBlendTransform;             // +0x08
    float                mShakeOverride;              // +0x38
    float                mShakeX;                     // +0x3C
    float                mShakeY;                     // +0x40
    int                  mAttachmentID;               // +0x44
    Sexy::Image         *mImageOverride;              // +0x48
    int                  mRenderGroup;                // +0x4C
    Sexy::Color          mTrackColor;                 // +0x50
    bool                 mIgnoreClipRect;             // +0x60
    bool                 mTruncateDisappearingFrames; // +0x61
    bool                 mIgnoreColorOverride;        // +0x62
    bool                 mIgnoreExtraAdditiveColor;   // +0x63

    ReanimatorTrackInstance();
};
class TodTriangleGroup;

// The 12-byte record the frame-time queries fill in. GetFrameTime @0x1FA2DC
// writes the fraction between the two frames and their indices, and every
// caller passes the object's own record at +0xC0 -- which the constructor
// zero-fills along with the rest of the block.
class ReanimatorFrameTime
{
public:
    float mFraction;           // +0x00
    int   mAnimFrameBeforeInt; // +0x04
    int   mAnimFrameAfterInt;  // +0x08
};


class Reanimation : public SyncObject
{
public:
    virtual ~Reanimation();

    ReanimationType            mReanimationType;           // +0x10
    float                      mAnimTime;                  // +0x14
    float                      mAnimRate;                  // +0x18
    ReanimatorDefinition      *mDefinition;                // +0x1C
    ReanimLoopType             mLoopType;                  // +0x20
    bool                       mDead;                      // +0x24
    int                        mFrameStart;                // +0x28
    int                        mFrameCount;                // +0x2C
    int                        mFrameBasePose;             // +0x30
    Sexy::SexyTransform2D      mOverlayMatrix;             // +0x34 (36 bytes)
    Sexy::Color                mColorOverride;             // +0x58 (16 bytes)
    ReanimatorTrackInstance   *mTrackInstances;            // +0x68
    int                        mLoopCount;                 // +0x6C
    ReanimationHolder         *mReanimationHolder;         // +0x70
    bool                       mIsAttachment;              // +0x74
    int                        mRenderOrder;               // +0x78
    Sexy::Color                mExtraAdditiveColor;        // +0x7C (16 bytes)
    bool                       mEnableExtraAdditiveDraw;   // +0x8C
    Sexy::Color                mExtraOverlayColor;         // +0x90 (16 bytes)
    bool                       mEnableExtraOverlayDraw;    // +0xA0
    float                      mLastFrameTime;             // +0xA4
    FilterEffect               mFilterEffect;              // +0xA8
    Sexy::Color                mCustomFilterEffectColor;   // +0xAC (16 bytes)
    // +0xBC is the dirty-flag word (bit 0 pending frame time, bits 1..2 pending
    // transforms/attachments) and +0xC0 is the object's own frame-time record;
    // together they fill the 16 bytes the header used to call `int unk2[4]`.
    int                        mDirtyFlags;                // +0xBC
    ReanimatorFrameTime        mFrameTime;                 // +0xC0 (12 bytes)
    ReanimatorTransform       *mReanimatorTransforms;      // +0xCC
    bool                       mFrameTimeReady;            // +0xD0
    float                      mAnimTimeStep;              // +0xD4
    // size 0xD8

    Reanimation * SetAnimRate(float theRate);
    ReanimatorTransform * GetTransformAtTime(int theTrackIndex, ReanimatorTransform *theTransform, ReanimatorFrameTime *theFrameTime);
    int GetCurrentTransform(int theTrackIndex, ReanimatorFrameTime *theFrameTime, ReanimatorTransform *theTransform);
    int UpdateCurrentTransform(int theTrackIndex, ReanimatorFrameTime *theFrameTime);
    float MatrixFromTransform(ReanimatorTransform &theTransform, SexyMatrix3 &theMatrix);
    void ReanimBltMatrix(Sexy::Graphics *g, Sexy::Image *theImage, SexyMatrix3 const &theTransform, TRect<int> const &theClipRect, Color const &theColor, int theDrawMode, TRect<int> const &theSrcRect);
    void SetFilterEffectShader(FilterEffect theFilterEffect);
    bool DrawTrack(Sexy::Graphics *g, int theTrackIndex, int theRenderGroup, TodTriangleGroup *theTriangleGroup);
    float GetFrameTime(ReanimatorFrameTime *theFrameTime);
    ReanimatorTransform * GetCurrentTransform(int theTrackIndex, ReanimatorTransform *theTransform);
    void GetTrackMatrix(int theTrackIndex, Sexy::SexyTransform2D &theMatrix);
    int FindTrackIndexById(const char *theTrackId);
    Sexy::Image * GetCurrentTrackImage(char *theTrackName);
    int FindTrackIndex(char const *theTrackName);
    Sexy::Image * GetCurrentTrackImage(char const *theTrackName);
    void AttachToAnotherReanimation(Reanimation *theAttachReanim, const char *theTrackName);
    void GetTrackBasePoseMatrix(int theTrackIndex, Sexy::SexyTransform2D &theBaseMatrix);
    AttachEffect * AttachParticleToTrack(char const *theTrackName, TodParticleSystem *theParticleSystem, float thePosX, float thePosY);
    Sexy::SexyTransform2D & GetAttachmentOverlayMatrix(int theTrackIndex, ReanimatorTransform &theTransform, Sexy::SexyTransform2D &theOverlayMatrix);
    void CheckDirtyFlags();
    void DrawRenderGroup(Sexy::Graphics *g, int theRenderGroup);
    void Draw(Sexy::Graphics *g);
    Sexy::SexyTransform2D & GetAttachmentOverlayMatrix(int theTrackIndex, Sexy::SexyTransform2D &theOverlayMatrix);
    void GetFramesForLayer(char *theTrackName, int &theFrameStart, int &theFrameCount);
    void GetFramesForLayer(char const *theTrackName, int &theFrameStart, int &theFrameCount);
    Reanimation * SetFramesForLayer(char const *theTrackName);
    bool TrackExists(char const* theTrackName);
    // `_ZN11Reanimation15TrackExistsByIdEPc` @0x1FAAC0. The body compares the
    // argument with the track record's first word, so it is the track id.
    bool TrackExistsById(const char* theTrackId);
    void StartBlend(int theBlendTime);
    TodAllocator * ReanimationDelete();
    void ReanimationDie();
    void SetShakeOverride(char const *theTrackName, float theShakeAmount);
    Sexy::Image * GetImageOverride(char *theTrackName);
    Sexy::Image * GetImageOverride(char const *theTrackName);
    void SetImageOverride(char *theTrackName, Sexy::Image *theImage);
    void SetImageOverride(char const *theTrackName, Sexy::Image *theImage);
    void SetImageDefinition(char const *theTrackName, Sexy::Image *theImage);
    void SetTruncateDisappearingFrames(char const *theTrackName, bool theTruncateDisappearingFrames);
    void SetPosition(float theX, float theY);
    void ReanimationInitialize(float theX, float theY, ReanimatorDefinition *theDefinition);
    void GetPosition(float &theX, float &theY);
    void OverrideScale(float theScaleX, float theScaleY);
    float GetTrackVelocity(int theTrackIndex);
    float GetTrackVelocity(char const *theTrackName);
    bool IsTrackShowing(char const *theTrackName);
    void ShowOnlyTrack(char const *theTrackName);
    void AssignRenderGroupToTrack(char const *theTrackName, int theRenderGroup);
    void AssignRenderGroupToPrefix(char const *theTrackName, int theRenderGroup);
    void IgnoreClipRectForPrefix(char const *theTrackName, bool theIgnore);
    void PropogateColorToAttachments();
    bool ShouldTriggerTimedEvent(float theEventTime);
    Reanimation * PlayReanim(char const *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);
    ReanimatorTrackInstance * GetTrackInstanceByName(char const *theTrackName);
    void ParseAttacherTrack(ReanimatorTransform const &theTransform, AttacherInfo &theAttacherInfo);
    void AttacherSynchWalkSpeed(int theTrackIndex, Reanimation *theAttachReanim, AttacherInfo const &theAttacherInfo);
    bool IsAnimPlaying(char *theTrackName);
    bool IsAnimPlaying(char const *theTrackName);
    void SetBasePoseFromAnim(char const *theTrackName);
    Reanimation * FindSubReanim(ReanimationType theReanimType);
    Reanimation();
    void ReanimationInitializeType(float theX, float theY, ReanimationType theReanimType);
    void UpdateAttacherTrack(int theTrackIndex);
    void Update();
};

struct ReanimationSlot
{
    Reanimation mItem;
    unsigned int mID;
};

// Layout check: 0x10 (SyncObject) + 50 words of members.

// Interned reanimation track-name ids. Startup fills them through
// ReanimatorTrackNameToId, and TrackExistsById compares that pointer with
// ReanimatorTrack::mTrackId.
const char* ReanimatorTrackNameToId(const char* theName);
void InitReanimTrackIds();
extern const char* ReanimTrackId__ground;
extern const char* ReanimTrackId_anim_death;
extern const char* ReanimTrackId_anim_death2;
extern const char* ReanimTrackId_anim_head1;
extern const char* ReanimTrackId_anim_idle;
extern const char* ReanimTrackId_anim_idle2;
extern const char* ReanimTrackId_anim_sleep;
extern const char* ReanimTrackId_anim_stem;
extern const char* ReanimTrackId_anim_superlongdeath;
extern const char* ReanimTrackId_anim_swim;
extern const char* ReanimTrackId_anim_walk;
extern const char* ReanimTrackId_anim_walk2;
extern const char* ReanimTrackId_anim_walnutdeath;
extern const char* ReanimTrackId_anim_waterdeath;
extern const char* ReanimTrackId_boss_jaw;
extern const char* ReanimTrackId_fullscreen;
extern const char* ReanimTrackId_zombie_catapult_pole;
extern const char* ReanimTrackId_zombie_mustache;
