// Coin -- reconstructed from libGameMain.so.
//
// Real inheritance: `class Coin : public GameObject` (GameObject : SyncObject),
// so the base class owns +0x00..+0x33 and Coin's own members start at +0x34.
// Object size is 0xC8, which the authority header's trailing comment states as
// "// 50 ints" and which is corroborated by Board.cpp: the DataArray<Coin> step
// there is 204 (0xCC), and `struct DataArrayItem { T mItem; uint32_t mID; }`
// makes that sizeof(Coin) + 4.
//
// vtable order as it exists in the binary. Virtuals must be declared in this
// order so the compiler reproduces the layout exactly:
//   [  0] 0x1963b4  Coin::~Coin                            (D1)
//   [  1] 0x1963f8  Coin::~Coin                            (D0)
//   [  2] 0x1b7b80  GameObject::BeginDraw
//   [  3] 0x1b7af0  GameObject::EndDraw
//   [  4] 0x1b7b1c  GameObject::MakeParentGraphicsFrame
// Slots 2/3/4 hold GameObject's own implementations, so this class does not
// override them. (PvZ\Lawn\Board\Coin.h declares BeginDraw/EndDraw as well, but
// that is its hook build intercepting the virtual calls; this reconstruction has
// no hooks, and overriding them here would point those slots at different code.)
// Update/Draw/Die/MouseDown are not virtual in the binary.
//
// Member names, types and order come from PvZ\Lawn\Board\Coin.h (authority,
// member ordinals 13..48). Every absolute offset below was checked against the
// raw `(char*)this + N` accesses of _recon/ida/Coin.cpp, which the bodies in
// Coin.cpp used to spell out one by one.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_COIN_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_COIN_H

#include "ReconTypes.h"
// These headers are included because the bodies in Coin.cpp need the complete
// types they call methods on. They are kept as-is so no other file's transitive
// dependencies change.
#include "BaseGamepadControls.h"
#include "GameObject.h"
#include "HitResult.h"
#include "Graphics.h"
#include "Image.h"
#include "MessageWidget.h"
#include "PottedPlant.h"
#include "Reanimation.h"
#include "SeedPacket.h"
#include "SexyAppBase.h"

// Board, CutScene, GamepadControls, LawnApp, LawnPlayerInfo and ZenGarden were
// each half of an include cycle with this file and none of them is named in it:
// LawnPlayerInfo.h in particular closed
// LawnPlayerInfo.h -> GameStats.h -> SeedBank.h -> CutScene.h -> Challenge.h ->
// Coin.h -> LawnPlayerInfo.h. Board and LawnApp are kept as declarations; the
// rest are not referenced at all, so nothing replaces them.
// Sexy\ReconTypes.h already declares LawnApp.
class Board;
class LawnPlayerInfo;
class LawnApp;

class Coin : public GameObject
{
public:
    virtual ~Coin();

