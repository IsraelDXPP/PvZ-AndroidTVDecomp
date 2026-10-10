#include "SeedPacket.h"
#include "game/Board.h"
#include "Challenge.h"
#include "CursorObject.h"
#include "GamepadControls.h"
#include "Graphics.h"
#include "HitResult.h"
#include "Image.h"
#include "LawnApp.h"
#include "MessageWidget.h"
#include "Plant.h"
#include "SeedBank.h"
#include "SexyTransform2D.h"
#include "TodParticleEmitter.h"
#include "TodParticleSystem.h"
#include <cmath>
#include <cstring>

// LawnApp::AddTodParticle's effect id for the ready-to-pick flash.

// ChallengeState is a one-int surrogate; this is the value Update compares.

namespace
{

SeedType DisplayedSeed(SeedType packetType, SeedType imitaterType)
{
    if (packetType == SEED_IMITATER && imitaterType != SEED_NONE)
        return imitaterType;
    return packetType;
}

// Refresh darkens the packet; an active packet is fully lit.
float RefreshDarkness(bool active, int refreshTime, int refreshCounter)
{
    if (active)
        return 0.0f;
    if (refreshTime == 0)
        return 1.0f;
    return (float)(refreshTime - refreshCounter) / (float)refreshTime;
}

bool TutorialWantsFlash(Board *board, SeedType packetType)
{
    switch (board->mTutorialState)
    {
        case TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER:
            return board->mTutorialTimer == -1 && packetType == SEED_PEASHOOTER;
        case TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER:
        case TUTORIAL_MORESUN_PICK_UP_SUNFLOWER:
            return packetType == SEED_SUNFLOWER;
        case TUTORIAL_WHACK_A_ZOMBIE_PICK_SEED:
            return true;
        default:
            return false;
    }
}

// Grayness of the packet art. Flashing tutorial packets use the flash red
// channel. Playing packets go full white when they can be planted, else 128.
int PacketGrayness(
    SeedPacket *packet, SeedType shownSeed, int plantCost, bool isZombieBank, bool costsSun, float *darkness)
{
    LawnApp *app = packet->mApp;
    Board *board = packet->mBoard;
    GameMode mode = app->mGameMode;

    if ((mode == GAMEMODE_CHALLENGE_BEGHOULED || mode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST) && !packet->mActive)
        return 64;

    if (app->mGameScene != SCENE_PLAYING)
    {
        *darkness = 0.0f;
        if (packet->mSeedBank != nullptr)
            return packet->mSeedBank->mCutSceneDarken;
        return 255;
    }

    if (TutorialWantsFlash(board, packet->mPacketType))
    {
        Sexy::Color flash;
        GetFlashingColor(&flash, board->mMainCounter, 75);
        return flash.mRed;
    }

    if (app->mEasyPlantingCheat)
    {
        *darkness = 0.0f;
        return 255;
    }

    if (!app->IsSlotMachineLevel())
    {
        bool canAfford;
        if (isZombieBank)
            canAfford = board->CanTakeDeathMoney(plantCost);
        else
            canAfford = board->CanTakeSunMoney(plantCost, packet->GetPlayerIndex());

        if (!canAfford && costsSun)
            return 128;
    }

    if (*darkness <= 0.0f && board->PlantingRequirementsMet(shownSeed))
        return 255;
    return 128;
}

// Selection grows the packet to 1.2 over 0.15s. A released packet shrinks back
// over the same window and then stops transforming.
void ApplySelectScale(SeedPacket *packet, Sexy::Graphics *g)
{
    if (packet->mApp->IsSlotMachineLevel())
        return;

    if (packet->mSelected)
    {
        if (packet->mLastSelectedTime < SEED_PACKET_SELECT_TIME)
            packet->mSelectScale = TodAnimateCurveFloatTime(0.0f,
                SEED_PACKET_SELECT_TIME,
                packet->mLastSelectedTime,
                1.0f,
                SEED_PACKET_SELECT_SCALE,
                CURVE_EASE_OUT);
        else
            packet->mSelectScale = SEED_PACKET_SELECT_SCALE;
    }
    else
    {
        packet->mSelectScale = TodAnimateCurveFloatTime(
            0.0f, SEED_PACKET_SELECT_TIME, packet->mLastSelectedTime, SEED_PACKET_SELECT_SCALE, 1.0f, CURVE_EASE_OUT);
        if (packet->mLastSelectedTime >= SEED_PACKET_SELECT_TIME)
            return;
    }

    g->SetScale(packet->mSelectScale, packet->mSelectScale, (float)(packet->mOffsetY + 25), 35.0f);
    g->TranslateF((1.0f - packet->mSelectScale) * 25.0f, (1.0f - packet->mSelectScale) * 35.0f);
}

void UndoSelectScale(SeedPacket *packet, Sexy::Graphics *g)
{
    if (packet->mApp->IsSlotMachineLevel())
        return;
    if (!packet->mSelected && packet->mLastSelectedTime >= SEED_PACKET_SELECT_TIME)
        return;

    g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
    g->TranslateF((packet->mSelectScale - 1.0f) * 25.0f, (packet->mSelectScale - 1.0f) * 35.0f);
}

void StartRefreshing(SeedPacket *packet, int refreshTime)
{
    packet->mRefreshTime = refreshTime;
    packet->mRefreshing = true;
    packet->mActive = false;
}

// Versus mode starts a handful of plants already cooling down. I-Zombie seeds
// (normal, trashcan, traffic cone) stay ready.
bool VersusStartsCooling(SeedType seedType, bool zombieBank)
{
    if (zombieBank)
        return seedType < SEED_ZOMBIE_NORMAL || seedType > SEED_ZOMBIE_TRASHCAN;

    switch (seedType)
    {
        case SEED_PEASHOOTER:
        case SEED_SUNFLOWER:
        case SEED_WALLNUT:
        case SEED_POTATOMINE:
        case SEED_PUFFSHROOM:
        case SEED_TALLNUT:
        case SEED_CABBAGEPULT:
            return false;
        default:
            return true;
    }
}

void DrawSuddenDeathMark(Sexy::Graphics *g, int centerX, int centerY)
{
    Sexy::Image *mark = IMAGE_MP_TARGETS_X;
    g->DrawImage(mark, centerX - mark->mWidth / 2, centerY - mark->mHeight / 2, mark->mWidth, mark->mHeight);
}

} // namespace

