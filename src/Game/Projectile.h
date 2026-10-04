// Projectile -- reconstructed from libGameMain.so.
//
// Real polymorphic class: `class Projectile : public GameObject`
// (GameObject : SyncObject). The base occupies +0x00..+0x33, the derived
// members start at +0x34 and the whole object is 0xA0 bytes.
//
// Size evidence:
//   * Board::AddProjectile (@001520BC) constructs the object inside the
//     mProjectiles data array with `memset(v12, 0, 0xA0u)`, and the array
//     stride is 164 = 0xA4 with the data array's own ID word at +0xA0.
//   * A `DataArray<T>` element is
//     `struct DataArrayItem { T mItem; uint32_t mID; }`
//     (recon-src\PvZ\include\PvZ\TodLib\Common\DataArray.h:39), which is why
//     the stride is 0xA0 + 4.
//   * The constructor (@00194BA0) registers exactly one sync block for this
//     object, { &this->mFrame, 108 }; 108 = 0xA0 - 0x34, i.e. the derived
//     region is precisely +0x34..+0x9F, and it then zeroes those 108 bytes.
//
// vtable order -- the actual contents of the binary; `virtual` must be
// declared in this order:
//   [  0] 0x192dfc  Projectile::~Projectile
//   [  1] 0x192e30  Projectile::~Projectile
//   [  2] 0x1b7b80  GameObject::BeginDraw
//   [  3] 0x1b7af0  GameObject::EndDraw
//   [  4] 0x1b7b1c  GameObject::MakeParentGraphicsFrame
// Slots 2/3/4 hold GameObject's own implementations, so this class overrides
// none of them -- the authoritative header (PvZ\Lawn\Board\Projectile.h) has
// no matching overrides either. This class therefore adds no virtual function
// at all: a virtual destructor is all that is needed. (Note: in an MSVC x86
// vtable a virtual destructor takes one slot; entries [0]/[1] above are the
// ARM complete/deleting pair. Declaring them in this order is what matters.)

#pragma once

#include "ReconTypes.h"
// These headers are included so the transcribed bodies can call methods on
// complete types; they are kept as they are to avoid disturbing the
// transitive dependencies other files have on this header.
#include "GameObject.h"
#include "Graphics.h"
#include "GridItem.h"
#include "Image.h"
#include "Reanimation.h"
#include "SexyTransform2D.h"
#include "Zombie.h"

// Board and LawnApp are not named in this file at all, and CutScene is not
// either, so all three includes were pure cycle (Projectile.h <-> Board.h,
// Projectile.h <-> CutScene.h, Projectile.h <-> LawnApp.h). The Projectile.h
// -> Board.h edge in particular sat on Board.h's own heavy cycle cluster.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
class CutScene;
class LawnApp;

// One 12-byte record of the projectile definition table (gProjectileDefinition @
// 0x69702C, one entry per ProjectileType). The authoritative header
// (PvZ\Lawn\Board\Projectile.h:34) declares this class and spells the first two
// fields. Which word is which is fixed by the two bodies that read it:
// Draw's source rectangle is `mImageRow * GetCelHeight()` off +0x04, and every
// DoImpact/DoSplashDamage/CheckForCollision body subtracts +0x08 from a health
// field -- so +0x04 is the image row and +0x08 the damage. Three words make 12.
class ProjectileDefinition
{
public:
    int mProjectileType; // +0x00
    int mImageRow;       // +0x04
    int mDamage;         // +0x08
    // size 0x0C
};

class Projectile : public GameObject
{
public:
    virtual ~Projectile();

    int CantHitHighGround();
    int IsSplashDamage(Zombie *theZombie);
    bool IsSplashDamage(GridItem *theGridItem);
    int GetDamageFlags(Zombie *theZombie);
    // The binary's PlayImpactSound falls off the end of every path (IDA types it
    // `void`, and neither DoImpact nor DoImpactGridItem looks at a result), so it
    // returns nothing.
    void PlayImpactSound(Zombie *theZombie);
    void DrawShadow(Sexy::Graphics *theGraphics);
    void Die();
    // +0x04 of the record is the image row and +0x08 its damage, so the return
    // really is a ProjectileDefinition; the transcribed `char *` only stood in for
    // "pointer into the gProjectileDefinition table".
    ProjectileDefinition * GetProjectileDef();
    // The binary returns a 16-byte Sexy::Rect BY VALUE -- ARM passes the
    // destination buffer in r0, which is why the transcription merged it with
    // `this` and wrote the rectangle over the object's own +0x00..+0x0C (the
    // vptr). The sret shape is restored here; the call sites outside this file
    // still need the same treatment.
    Sexy::Rect GetProjectileRect();
    Plant * FindCollisionTargetPlant();
    GridItem * FindCollisionTargetGridItem();
    int PeaAboutToHitTorchwood();
    Zombie * FindCollisionTarget();
    bool IsZombieHitBySplash(Zombie *theZombie);
    bool IsGridItemHitBySplash(GridItem *theGridItem);
    void DoSplashDamage(Zombie *theHitZombie, GridItem *theHitGridItem);
    void DoImpact(Zombie *theZombie);
    // The binary's CheckForHighGround has no result -- UpdateNormalMotion calls it
    // for its side effect only (it may run DoImpact and kill the projectile).
    void CheckForHighGround();
    void DoImpactGridItem(GridItem *theGridItem);
    void UpdateLobMotion();
    void CheckForCollision();
    void UpdateNormalMotion();
    void UpdateMotion();
    void Update();
    void ConvertToFireball(int theGridX);
    void ProjectileInitialize(int theX, int theY, int theRenderOrder, int theRow, ProjectileType theType);
    void ConvertToPea(int theGridX);
    Projectile();
    void Draw(Sexy::Graphics *theGraphics);

