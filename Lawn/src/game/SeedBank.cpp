#include "SeedBank.h"

#include "game/Board.h"
#include "CutScene.h"
#include "GamepadControls.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "NativeDisplay.h"
#include "LawnUser.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "SeedPacket.h"
#include "SexyTransform2D.h"
#include "ShopSeedPacket.h"
#include "ToolTipWidget.h"

#include <cstring>

namespace {

const Sexy::Color kTipColor(224, 187, 98);
const int kSunLabelJustify = 2;
const int kShopPacketCount = 7;

int NonNegative(int theValue)
{
    return theValue < 0 ? 0 : theValue;
}

bool BothGamepadsOn(LawnApp *theApp)
{
    return theApp->mGamePad1IsOn && theApp->mGamePad2IsOn;
}

// Conveyor art advances one cel every four ticks. The counter only increases.
int ConveyorCel(int theCounter)
{
    int aFrame = theCounter / 4;
    if (aFrame < 0)
        aFrame = -aFrame;
    return aFrame % 6;
}

void DrawTip(Sexy::Graphics *theGraphics, const char *theKey, int theX, int theY)
{
    TodDrawString(theGraphics, Sexy::String(theKey), theX, theY, FONT_HOUSEOFTERROR16, kTipColor, kSunLabelJustify);
}

void DrawShopPackets(Sexy::Graphics *theGraphics, SeedBank *theBank)
{
    for (int i = 0; i < kShopPacketCount; i++)
    {
        ShopSeedPacket *aPacket = &theBank->mShopSeedPackets[i];
        if (!aPacket->BeginDraw(theGraphics))
            continue;
        aPacket->Draw(theGraphics);
        aPacket->EndDraw(theGraphics);
    }
}

void DrawCoinCount(Sexy::Graphics *theGraphics)
{
    LawnUser *aUser = LawnUser::getCurUser();
    TodDrawString(theGraphics, aUser->mCoinText, 38, 68, FONT_CONTINUUMBOLD14, Sexy::Color(0, 0, 0), kSunLabelJustify);
}

// Which held-packet slot this index belongs to. Slot machine never holds a
// packet. Coop and versus use the controls of this bank. A single bank checks
// player 1, then player 2. 0 is not held, 1 is the first holder, 2 the second.
int HeldSlot(SeedBank *theBank, int theIndex)
{
    if (theBank->mApp->IsSlotMachineLevel())
        return 0;

    Board *aBoard = theBank->mBoard;
    if (theBank->mApp->IsCoopMode() || theBank->mApp->mGameMode == GAMEMODE_MP_VS)
    {
        GamepadControls *aControls = aBoard->mSeedBank[0] == theBank
            ? aBoard->mGamepadControls[0]
            : aBoard->mGamepadControls[1];
        if (aControls->mGamepadIndex != -1 && theIndex == aControls->mSelectedSeedIndex)
            return 1;
        return 0;
    }

    GamepadControls *aFirst = aBoard->mGamepadControls[0];
    if (aFirst->mGamepadIndex != -1 && theIndex == aFirst->mSelectedSeedIndex)
        return 1;
    GamepadControls *aSecond = aBoard->mGamepadControls[1];
    if (aSecond->mGamepadIndex != -1 && theIndex == aSecond->mSelectedSeedIndex)
        return 2;
    return 0;
}

} // namespace

int SeedBank::GetShopPacketPositionX(int theIndex)
{
    if (mApp->IsSlotMachineLevel())
        return 59 * theIndex + 247;
    if (!mApp->mBoard->HasConveyorBeltSeedBank(false))
        return 59 * theIndex + 85;
    if (mApp->IsCoopMode())
        return 50 * theIndex + 10;
    return 50 * theIndex + 91;
}

void SeedBank::InitShopSeedBank()
{
    for (int i = 0; i < kShopPacketCount; i++)
    {
        mShopSeedPackets[i].mSeedBank = this;
        mShopSeedPackets[i].mStoreItemType = i + 10;
        mShopSeedPackets[i].mSeedType = SEED_NONE;
        mShopSeedPackets[i].mPacketOffsetX = GetShopPacketPositionX(i);
    }

    mShopSeedPackets[4].mSeedType = SEED_GATLINGPEA;
    mShopSeedPackets[5].mSeedType = SEED_JALAPENO;
}

