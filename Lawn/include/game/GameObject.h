// GameObject -- the base class of every drawable game object (plants, zombies,
// projectiles, coins, lawn mowers and the rest all derive from it).
//
// The layout is proven by the binary (GameObject::GameObject @0x1B7BB4):
//   `ADD.W R7, R0, #0x34`          -- the object ends at +0x34
//   `memset(this+0x18, 0, 0x1C)`   -- +0x18..+0x33 is the state area, 7 words
//   `STR R2,[R4,#0x30]` / `STR R0,[R4,#0x2C]` / `STRB.W R6,[R4,#0x28]`
//   `STR R3,[R4,#0x10]` / `STR R2,[R4,#0x14]`
// It matches the member order of the PC reference source Lawn\GameObject.cpp
// word for word.
//
// vtable order -- declaring the virtuals in this order makes the compiler emit
// the same table the binary holds. The order matters: the recon layer used to
// reach these through a computed slot index, and an index is only correct while
// the order is:
//   [  0] 0x1b7b48  GameObject::~GameObject              (D1)
//   [  1] 0x1b7b6c  GameObject::~GameObject              (D0)
//   [  2] 0x1b7b80  GameObject::BeginDraw
//   [  3] 0x1b7af0  GameObject::EndDraw
//   [  4] 0x1b7b1c  GameObject::MakeParentGraphicsFrame

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_GAMEOBJECT_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_GAMEOBJECT_H

#include "ReconTypes.h"
#include "SyncObject.h"

class LawnApp;
class Board;

class GameObject : public SyncObject
{
public:
    virtual ~GameObject() = default;
    virtual bool BeginDraw(Sexy::Graphics *theGraphics);
    virtual void EndDraw(Sexy::Graphics *theGraphics);
    virtual void MakeParentGraphicsFrame(Sexy::Graphics *theGraphics);

    LawnApp *mApp; // +0x10
    Board *mBoard; // +0x14
    int mX; // +0x18
    int mY; // +0x1C
    int mWidth; // +0x20
    int mHeight; // +0x24
    bool mVisible; // +0x28
    int mRow; // +0x2C
    int mRenderOrder; // +0x30
    // Total size 0x34 (13 words).

    GameObject();
};

// Layout check: 0x10 of SyncObject plus the 9 members above.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_GAMEOBJECT_H
