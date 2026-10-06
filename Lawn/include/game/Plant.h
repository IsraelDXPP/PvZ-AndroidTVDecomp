// Plant -- restored from libGameMain.so.
//
// Derives from `GameObject` (0x34 bytes: SyncObject's vptr + vector in 0x00..0x0F,
// then mApp..mRenderOrder). Evidence: slot [2] of the vtable comment block below is
// `0x1b7b80 GameObject::BeginDraw` -- Plant itself overrides none of
// BeginDraw/EndDraw/MakeParentGraphicsFrame, and its vtable only adds its own two
// destructor slots, so `class Plant : public GameObject` is the right shape.
//
// The member table comes from the authoritative header PvZ\Lawn\Board\Plant.h (the
// ordinal comment N there is the absolute offset 0x(N*4) here) and was checked
// member by member against the IDA type export libGameMain115.h:1119-1172; all of
// it agrees.
//
// Size **0x15C**: the element stride of `DataArray<Plant>` is 352 (0x160) (Board.cpp
// `mPlants`), and `DataArrayItem { T mItem; uint32_t mID; }` is 4 bytes larger than
// T (recon-src\PvZ\include\PvZ\TodLib\Common\DataArray.h:39), so sizeof(Plant) =
// 0x15C. WARNING: libGameMain115.h:1171 has a `__int16 mPlantIndexInList` after
// `int unk`; that is IDA merging the tail of the array element
// (`DataArrayItem::mID`) into the struct, **not** a Plant member. There is no Plant
// field at +0x15C, which is why this class ends at 0x15C.
//
// vtable order (same order as the virtual functions declared below):
//   [  0] 0x1518dc  Plant::~Plant
//   [  1] 0x1518fc  Plant::~Plant
//   [  2] 0x1b7b80  GameObject::BeginDraw
//   [  3] 0x1b7af0  GameObject::EndDraw
//   [  4] 0x1b7b1c  GameObject::MakeParentGraphicsFrame

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "game/Board.h"
#include "Challenge.h"
#include "Coin.h"
#include "GameObject.h"
#include "GameStats.h"
#include "Graphics.h"
#include "GridItem.h"
#include "Image.h"
#include "LawnApp.h"
#include "Projectile.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "ReanimatorCache.h"
#include "ReanimatorTransform.h"
#include "SexyAppBase.h"
#include "TodParticleSystem.h"
#include "Zombie.h"

// LawnApp is reached only through `LawnApp *mApp`, so it is declared rather than
// included. Every one of the eight headers above reaches LawnApp.h sooner or
// later, and LawnApp.h reaches Board.h and Challenge.h, so this edge put Plant.h
// on a very large number of cycles. Sexy\ReconTypes.h already declares LawnApp.
class LawnApp;

// Number of magnet slots (authoritative header Plant.h:29). The `memset(..., 0,
// 0x64)` that the constructor and PlantInitialize perform on `mMagnetItems` is
// 100 = 5 * sizeof(MagnetItem), which is the proof.
inline constexpr int MAX_MAGNET_ITEMS = 5;

// MagnetItemType -- a real enum. Values taken from the authoritative header
// PvZ\Lawn\Board\Plant.h:131 and identical to libGameMain115.h:1076
// (Pail1=1 .. GoldCoin=0x13, PickAxe=0x15); the IDA export is missing 0x14, which
// the authoritative header supplies as `MAGNET_ITEM_DIAMOND = 20`.
enum MagnetItemType {
    MAGNET_ITEM_NONE = 0,
    MAGNET_ITEM_PAIL_1 = 1,
    MAGNET_ITEM_PAIL_2 = 2,
    MAGNET_ITEM_PAIL_3 = 3,
    MAGNET_ITEM_FOOTBALL_HELMET_1 = 4,
    MAGNET_ITEM_FOOTBALL_HELMET_2 = 5,
    MAGNET_ITEM_FOOTBALL_HELMET_3 = 6,
    MAGNET_ITEM_DOOR_1 = 7,
    MAGNET_ITEM_DOOR_2 = 8,
    MAGNET_ITEM_DOOR_3 = 9,
    // MAGNET_ITEM_PROPELLER,
    MAGNET_ITEM_POGO_1 = 10,
    MAGNET_ITEM_POGO_2 = 11,
    MAGNET_ITEM_POGO_3 = 12,
    MAGNET_ITEM_JACK_IN_THE_BOX = 13,
    MAGNET_ITEM_LADDER_1 = 14,
    MAGNET_ITEM_LADDER_2 = 15,
    MAGNET_ITEM_LADDER_3 = 16,
    MAGNET_ITEM_LADDER_PLACED = 17,
    MAGNET_ITEM_SILVER_COIN = 18,
    MAGNET_ITEM_GOLD_COIN = 19,
    MAGNET_ITEM_DIAMOND = 20,
    MAGNET_ITEM_PICK_AXE = 21,
};