bool SeedBank::MouseHitTest(int theX, int theY, HitResult *theHit)
{
    if (mWidth - 4 <= theX - mX || mNumPackets <= 0)
    {
        theHit->mObject = nullptr;
        theHit->mObjectType = OBJECT_TYPE_NONE;
        return false;
    }

    for (int i = 0; i < mNumPackets; i++)
    {
        if (mSeedPackets[i].MouseHitTest(theX - mX, theY - mY, theHit))
            return true;
    }

    theHit->mObject = nullptr;
    theHit->mObjectType = OBJECT_TYPE_NONE;
    return false;
}

void SeedBank::Move(int theX, int theY)
{
    mX = theX;
    mY = theY;
}

bool SeedBank::ContainsPoint(int theX, int theY)
{
    if (mX > theX)
        return false;
    if (theX >= mX + mWidth)
        return false;
    return mY <= theY && theY < mHeight + mY;
}

void SeedBank::RemoveSeed(int theIndex)
{
    if (theIndex >= mNumPackets)
        return;
    if (mSeedPackets[theIndex].mPacketType == SEED_NONE)
        return;

    // Slide every later packet down one slot. The last occupied slot is cleared.
    // Spacing between neighbours stays 51.
    for (int i = theIndex; i < mNumPackets; i++)
    {
        SeedPacket *aPacket = &mSeedPackets[i];
        if (aPacket->mPacketType == SEED_NONE)
            return;

        if (i == mNumPackets - 1)
        {
            aPacket->mOffsetY = 0;
            aPacket->mPacketType = SEED_NONE;
        }
        else
        {
            SeedPacket *aNext = &mSeedPackets[i + 1];
            aPacket->mOffsetY = aNext->mOffsetY + 51;
            aPacket->mPacketType = aNext->mPacketType;
        }
        aPacket->mRefreshCounter = 0;
        aPacket->mRefreshTime = 0;
        aPacket->mRefreshing = false;
        aPacket->mActive = true;
    }
}

int SeedBank::GetNumSeedsOnConveyorBelt()
{
    if (mNumPackets <= 0)
        return 0;
    if (mSeedPackets[0].mPacketType == SEED_NONE)
        return 0;

    for (int i = 1; i < mNumPackets; i++)
    {
        if (mSeedPackets[i].mPacketType == SEED_NONE)
            return i;
    }
    return mNumPackets;
}

void SeedBank::AddSeed(SeedType theSeedType, bool thePlaceImmediately)
{
    int anIndex = GetNumSeedsOnConveyorBelt();
    if (anIndex == mNumPackets)
        return;

    SeedPacket *aPacket = &mSeedPackets[anIndex];
    aPacket->mPacketType = theSeedType;
    aPacket->mSeedBank = this;
    aPacket->mActive = true;
    aPacket->mOffsetY = -51 * anIndex + 515;
    aPacket->mRefreshCounter = 0;
    aPacket->mRefreshTime = 0;
    aPacket->mRefreshing = false;

    if (mApp->IsCoopMode())
        aPacket->mOffsetY = -51 * anIndex + 341;
    if (thePlaceImmediately)
        aPacket->mOffsetY = 0;

    if (anIndex > 0 && aPacket->mOffsetY < mSeedPackets[anIndex - 1].mOffsetY)
        aPacket->mOffsetY = mSeedPackets[anIndex - 1].mOffsetY + 40;
}

int SeedBank::GetNumVisibleSeedsOnConveyorBelt()
{
    if (mNumPackets <= 0)
        return mNumPackets;

    int aVisibleEdge = mApp->IsCoopMode() ? 313 : 501;
    for (int i = 0; i < mNumPackets; i++)
    {
        if (mSeedPackets[i].mPacketType == SEED_NONE)
            return i;
        if (aVisibleEdge <= mSeedPackets[i].mOffsetY)
            return i > 0 ? i - 1 : 0;
    }
    return mNumPackets;
}

int SeedBank::CountOfTypeOnConveyorBelt(SeedType theSeedType)
{
    if (mNumPackets <= 0)
        return 0;

    int aCount = 0;
    for (int i = 0; i < mNumPackets; i++)
    {
        if (mSeedPackets[i].mPacketType == theSeedType)
            ++aCount;
    }
    return aCount;
}

void SeedBank::UpdateConveyorBelt()
{
    ++mConveyorBeltCounter;
    if ((mConveyorBeltCounter & 3) != 0)
        return;

    for (int i = 0; i < mNumPackets; i++)
    {
        if (mSeedPackets[i].mOffsetY > 0)
            --mSeedPackets[i].mOffsetY;
    }
    mBoard->UpdateToolTip();
}