    // ---- Derived member area +0x34..+0x9F (108 bytes); the constructor
    // registers it as one sync block. Names and types come from the
    // authoritative header PvZ\Lawn\Board\Projectile.h; where that header
    // spells the offset as an ordinal (`mFrame // 13`) the byte offset is
    // ordinal * 4, and the two entries that spell it in bytes
    // (`mDead // 96`, `mOnHighGround // 128`) are used as written.
    int      mFrame;                // +0x34  (ordinal 13)
    int      mNumFrames;            // +0x38  (14)
    int      mAnimCounter;          // +0x3C  (15)
    float    mPosX;                 // +0x40  (16)
    float    mPosY;                 // +0x44  (17)
    float    mPosZ;                 // +0x48  (18)
    float    mVelX;                 // +0x4C  (19)
    float    mVelY;                 // +0x50  (20)
    float    mVelZ;                 // +0x54  (21)
    float    mAccZ;                 // +0x58  (22)
    float    mShadowY;              // +0x5C  (23)
    bool     mDead;                 // +0x60  (authoritative header says 96)
    int      mAnimTicksPerFrame;    // +0x64  (25)
    // TODO(recon): the authoritative header says `ProjectileMotion mMotionType`
    // but this project has no such type (only the enum definition in
    // PvZ\Lawn\Common\ConstEnums.h:980). Its value domain is
    // MOTION_STRAIGHT=0 / MOTION_LOBBED=1 / MOTION_THREEPEATER=2 / MOTION_BEE=3 /
    // MOTION_BEE_BACKWARDS=4 / MOTION_PUFF=5 / MOTION_BACKWARDS=6 / MOTION_STAR=7 /
    // MOTION_FLOAT_OVER=8 / MOTION_HOMING=9, which is what the plain integers in
    // the .cpp compare against, so keep the int placeholder.
    int      mMotionType;           // +0x68  (26)
    ProjectileType mProjectileType; // +0x6C  (27)
    int      mProjectileAge;        // +0x70  (28)
    int      mClickBackoffCounter;  // +0x74  (29)
    float    mRotation;             // +0x78  (30)
    float    mRotationSpeed;        // +0x7C  (31)
    bool     mOnHighGround;         // +0x80  (authoritative header says 128)
    int      mDamageRangeFlags;     // +0x84  (33)
    int      mHitTorchwoodGridX;    // +0x88  (34)
    // TODO(recon): the authoritative header says `AttachmentID mAttachmentID`;
    // this project has no AttachmentID, and the Attachment* family
    // (Sexy\ReconFreeFunctions.h:221 onwards) are variadic stubs, so this word
    // is only ever passed by value or by address -- an int placeholder is enough.
    int      mAttachmentID;         // +0x8C  (35)
    float    mCobTargetX;           // +0x90  (36)
    int      mCobTargetRow;         // +0x94  (37)
    ZombieID mTargetZombieID;       // +0x98  (38)
    int      mLastPortalX;          // +0x9C  (39)

    // The authoritative header has five more members after +0xA0
    // (mPierceHitCount, mHitZombieIDs[3], mHitGridItemIDs[3], mRelatedPlantID,
    // mReturning), none of them with an offset comment. This binary's
    // Projectile has no access evidence for them either -- the largest `this`
    // offset reached by any method is +0x9C -- so they are a pierce/boomerang
    // feature added by the hook project. Migrating them would push the object
    // to 0xC4, which contradicts the measured 0xA0, so they are not migrated.
};

struct ProjectileSlot
{
    Projectile mItem;
    unsigned int mID;
};

// Layout self-check: 0x34 (GameObject) + 27 four-byte slots (two of them bools,
// which padding keeps four bytes apart).