SeedPacket::~SeedPacket() {}

SeedType SeedPacket::PickNextSlotMachineSeed()
{
    // Peashooter weight falls as the board fills with them. Diamond stays rare
    // and never gets the "already queued" bonus. The third reel prefers a seed
    // the first two reels are already about to show.
    static const SeedType kSeedList[6] = {
        SEED_SUNFLOWER, SEED_PEASHOOTER, SEED_SNOWPEA, SEED_WALLNUT, SEED_SLOT_MACHINE_SUN, SEED_SLOT_MACHINE_DIAMOND};

    TodWeightedArray weighted[6];
    int peashooterCount = mBoard->CountPlantByType(SEED_PEASHOOTER);

    for (int i = 0; i < 6; i++)
    {
        SeedType seedType = kSeedList[i];
        int weight;
        if (seedType == SEED_SLOT_MACHINE_DIAMOND)
        {
            weight = 30;
        }
        else
        {
            if (seedType == SEED_PEASHOOTER)
                weight = TodAnimateCurve(0, 5, peashooterCount, 200, 100, CURVE_LINEAR);
            else
                weight = 100;

            if (mIndex == 2 &&
                (mSeedBank->mSeedPackets[0].mSlotMachiningNextSeed == seedType ||
                    mSeedBank->mSeedPackets[1].mSlotMachiningNextSeed == seedType))
                weight += weight / 2;
        }

        weighted[i].mItem = seedType;
        weighted[i].mWeight = weight;
    }

    mSlotMachiningNextSeed = (SeedType)TodPickFromWeightedArray(weighted, 6);
    return mSlotMachiningNextSeed;
}

bool SeedPacket::GetPlayerIndex()
{
    // Player 2 owns every packet that is not in the board's first bank.
    return mSeedBank != mBoard->mSeedBank[0];
}

