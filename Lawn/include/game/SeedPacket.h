#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_SEEDPACKET_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_SEEDPACKET_H

#include "GameObject.h"
#include "ReconTypes.h"

class Board;
class Challenge;
class HitResult;
class LawnApp;
class SeedBank;

// The binary's mSlotMachineCountDown start value; SeedPacket::Update counts it
// down from here.
inline constexpr int SLOT_MACHINE_TIME = 400;
inline constexpr int CONVEYOR_SPEED = 4;

// Selection scale animation: 1.0 grows to 1.2 over 0.15s and holds.
inline constexpr float SEED_PACKET_SELECT_TIME = 0.15f;
inline constexpr float SEED_PACKET_SELECT_SCALE = 1.2f;

class SeedPacket : public GameObject
{
public:
    int mRefreshCounter;
    int mRefreshTime;
    int mIndex;
    int mOffsetY;
    SeedType mPacketType;
    SeedType mImitaterType;
    int mSlotMachineCountDown;
    SeedType mSlotMachiningNextSeed;
    float mSlotMachiningPosition;
    bool mActive;
    bool mRefreshing;
    int mTimesUsed;
    SeedBank *mSeedBank;
    float mLastSelectedTime;
    // Phase of the selection-arrow bob. Update adds 0.006 each tick.
    float mArrowWaveTime;
    // Current draw scale while the packet is selected or just released.
    float mSelectScale;
    bool mSelectedBy2P;
    bool mSelected;
    bool mSelectedByBothPlayer;

    virtual ~SeedPacket();

    SeedPacket();

    SeedType PickNextSlotMachineSeed();
    bool GetPlayerIndex();
    void UpdateSelected();
    void DrawBackground(Sexy::Graphics *g);
    void DrawMiddle(Sexy::Graphics *g);
    void DrawOverlay(Sexy::Graphics *g);
    void Draw(Sexy::Graphics *g);
    bool CanPickUp();
    void FlashIfReady();
    void Deactivate();
    void MouseDown(int x, int y, int clickCount, int playerIndex);
    void Activate();
    void Update();
    void SlotMachineStart();
    bool MouseHitTest(int x, int y, HitResult *hitResult);
    bool IsZombie();
    void WasPlanted(int playerIndex);
    void SetPacketType(SeedType theSeedType, SeedType theImitaterType);
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_SEEDPACKET_H
