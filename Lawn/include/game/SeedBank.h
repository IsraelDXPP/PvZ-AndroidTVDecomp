// SeedBank holds the seed packets for one side of the board, plus the seven
// shop packets drawn under the bank on levels that sell upgrades.
//
// GameObject ends at +0x34. The fields here follow it:
//   mNumPackets, mCutSceneDarken, mConveyorBeltCounter, mIsZombie,
//   mToolTipWidget, SeedPacket[10] at stride 0x74, ShopSeedPacket[7] at stride 0x54.
// Board allocates one of these per side. Draw, Move and the belt helpers are
// not virtual; the vtable is GameObject's.

#pragma once

#include "ReconTypes.h"
#include "HitResult.h"
#include "CutScene.h"
#include "GameObject.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "LawnUser.h"
#include "SeedPacket.h"
#include "SexyTransform2D.h"
#include "ShopSeedPacket.h"
#include "ToolTipWidget.h"

class Board;
class LawnApp;

class SeedBank : public GameObject
{
public:
    virtual ~SeedBank();

    explicit SeedBank(bool theIsZombie);

    int mNumPackets;
    int mCutSceneDarken;
    int mConveyorBeltCounter;
    bool mIsZombie;
    ToolTipWidget *mToolTipWidget;
    SeedPacket mSeedPackets[10];
    ShopSeedPacket mShopSeedPackets[7];

    int GetShopPacketPositionX(int theIndex);
    void InitShopSeedBank();
    bool MouseHitTest(int theX, int theY, HitResult *theHit);
    void Move(int theX, int theY);
    bool ContainsPoint(int theX, int theY);
    void RemoveSeed(int theIndex);
    int GetNumSeedsOnConveyorBelt();
    void AddSeed(SeedType theSeedType, bool thePlaceImmediately);
    int GetNumVisibleSeedsOnConveyorBelt();
    int CountOfTypeOnConveyorBelt(SeedType theSeedType);
    void UpdateConveyorBelt();
    void UpdateWidth();
    void RefreshAllPackets();
    void Draw(Sexy::Graphics *theGraphics);
};