void SeedPacket::UpdateSelected()
{
    Board *board = mBoard;
    SeedBank *bank = mSeedBank;
    GamepadControls *pad1 = board->mGamepadControls[1];
    GamepadControls *pad0 = board->mGamepadControls[0];
    const auto pointsAtPacket = [this, board](GamepadControls *controls)
    {
        if (controls == nullptr || controls->mSelectedSeedIndex != mIndex)
            return false;
        if (!board->mMouseInputActive[controls->mPlayerIndex])
            return true;
        const CursorObject *cursor = board->mCursorObject[controls->mPlayerIndex];
        return cursor->mCursorType == CURSOR_TYPE_PLANT_FROM_BANK && cursor->mSelectedIndex == mIndex;
    };

    // UpdateSelected @0x172A40 identifies the bank owner before selecting its
    // packet. Keep that ownership for separate VS/coop banks; the shared-bank
    // selection below would otherwise mark the other player's index as well.
    if (mApp->mGameMode == GAMEMODE_MP_VS || mApp->IsCoopMode())
    {
        GamepadControls *owner = GetPlayerIndex() ? pad1 : pad0;
        mSelected = owner != nullptr && !owner->mIsInShopSeedBank && pointsAtPacket(owner);
        mSelectedBy2P = mSelected && owner->mPlayerIndex == 1;
        mSelectedByBothPlayer = false;
        return;
    }

    // Player 2's frame: the second pad pointing at this packet in the first
    // bank (solo only), or any packet in the second bank that pad is on.
    bool selectedBy2P = false;
    if (bank == board->mSeedBank[0] && pointsAtPacket(pad1) && pad1->mGamepadIndex != -1 && !pad1->mIsZombie &&
        !mApp->IsCoopMode())
        selectedBy2P = true;

    if (bank == board->mSeedBank[1] && pad1 != nullptr)
    {
        bool pad1Counts = pad1->mIsZombie || mApp->IsCoopMode();
        if (pad1Counts && pointsAtPacket(pad1))
            selectedBy2P = true;
    }
    mSelectedBy2P = selectedBy2P;

    bool selected = false;
    if (bank == board->mSeedBank[0] && pointsAtPacket(pad0))
        selected = true;
    else if (board->mSeedBank[1] == bank && pointsAtPacket(pad1))
        selected = true;
    else
        selected = mSelectedBy2P;
    mSelected = selected;

    bool bothPlayers = false;
    if (pad1->mGamepadIndex != -1 && pointsAtPacket(pad0) && pointsAtPacket(pad1))
    {
        if (mApp->IsCoopMode())
            bothPlayers = false;
        else
            bothPlayers = mApp->mGameMode != GAMEMODE_MP_VS;
    }
    mSelectedByBothPlayer = bothPlayers;

    // A pad sitting in the shop bank does not light a board packet. Outside
    // the shop, the highlight is whichever pad is on this index.
    if (pad0->mIsInShopSeedBank)
    {
        mSelected = false;
        return;
    }

    mSelected = pointsAtPacket(pad0);
    if (!mSelected && mApp->IsTwoPlayerGame())
    {
        GamepadControls *other = board->mGamepadControls[1];
        if (other != nullptr && !other->mIsInShopSeedBank)
            mSelected = pointsAtPacket(other);
    }
}

void SeedPacket::DrawBackground(Sexy::Graphics *g)
{
    float darkness = RefreshDarkness(mActive, mRefreshTime, mRefreshCounter);

    if (mSlotMachineCountDown > 0)
    {
        // The current seed slides up and the next one follows it into the box.
        float slide = (float)-mHeight * mSlotMachiningPosition;
        if (slide <= 0.0f)
            slide -= 0.5f;
        else
            slide += 0.5f;
        int slideY = (int)slide;

        g->PushState();
        g->ClipRect(0, 0, mWidth, mHeight);
        DrawSeedPacket(g, 0.0f, (float)slideY, mPacketType, SEED_NONE, 0.0f, 128, false, false, false, true);
        DrawSeedPacket(g,
            0.0f,
            (float)(mHeight + slideY),
            mSlotMachiningNextSeed,
            SEED_NONE,
            0.0f,
            128,
            false,
            false,
            false,
            true);
        g->PopState();
        return;
    }

    bool zombieBank = mSeedBank != nullptr && mSeedBank->mIsZombie;
    bool costsSun = !mBoard->HasConveyorBeltSeedBank(zombieBank) && !mApp->IsSlotMachineLevel();
    int plantCost = mBoard->GetCurrentPlantCost(mPacketType, mImitaterType);
    SeedType shown = DisplayedSeed(mPacketType, mImitaterType);
    int grayness = PacketGrayness(this, shown, plantCost, zombieBank, costsSun, &darkness);

    // Packets past the tenth stay fully lit and undarkened.
    if (mIndex >= 10)
    {
        grayness = 255;
        darkness = 0.0f;
    }

    ApplySelectScale(this, g);

    if (!mApp->IsSlotMachineLevel() && mSelected)
    {
        Sexy::Image *frame = mSelectedBy2P ? IMAGE_SEED_SELECTOR_BLUE : IMAGE_SEED_SELECTOR;
        g->DrawImage(frame,
            (int)((float)mOffsetY - 1.0f - g->mScaleX * 5.0f),
            (int)-(g->mScaleY * 5.0f + 2.5f),
            (int)(g->mScaleX * 60.0f),
            (int)(g->mScaleY * 80.0f));
    }

    bool versusZombie = false;
    if (mApp->mGameMode == GAMEMODE_MP_VS && mSeedBank != nullptr)
        versusZombie = mSeedBank->mIsZombie;

    DrawSeedPacket(
        g, (float)mOffsetY, 0.0f, mPacketType, mImitaterType, darkness, grayness, false, true, versusZombie, true);
    UndoSelectScale(this, g);
}