    // ---- derived members, +0x34..+0xC7 -------------------------------------
    float mPosX; // +0x34
    float mPosY; // +0x38
    float mPrevPosX; // +0x3C
    float mPrevPosY; // +0x40
    float mVelX; // +0x44
    float mVelY; // +0x48
    float mScale; // +0x4C
    bool mDead; // +0x50
    int mFadeCount; // +0x54
    float mCollectX; // +0x58
    float mCollectY; // +0x5C
    int mGroundY; // +0x60
    int mCoinAge; // +0x64
    bool mIsBeingCollected; // +0x68
    int mCollectedByPlayerIndex; // +0x6C
    int mDisappearCounter; // +0x70
    // This tree's ReconTypes.h only has `typedef int CoinType`, so the field is
    // a plain int here rather than the authority header's enum. The constants
    // Coin.cpp compares it against are, from PvZ\Lawn\Common\ConstEnums.h:186 --
    //     1 COIN_SILVER  2 COIN_GOLD  3 COIN_DIAMOND
    //     4 COIN_SUN  5 COIN_SMALLSUN  6 COIN_LARGESUN
    //     7 COIN_FINAL_SEED_PACKET  8 COIN_TROPHY  9 COIN_SHOVEL
    //     10 COIN_ALMANAC  11 COIN_CARKEYS  12 COIN_VASE
    //     13 COIN_WATERING_CAN  14 COIN_TACO  15 COIN_NOTE
    //     16 COIN_USABLE_SEED_PACKET  17 COIN_PRESENT_PLANT
    //     18 COIN_AWARD_MONEY_BAG  19 COIN_AWARD_PRESENT
    //     20 COIN_AWARD_BAG_DIAMOND  21 COIN_AWARD_SILVER_SUNFLOWER
    //     22 COIN_AWARD_GOLD_SUNFLOWER  23 COIN_CHOCOLATE
    //     24 COIN_AWARD_CHOCOLATE  25 COIN_PRESENT_MINIGAMES
    //     26 COIN_PRESENT_PUZZLE_MODE  27 Present32  28 Present1024
    //     29 COIN_COOP_DOUBLE_SUN  30 COIN_VS_ZOMBIE_BRAIN
    //     31 COIN_VS_PLANT_TROPHY  32 COIN_VS_ZOMBIE_TROPHY
    //     33 COIN_MINISUN  34 COIN_SMALL_VS_ZOMBIE_BRAIN
    //     35 COIN_LARGE_VS_ZOMBIE_BRAIN
    CoinType mType; // +0x74
    // CoinMotion is a one-int proxy in ReconTypes.h, not an enum, so the
    // motion constants stay numeric too (PvZ\Lawn\Common\ConstEnums.h:174:
    // 0 FROM_SKY, 1 FROM_SKY_SLOW, 2 FROM_PLANT, 3 COIN, 4 LAWNMOWER_COIN,
    // 5 FROM_PRESENT, 6 FROM_BOSS, 7 FROM_NEAR_CURSOR, 8 FROM_FROM_VS_WON,
    // 9 FROM_GRAVE_STONE).
    CoinMotion mCoinMotion; // +0x78
    // The authority header types this `AttachmentID[3]`; this tree has no
    // AttachmentID, and every use is a word-sized handle that is only stored,
    // compared against 0 or passed to the Attachment* helpers, so a word is
    // enough. Same choice as Zombie.h's mAttachmentID.
    int mAttachmentID[3]; // +0x7C, +0x80, +0x84
    float mCollectionDistance; // +0x88
    SeedType mUsableSeedType; // +0x8C
    PottedPlant mPottedPlantSpec; // +0x90..+0xA3 (0x14 bytes)
    bool mNeedsBouncyArrow; // +0xA4
    bool mHasBouncyArrow; // +0xA5
    bool mHitGround; // +0xA6
    int mTimesDropped; // +0xA8
    int mPlayerIndex; // +0xAC
    // Speed of a coin flying toward the gamepad cursor (COIN_MOTION_FROM_NEAR_CURSOR).
    float mCursorChaseSpeed; // +0xB0
    // Either half of a coop double sun is currently highlighted by a cursor.
    bool mDoubleSunGlowing; // +0xB4
    int mCustomSunValue; // +0xB8
    bool mAutoCollect; // +0xBC
    int mAutoCollectAge; // +0xC0
    // Per-half highlight of the coop double sun (attachment 0, then attachment 1).
    bool mDoubleSunHighlight0; // +0xC4
    bool mDoubleSunHighlight1; // +0xC5

    bool IsMoney();
    bool IsSun();
    bool IsDoubleSun();
    bool IsDeath();
    bool IsPresentWithAdvice();
    // The authority header declares `Sexy::Color GetColor();`. The binary returns
    // the colour through the hidden return pointer (r0), so IDA prints the
    // destination as `this` and the receiver as the second argument -- which is
    // why the transcribed body wrote the four colour words over the object's
    // vptr. Coin.cpp now uses the real by-value form.
    Sexy::Color GetColor();
    int GetFinalSeedPacketType();
    // Every method below returns nothing in the binary: the ARM epilogue leaves
    // r0 untouched, so the transcribed `int`/`char *` return types were only the
    // register the decompiler happened to see. No call site anywhere in the tree
    // uses the value, so they are declared as the binary has them.
    void FanOutCoins(CoinType theCoinType, int theNumCoins);
    void StartFade();
    void SetAutoCollect(bool theAutoCollect, int theAutoCollectAge);
    void SetSunValue(int theSunValue);
    int GetSunValue();
    float GetSunScale();
    // _ZN4Coin12GetCoinValueE8CoinType -- no receiver in the mangled name, so
    // it is STATIC; call sites are written `Coin::GetCoinValue(type)`.
    static int GetCoinValue(CoinType result);
    void PlayLaunchSound();
    void PlayCollectSound();
    void Die();
    void ScoreCoin();
    // A hit writes the coin into HitResult::mObject and the hit kind into
    // mObjectType. Board::MouseHitTest passes a HitResult* and tests the bool.
    bool MouseHitTest(int theX, int theY, HitResult *theHitResult, int thePlayerIndex);
    bool IsLevelAward();
    void UpdateFade();
    void PlayGroundSound();
    void UpdateCollected();
    void Collect(int thePlayerIndex);
    void TryAutoCollectAfterLevelAward();
    void MouseDown(int theX, int theY, int theClickCount);
    void GamepadCursorOver(int thePlayerIndex);
    bool CoinGetsBouncyArrow();
    void CoinInitialize(int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion);
    int GetDisappearTime();
    void Draw(Sexy::Graphics *g);
    void UpdateFall();
    void Update();
    void DroppedUsableSeed();
    Coin();
};

struct CoinSlot
{
    Coin mItem;
    unsigned int mID;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_COIN_H