// MagnetItem -- 20 bytes, identical in the authoritative header Plant.h:157 and in
// libGameMain115.h:1101.
class MagnetItem
{
public:
    float          mPosX;         // +0x00
    float          mPosY;         // +0x04
    float          mDestOffsetX;  // +0x08
    float          mDestOffsetY;  // +0x0C
    MagnetItemType mItemType;     // +0x10
};

// PlantOnBungeeState -- authoritative header Plant.h:41, identical to
// libGameMain115.h:1111.
enum PlantOnBungeeState {
    NOT_ON_BUNGEE = 0,
    GETTING_GRABBED_BY_BUNGEE = 1,
    RISING_WITH_BUNGEE = 2,
};

// PlantState -- values from the authoritative header Plant.h:47, cross-checked
// against libGameMain115.h:1022: Notready=0 .. CobcannonReady=0x25(37) ..
// LilypadInvulnerable=0x30(48) all agree. The trailing STATE_CELERY_STALKER_* block
// in the authoritative header is a mod extension that this .so does not have, but
// keeping it does not disturb the values 0..48 (it is needed when porting PvZ-src).
enum PlantState {
    STATE_NOTREADY = 0,
    STATE_READY = 1,
    STATE_DOINGSPECIAL = 2,
    STATE_SQUASH_LOOK = 3,
    STATE_SQUASH_PRE_LAUNCH = 4,
    STATE_SQUASH_RISING = 5,
    STATE_SQUASH_FALLING = 6,
    STATE_SQUASH_DONE_FALLING = 7,
    STATE_GRAVEBUSTER_LANDING = 8,
    STATE_GRAVEBUSTER_EATING = 9,
    STATE_CHOMPER_BITING = 10,
    STATE_CHOMPER_BITING_GOT_ONE = 11,
    STATE_CHOMPER_BITING_MISSED = 12,
    STATE_CHOMPER_DIGESTING = 13,
    STATE_CHOMPER_SWALLOWING = 14,
    STATE_POTATO_RISING = 15,
    STATE_POTATO_ARMED = 16,
    STATE_POTATO_MASHED = 17,
    STATE_SPIKEWEED_ATTACKING = 18,
    STATE_SPIKEWEED_ATTACKING_2 = 19,
    STATE_SCAREDYSHROOM_LOWERING = 20,
    STATE_SCAREDYSHROOM_SCARED = 21,
    STATE_SCAREDYSHROOM_RAISING = 22,
    STATE_SUNSHROOM_SMALL = 23,
    STATE_SUNSHROOM_GROWING = 24,
    STATE_SUNSHROOM_BIG = 25,
    STATE_MAGNETSHROOM_SUCKING = 26,
    STATE_MAGNETSHROOM_CHARGING = 27,
    STATE_BOWLING_UP = 28,
    STATE_BOWLING_DOWN = 29,
    STATE_CACTUS_LOW = 30,
    STATE_CACTUS_RISING = 31,
    STATE_CACTUS_HIGH = 32,
    STATE_CACTUS_LOWERING = 33,
    STATE_TANGLEKELP_GRABBING = 34,
    STATE_COBCANNON_ARMING = 35,
    STATE_COBCANNON_LOADING = 36,
    STATE_COBCANNON_READY = 37,
    STATE_COBCANNON_FIRING = 38,
    STATE_KERNELPULT_BUTTER = 39,
    STATE_UMBRELLA_TRIGGERED = 40,
    STATE_UMBRELLA_REFLECTING = 41,
    STATE_IMITATER_MORPHING = 42,
    STATE_ZEN_GARDEN_WATERED = 43,
    STATE_ZEN_GARDEN_NEEDY = 44,
    STATE_ZEN_GARDEN_HAPPY = 45,
    STATE_MARIGOLD_ENDING = 46,
    STATE_FLOWERPOT_INVULNERABLE = 47,
    STATE_LILYPAD_INVULNERABLE = 48,
    STATE_CELERY_STALKER_LOW = 49,
    STATE_CELERY_STALKER_RISING = 50,
    STATE_CELERY_STALKER_HIGH = 51,
    STATE_CELERY_STALKER_LOWERING = 52,
    STATE_CELERY_STALKER_ATTACKING = 53,
    STATE_CELERY_STALKER_PUNCHING = 54,
    STATE_CELERY_STALKER_STOPPING = 55,
    STATE_BLOOMERANG_CATCHING = 56,
    STATE_BONK_CHOY_PUNCHING = 57,
    STATE_BONK_CHOY_UPPERCUTTING = 58,
    STATE_ENDURIAN_STARTING = 59,
    STATE_ENDURIAN_ATTACKING = 60,
    STATE_ENDURIAN_STOPPING = 61,
};