void SeedPacket::DrawMiddle(Sexy::Graphics *g)
{
    if (mSlotMachineCountDown > 0)
        return;

    float darkness = RefreshDarkness(mActive, mRefreshTime, mRefreshCounter);
    bool zombieBank = mSeedBank != nullptr && mSeedBank->mIsZombie;
    bool costsSun = !mBoard->HasConveyorBeltSeedBank(zombieBank) && !mApp->IsSlotMachineLevel();
    int plantCost = mBoard->GetCurrentPlantCost(mPacketType, mImitaterType);
    SeedType shown = DisplayedSeed(mPacketType, mImitaterType);
    int grayness = PacketGrayness(this, shown, plantCost, zombieBank, costsSun, &darkness);

    ApplySelectScale(this, g);

    bool versusZombie = false;
    if (mApp->mGameMode == GAMEMODE_MP_VS && mSeedBank != nullptr)
        versusZombie = mSeedBank->mIsZombie;

    if (mIndex >= 10)
    {
        grayness = 255;
        darkness = 0.0f;
    }

    DrawSeedPacket(
        g, (float)mOffsetY, 0.0f, mPacketType, mImitaterType, darkness, grayness, costsSun, true, versusZombie, false);
    UndoSelectScale(this, g);
}

void SeedPacket::DrawOverlay(Sexy::Graphics *g)
{
    if (mSlotMachineCountDown > 0)
        return;

    if (!mApp->IsSlotMachineLevel())
    {
        if (mSelected)
        {
            if (mLastSelectedTime < SEED_PACKET_SELECT_TIME)
                mSelectScale = TodAnimateCurveFloatTime(
                    0.0f, SEED_PACKET_SELECT_TIME, mLastSelectedTime, 1.0f, SEED_PACKET_SELECT_SCALE, CURVE_EASE_OUT);
            else
                mSelectScale = SEED_PACKET_SELECT_SCALE;
        }
        else
        {
            mSelectScale = TodAnimateCurveFloatTime(
                0.0f, SEED_PACKET_SELECT_TIME, mLastSelectedTime, SEED_PACKET_SELECT_SCALE, 1.0f, CURVE_EASE_OUT);
        }
    }

    if (!mApp->IsSlotMachineLevel() && mSelected)
    {
        // The arrow bobs by twice the sine of the wave timer.
        float bob = sinf(mArrowWaveTime * 5.0f);
        float bobY = bob + bob;

        if (!mApp->IsTwoPlayerGame())
        {
            Sexy::Image *arrow = mSelectedBy2P ? IMAGE_CURSOR_ARROW_P2 : IMAGE_CURSOR_ARROW_P1;
            SexyTransform2D transform(true);
            transform.Scale(1.5f, 1.5f);
            transform.Translate((float)(mOffsetY + 23), bobY - 8.0f);
            g->DrawImageMatrix(arrow, transform, 0.0f, 0.0f, true);
        }
        else if (mSelectedBy2P && mSelectedByBothPlayer)
        {
            // Both players: P2's arrow is flipped under the packet, P1's sits above.
            Sexy::Image *arrow2 = IMAGE_CURSOR_ARROW_P2;
            g->DrawImageRotatedF(
                arrow2, (float)(mOffsetY + 23 - arrow2->mWidth / 2), bobY + 63.0f, 3.14159274, nullptr);
            Sexy::Image *label2 = IMAGE_CURSOR_P2_TEXT;
            g->DrawImageF(label2, (float)(mOffsetY + 23 - label2->mWidth / 2), 75.0f);

            Sexy::Image *arrow1 = IMAGE_CURSOR_ARROW_P1;
            g->DrawImageF(arrow1, (float)(mOffsetY + 23 - arrow1->mWidth / 2), bobY - 8.0f);
            Sexy::Image *label1 = IMAGE_CURSOR_P1_TEXT;
            g->DrawImageF(label1, (float)(mOffsetY + 23 - label1->mWidth / 2), -28.0f);
        }
        else
        {
            Sexy::Image *label = mSelectedBy2P ? IMAGE_CURSOR_P2_TEXT : IMAGE_CURSOR_P1_TEXT;
            Sexy::Image *arrow = mSelectedBy2P ? IMAGE_CURSOR_ARROW_P2 : IMAGE_CURSOR_ARROW_P1;
            g->DrawImageF(arrow, (float)(mOffsetY + 23 - arrow->mWidth / 2), bobY - 8.0f);
            g->DrawImageF(label, (float)(mOffsetY + 23 - label->mWidth / 2), -28.0f);
        }
    }

    // Versus sudden death crosses out the packets it has turned off.
    if (mApp->mGameMode != GAMEMODE_MP_VS)
        return;
    if ((unsigned)Challenge::gVSSuddenDeathMode > 1)
        return;
    if (!mApp->mBoard->mChallenge->mIsMPSuddenDeathNow)
        return;

    int centerX = mOffsetY + IMAGE_SEEDS->GetCelWidth() / 2;
    int centerY = IMAGE_SEEDS->GetCelHeight() / 2;
    bool zombieBank = mSeedBank->mIsZombie;
    if (Challenge::IsMPResourceProducer(mPacketType) ||
        mApp->mBoard->mChallenge->ISMPSeedSuddenDeathDisabled(zombieBank, mPacketType))
        DrawSuddenDeathMark(g, centerX, centerY);
}