void SeedBank::UpdateWidth()
{
    mNumPackets = mBoard->GetNumSeedsInBank(mIsZombie);
    mWidth = IMAGE_SEEDBANK->GetWidth() + mBoard->GetSeedBankExtraWidth();

    bool aRightBank = mBoard->mSeedBank[0] != this;
    for (int i = 0; i < mNumPackets; i++)
        mSeedPackets[i].mX = mBoard->GetSeedPacketPositionX(i, aRightBank, false);
}

void SeedBank::RefreshAllPackets()
{
    if (mNumPackets <= 0)
        return;
    if (mSeedPackets[0].mPacketType == SEED_NONE)
        return;

    for (int i = 0; i < mNumPackets; i++)
    {
        if (mSeedPackets[i].mRefreshing)
        {
            mSeedPackets[i].mRefreshCounter = 0;
            mSeedPackets[i].mRefreshing = false;
            mSeedPackets[i].Activate();
            mSeedPackets[i].FlashIfReady();
        }

        if (i + 1 >= mNumPackets)
            return;
        if (mSeedPackets[i + 1].mPacketType == SEED_NONE)
            return;
    }
}

SeedBank::SeedBank(bool theIsZombie)
{
    SyncBlockInfo aBlock = MakeSyncBlock(mNumPackets, mIsZombie);
    mSyncBlocks.push_back(aBlock);
    memset(aBlock.mData, 0, aBlock.mSize);

    mWidth = IMAGE_SEEDBANK->GetWidth();
    mHeight = IMAGE_SEEDBANK->GetHeight();
    mIsZombie = theIsZombie;
    mNumPackets = 0;
    mConveyorBeltCounter = 0;
    mCutSceneDarken = 255;

    for (int i = 0; i < 10; i++)
    {
        mSeedPackets[i].mSeedBank = this;
        for (unsigned int j = 0; j < mSeedPackets[i].mSyncBlocks.size(); j++)
            mSyncBlocks.push_back(mSeedPackets[i].mSyncBlocks[j]);
        mSeedPackets[i].mSyncBlocks.clear();
    }

    InitShopSeedBank();
    mToolTipWidget = new ToolTipWidget();
}

