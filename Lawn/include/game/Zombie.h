// Zombie -- reconstructed from libGameMain.so.
//
// Real inheritance chain: `class Zombie : public GameObject` (and GameObject itself
// derives from SyncObject). Evidence: slot [2] of the vtable comment block is
// `GameObject::BeginDraw`, so Zombie's vtable overrides only the destructor on top of
// GameObject's five slots; `Zombie::~Zombie` also calls `GameObject::~GameObject(this)`
// explicitly (0x1AFD88).
//
// The member table is copied from the authoritative header PvZ\Lawn\Board\Zombie.h
// (names, types and order verbatim). Absolute offset = the ordinal in that header's
// comment times 4; the comments on bool/short members are byte offsets already. The
// eight fields that header marks as newly added members sit in the padding after the
// preceding bool and do not change 0x184.
//
// Size **0x184**:
//   * AlmanacDialog.cpp:291 / :412 allocate a Zombie of 0x184 bytes;
//   * the array stride of `mZombies` in Board.cpp:4012 is 392 (0x188) = 0x184 + 4,
//     the extra word being DataArrayItem::mID;
//   * the tail is self-consistent: the last member mCachedZombieRect sits at
//     +0x174 and ends exactly at +0x184.
// The static_assert below pins it down.
//
// vtable order (slots 0/1 are the destructor, 2..4 come from GameObject; an MSVC x86
// virtual destructor occupies a single slot):
//   [  0] 0x1afd68  Zombie::~Zombie
//   [  1] 0x1afda4  Zombie::~Zombie
//   [  2] 0x1b7b80  GameObject::BeginDraw
//   [  3] 0x1b7af0  GameObject::EndDraw
//   [  4] 0x1b7b1c  GameObject::MakeParentGraphicsFrame

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "GameObject.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnUser.h"
#include "Reanimation.h"
#include "ReanimatorTransform.h"
#include "SexyTransform2D.h"
#include "TodFoley.h"
#include "TodParticleSystem.h"

// Board, BuySunMoneyDialog, Challenge, CutScene, GameStats, GridItem, LawnApp,
// LawnMower and RevivalGameDialog are each reached only through a pointer here,
// or not named at all, so every one of those includes was pure cycle. Zombie.h is
// one of the busiest headers in the tree and it sat on chains like
// Board -> Challenge -> Coin -> MessageWidget -> LawnApp -> Zombie -> Board, so
// cutting all nine at once removes a large family of cycles.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
class BuySunMoneyDialog;
class Challenge;
class CutScene;
class GameStats;
class GridItem;
class LawnMower;
class RevivalGameDialog;
class LawnApp;

// Draw offsets filled by GetDrawPos @0x1A96DC and used by the draw helpers.
struct ZombieDrawPosition
{
    int mHeadX;
    int mHeadY;
    int mArmY;
    float mBodyY;
    float mImageOffsetX;
    float mImageOffsetY;
    float mClipHeight;
};

class Zombie : public GameObject
{
public:
    // ---- inherited from GameObject (+0x10..+0x33) -------------------------
    // mApp(+0x10) / mBoard(+0x14) / mX(+0x18) / mY(+0x1C) / mWidth(+0x20) /
    // mHeight(+0x24) / mVisible(+0x28) / mRow(+0x2C) / mRenderOrder(+0x30)
    // all live in Game\GameObject.h and are not redeclared here.
    // SyncObject's mSyncBlocks is at +0x04 (_M_start/+0x04, _M_finish/+0x08,
    // _M_end_of_storage/+0x0C).