void SeedPacket::Draw(Sexy::Graphics *g)
{
    DrawBackground(g);
    DrawMiddle(g);
    DrawOverlay(g);
}

bool SeedPacket::CanPickUp()
{
    if (mBoard->mPaused)
        return false;
    if (mApp->mGameScene != SCENE_PLAYING)
        return false;

    SeedType seedType = DisplayedSeed(mPacketType, mImitaterType);
    if (mPacketType == SEED_NONE)
        return false;
    if (mApp->IsSlotMachineLevel())
        return false;
    if (mApp->mEasyPlantingCheat)
        return true;
    if (!mActive)
        return false;

    int plantCost = mBoard->GetCurrentPlantCost(mPacketType, mImitaterType);
    if (mSeedBank != nullptr && mSeedBank->mIsZombie)
    {
        if (!mBoard->CanTakeDeathMoney(plantCost) && !mBoard->HasConveyorBeltSeedBank(true))
            return false;
    }
    else if (!mBoard->CanTakeSunMoney(plantCost, GetPlayerIndex()) && !mBoard->HasConveyorBeltSeedBank(false))
    {
        return false;
    }

    return mBoard->PlantingRequirementsMet(seedType);
}

void SeedPacket::FlashIfReady()
{
    if (!CanPickUp() || mApp->mEasyPlantingCheat)
        return;

    bool zombieBank = mSeedBank != nullptr && mSeedBank->mIsZombie;
    if (!mBoard->HasConveyorBeltSeedBank(zombieBank) && mSeedBank != nullptr)
    {
        // Versus shrinks the flash to match the smaller packet.
        float scale = 1.0f;
        float offsetX = 0.0f;
        float offsetY = 0.0f;
        if (mApp->mGameMode == GAMEMODE_MP_VS)
        {
            scale = 0.9f;
            offsetX = -3.0f;
            offsetY = -4.0f;
        }

        TodParticleSystem *particles = mApp->AddTodParticle(offsetX + (float)mSeedBank->mX + scale * (float)mX,
            offsetY + (float)mSeedBank->mY + scale * (float)mY,
            100002,
            PARTICLE_SEED_PACKET_FLASH);
        TodParticleEmitter *emitter = particles->FindEmitterByName("SeedPacketFlash");
        if (emitter != nullptr)
            emitter->mSyncState.mScaleOverride = scale;
    }

    TutorialState tutorial = mBoard->mTutorialState;
    if (tutorial == TUTORIAL_LEVEL_1_REFRESH_PEASHOOTER)
    {
        mBoard->SetTutorialState(TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER);
        return;
    }
    if (tutorial == TUTORIAL_LEVEL_2_REFRESH_SUNFLOWER && mPacketType == SEED_SUNFLOWER)
    {
        mBoard->SetTutorialState(TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER);
        return;
    }
    if (tutorial == TUTORIAL_MORESUN_REFRESH_SUNFLOWER && mPacketType == SEED_SUNFLOWER)
        mBoard->SetTutorialState(TUTORIAL_MORESUN_PICK_UP_SUNFLOWER);
}

void SeedPacket::Deactivate()
{
    mActive = false;
    mRefreshCounter = 0;
    mRefreshTime = 0;
    mRefreshing = false;
}

