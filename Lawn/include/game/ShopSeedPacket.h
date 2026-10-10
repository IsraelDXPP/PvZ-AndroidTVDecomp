#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_SHOPSEEDPACKET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_SHOPSEEDPACKET_H

#include "ReconTypes.h"
#include "GameObject.h"
#include "Graphics.h"
#include "LawnUser.h"
#include "SexyTransform2D.h"

class LawnApp;
class SeedBank;

class ShopSeedPacket : public GameObject
{
public:
    virtual ~ShopSeedPacket();

    ShopSeedPacket();

    int mStoreItemType;
    int mPacketOffsetX;
    SeedBank *mSeedBank;
    float mSelectAnimCounter;
    float mSelectArrowAnimTime;
    float mSelectScale;
    SeedType mSeedType;
    bool mSelectedPlayerIndex;
    bool mIsSelected;
    bool mShowBothPlayerCursors;

    void UpdateSelected();
    void Update();
    void DrawMiddle();
    void DrawOverlay(Sexy::Graphics *g);
    void DrawBackground(Sexy::Graphics *g);
    void Draw(Sexy::Graphics *g);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_SHOPSEEDPACKET_H