    // ---- members of Zombie itself (+0x34..+0x183, 0x150 bytes) -------------
    ZombieType mZombieType;                           // +0x34 (13)
    ZombiePhase mZombiePhase;                         // +0x38 (14)
    float mPosX;                                      // +0x3C (15)
    float mPosY;                                      // +0x40 (16)
    float mVelX;                                      // +0x44 (17)
    int mAnimCounter;                                 // +0x48 (18)
    int mGroanCounter;                                // +0x4C (19)
    int mAnimTicksPerFrame;                           // +0x50 (20)
    int mAnimFrames;                                  // +0x54 (21)
    int mFrame;                                       // +0x58 (22)
    int mPrevFrame;                                   // +0x5C (23)
    bool mVariant;                                    // +0x60 (//96)
    bool mIsEating;                                   // +0x61 (//97)
    int mJustGotShotCounter;                          // +0x64 (25)
    int mShieldJustGotShotCounter;                    // +0x68 (26)
    int mShieldRecoilCounter;                         // +0x6C (27)
    int mZombieAge;                                   // +0x70 (28)
    // TODO(recon): the authoritative header types this as `ZombieHeight`, but this
    // tree's ReconTypes.h has no such enum (PvZ\Lawn\Common\ConstEnums.h does, yet
    // that header's enums clash with ReconTypes.h, so it cannot be included).
    // int for now; that matches every `_DWORD` read and write in the .cpp.
    int mZombieHeight;                                // +0x74 (29)
    int mPhaseCounter;                                // +0x78 (30)
    int mFromWave;                                    // +0x7C (31)
    bool mDroppedLoot;                                // +0x80 (//128)
    int mZombieFade;                                  // +0x84 (33)
    bool mFlatTires;                                  // +0x88 (//136)
    int mUseLadderCol;                                // +0x8C (35)
    int mTargetCol;                                   // +0x90 (36)
    float mAltitude;                                  // +0x94 (37)
    bool mHitUmbrella;                                // +0x98 (//152)
    Sexy::Rect mZombieRect;                           // +0x9C (39 ~ 42)
    Sexy::Rect mZombieAttackRect;                     // +0xAC (43 ~ 46)
    int mChilledCounter;                              // +0xBC (47)
    int mButteredCounter;                             // +0xC0 (48)
    int mIceTrapCounter;                              // +0xC4 (49)
    bool mMindControlled;                             // +0xC8 (//200)
    bool mBlowingAway;                                // +0xC9 (//201)
    bool mHasHead;                                    // +0xCA (//202)
    bool mHasArm;                                     // +0xCB (//203)
    bool mHasObject;                                  // +0xCC (//204)
    bool mInPool;                                     // +0xCD (//205)
    bool mOnHighGround;                               // +0xCE (//206)
    bool mYuckyFace;                                  // +0xCF (//207)
    int mYuckyFaceCounter;                            // +0xD0 (52)
    // TODO(recon): the authoritative header types this as `HelmType`; this tree's
    // ReconTypes.h defines no such enum, so int is used instead.
    int mHelmType;                                    // +0xD4 (53)
    int mBodyHealth;                                  // +0xD8 (54)
    int mBodyMaxHealth;                               // +0xDC (55)
    int mHelmHealth;                                  // +0xE0 (56)
    int mHelmMaxHealth;                               // +0xE4 (57)
    // TODO(recon): the authoritative header types this as `ShieldType`; this tree's
    // ReconTypes.h defines no such enum, so int is used instead.
    int mShieldType;                                  // +0xE8 (58)
    int mShieldHealth;                                // +0xEC (59)
    int mShieldMaxHealth;                             // +0xF0 (60)
    int mFlyingHealth;                                // +0xF4 (61)
    int mFlyingMaxHealth;                             // +0xF8 (62)
    bool mDead;                                       // +0xFC (//252)
    short mSunBeanDamageRemainder;                    // +0xFD  damage not yet converted to sun
    ZombieID mRelatedZombieID;                        // +0x100 (64)
    ZombieID mFollowerZombieID[4];                    // +0x104 (65 ~ 68) MAX_ZOMBIE_FOLLOWERS
    bool mPlayingSong;                                // +0x114 (//276)
    int mParticleOffsetX;                             // +0x118 (70)
    int mParticleOffsetY;                             // +0x11C (71)
    // TODO(recon): the authoritative header types this as `AttachmentID`; this tree's
    // ReconTypes.h has no such enum, so int. Only `&mAttachmentID` is ever taken.
    int mAttachmentID;                                // +0x120 (72)
    int mSummonCounter;                               // +0x124 (73)
    ReanimationID mBodyReanimID;                      // +0x128 (74)
    float mScaleZombie;                               // +0x12C (75)
    float mVelZ;                                      // +0x130 (76)
    float mOriginalAnimRate;                          // +0x134 (77)
    PlantID mTargetPlantID;                           // +0x138 (78)
    int mBossMode;                                    // +0x13C (79)
    int mTargetRow;                                   // +0x140 (80)
    int mBossBungeeCounter;                           // +0x144 (81)
    int mBossStompCounter;                            // +0x148 (82)
    int mBossHeadCounter;                             // +0x14C (83)
    ReanimationID mBossFireBallReanimID;              // +0x150 (84)
    ReanimationID mSpecialHeadReanimID;               // +0x154 (85)
    int mFireballRow;                                 // +0x158 (86)
    bool mIsFireBall;                                 // +0x15C (//348)
    ReanimationID mMoweredReanimID;                   // +0x160 (88)
    int mLastPortalX;                                 // +0x164 (89)
    bool mBloated;                                    // +0x168 (//360)
    int mZombieInitWord;                              // +0x16C (91) ZombieInitialize clears it
    bool mWalnutDeath;                                // +0x170 (92)
    // Words 93..96 are not four loose ints: UpdateZombieRect (0x1AA3EC) copies
    // mZombieRect (words 39..42) into them, GetZombieRect hands exactly these four
    // words back by value, and IsWalkingBackwards mirrors word 93 with
    // `4*93 = mX - 4*93 - 4*95`. That is a `Sexy::Rect`, and it still ends the
    // object at +0x184 exactly as before.
    Sexy::Rect mCachedZombieRect;                     // +0x174 (93 ~ 96)
    // size 0x184 (97 integers)