void SeedPacket::MouseDown(int x, int y, int clickCount, int playerIndex)
{
    (void)x;
    (void)y;
    (void)clickCount;

    if (mBoard->mPaused)
        return;
    if (mApp->mGameScene != SCENE_PLAYING || mPacketType == SEED_NONE)
        return;

    if (mApp->IsSlotMachineLevel())
    {
        if (!mBoard->mAdvice->IsBeingDisplayed())
            mBoard->DisplayAdvice("[ADVICE_SLOT_MACHINE_PULL]", MESSAGE_STYLE_HINT_TALL_FAST, ADVICE_NONE);

        // At most two extra rolls stay queued.
        int rolls = mBoard->mChallenge->mSlotMachineRollCount;
        if (rolls >= 2)
            rolls = 2;
        mBoard->mChallenge->mSlotMachineRollCount = rolls;
        return;
    }

    SeedType seedType = mPacketType;
    if (seedType == SEED_IMITATER)
    {
        seedType = mImitaterType;
        if (seedType == SEED_NONE)
            seedType = SEED_IMITATER;
    }

    // Versus lets player 1 pick without the extra checks. A click that already
    // carries a player index does too. The cheat skips them entirely.
    bool skipChecks = mApp->mEasyPlantingCheat || (mApp->mGameMode == GAMEMODE_MULTI_PLAYER && playerIndex > 0);
    if (!skipChecks)
    {
        if (!mActive)
        {
            mApp->PlaySample(SOUND_BUZZER, true);
            if (!mApp->IsFirstTimeAdventureMode())
                return;
            if (mBoard->mLevel != 1 || !mBoard->mHelpDisplayed[0])
                return;
            mBoard->DisplayAdvice("[ADVICE_SEED_REFRESH]", MESSAGE_STYLE_TUTORIAL_LEVEL1, ADVICE_SEED_REFRESH);
            return;
        }

        int plantCost = mBoard->GetCurrentPlantCost(mPacketType, mImitaterType);
        if (!mBoard->CanTakeSunMoney(plantCost, GetPlayerIndex()) && !mBoard->HasConveyorBeltSeedBank(false))
        {
            mApp->PlaySample(SOUND_BUZZER, true);
            mBoard->mOutOfMoneyCounter = 70;
            if (!mApp->IsFirstTimeAdventureMode())
                return;
            if (mBoard->mLevel != 1 || !mBoard->mHelpDisplayed[0])
                return;
            mBoard->DisplayAdvice(
                "[ADVICE_CANT_AFFORD_PLANT]", MESSAGE_STYLE_TUTORIAL_LEVEL1, ADVICE_CANT_AFFORD_PLANT);
            return;
        }

        if (!mBoard->PlantingRequirementsMet(seedType))
        {
            mApp->PlaySample(SOUND_BUZZER, true);
            switch (seedType)
            {
                case SEED_GATLINGPEA:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_REPEATER]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_REPEATER);
                    break;
                case SEED_WINTERMELON:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_MELONPULT]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_MELONPULT);
                    break;
                case SEED_TWINSUNFLOWER:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_SUNFLOWER]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_SUNFLOWER);
                    break;
                case SEED_SPIKEROCK:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_SPIKEWEED]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_SPIKEWEED);
                    break;
                case SEED_COBCANNON:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_KERNELPULT]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_KERNELPULT);
                    break;
                case SEED_GOLD_MAGNET:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_MAGNETSHROOM]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_MAGNETSHROOM);
                    break;
                case SEED_GLOOMSHROOM:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_FUMESHROOM]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_FUMESHROOM);
                    break;
                case SEED_CATTAIL:
                    mBoard->DisplayAdvice(
                        "[ADVICE_PLANT_NEEDS_LILYPAD]", MESSAGE_STYLE_HINT_LONG, ADVICE_PLANT_NEEDS_LILYPAD);
                    break;
                default:
                    break;
            }
            return;
        }
    }

    mBoard->ClearAdvice(ADVICE_CANT_AFFORD_PLANT);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_REPEATER);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_MELONPULT);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_SUNFLOWER);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_KERNELPULT);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_SPIKEWEED);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_MAGNETSHROOM);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_FUMESHROOM);
    mBoard->ClearAdvice(ADVICE_PLANT_NEEDS_LILYPAD);

    switch (mApp->mGameMode)
    {
        case GAMEMODE_CHALLENGE_BEGHOULED:
        case GAMEMODE_CHALLENGE_BEGHOULED_TWIST:
            mBoard->mChallenge->BeghouledPacketClicked(this);
            return;
        case GAMEMODE_CHALLENGE_HEAVY_WEAPON:
            mBoard->mChallenge->HeavyWeaponPacketClicked(this);
            return;
        case GAMEMODE_CHALLENGE_ZOMBIQUARIUM:
            mBoard->mChallenge->ZombiquariumPacketClicked(this);
            return;
        default:
            break;
    }

    CursorObject *cursor = mBoard->mCursorObject[playerIndex];
    cursor->mSelectedIndex = mIndex;
    cursor->mType = mPacketType;
    cursor->mImitaterType = mImitaterType;
    cursor->mCursorType = CURSOR_TYPE_PLANT_FROM_BANK;
    mApp->PlaySample(SOUND_SEEDLIFT, true);

    switch (mBoard->mTutorialState)
    {
        case TUTORIAL_LEVEL_1_PICK_UP_PEASHOOTER:
            mBoard->SetTutorialState(TUTORIAL_LEVEL_1_PLANT_PEASHOOTER);
            break;
        case TUTORIAL_LEVEL_2_PICK_UP_SUNFLOWER:
            mBoard->SetTutorialState(
                mPacketType == SEED_SUNFLOWER ? TUTORIAL_LEVEL_2_PLANT_SUNFLOWER : TUTORIAL_LEVEL_2_REFRESH_SUNFLOWER);
            break;
        case TUTORIAL_MORESUN_PICK_UP_SUNFLOWER:
            mBoard->SetTutorialState(
                mPacketType == SEED_SUNFLOWER ? TUTORIAL_MORESUN_PLANT_SUNFLOWER : TUTORIAL_MORESUN_REFRESH_SUNFLOWER);
            break;
        case TUTORIAL_WHACK_A_ZOMBIE_BEFORE_PICK_SEED:
        case TUTORIAL_WHACK_A_ZOMBIE_PICK_SEED:
            mBoard->SetTutorialState(TUTORIAL_WHACK_A_ZOMBIE_COMPLETED);
            break;
        default:
            break;
    }

    Deactivate();
}