class Plant : public GameObject
{
public:
    // Every offset below is absolute (GameObject occupies 0x00..0x33).
    SeedType    mSeedType;                  // +0x34
    int         mPlantCol;                  // +0x38
    int         mAnimCounter;               // +0x3C
    int         mFrame;                     // +0x40
    int         mFrameLength;               // +0x44
    int         mNumFrames;                 // +0x48
    PlantState  mState;                     // +0x4C
    int         mPlantHealth;               // +0x50
    int         mPlantMaxHealth;            // +0x54
    int         mSubclass;                  // +0x58
    int         mDisappearCountdown;        // +0x5C
    int         mDoSpecialCountdown;        // +0x60
    int         mStateCountdown;            // +0x64
    int         mLaunchCounter;             // +0x68
    int         mLaunchRate;                // +0x6C
    Sexy::Rect  mPlantRect;                 // +0x70 ~ +0x7F
    Sexy::Rect  mPlantAttackRect;           // +0x80 ~ +0x8F
    int         mTargetX;                   // +0x90
    int         mTargetY;                   // +0x94
    int         mStartRow;                  // +0x98
    // The authoritative header Plant.h:192 and the IDA export libGameMain115.h:1141
    // both spell this `int *mParticleID`, but in the recon body the slot only ever
    // carries a value: `ParticleTryToGet(mParticleID)`, the return value of
    // `ParticleGetID(...)` stored back, and `= 0`. The PC reference source
    // (Lawn\Plant.h) calls it `ParticleSystemID` as well, so it is declared here as
    // ParticleSystemID (same 4 bytes, identical layout).
    ParticleSystemID mParticleID;           // +0x9C
    int         mShootingCounter;           // +0xA0
    ReanimationID mBodyReanimID;            // +0xA4
    ReanimationID mHeadReanimID;            // +0xA8
    ReanimationID mHeadReanimID2;           // +0xAC
    ReanimationID mHeadReanimID3;           // +0xB0
    ReanimationID mBlinkReanimID;           // +0xB4
    ReanimationID mLightReanimID;           // +0xB8
    ReanimationID mSleepingReanimID;        // +0xBC
    int         mBlinkCountdown;            // +0xC0
    int         mRecentlyEatenCountdown;    // +0xC4
    int         mEatenFlashCountdown;       // +0xC8
    int         mBeghouledFlashCountdown;   // +0xCC
    float       mShakeOffsetX;              // +0xD0
    float       mShakeOffsetY;              // +0xD4
    MagnetItem  mMagnetItems[MAX_MAGNET_ITEMS]; // +0xD8 ~ +0x13B
    ZombieID    mTargetZombieID;            // +0x13C
    int         mWakeUpCounter;             // +0x140
    PlantOnBungeeState mOnBungeeState;      // +0x144
    SeedType    mImitaterType;              // +0x148
    int         mPottedPlantIndex;          // +0x14C
    bool        mAnimPing;                  // +0x150
    bool        mDead;                      // +0x151
    bool        mSquished;                  // +0x152
    bool        mIsAsleep;                  // +0x153
    bool        mIsOnBoard;                 // +0x154
    bool        mHighlighted;               // +0x155
    bool        mInFlowerPot;               // +0x156
    bool        mGloveGrabbed;              // +0x157
    // Player who planted this. -1 is none. DoSpecial checks 0 and 1 for the
    // cherry-bomb achievement, and ImitaterMorph forwards it to AddPlant.
    int         mPlayerIndex;               // +0x158
    // size 0x15C