void SeedBank::Draw(Sexy::Graphics *theGraphics)
{
    if (mBoard->mCutScene != nullptr && mBoard->mCutScene->IsBeforePreloading())
        return;

    // No pad, and not both pads connected: a controller tip sits above the
    // bank while the player is choosing seeds. The shop itself is drawn later.
    bool aPadsOn = BothGamepadsOn(mApp);
    bool aHasPad = mApp->HasGamepad() != 0;
    GamepadControls *aControls = mBoard->mGamepadControls[0];
    bool aCanShop = mApp->CanShopLevel();
    bool aSelectingSeeds = aControls->mGamepadState == BaseGamepadControls::MOVEMENT_STATE_SELECT_SEED;
    bool aShowShop = aCanShop && (aSelectingSeeds || aControls->mIsInShopSeedBank || mBoard->mMouseShopOpen);
    if (!aHasPad && !aPadsOn && aCanShop && aSelectingSeeds)
    {
        DrawTip(theGraphics, "[GAMEPAD_CONTROLLER_TIP_1]", 220, -12);
    }

    if ((aHasPad || aPadsOn) && aCanShop)
    {
        DrawTip(theGraphics, aControls->mIsInShopSeedBank
            ? "[GAMEPAD_SHOPTIP_2]" : "[GAMEPAD_SHOPTIP_1]", 220, -9);
    }

    float aSavedTransX = theGraphics->mTransX;
    float aSavedTransY = theGraphics->mTransY;
    float aSavedScaleX = theGraphics->mScaleX;
    float aSavedScaleY = theGraphics->mScaleY;
    float aSavedScaleOrigX = theGraphics->mScaleOrigX;
    float aSavedScaleOrigY = theGraphics->mScaleOrigY;
    Sexy::TRect<int> aSavedClip = theGraphics->mClipRect;
    Sexy::Color aSavedColor = theGraphics->GetColor();
    bool aSavedColorize = theGraphics->GetColorizeImages();

    // Versus shrinks the bank to 0.9 and parks it against the fullscreen rect.
    // A wide display uses the rect's left edge; a 4:3 display uses -80.
    if (mApp->mGameMode == GAMEMODE_MP_VS)
    {
        theGraphics->mTransX = 0.0f;
        theGraphics->mTransY = 0.0f;
        SexyTransform2D aTransform;
        aTransform.Scale(0.9f, 0.9f);
        int aOriginX = -80;
        if (mApp->mPlatformDriverOrQueue->mWideScreen)
            aOriginX = LawnApp::FULLSCREEN_RECT.mX;
        aTransform.Translate((float)(mX - aOriginX), (float)(mY - LawnApp::FULLSCREEN_RECT.mY));
        theGraphics->PushTransform(aTransform, true);
    }

    // Before the match the bank is drawn in board space, so the camera comes off.
    bool aPlaying = mApp->mGameScene == SCENE_PLAYING;
    int aBoardX = mBoard->mX;
    int aBoardY = mBoard->mY;
    if (!aPlaying)
    {
        theGraphics->mTransX -= (float)aBoardX;
        theGraphics->mTransY -= (float)aBoardY;
    }

    if (mApp->IsSlotMachineLevel())
    {
        theGraphics->DrawImage(IMAGE_SUNBANK, 0, 0);
    }
    else if (mBoard->HasConveyorBeltSeedBank(mIsZombie))
    {
        if (mApp->IsCoopMode())
        {
            theGraphics->DrawImage(IMAGE_CONVEYORBELT_BACKDROP_COOP, 0, 0);
            theGraphics->DrawImageCel(IMAGE_CONVEYORBELT_COOP, 7, 63, 0, ConveyorCel(mConveyorBeltCounter));
            theGraphics->SetClipRect(7, 0, 313, 600);
        }
        else
        {
            theGraphics->DrawImage(IMAGE_CONVEYORBELT_BACKDROP, 83, 0);
            theGraphics->DrawImageCel(IMAGE_CONVEYORBELT, 90, 63, 0, ConveyorCel(mConveyorBeltCounter));
            theGraphics->SetClipRect(90, 0, 501, 600);
        }
    }
    else if (mApp->IsCoopMode())
    {
        theGraphics->DrawImage(IMAGE_SEEDBANK_COOP, 0, 0);
    }
    else if (mApp->mGameMode == GAMEMODE_MP_VS && mIsZombie)
    {
        theGraphics->DrawImage(IMAGE_ZOMBIE_SEEDBANK, 0, 0);
    }
    else
    {
        int aExtraWidth = mBoard->GetSeedBankExtraWidth();
        Sexy::TRect<int> aSrc(
            IMAGE_SEEDBANK->mWidth - aExtraWidth - 12,
            0,
            aExtraWidth + 12,
            IMAGE_SEEDBANK->mHeight);
        theGraphics->DrawImage(IMAGE_SEEDBANK, 0, 0);
        theGraphics->DrawImage(IMAGE_SEEDBANK, IMAGE_SEEDBANK->mWidth - 12, 0, aSrc);
    }

    // Three passes, then the held packet on top: background, middle, overlay.
    // The first matching holder fills the primary slot; the other fills the
    // secondary. A later match for the same holder replaces that slot.
    SeedPacket *aHeldPrimary = nullptr;
    SeedPacket *aHeldSecondary = nullptr;
    for (int i = 0; i < mNumPackets; i++)
    {
        SeedPacket *aPacket = &mSeedPackets[i];
        if (aPacket->mPacketType == SEED_NONE)
            continue;

        int aSlot = HeldSlot(this, i);
        if (aSlot == 1)
        {
            aHeldPrimary = aPacket;
            continue;
        }
        if (aSlot == 2)
        {
            aHeldSecondary = aPacket;
            continue;
        }

        if (!aPacket->BeginDraw(theGraphics))
            continue;
        aPacket->DrawBackground(theGraphics);
        aPacket->EndDraw(theGraphics);
    }

    for (int i = 0; i < mNumPackets; i++)
    {
        SeedPacket *aPacket = &mSeedPackets[i];
        if (aPacket->mPacketType == SEED_NONE)
            continue;
        if (aPacket == aHeldPrimary || aPacket == aHeldSecondary)
            continue;
        if (!aPacket->BeginDraw(theGraphics))
            continue;
        aPacket->DrawMiddle(theGraphics);
        aPacket->EndDraw(theGraphics);
    }

    for (int i = 0; i < mNumPackets; i++)
    {
        SeedPacket *aPacket = &mSeedPackets[i];
        if (aPacket->mPacketType == SEED_NONE)
            continue;
        if (aPacket == aHeldPrimary || aPacket == aHeldSecondary)
            continue;
        if (!aPacket->BeginDraw(theGraphics))
            continue;
        aPacket->DrawOverlay(theGraphics);
        aPacket->EndDraw(theGraphics);
    }

    theGraphics->mClipRect.mX -= 20;
    theGraphics->mClipRect.mY -= 20;
    theGraphics->mClipRect.mWidth += 20;
    theGraphics->mClipRect.mHeight += 30;
    if (aHeldPrimary != nullptr && aHeldPrimary->BeginDraw(theGraphics))
    {
        aHeldPrimary->Draw(theGraphics);
        aHeldPrimary->EndDraw(theGraphics);
    }
    if (aHeldSecondary != nullptr && aHeldSecondary->BeginDraw(theGraphics))
    {
        aHeldSecondary->Draw(theGraphics);
        aHeldSecondary->EndDraw(theGraphics);
    }
    theGraphics->ClearClipRect();

    // Use the TV two-row layout for both mouse and gamepad selection.
    if (aShowShop)
    {
        theGraphics->Translate(0, 96);
        theGraphics->DrawImage(IMAGE_SHOP_SEED_BANK, 0, -10);
        DrawShopPackets(theGraphics, this);
        DrawCoinCount(theGraphics);
        theGraphics->Translate(0, -96);
    }

    if (mToolTipWidget != nullptr && aShowShop
        && aControls->mIsInShopSeedBank && aControls->mSelectedShopSeedIndex <= 5)
    {
        Sexy::String aLabelKey;
        Sexy::String aTitleKey;
        StrFormat(&aLabelKey, "[SHOP_%d_LABEL]", aControls->mSelectedShopSeedIndex);
        StrFormat(&aTitleKey, "[SHOP_%d_TITLE]", aControls->mSelectedShopSeedIndex);
        mToolTipWidget->SetLabel(TodStringTranslate(aLabelKey));
        mToolTipWidget->SetTitle(TodStringTranslate(aTitleKey));
        mToolTipWidget->SetWarningText(Sexy::String());
        mToolTipWidget->CalculateSize();
        mToolTipWidget->mY = 174;
        mToolTipWidget->mVisible = true;
        mToolTipWidget->mX = 60 * aControls->mSelectedShopSeedIndex + 108 - mToolTipWidget->mWidth / 2;
        mToolTipWidget->Update();
        mToolTipWidget->Draw(theGraphics);
    }
    else if (mToolTipWidget != nullptr && aCanShop)
    {
        mToolTipWidget->mVisible = false;
        mToolTipWidget->Update();
        mToolTipWidget->Draw(theGraphics);
    }

    // The slot-machine cover follows the bank as the cutscene slides it in.
    if (mApp->IsSlotMachineLevel() && mCutSceneDarken > -IMAGE_SEEDBANK->GetHeight())
        theGraphics->DrawImage(IMAGE_SLOTMACHINE_OVERLAY, 189, -2);

    if (!mBoard->HasConveyorBeltSeedBank(false))
    {
        int aSun = mIsZombie ? mBoard->mDeathMoney : mBoard->mSunMoney1;
        if (mApp->IsCoopMode() && mBoard->mSeedBank[1] == this)
            aSun = mBoard->mSunMoney2;
        aSun = NonNegative(aSun);

        Sexy::String aSunCount;
        StrFormat(&aSunCount, "%d", aSun);
        Sexy::Color aSunColor(0, 0, 0);
        if (mBoard->mOutOfMoneyCounter > 0 && mBoard->mOutOfMoneyCounter % 20 <= 9)
            aSunColor = Sexy::Color(255, 0, 0);

        int aSunX = mIsZombie ? 408 : 38;
        TodDrawString(theGraphics, aSunCount, aSunX, 78, FONT_CONTINUUMBOLD14, aSunColor, kSunLabelJustify);

        if (mApp->IsTwinSunbankMode())
        {
            Sexy::String aTwinSun;
            StrFormat(&aTwinSun, "%d", NonNegative(mBoard->mSunMoney2));
            TodDrawString(theGraphics, aTwinSun, 644, 49, FONT_CONTINUUMBOLD14, aSunColor, kSunLabelJustify);
        }
    }

    if (!aPlaying)
    {
        theGraphics->mTransX += (float)aBoardX;
        theGraphics->mTransY += (float)aBoardY;
    }
    if (mApp->mGameMode == GAMEMODE_MP_VS)
        theGraphics->PopTransform();

    theGraphics->mTransX = aSavedTransX;
    theGraphics->mTransY = aSavedTransY;
    theGraphics->mScaleX = aSavedScaleX;
    theGraphics->mScaleY = aSavedScaleY;
    theGraphics->mScaleOrigX = aSavedScaleOrigX;
    theGraphics->mScaleOrigY = aSavedScaleOrigY;
    theGraphics->mClipRect = aSavedClip;
    theGraphics->SetColor(aSavedColor);
    theGraphics->SetColorizeImages(aSavedColorize);
}

SeedBank::~SeedBank()
{
    delete mToolTipWidget;
}