void SeedPacket::Activate()
{
    mActive = true;
}

void SeedPacket::Update()
{
    UpdateSelected();
    mLastSelectedTime += 0.006f;
    mArrowWaveTime += 0.006f;

    if (mApp->mGameScene != SCENE_PLAYING || mPacketType == SEED_NONE)
        return;

    if (mBoard->mMainCounter == 0)
        FlashIfReady();

    if (!mActive && mRefreshing)
    {
        mRefreshCounter++;
        if (mRefreshCounter > mRefreshTime)
        {
            mRefreshCounter = 0;
            mRefreshing = false;
            Activate();
            FlashIfReady();
        }
    }

    if (mSlotMachineCountDown <= 0)
        return;

    mSlotMachineCountDown--;
    float flipsPerSecond = TodAnimateCurveFloat(SLOT_MACHINE_TIME, 0, mSlotMachineCountDown, 6.0f, 2.0f, CURVE_LINEAR);
    mSlotMachiningPosition += flipsPerSecond * 0.01f;

    if (mSlotMachiningPosition < 1.0f)
    {
        // Hold the last frame until the next seed has finished sliding in.
        if (mSlotMachineCountDown == 0)
            mSlotMachineCountDown = 1;
        return;
    }

    mPacketType = mSlotMachiningNextSeed;
    if (mSlotMachineCountDown != 0)
    {
        mSlotMachiningPosition -= 1.0f;
        PickNextSlotMachineSeed();
        return;
    }

    mSlotMachiningPosition = 0.0f;
    Activate();
}

void SeedPacket::SlotMachineStart()
{
    mSlotMachineCountDown = SLOT_MACHINE_TIME;
    mSlotMachiningPosition = 0.0f;
    PickNextSlotMachineSeed();
}

bool SeedPacket::MouseHitTest(int x, int y, HitResult *hitResult)
{
    // Spinning reels and empty packets are not clickable. The hit box is the
    // packet rect shifted by the conveyor offset.
    if (mSlotMachineCountDown > 0 || mPacketType == SEED_NONE || x < mX + mOffsetY || x >= mX + mOffsetY + mWidth ||
        y < mY || y >= mY + mHeight)
    {
        hitResult->mObject = nullptr;
        hitResult->mObjectType = OBJECT_TYPE_NONE;
        return false;
    }

    hitResult->mObject = this;
    hitResult->mObjectType = OBJECT_TYPE_SEEDPACKET;
    return true;
}

bool SeedPacket::IsZombie()
{
    if (mSeedBank != nullptr)
        return mSeedBank->mIsZombie;
    return false;
}