    virtual ~Plant();
    int GetDamageRangeFlags(PlantWeapon theWeapon);
    bool IsOnHighGround();
    TodParticleSystem * AddAttachedParticle(int thePosX, int thePosY, int theRenderOrder, ParticleEffect theEffect);
    int FindStarFruitTarget();
    int StarFruitFire();
    bool MakesSun();
    void PlayBodyReanim(char const* theTrackName, ReanimLoopType theLoopType, int theBlendTime, float theAnimRate);
    int LaunchStarFruit();
    int SpikeweedAttack();
    void UpdateDoomShroom();
    int UpdateIceShroom();
    int UpdateFlowerPot();
    int UpdateLilypad();
    MagnetItem * GetFreeMagnetItem();
    float MagnetShroomAttactItem(Zombie * theZombie);
    int DrawMagnetItemsOnTop();
    float UpdateMagnetShroom();
    Coin * FindGoldMagnetTarget();
    int GoldMagnetFindTargets();
    int RemoveEffects();
    int IsUpgradableTo(SeedType theSeedType);
    int IsPartOfUpgradableTo(SeedType theSeedType);
    int IsOnBoard();
    int UpdateReanimColor();
    int IsInPlay();
    int UpdateProductionPlant();
    int UpdateSunShroom();
    int NotOnGround();
    int IsAGoldMagnetAboutToSuck();
    Reanimation * AttachBlinkAnim(Reanimation * theReanim);
    void EndBlink();
    void SetSleeping(bool theIsAsleep);
    Reanimation * DoBlink();
    Reanimation * UpdateBlink();
    Reanimation * AnimateNuts();
    void AnimateGarlic();
    void AnimatePumpkin();
    void UpdateNeedsFood();
    void Animate();
    int GetPeaHeadOffset(int &theOffsetX, int &theOffsetY);
    void MouseDown(int x, int y, int theClickCount);
    Zombie * IceZombies();
    Zombie * BurnRow(int theRow);
    int BlowAwayFliers(int theX, int theRow);
    int CobCannonFire(int theTargetX, int theTargetRow);
    // _ZN5Plant18FindTargetGridItemEi11PlantWeapon @0x16bb70 -- one explicit
    // parameter. The entry is `LDR R3,[R0,#0x10] / MOV R7,R1`: R0 is the plant,
    // R1 is the row. The old `int a1` was the plant itself, which is why every
    // call site passed `this` a second time.
    GridItem * FindTargetGridItem(int theRow);
    // _ZN5Plant8GetImage8SeedType @0x16bbf0 -- no receiver in the mangled name;
    // the disassembly is `BL GetPlantDefinition` then `LDR R0,[R0,#4]`, with R0
    // carrying the SeedType. STATIC.
    // The body returns the Image* stored at plant-definition +4, or that word
    // dereferenced when it is not null. Both callers treat the result as an Image*.
    static Sexy::Image * GetImage(SeedType theSeedType);
    // DrawSeedType is STATIC in the binary: the mangled name
    // `_ZN5Plant12DrawSeedTypeEPN4Sexy8GraphicsE8SeedTypeS3_13DrawVariationff`
    // carries no Plant* receiver -- the first argument is the Graphics. The PC
    // source declares it static for the same reason
    // (LawnProject/src/Lawn/Plant.h:243). Declaring it static is what lets the
    // transcribed call sites `Plant::DrawSeedType(g, seed, ...)` compile as
    // written.
    static int DrawSeedType(Sexy::Graphics *g, SeedType theSeedType, SeedType theImitaterType, DrawVariation theDrawVariation, float thePosX, float thePosY);
    // _ZN5Plant7GetCostE8SeedTypeS0_ -- no receiver in the mangled name, so it
    // is STATIC like the other seed-table queries below.
    static int GetCost(SeedType theSeedType, SeedType theImitaterType);
    // Both are STATIC and return a string BY VALUE in the binary:
    //   _ZN5Plant13GetNameStringE8SeedTypeS0_  -- no receiver, r0 is the sret slot
    //   _ZN5Plant10GetToolTipE8SeedType        -- same shape
    // The PC source agrees (Lawn/Plant.h declares them static).
    static Sexy::String GetNameString(SeedType theSeedType, SeedType theImitaterType);
    static Sexy::String GetToolTip(SeedType theSeedType);
    // Also static in the binary: the body takes no `this`.
    static int GetRefreshTime(SeedType theSeedType, SeedType theImitaterType);
    // These three are static for the same reason -- `_recon/ida/Plant.cpp`
    // shows `bool __fastcall Plant::IsNocturnal(unsigned int a1)` with a body
    // that only switches on `a1` -- and the call sites write them as
    // `Plant::IsNocturnal(seed)`.
    static bool IsNocturnal(SeedType theSeedType);
    static bool IsAquatic(SeedType theSeedType);
    static bool IsFlying(SeedType theSeedType);
    // IsUpgrade and the other seed-table queries are static as well.
    static bool IsUpgrade(SeedType theSeedType);
    int CalcRenderOrder();
    int PlantInitialize(int theGridX, int theGridY, SeedType theSeedType, SeedType theImitaterType, int thePlayerIndex);
    int FinishPlantInitialize();
    float DrawMagnetItems(Sexy::Graphics *g);
    int UpdateReanim();
    void DrawShadow(Sexy::Graphics *g, float theOffsetX, float theOffsetY);
    int Die();
    int SpikeRockTakeDamage();
    TodParticleSystem * UpdateGraveBuster();
    Plant * UpdateCoffeeBean();
    int KillAllPlantsNearDoom();
    int DoSpecial();
    Reanimation * UpdateBlover();
    int Squish();
    int ImitaterMorph();
    Plant * UpdateImitater();
    float Draw(Sexy::Graphics *g);
    bool IsSpiky();
    // Both return a 16-byte Rect BY VALUE, so on this target the result buffer
    // arrives in r0 and the real object in r1:
    //   _ZN5Plant12GetPlantRectEv                   @0x16e818  -> (sret, this)
    //   _ZN5Plant18GetPlantAttackRectE11PlantWeapon @0x16e890  -> (sret, this, weapon)
    //
    // Now declared by value, like GridItem::GetItemRect. The old `_DWORD *` /
    // `int *` forms made the body write the four rect words over the object's
    // vptr, and forced every call site into "the receiver is also the result
    // buffer". The call sites were already converted to
    // `Sexy::Rect aRect = aPlant->GetPlantRect();`, so this is what unblocks
    // Game/Projectile.cpp.
    Sexy::Rect GetPlantRect();
    Sexy::Rect GetPlantAttackRect(PlantWeapon theWeapon);
    GridItem * DoRowAreaDamage(int theDamage, unsigned int theDamageFlags);
    float Fire(Zombie *theTargetZombie, int theRow, PlantWeapon theWeapon, GridItem *theTargetGridItem);
    int UpdateTorchwood();
    int DoSquashDamage();
    Zombie * FindSquashTarget();
    Zombie * UpdateSquash();
    Zombie * FindTargetZombie(int theRow, PlantWeapon theWeapon);
    int FindTargetAndFire(int theRow, PlantWeapon theWeapon);
    GridItem * LaunchThreepeater();
    int UpdateShooter();
    int * UpdateTanglekelp();
    Zombie * UpdateBowling();
    int DistanceToClosestZombie();
    int UpdatePotato();
    // _ZN5Plant21PreloadPlantResourcesE8SeedType @0x1704dc -- no receiver in the
    // mangled name, so it is STATIC.
    static int PreloadPlantResources(SeedType theSeedType);
    int PlayIdleAnim(float theRate);
    int UpdateScaredyShroom();
    Plant * UpdateUmbrella();
    int UpdateCobCannon();
    float UpdateGoldMagnetShroom();
    Zombie * UpdateSpikeweed();
    Zombie * UpdateCactus();
    Zombie * UpdateChomper();
    float UpdateShooting();
    float UpdateAbilities();
    int Update();
    Plant();
};

struct PlantSlot
{
    Plant mItem;
    unsigned int mID;
};

