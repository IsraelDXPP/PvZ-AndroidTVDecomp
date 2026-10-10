// LawnMower -- reconstructed from libGameMain.so.
//
// Inherits `SyncObject` (0x10); the member table matches the PC reference source
// Lawn\LawnMower.h and the authoritative header PvZ\Lawn\Board\LawnMower.h field by
// field, except that every absolute offset is shifted by +0x10 (the PC version has
// no SyncObject base).
//
// Size **0x54**, not 0x58: the elements of `DataArray<LawnMower>` are
//     struct DataArrayItem { LawnMower mItem; uint32_t mID; };   // recon-src DataArray.h:39
// so the 88(0x58) stride of the array in Board is sizeof(LawnMower) + 4.
// Corroboration: Board.cpp:3313 `memset(v7, 0, 0x54u)` clears only the item, and
// Board.cpp:3321 `*(v7 + 84) = v11` (v11 = index | key<<16) writes mID.
//
// The class has no true virtual functions; its vtable holds two destructor slots:
//   [  0] 0x17c684  LawnMower::~LawnMower
//   [  1] 0x17c6a8  LawnMower::~LawnMower

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNMOWER_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNMOWER_H

#include "ReconTypes.h"
// The bodies need the complete types, so these includes stay for now (stage C
// narrows them to forward declarations).
#include "Graphics.h"
#include "Reanimation.h"
#include "ReanimatorCache.h"
#include "TodFoley.h"
#include "Zombie.h"

// Board and LawnApp are stored only as pointers (`Board *mBoard`,
// `LawnApp *mApp`) and CutScene is not named in this file at all, so all three
// includes were pure cycle. Board.h cannot be edited from its own side in this
// pass, so that edge is cut here. Sexy\ReconTypes.h already declares LawnApp.
class Board;
class CutScene;
class LawnApp;

class LawnMower : public SyncObject
{
public:
    LawnApp *mApp; // +0x10
    Board *mBoard; // +0x14
    float mPosX; // +0x18
    float mPosY; // +0x1C
    int mRenderOrder; // +0x20
    int mRow; // +0x24
    int mAnimTicksPerFrame; // +0x28
    ReanimationID mReanimID; // +0x2C
    int mChompCounter; // +0x30
    int mRollingInCounter; // +0x34
    int mSquishedCounter; // +0x38
    LawnMowerState mMowerState; // +0x3C
    bool mDead; // +0x40
    bool mVisible; // +0x41
    LawnMowerType mMowerType; // +0x44
    float mAltitude; // +0x48
    MowerHeight mMowerHeight; // +0x4C
    int mLastPortalX; // +0x50
    // size 0x54

    // UpdatePool, StartMower, Update and Die are `void` in the binary -- their
    // mangled names carry no return type, and the types below used to be whatever
    // the last call left in r0. The three that still return a value keep their
    // declared type because callers in other translation units assign it.
    void UpdatePool();
    void Draw(Sexy::Graphics *theGraphics);
    void StartMower();
    int MowZombie(Zombie *theZombie);
    // Returns the rect BY VALUE. ARM passes a struct return in r0, so IDA read the
    // signature as `LawnMower *GetLawnMowerAttackRect(LawnMower *this, _DWORD *a2)`
    // with `this` being the destination buffer; the transcription merged the two and
    // wrote the four rect words at object offset 0, over the vptr.
    Sexy::Rect GetLawnMowerAttackRect();
    void SquishMower();
    void EnableSuperMower(bool theEnable);
    void LawnMowerInitialize(int theRow);
    LawnMower();
    void Die();
    void Update();
};

struct LawnMowerSlot
{
    LawnMower mItem;
    unsigned int mID;
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_LAWNMOWER_H