    // Both of these are STATIC. The disassembly is what proves it: the first
    // argument register is the Reanimation that every call is made on
    // (`MOV R4,R0` at 0x1A7F7E and then `MOV R0,R4` before each
    // Reanimation::AssignRenderGroupToPrefix at 0x1A7F8E onward), and
    // SetupDoorArms' null test is on R1, the *second* argument. No Zombie member
    // is ever touched. The mangled name cannot say this -- Itanium does not encode
    // static-ness -- but the ARMs' register use does; PreloadZombieResources below
    // is static for the same reason. Call sites written as `z->SetupX(...)` still
    // compile unchanged.
    static void SetupDoorArms(Reanimation * theReanim, bool theVisible);
    static void SetupReanimLayers(Reanimation * theReanim, ZombieType theType);
    int ReanimIgnoreClipRect(char const *theTrackName, bool theIgnore);
    int ReanimReenableClipping();
    bool IsBouncingPogo();
    int ZombieCatapultFire(Plant *thePlant);
    Plant * FindCatapultTarget();
    int IsTanglekelpTarget();
    void UpdateZombiePeaHead();
    void UpdateZombieGatlingHead();
    int NeedsMoreBackupDancers();
    int UpdateZombieHighGround();
    int OverrideParticleScale(TodParticleSystem *theParticleSystem);
    int OverrideParticleColor(TodParticleSystem *theParticleSystem);
    void UpdateZamboni();
    bool IsOnBoard();
    GridItem * UpdateClimbingLadder();
    int DoRevivalDialog(int theDialogId);
    int DoBuySunMoneyDialog(int theDialogId);
    bool HasYuckyFaceImage();
    // 0x1A8920. Early outs do nothing. The other paths hide or restore the
    // yucky-face tracks. No caller reads a return.
    void ShowYuckyFace(bool theShow);
    // The two halves of UpdateYuckyFace's body, split out because the ARM
    // reaches them from two different entry points in the same function.
    int UpdateYuckyFaceAt70();
    int StopYuckyFace();
    void UpdateAnaphylaxis();
    // The three draw helpers take the Graphics and the draw position; the
    // mangled names (`_ZN6Zombie14DrawZombieHeadEPN4Sexy8GraphicsER18ZombieDrawPositioni`
    // and its two siblings) carry both parameters, and Zombie::Draw passes them.
    // The bodies are empty in this build but the signatures are not.
    void DrawZombieHead(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos, int theRenderOrder);
    void DrawZombieWithParts(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos);
    void DrawZombie(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos);
    int IsWalkingBackwards();
    int DrawDancerReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos);
    int GetHelmDamageIndex();
    int GetBodyDamageIndex();
    int GetShieldDamageIndex();
    int DrawIceTrap(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos, int theIceTrapCounter);
    int CanTargetPlant(Plant *thePlant, ZombieAttackType theAttackType);
    int SquishAllInSquare(int theGridX, int theGridY, ZombieAttackType theAttackType);
    bool IsImmobilizied();
    bool ZombieNotWalking();
    int GetDancerFrame();
    int GetDancerPhase();
    int DrawZombiePart(Sexy::Graphics *g, Sexy::Image *theImage, int theFrame, int theRow, ZombieDrawPosition &theDrawPos);
    int IsMovingAtChilledSpeed();
    void ApplyAnimRate(float theAnimRate);
    void SetAnimRate(float theAnimRate);
    void PoolSplash(bool theIntoPool);
    bool IsOnHighGround();
    int UpdateZombieRiseFromGrave();
    bool UpdateZombieFalling();
    void GetDrawPos(ZombieDrawPosition &theDrawPos);
    Reanimation *UpdateReanim();
    int DrawBungeeTarget(Sexy::Graphics *g);
    int CheckForHighGround();
    int StartMindControlled();
    int BungeeDropPlant();
    void PlayZombieAppearSound();
    int StartZombieSound();
    int SetRow(int theRow);
    float GetPosYBasedOnRow(int theRow);
    int SummonBackupDancer(int theRow, int thePosX);
    int SummonBackupDancers();
    // 0x1AA3EC. Returns void: IDA types it `_DWORD *` only because r0 still holds
    // the receiver on the way out, and its one caller ignores the result.
    void UpdateZombieRect(bool theAttackRect);
    // 0x1AA48C. Returns `Sexy::Rect` BY VALUE. ARM passes a struct return in r0, so
    // IDA reads the signature as `Zombie *GetZombieRect(Zombie *this, _DWORD *theZombie)`
    // with `this` being the destination buffer; the transcription merged the two and
    // wrote the four rect words into object words 0..3, i.e. over the vptr and
    // mSyncBlocks. Every call site must now take the rect by value.
    Sexy::Rect GetZombieRect();
    float ZombieTargetLeadX(float theTime);
    Plant * IsStandingOnSpikeweed();
    bool TrySpawnLevelAward();
    int DropLoot();
    // 0x1AA9D4. Same sret story as GetZombieRect: the rect is returned by value,
    // built from mZombieAttackRect (words 43..46) and then adjusted.
    Sexy::Rect GetZombieAttackRect();
    Plant * FindPlantTarget(ZombieAttackType theAttackType);
    int AnimateChewEffect();
    GridItem * CheckSquish(ZombieAttackType theAttackType);
    int CheckForZombieStep();
    void AddAttachedParticle(int thePosX, int thePosY, ParticleEffect theEffect);
    Reanimation * AddAttachedReanim(int thePosX, int thePosY, ReanimationType theReanimType);
    int IsTangleKelpTarget();
    void BalloonPropellerHatSpin(bool theSpin);
    void ReanimShowPrefix(char const *theTrackPrefix, int theRenderGroup);
    void ShowDoorArms(bool theShow);
    void DropPole();
    void AttachShield();
    void ReanimShowTrack(char const *theTrackName, int theRenderGroup);
    int DoDaisies();
    bool HasShadow();
    int SetupDrawZombieWon(Sexy::Graphics *g);
    Reanimation * GetTrackPosition(char const *theTrackName, float &thePosX, float &thePosY);
    int DropFlag();
    int DrawButter(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos);
    int DrawBungeeCord(Sexy::Graphics *g, int theCordEndX, int theCordEndY);
    bool IsFlying();
    int DrawShadow(Sexy::Graphics *g);
    int GetBobsledPosition();
    int DrawBobsledReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos, bool theClip);
    bool IsBobsledTeamWithSled();
    bool CanLoseBodyParts();
    int DropArm(unsigned int theDamageFlags);
    // 0x1AC4B4. r0 is whatever the last call left (ZombieNotWalking, a bool, or
    // AddTodParticle). The one caller that keeps it recovers with (char *)(intptr_t).
    void UpdateZombieWalking();
    // 0x1AC760. Early outs return this; every other path returns UpdateZombieWalking's
    // r0. The only caller ignores it.
    void UpdateZombieWalkingIntoHouse();
    int DrawReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos, int theBaseRenderGroup);
    void DrawBungeeReanim(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos);
    int Draw(Sexy::Graphics *g);
    int IsDeadOrDying();
    int CountBungeesTargetingSunFlowers();
    // 0x1AD144. Every path either does nothing or calls ApplyAnimRate. The
    // callers that used to cast the leftover register to Reanimation* never
    // read that pointer.
    void UpdateAnimSpeed();
    Reanimation * PickRandomSpeed();
    Reanimation * PlayZombieReanim(char const *theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);
    Reanimation * BungeeDropZombie(Zombie *theZombie, int theGridX, int theGridY);
    // 0x1AD500. Early outs leave this in r0; the shoot and walk paths return
    // PlayZombieReanim's r0. The only caller ignores it.
    int UpdateZombieCatapult();
    // 0x1AD694. Mixed r0 (this, a curve, a phase, or a reanim). The only caller ignores it.
    int UpdateZombieBackupDancer();
    // 0x1AD770. Mixed r0. The only caller ignores it.
    int UpdateZombieDancer();
    Zombie * StartEating();
    // 0x1AF298 `_ZN6Zombie8EatPlantEP5Plant`. It was missing from this header
    // entirely, which made the .cpp definition a C2039 and turned every `this`
    // in its body into collateral C2355. Returns void: the dump's trailing value
    // is the leftover r0 of the last call.
    void EatPlant(Plant *thePlant);
    Reanimation * StartWalkAnim(int theBlendTime);
    // 0x1ADAEC. Mixed r0. The only caller ignores it.
    void UpdateZombieNewspaper();
    void UpdateZombiePool();
    void RiseFromGrave(int theGridX, int theGridY);
    void BobsledCrash();
    // 0x1ADF08. Every path returns this.
    Zombie * DropHelm(unsigned int theDamageFlags);
    int PogoBreak(unsigned int theDamageFlags);
    int LoadReanim(ReanimationType theReanimType);
    float UpdateZombieImp();
    Zombie * DetachShield();
    void BungeeStealTarget();
    void BungeeLiftTarget();
    // 0x1AE474. The early paths leave this in r0. The path that drops a
    // carried zombie tail-calls PlayZombieReanim. The one caller,
    // UpdateZombieBungee, discards the word, so the function returns nothing.
    void BungeeLanding();
    int ZombiquariumFindClosestBrain();
    float UpdateZombiePolevaulter();
    int UpdateZombieDolphinRider();
    // The shared tail of the three dolphin-jump arms of the above.
    int UpdateDolphinJumpLanding();
    // 0x1AEDD4. Returns this. The last path tail-calls UpdateAnimSpeed.
    Zombie * StopEating();
    int UpdateYuckyFace();
    int DragUnder();
    int DropShield(unsigned int theDamageFlags);
    int TakeShieldDamage(int theDamage, unsigned int theDamageFlags);
    int RemoveIceTrap();
    int RemoveButter();
    int RemoveColdEffects();
    // 0x1AF58C. Returns this. The add-on-head path returns that head reanimation.
    Zombie * DropHead(unsigned int theDamageFlags);
    // 0x1AF974. Returns this, or PickRandomSpeed's reanimation when it wakes.
    Reanimation * UpdateYeti();
    int StopZombieSound();
    void DiggerLoseAxe();
    Reanimation * UpdateZombieDigger();
    // Real polymorphism: the compiler writes the vptr (slots 0/1 = destructor,
    // overriding GameObject's virtual destructor).
    virtual ~Zombie();
    Reanimation * ConvertToNormalZombie();
    int CanBeChilled();
    bool CanBeFrozen();
    int ApplyButter();
    Reanimation * ApplyChill(bool theIsIceTrap);
    void CheckForPool();
    int TakeHelmDamage(int theDamage, unsigned int theDamageFlags);
    int EffectedByDamage(unsigned int theDamageFlags);
    int UpdateZombiePogo();
    Zombie * UpdateLadder();
    Zombie * FindZombieTarget();
    // 0x1B0734. r0 is this or TodAnimateCurve's int. Both callers ignore it.
    void UpdateZombieChimney();
    // 0x1B0778. r0 is whatever the last call left. The only caller ignores it.
    void WalkIntoHouse();
    Reanimation * BossPlayIdle();
    // 0x1B090C. The last call is DrawRenderGroup, and Board ignores the result.
    void DrawBossFireBall(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos);
    int DrawBossBackArm(Sexy::Graphics *g, ZombieDrawPosition &theDrawPos);
    void BossRVAttack();
    // 0x1B0A68. PlaySample is void; the only caller ignores r0.
    void BossRVLanding();
    void BossSpawnAttack();
    Zombie * BossSpawnContact();
    int BossCanStompRow(int theRow);
    void BossStompAttack();
    void BossStompContact();
    void BossBungeeAttack();
    bool BossAreBungeesDone();
    void BossHeadAttack();
    void BossHeadSpit();
    int BossDestroyIceballInRow(int theRow);
    int BossDestroyFireball();
    void BossHeadSpitEffect();
    void BossHeadSpitContact();
    // 0x1B1304. Reanimation::Update is void; the only caller ignores r0.
    void UpdateBossFireball();
    int BossDie();
    void DieNoLoot();
    int DieWithLoot();
    // 0x1B17CC. r0 is PlayZombieReanim, DieWithLoot, or aFlags<<27. Callers
    // that read it cast the int.
    int LandFlyer(unsigned int theDamageFlags);
    GridItem * UpdateZombieFlyer();
    int TakeFlyingDamage(int theDamage, unsigned int theDamageFlags);
    Reanimation * ZamboniDeath(unsigned int theDamageFlags);
    Reanimation * CatapultDeath(unsigned int theDamageFlags);
    Zombie * UpdateBurn();
    int UpdateMowered();
    Reanimation * BossBungeeLeave();
    int MowDown();
    float UpdateZombiePosition();
    void PickBungeeZombieTarget(int theTargetCol);
    int BossBungeeSpawn();
    void UpdateZombieBungee();
    // 0x1B212C. The idle paths return this. The pop path tail-calls
    // PlayZombieReanim and returns that reanimation.
    Reanimation * UpdateZombieJackInTheBox();
    void UpdateZombieJalapenoHead();
    void BobsledDie();
    void BungeeDie();
    void UpdateDeath();
    void PlayDeathAnim(unsigned int theDamageFlags);
    void UpdateDamageStates(unsigned int theDamageFlags);
    void AnimateChewSound();
    // 0x1B2CEC. Returns this.
    Zombie * Animate();
    int BossStartDeath();
    void BossSetupReanim();
    void DrawBossPart(Sexy::Graphics *g, BossPart thePart);
    // _ZN6Zombie22PreloadZombieResourcesE10ZombieType @0x1b300c -- no receiver
    // in the mangled name, so it is STATIC.
    static int PreloadZombieResources(ZombieType theZombieType);
    void ApplyBossSmokeParticles(bool theEnable);
    // 0x1B31FC. Returns this. TakeDamage forwards the word and its callers
    // discard it.
    Zombie * TakeBodyDamage(int theDamage, unsigned int theDamageFlags);
    int TakeDamage(int theDamage, unsigned int theDamageFlags);
    void UpdateZombieSnorkel();
    int UpdateZombieSquashHead();
    float UpdateZombieBobsled();
    int UpdateZombiquarium();
    Zombie * EatZombie(Zombie *theZombie);
    void CheckIfPreyCaught();
    Reanimation * HitIceTrap();
    void ApplyBurn();
    int BobsledBurn();
    // 0x1B45EC. The idle paths return this. The paths that start a walk, a
    // throw or a smash tail-call that animation and return the reanimation.
    Reanimation * UpdateZombieGargantuar();
    float UpdateActions();
    int UpdateBoss();
    int IsFireResistant();
    void EnableMustache(bool theEnable);
    void EnableFuture(bool theEnable);
    int EnableDanceMode(bool theEnable);
    // 0x1B5140 `_ZN6Zombie15SetupWaterTrackEPKc` -- the parameter is a track
    // name, not an int; the transcription's `int theDrawPos` was a mangled-name slip.
    void SetupWaterTrack(char const *theTrackName);
    Reanimation * LoadPlainZombieReanim();
    // 0x1B5230 `_ZN6Zombie16ZombieInitializeEi10ZombieTypebPS_ib`. It was missing
    // from this header entirely, which made the .cpp definition a C2039 and turned
    // every `this` in its 700-line body into collateral C2355. Returns void; the
    // transcribed `float` is the leftover r0 of the last store.
    void ZombieInitialize(int theRow, ZombieType theZombieType, bool theVariant, Zombie *theParentZombie, int theFromWave, bool theIsVisible);
    // The shared tail of ZombieInitialize: aquarium swim, Little Trouble scaling,
    // health caps, render order, appear sound, and the rare bloated normal.
    void FinishInitialize(ZombieType theType, int theFromWave, int theRenderLayer, int theRenderOffset);
    // 0x1B6B0C `_ZN6Zombie18SetupShieldReanimsE10ZombieTypeP11Reanimation`.
    // Static, like SetupDoorArms: the caller at 0x1C3418 loads the ZombieType into
    // R0 and the reanimation into R1, so there is no `this`.
    static void SetupShieldReanims(ZombieType theZombieType, Reanimation *theReanim);
    Zombie();
    void BuySunMoneyItem();
    void CheckForBoardEdge();
    int UpdatePlaying();
    int Update();
    void BuyRevivalItem();
};

struct ZombieSlot
{
    Zombie mItem;
    unsigned int mID;
};

// Layout anchor: any deviation in member type or order shows up here, not at runtime.