void SeedPacket::WasPlanted(int playerIndex)
{
    if (playerIndex != 0)
    {
        if (mApp->mSecondPlayerGamepadIndex != -1 && mApp->mPlayer2Info != nullptr)
            mApp->mPlayer2Info->FavoritePlants()[mPacketType]++;
    }
    else
    {
        mApp->mPlayerInfo->FavoritePlants()[mPacketType]++;
    }

    if (mBoard->HasConveyorBeltSeedBank(mSeedBank->mIsZombie))
    {
        mSeedBank->RemoveSeed(mIndex);
        return;
    }

    if (mApp->IsSlotMachineLevel())
    {
        Deactivate();
        return;
    }

    if (mApp->mGameMode != GAMEMODE_CHALLENGE_LAST_STAND ||
        mBoard->mChallenge->mChallengeState == STATECHALLENGE_LAST_STAND_ONSLAUGHT)
    {
        mRefreshing = true;
        mTimesUsed++;
        mRefreshTime = Plant::GetRefreshTime(mPacketType, mImitaterType);
    }
    else
    {
        // Last Stand before the onslaught hands the packet straight back.
        mActive = true;
        mTimesUsed++;
        FlashIfReady();
    }

    // Only a peashooter, or an imitater of one, feeds the favourite-plant flag.
    if (mPacketType != SEED_PEASHOOTER && (mPacketType != SEED_IMITATER || mImitaterType != SEED_PEASHOOTER))
        return;

    if ((unsigned char)mApp->mPlayerInfo->FavoritePlants()[SEED_PEASHOOTER] > 9)
        mApp->mPlayerInfo->mHasFavoritePlant = true;

    if (mApp->mSecondPlayerGamepadIndex != -1)
    {
        Sexy::DefaultPlayerInfo *info2 = mApp->mPlayer2Info;
        if (info2 != nullptr && (unsigned char)info2->FavoritePlants()[SEED_PEASHOOTER] > 9)
            info2->mHasFavoritePlant = true;
    }
}

void SeedPacket::SetPacketType(SeedType theSeedType, SeedType theImitaterType)
{
    SeedType useSeed = theSeedType;
    if (theSeedType == SEED_IMITATER && theImitaterType != SEED_NONE)
        useSeed = theImitaterType;

    mActive = true;
    mPacketType = theSeedType;
    mImitaterType = theImitaterType;
    mRefreshCounter = 0;
    mRefreshTime = 0;
    mRefreshing = false;

    GameMode mode = mApp->mGameMode;

    // Versus starts most packets on a shortened cooldown. The length follows
    // the plant's real refresh time.
    if (mode == GAMEMODE_MP_VS)
    {
        bool zombieBank = mSeedBank != nullptr && mSeedBank->mIsZombie;
        if (!VersusStartsCooling(theSeedType, zombieBank))
            return;

        int refresh = Plant::GetRefreshTime(theSeedType, theImitaterType);
        // 3000..5999 cools for 2s. A shorter refresh cools for 1s, a longer one for 3.5s.
        int cooldown = 3500;
        if ((unsigned)(refresh - 3000) <= 0xBB7)
            cooldown = 2000;
        else if (refresh <= 5999)
            cooldown = 1000;
        StartRefreshing(this, cooldown);
        return;
    }

    // These modes hand the packet over already usable.
    if (mode == GAMEMODE_CHALLENGE_BEGHOULED || mode == GAMEMODE_CHALLENGE_ZOMBIQUARIUM ||
        mode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST || mode == GAMEMODE_CHALLENGE_LAST_STAND || mApp->IsIZombieLevel() ||
        mApp->IsScaryPotterLevel() || mApp->IsWhackAZombieLevel() || mode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
        return;

    if (mApp->IsSurvivalMode() && mBoard->mChallenge->mSurvivalStage > 0)
        return;

    if ((Plant::IsUpgrade(useSeed) && !gLawnApp->IsSurvivalMode()) ||
        Plant::GetRefreshTime(mPacketType, mImitaterType) == 5000)
    {
        StartRefreshing(this, 3500);
        return;
    }

    if (Plant::IsUpgrade(useSeed) && gLawnApp->IsSurvivalMode())
    {
        StartRefreshing(this, 8000);
        return;
    }

    if (Plant::GetRefreshTime(mPacketType, mImitaterType) == 3000)
        StartRefreshing(this, 2000);
}

SeedPacket::SeedPacket()
{
    // The packet's own synced range is the 60 bytes from the refresh counter
    // through the selection scale.
    SyncBlockInfo block = MakeSyncBlock(mRefreshCounter, mSelectScale);
    mSyncBlocks.push_back(block);
    std::memset(block.mData, 0, block.mSize);

    mSlotMachiningPosition = 0.0f;
    mActive = true;
    mArrowWaveTime = 0.0f;
    mSelectScale = 0.0f;
    mLastSelectedTime = SEED_PACKET_SELECT_TIME;
    mPacketType = SEED_NONE;
    mImitaterType = SEED_NONE;
    mIndex = -1;
    mSlotMachiningNextSeed = SEED_NONE;
    mWidth = 50;
    mHeight = 70;
    mRefreshCounter = 0;
    mRefreshTime = 0;
    mRefreshing = false;
    mOffsetY = 0;
    mSlotMachineCountDown = 0;
    mTimesUsed = 0;
    mSeedBank = nullptr;
    mSelectedBy2P = false;
    mSelected = false;
    mSelectedByBothPlayer = false;
}
