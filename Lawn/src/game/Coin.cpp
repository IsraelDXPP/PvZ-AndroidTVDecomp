// Coin -- transcribed from libGameMain.so, then rewritten against the
// reconstructed headers (stage 2).
//
// The bodies below are the IDA decompilation with the register plumbing removed:
// every `this`-relative byte offset is the named member declared by Coin.h,
// GameObject.h or one of the other base headers, and every hand-written vtable
// slot call is a real virtual call. CoinType, CursorType, MessageStyle, SeedType,
// TodCurves and FoleyType are real enums in Sexy\ReconTypes.h, so their
// enumerators are used directly. CoinMotion, ParticleEffect, ReanimationType and
// AdviceType are still one-int surrogate structs there, so the values this file
// compares against are named in the anonymous namespace below.
//
// Coin lives in a `DataArray<Coin>` (Board.h's `mCoins`) and a DataArray slot is
// the item followed by the container's 32-bit id word, so the stride Board walks
// is sizeof(Coin) + 4 == 0xCC. CoinSlot names that pair, and the assertion on it
// is what keeps the stride in step with Coin.h's own sizeof check.

#include "Coin.h"
#include "DataArrayMethods.h"
#include "BaseGamepadControls.h"
#include "game/Board.h"
#include "NativeDisplay.h"
#include "CutScene.h"
#include "GameObject.h"
#include "GamepadControls.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnPlayerInfo.h"
#include "MessageWidget.h"
#include "PottedPlant.h"
#include "Reanimation.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "SeedPacket.h"
#include "SexyAppBase.h"
#include "ZenGarden.h"
#include <cstring>
#include <cmath>

namespace
{
// CoinMotion, ParticleEffect, ReanimationType and AdviceType are one-int
// surrogate structs in Sexy\ReconTypes.h, so their enumerators are not in scope.
// The names and values below are the ones PvZ\Lawn\Common\ConstEnums.h gives and
// the ones this binary compares against. They are plain ints, and the surrogates
// convert both ways, so they work as switch labels and as the arguments these
// bodies pass.
//
// The ten COIN_MOTION_* values are NOT repeated here: Sexy\ReconTypes.h:3144-3153
// names them at namespace scope with exactly these values (0..9, the order
// Coin::CoinInitialize's switch at 0x197E78 uses), and defining them a second
// time in this translation unit is C2872. Keep that header block.

// The coin body reanimations are ReanimationType enumerators.

// ParticleEffect is re-anchored by the reference the coin attaches to.

// AdviceType is an enum in the binary; these are the entries the coin uses.
// Sexy\ReconFreeFunctions.h declares two linker stubs spelled
// ADVICE_CLICKED_ON_SUN / _COIN, so the two local names carry an ID_ marker to
// keep them unambiguous.

// StoreItem; the chocolate counter lives in the profile's purchase table.

// ChallengeState is a one-int surrogate struct in Sexy\ReconTypes.h, so the one
// value this file compares against is named here (ConstEnums.h order).

// Dialogs has no enum in this tree yet; this is the one entry the coin uses.
constexpr int DIALOG_STORE = 6;
} // namespace

// SexyAppBase.h names LawnApp's word at +0x2E4 `mPlatformDriverOrQueue` and
// records that its type could not be recovered. Its byte at +96 is the
// wide-screen (landscape) flag the collected-sun destinations are chosen by, so
// it is read once here instead of spelling the offset at each site.
static bool IsWideScreen(LawnApp *theApp)
{
    return theApp->mPlatformDriverOrQueue->mWideScreen;
}

// ==========================================================================
// Coin::~Coin()
// @001963B4  48 bytes
// ==========================================================================
Coin::~Coin()
{
    // The transcription's first statement was a vptr store the compiler emits
    // itself, and its trailing explicit base-destructor call would destroy
    // mSyncBlocks a second time; both are gone. AttachmentDie takes the id by
    // reference, so the address of the slot is what the binary passes.
    AttachmentDie(mAttachmentID[0]);
    AttachmentDie(mAttachmentID[1]);
    AttachmentDie(mAttachmentID[2]);
}

// ==========================================================================
// Coin::IsMoney(void)
// @0019640C  14 bytes
// ==========================================================================
bool Coin::IsMoney()
{
    return (unsigned int)(mType - COIN_SILVER) <= 2;
}

// ==========================================================================
// Coin::IsSun(void)
// @0019641C  14 bytes
// ==========================================================================
bool Coin::IsSun()
{
    return (unsigned int)(mType - COIN_SUN) <= 2;
}

// ==========================================================================
// Coin::IsDoubleSun(void)
// @0019642C  12 bytes
// ==========================================================================
bool Coin::IsDoubleSun()
{
    return mType == COIN_COOP_DOUBLE_SUN;
}

// ==========================================================================
// Coin::IsDeath(void)
// @00196438  12 bytes
// ==========================================================================
bool Coin::IsDeath()
{
    return mType == COIN_VS_ZOMBIE_BRAIN;
}

// ==========================================================================
// Coin::IsPresentWithAdvice(void)
// @00196444  14 bytes
// ==========================================================================
bool Coin::IsPresentWithAdvice()
{
    return (unsigned int)(mType - COIN_PRESENT_MINIGAMES) <= 3;
}

// ==========================================================================
// Coin::GetColor(void)
// @00196454  186 bytes
// ==========================================================================
// The binary returns the colour through the hidden return pointer: IDA prints
// the destination as `this` and the receiver as the second argument, and the
// body writes the four colour words over the object's vptr. The signature in
// Coin.h is the by-value form the authority header declares.
Sexy::Color Coin::GetColor()
{
    if ((IsSun() || IsDeath() || IsMoney()) && mIsBeingCollected)
    {
        // ClampFloat(mCollectionDistance * 0.035f, 0.35f, 1.0f), inlined by the
        // binary: the alpha tracks how close the coin is to its destination.
        float aAlpha = mCollectionDistance * 0.035f;
        if (aAlpha > 1.0f)
            aAlpha = 1.0f;
        if (aAlpha < 0.35f)
            aAlpha = 0.35f;
        return Sexy::Color(255, 255, 255, (int)(aAlpha * 255.0f));
    }

    if (mFadeCount > 0)
    {
        int aAlpha = TodAnimateCurve(15, 0, mFadeCount, 255, 0, CURVE_LINEAR);
        return Sexy::Color(255, 255, 255, aAlpha);
    }

    return Sexy::Color::White;
}

// ==========================================================================
// Coin::GetFinalSeedPacketType(void)
// @00196524  42 bytes
// ==========================================================================
int Coin::GetFinalSeedPacketType()
{
    if (mApp->IsFirstTimeAdventureMode() != 0 && mBoard != nullptr && mBoard->mLevel <= 50)
        return mApp->GetAwardSeedForLevel(mBoard->mLevel);

    return -1;
}

// ==========================================================================
// Coin::FanOutCoins(CoinType,int)
// @00196550  188 bytes
// ==========================================================================
void Coin::FanOutCoins(CoinType theCoinType, int theNumCoins)
{
    // The coins leave the parent on a half-circle: the loop index runs 1..theNumCoins and
    // the angle is `i * PI / (theNumCoins + 1) + PI/2`, so the fan is symmetric.
    for (int i = 1; i <= theNumCoins; ++i)
    {
        Coin *aCoin = (Coin *)mBoard->AddCoin((int)(mPosX + 20.0f), (int)mPosY, theCoinType, COIN_MOTION_FROM_PRESENT);
        aCoin->mVelX = sinf((float)i * 3.1416f * (1.0f / (float)(theNumCoins + 1)) + 1.5708f) * 5.0f;
        aCoin->mVelY = cosf((float)i * 3.1416f * (1.0f / (float)(theNumCoins + 1)) + 1.5708f) * 5.0f;
    }
}

// ==========================================================================
// Coin::StartFade(void)
// @00196614  6 bytes
// ==========================================================================
void Coin::StartFade()
{
    mFadeCount = 15;
}

// ==========================================================================
// Coin::SetAutoCollect(bool,int)
// @0019661C  10 bytes
// ==========================================================================
void Coin::SetAutoCollect(bool theAutoCollect, int theAutoCollectAge)
{
    mAutoCollect = theAutoCollect && !IsSun() && !IsDoubleSun();
    mAutoCollectAge = theAutoCollectAge;
}

// ==========================================================================
// Coin::SetSunValue(int)
// @00196628  6 bytes
// ==========================================================================
void Coin::SetSunValue(int theSunValue)
{
    mCustomSunValue = theSunValue;
}

// ==========================================================================
// Coin::GetSunValue(void)
// @00196630  44 bytes
// ==========================================================================
int Coin::GetSunValue()
{
    if (mCustomSunValue != 0)
        return mCustomSunValue;

    // The binary folds COIN_SUN, COIN_COOP_DOUBLE_SUN and COIN_VS_ZOMBIE_BRAIN into
    // one `mType == COIN_SUN || mType - COIN_COOP_DOUBLE_SUN <= 1` test; the
    // mini-sun and VS-brain sizes the hook build adds are NOT handled here.
    if (mType == COIN_SMALLSUN)
        return 15;
    if (mType == COIN_LARGESUN)
        return 50;
    if (mType == COIN_SUN || (unsigned int)(mType - COIN_COOP_DOUBLE_SUN) <= 1)
        return 25;
    return mCustomSunValue;
}

// ==========================================================================
// Coin::GetSunScale(void)
// @0019665C  34 bytes
// ==========================================================================
float Coin::GetSunScale()
{
    // Only the two real sun sizes are scaled; COIN_SUN and COIN_COOP_DOUBLE_SUN
    // keep the neutral 1.0, exactly as the binary's two compares do.
    if (mType == COIN_SMALLSUN)
        return 0.5f;
    if (mType == COIN_LARGESUN)
        return 2.0f;
    return 1.0f;
}

// ==========================================================================
// Coin::GetCoinValue(CoinType)
// @00196680  22 bytes
// ==========================================================================
int Coin::GetCoinValue(CoinType theCoinType)
{
    switch (theCoinType)
    {
        case COIN_SILVER:
            return 1;
        case COIN_GOLD:
            return 5;
        case COIN_DIAMOND:
            return 100;
        default:
            return 0;
    }
}

// ==========================================================================
// Coin::PlayLaunchSound(void)
// @00196698  46 bytes
// ==========================================================================
void Coin::PlayLaunchSound()
{
    // The dump tests `(mType & ~0x10) == 3` first, which folds COIN_DIAMOND (3)
    // together with COIN_AWARD_PRESENT (19), then the two chocolate coins as
    // `mType - 23 <= 1`.
    if (mType == COIN_DIAMOND || mType == COIN_AWARD_PRESENT || mType == COIN_CHOCOLATE ||
        mType == COIN_AWARD_CHOCOLATE || mType == COIN_PRESENT_PLANT || IsPresentWithAdvice())
    {
        mApp->PlayFoley(FOLEY_CHIME);
    }
}

// ==========================================================================
// Coin::PlayCollectSound(void)
// @001966C8  154 bytes
// ==========================================================================
void Coin::PlayCollectSound()
{
    if (mType == COIN_USABLE_SEED_PACKET)
    {
        mApp->PlaySample(SOUND_SEEDLIFT, true);
        return;
    }

    if (mType == COIN_SILVER || mType == COIN_GOLD)
    {
        mApp->PlayFoley(FOLEY_COIN);
        return;
    }

    if (mType == COIN_DIAMOND)
    {
        mApp->PlaySample(SOUND_DIAMOND, true);
        return;
    }

    if (mType == COIN_CHOCOLATE || mType == COIN_PRESENT_PLANT || IsPresentWithAdvice() ||
        mType == COIN_AWARD_PRESENT || mType == COIN_AWARD_CHOCOLATE)
    {
        mApp->PlayFoley(FOLEY_PRIZE);
        return;
    }

    if (IsSun() || IsDoubleSun())
    {
        mApp->PlayFoley(FOLEY_SUN);
        return;
    }

    if (IsDeath())
        mApp->PlayFoley(FOLEY_SLURP);
}

// ==========================================================================
// Coin::Die(void)
// @00196770  36 bytes
// ==========================================================================
void Coin::Die()
{
    mDead = true;
    AttachmentDie(mAttachmentID[0]);
    AttachmentDie(mAttachmentID[1]);
    AttachmentDie(mAttachmentID[2]);
}

// ==========================================================================
// Coin::ScoreCoin(void)
// @00196794  178 bytes
// ==========================================================================
void Coin::ScoreCoin()
{
    Die();

    if (IsSun())
        mBoard->AddSunMoney(GetSunValue(), mCollectedByPlayerIndex);

    // The allied double sun pays both players the same amount.
    if (IsDoubleSun())
    {
        int aSunValue = GetSunValue();
        mBoard->AddSunMoney(aSunValue, 0);
        mBoard->AddSunMoney(aSunValue, 1);
    }

    if (IsDeath())
    {
        mBoard->AddDeathMoney(GetSunValue());
    }
    else if (IsMoney())
    {
        int aCoinValue = Coin::GetCoinValue(mType);
        if (mApp->mPlayerInfo != nullptr)
            mApp->mPlayerInfo->AddCoins(aCoinValue);
        if (mBoard != nullptr)
            mBoard->mCoinsCollected += aCoinValue;
    }

    if (mType == COIN_DIAMOND && mBoard != nullptr)
        ++mBoard->mDiamondsCollected;
}

// ==========================================================================
// Coin::MouseHitTest(int,int,HitResult *,int)
// @00196848  506 bytes
// ==========================================================================
bool Coin::MouseHitTest(int theX, int theY, HitResult *theHitResult, int thePlayerIndex)
{
    // The hit box of a present hangs above the object origin, so it is lifted.
    // The dump folds COIN_PRESENT_PLANT and COIN_AWARD_PRESENT into one
    // `(mType & ~2) == 0x11` test.
    int aExtraY = 0;
    if (mType == COIN_PRESENT_PLANT || mType == COIN_AWARD_PRESENT || IsPresentWithAdvice())
        aExtraY = -20;

    // The challenge modes grow the box: Beghouled Twist widens it by 100 and adds
    // 40 to the height, Heavy Weapon uses a 20x30 box centred 25 right and 40 down
    // from the coin, and the VS brain coin uses a 100-wide box.
    int aHeight = 0;
    int aWidth = 0;
    if (mApp->IsWhackAZombieLevel() != 0)
    {
        aHeight = 30;
        aWidth = 15;
    }
    if (mType == COIN_SUN)
        aWidth = 100;

    if (mApp->mGameMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST)
    {
        aHeight += 40;
        aWidth += 100;
    }
    else if (mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
    {
        aHeight = 30;
        aWidth = 20;
        theX += 25;
        theY += 40;
    }
    else if (IsDeath())
    {
        aWidth = 100;
    }

    // theHitResult points at the caller's hit result: its first word is the object and its
    // second word the hit kind.
    bool aHit = false;
    if (!mDead && !mIsBeingCollected)
    {
        if (mType == COIN_USABLE_SEED_PACKET)
        {
            // A usable seed packet is picked up by grid cell, and a player who is
            // already carrying one cannot take a second.
            bool aAlreadyCarrying = mBoard->mGamepadControls[thePlayerIndex]->mCanPickUp &&
                mBoard->mCursorObject[thePlayerIndex]->mCursorType == CURSOR_TYPE_PLANT_FROM_USABLE_COIN;
            int aCenterX = (int)(mPosX + mWidth / 2);
            int aCenterY = (int)(mPosY + mHeight / 2);
            if (!aAlreadyCarrying &&
                mBoard->PixelToGridXKeepOnBoard(theX, theY) == mBoard->PixelToGridXKeepOnBoard(aCenterX, aCenterY) &&
                mBoard->PixelToGridYKeepOnBoard(theX, theY) == mBoard->PixelToGridYKeepOnBoard(aCenterX, aCenterY))
            {
                aHit = true;
            }
        }
        else if ((float)theX >= mPosX - (float)aWidth && (float)theX < mPosX + (float)mWidth + (float)aWidth &&
            (float)theY >= (float)aExtraY + mPosY - (float)aWidth &&
            (float)theY < mPosY + (float)mHeight + (float)aExtraY + (float)aWidth + (float)aHeight)
        {
            aHit = true;
        }
    }

    if (aHit)
    {
        // The two words of the caller's HitResult: the object, then the hit kind.
        theHitResult->mObject = this;
        theHitResult->mObjectType = (GameObjectType)3;
        return true;
    }

    theHitResult->mObject = nullptr;
    theHitResult->mObjectType = (GameObjectType)0;
    return false;
}

// ==========================================================================
// Coin::IsLevelAward(void)
// @00196A44  36 bytes
// ==========================================================================
bool Coin::IsLevelAward()
{
    return (unsigned int)(mType - COIN_FINAL_SEED_PACKET) <= 8 || (unsigned int)(mType - COIN_AWARD_MONEY_BAG) <= 4 ||
        mType == COIN_AWARD_CHOCOLATE || (unsigned int)(mType - COIN_VS_PLANT_TROPHY) <= 1;
}

// ==========================================================================
// Coin::UpdateFade(void)
// @00196A68  68 bytes
// ==========================================================================
void Coin::UpdateFade()
{
    // An endless level, the note and a non-award coin all keep fading; only when
    // none of those holds does the counter stop running.
    if (mApp->IsEndlessIZombie(mApp->mGameMode) || mApp->IsEndlessScaryPotter(mApp->mGameMode) || mType == COIN_NOTE ||
        !IsLevelAward())
    {
        if (--mFadeCount == 0)
            Die();
    }
}

// ==========================================================================
// Coin::PlayGroundSound(void)
// @00196AAC  60 bytes
// ==========================================================================
void Coin::PlayGroundSound()
{
    if (mType == COIN_GOLD)
    {
        mApp->PlayFoley(FOLEY_MONEYFALLS);
        return;
    }

    // The dump folds COIN_DIAMOND and COIN_AWARD_PRESENT into `(mType & ~0x10) == 3`
    // and the two chocolate coins into `mType - 23 <= 1`. None of those and no
    // present has a ground sound, so they leave through the plain epilogue.
    if (mType == COIN_DIAMOND || mType == COIN_AWARD_PRESENT || mType == COIN_CHOCOLATE ||
        mType == COIN_AWARD_CHOCOLATE || mType == COIN_PRESENT_PLANT || IsPresentWithAdvice())
    {
        return;
    }

    // This is the binary's tail branch, kept so the call is not silently dropped;
    // IsLevelAward only inspects mType and no caller reads its result.
    IsLevelAward();
}

// ==========================================================================
// Coin::UpdateCollected(void)
// @00196AE8  1276 bytes
// ==========================================================================
void Coin::UpdateCollected()
{
    // A sun flies to the left of the screen, or to the twin-sun destination when
    // it belongs to player 1; the double sun of the allied mode keeps a second
    // destination in mPrevPosX/mPrevPosY.
    float aDoubleSunDestX = IsWideScreen(mApp) ? 600.0f : 480.0f;
    float aDestX = 15.0f;
    float aDestY = 0.0f;

    if (IsSun())
    {
        if (mCollectedByPlayerIndex == 1)
        {
            aDestX = aDoubleSunDestX;
            aDoubleSunDestX = 0.0f;
        }
        else
        {
            aDoubleSunDestX = 0.0f;
        }
    }
    else if (mType == COIN_COOP_DOUBLE_SUN)
    {
        aDestX = 15.0f;
        aDestY = 0.0f;
    }
    else if (IsDeath())
    {
        aDestX = 760.0f;
        aDestY = 0.0f;
        aDoubleSunDestX = 0.0f;
    }
    else if (IsMoney())
    {
        aDestX = 39.0f;
        aDestY = 558.0f;
        aDoubleSunDestX = 0.0f;

        // The store and the zen garden each keep their own coin-bank corner, and
        // Crazy Dave's presence moves the target out of his way.
        if (mApp->GetDialog(DIALOG_STORE) != nullptr)
        {
            aDestX = 662.0f;
            aDestY = 546.0f;
        }
        else if (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN)
        {
            aDestX = 442.0f;
        }
        else if (mApp->mCrazyDaveState != CRAZY_DAVE_OFF)
        {
            aDestX = 442.0f;
        }
    }
    else if (IsPresentWithAdvice())
    {
        aDestX = 35.0f;
        aDestY = 487.0f;
        aDoubleSunDestX = 0.0f;
    }
    else if (mType == COIN_AWARD_PRESENT || mType == COIN_PRESENT_PLANT)
    {
        // The two dropping presents open after a delay instead of flying anywhere.
        ++mDisappearCounter;
        if (mDisappearCounter >= 200)
            StartFade();
        return;
    }
    else if (!IsLevelAward())
    {
        if (mType == COIN_USABLE_SEED_PACKET)
            ++mDisappearCounter;
        return;
    }
    else
    {
        aDestX = (float)(400 - mWidth / 2);
        aDestY = (float)(200 - mHeight / 2);
        ++mDisappearCounter;
    }

    if (IsLevelAward())
    {
        // A level award grows towards the middle of the screen instead of shrinking
        // into the counter.
        mScale = TodAnimateCurveFloat(0, 400, mDisappearCounter, 1.01f, 2.0f, CURVE_EASE_IN_OUT);
        mPosX = TodAnimateCurveFloat(0, 350, mDisappearCounter, mCollectX, aDestX, CURVE_EASE_OUT);
        mPosY = TodAnimateCurveFloat(0, 350, mDisappearCounter, mCollectY, aDestY, CURVE_EASE_OUT);
        return;
    }

    float aDeltaX = fabsf(mPosX - aDestX);
    float aDeltaY = fabsf(mPosY - aDestY);
    if (mPosX > aDestX)
        mPosX -= aDeltaX * 0.047619f;
    else if (mPosX < aDestX)
        mPosX += aDeltaX * 0.047619f;
    if (mPosY > aDestY)
        mPosY -= aDeltaY * 0.047619f;
    else if (mPosY < aDestY)
        mPosY += aDeltaY * 0.047619f;

    if (mType == COIN_COOP_DOUBLE_SUN)
    {
        float aDoubleSunDeltaX = fabsf(mPrevPosX - aDoubleSunDestX);
        float aDoubleSunDeltaY = fabsf(mPrevPosY);
        if (mPrevPosX > aDoubleSunDestX)
            mPrevPosX -= aDoubleSunDeltaX * 0.047619f;
        else if (mPrevPosX < aDoubleSunDestX)
            mPrevPosX += aDoubleSunDeltaX * 0.047619f;

        if (mPrevPosY > 0.0f)
            mPrevPosY -= aDoubleSunDeltaY * 0.047619f;
        // The binary really does test mPosY on this second branch, not mPrevPosY.
        else if (mPosY < 0.0f)
            mPrevPosY += aDoubleSunDeltaY * 0.047619f;
    }

    mCollectionDistance = sqrtf(aDeltaX * aDeltaX + aDeltaY * aDeltaY);

    if (IsPresentWithAdvice())
    {
        if (mCollectionDistance >= 15.0f)
            return;

        // The unlock advice is already on screen: retire the coin unless the advice
        // being displayed is this very message.
        if (mBoard->mHelpDisplayed[ADVICE_NEED_ACHIVEMENT_EARNED])
        {
            if (mBoard->mHelpIndex != ADVICE_NEED_ACHIVEMENT_EARNED || !mBoard->mAdvice->IsBeingDisplayed())
                Die();
            return;
        }

        Sexy::String aText;
        switch (mType)
        {
            case COIN_PRESENT_MINIGAMES:
                TodStringTranslate(&aText, "[UNLOCKED_MINIGAMES]");
                break;
            case Present32:
                TodStringTranslate(&aText, "[UNLOCKED_PUZZLE_MODE]");
                break;
            case Present1024:
                TodStringTranslate(&aText, "[UNLOCKED_SURVIVAL_MODE]");
                break;
            case COIN_PRESENT_PUZZLE_MODE:
                TodStringTranslate(&aText, "[UNLOCKED_COOP_CHALLENGES]");
                break;
            default:
                return;
        }
        mBoard->DisplayAdvice(aText, MESSAGE_STYLE_HINT_TALL_UNLOCKMESSAGE, ADVICE_NEED_ACHIVEMENT_EARNED);
        return;
    }

    // Otherwise the coin shrinks as it closes on its destination and is scored
    // once it is close enough.
    float aScoringDistance = 8.0f;
    if (IsMoney())
        aScoringDistance = 12.0f;

    float aDistance = mCollectionDistance;
    if (aDistance < aScoringDistance)
    {
        ScoreCoin();
        aDistance = mCollectionDistance;
    }

    // ClampFloat(aDistance * 0.05f, 0.5f, 1.0f), inlined by the binary.
    float aScale = aDistance * 0.05f;
    if (aScale > 1.0f)
        aScale = 1.0f;
    if (aScale < 0.5f)
        aScale = 0.5f;
    mScale = aScale;
    mScale *= GetSunScale();
}

// ==========================================================================
// Coin::Collect(int)
// @0019701C  1990 bytes
// ==========================================================================
void Coin::Collect(int thePlayerIndex)
{
    if (mDead)
        return;

    PlayCollectSound();

    // The coin remembers where it was taken so the collection animation can
    // interpolate from there, and which player took it.
    mIsBeingCollected = true;
    mCollectX = mPosX;
    mCollectY = mPosY;
    mCollectedByPlayerIndex = mApp->IsTwinSunbankMode() ? thePlayerIndex : 0;

    // In an endless level an award coin ends the level as soon as it is taken.
    bool aEndsLevel =
        (mApp->IsEndlessIZombie(mApp->mGameMode) || mApp->IsEndlessScaryPotter(mApp->mGameMode)) && IsLevelAward();

    if (mType == COIN_PRESENT_PLANT || mType == COIN_AWARD_PRESENT)
    {
        if (mApp->mZenGarden->IsZenGardenFull(false))
        {
            Sexy::String aText;
            TodStringTranslate(&aText, "[DIALOG_ZEN_GARDEN_FULL]");
            mBoard->DisplayAdvice(aText, MESSAGE_STYLE_HINT_FAST, ADVICE_NONE);
        }
        else
        {
            ++mBoard->mPottedPlantsCollected;
            Sexy::String aText;
            TodStringTranslate(&aText, "[ADVICE_FOUND_PLANT]");
            mBoard->DisplayAdvice(aText, MESSAGE_STYLE_HINT_FAST, ADVICE_NONE);
            mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_PRESENT_PICKUP);
            mApp->mZenGarden->AddPottedPlant(&mPottedPlantSpec);
        }

        mDisappearCounter = 0;
        mFadeCount = 0;
        if (aEndsLevel)
        {
            AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_AWARD_PICKUP_ARROW, nullptr);
            mBoard->FadeOutLevel();
        }
        return;
    }

    switch (mType)
    {
        case COIN_PRESENT_MINIGAMES:
            mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_PRESENT_PICKUP);
            mDisappearCounter = 0;
            mFadeCount = 0;
            AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_AWARD_PICKUP_ARROW, nullptr);
            mApp->mPlayerInfo->SetFlag(16, 1);
            return;
        case COIN_PRESENT_PUZZLE_MODE:
            mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_PRESENT_PICKUP);
            mDisappearCounter = 0;
            mFadeCount = 0;
            AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_AWARD_PICKUP_ARROW, nullptr);
            mApp->mPlayerInfo->SetFlag(0x4000, 1);
            return;
        case Present32:
            mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_PRESENT_PICKUP);
            mDisappearCounter = 0;
            mFadeCount = 0;
            AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_AWARD_PICKUP_ARROW, nullptr);
            mApp->mPlayerInfo->SetFlag(32, 1);
            return;
        case Present1024:
            mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_PRESENT_PICKUP);
            mDisappearCounter = 0;
            mFadeCount = 0;
            AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_AWARD_PICKUP_ARROW, nullptr);
            mApp->mPlayerInfo->SetFlag(1024, 1);
            return;
        default:
            break;
    }

    if (mType == COIN_CHOCOLATE || mType == COIN_AWARD_CHOCOLATE)
    {
        ++mBoard->mChocolateCollected;
        mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_PRESENT_PICKUP);

        // The profile counts the chocolates it has seen; the first one past 1000
        // shows the advice that explains what chocolate is for.
        if (mApp->mPlayerInfo->mPurchases[STORE_ITEM_CHOCOLATE] < 1000)
        {
            Sexy::String aText;
            TodStringTranslate(&aText, "[ADVICE_FOUND_CHOCOLATE]");
            mBoard->DisplayAdvice(aText, MESSAGE_STYLE_HINT_TALL_FAST, ADVICE_NONE);
            mApp->mPlayerInfo->mPurchases[STORE_ITEM_CHOCOLATE] = 1001;
        }
        else
        {
            ++mApp->mPlayerInfo->mPurchases[STORE_ITEM_CHOCOLATE];
        }

        mDisappearCounter = 0;
        StartFade();
        if (aEndsLevel)
        {
            AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_AWARD_PICKUP_ARROW, nullptr);
            mBoard->FadeOutLevel();
        }
        return;
    }

    if (IsLevelAward())
    {
        if (aEndsLevel)
        {
            // The last award of a finished endless level: burst it, hand out the
            // coin it stands for, then let the level fade.
            if (mType == COIN_AWARD_BAG_DIAMOND)
            {
                mApp->PlaySample(SOUND_DIAMOND, true);
                FanOutCoins(COIN_DIAMOND, 1);
                StartFade();
            }
            else if (mType == COIN_AWARD_MONEY_BAG)
            {
                mApp->PlayFoley(FOLEY_COIN);
                FanOutCoins(COIN_GOLD, 5);
                StartFade();
            }
        }
        else if (mApp->IsScaryPotterLevel() != 0)
        {
            if (mType == COIN_TROPHY)
            {
                mApp->PlayFoley(FOLEY_COIN);
                FanOutCoins(COIN_GOLD, 5);
            }
            else if (mType == COIN_AWARD_MONEY_BAG)
            {
                mApp->PlayFoley(FOLEY_COIN);
                FanOutCoins(COIN_GOLD, 2);
            }
        }
        else if (mApp->IsAdventureMode() != 0 && mBoard->mLevel == 50)
        {
            FanOutCoins(COIN_DIAMOND, 3);
        }
        else if (mType == COIN_AWARD_GOLD_SUNFLOWER)
        {
            FanOutCoins(COIN_DIAMOND, 5);
        }
        else if (mApp->IsFirstTimeAdventureMode() != 0 && mBoard->mLevel == 4)
        {
            mApp->PlaySample(SOUND_SHOVEL, true);
        }
        else if (!mApp->IsFirstTimeAdventureMode() ||
            (mBoard->mLevel != 24 && mBoard->mLevel != 34 && mBoard->mLevel != 44))
        {
            // A trophy or a money bag bursts into coins before the level fades; the
            // remaining awards only get their two pickup sounds.
            if (mType == COIN_TROPHY)
            {
                mApp->PlaySample(SOUND_DIAMOND, true);
                FanOutCoins(COIN_DIAMOND, 1);
            }
            else if (mType == COIN_AWARD_MONEY_BAG)
            {
                mApp->PlayFoley(FOLEY_COIN);
                FanOutCoins(COIN_GOLD, 5);
            }
            else
            {
                mApp->PlaySample(SOUND_SEEDLIFT, true);
                mApp->PlaySample(SOUND_TAP2, true);
            }
        }
        else
        {
            mApp->PlaySample(SOUND_TAP2, true);
        }

        mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_STARBURST);
        mBoard->FadeOutLevel();
        AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_SEED_PACKET, nullptr);
        AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_AWARD_PICKUP_ARROW, nullptr);
        AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_COIN_PICKUP_ARROW, nullptr);

        if (mType == COIN_NOTE)
        {
            mApp->AddTodParticle(mPosX + 30.0f, mPosY + 30.0f, mRenderOrder + 1, PARTICLE_PRESENT_PICKUP);
            StartFade();
        }
        else if (!aEndsLevel && mApp->Is3DAccelerated() != 0)
        {
            // On a 3D-accelerated platform the seed packet gives off a pickup effect
            // from the middle of the sprite.
            TodParticleSystem *aParticle = mApp->AddTodParticle(
                mWidth / 2 + mPosX, mHeight / 2 + mPosY, mRenderOrder - 1, PARTICLE_SEED_PACKET_PICKUP);
            AttachParticle(mAttachmentID[0], aParticle, (float)(mWidth / 2), (float)(mHeight / 2));
        }

        mDisappearCounter = 0;
        return;
    }

    if (mType == COIN_USABLE_SEED_PACKET)
    {
        // The coin is picked up by whoever is not already carrying a packet.
        if (mBoard->mCursorObject[thePlayerIndex]->mCursorType != CURSOR_TYPE_PLANT_FROM_USABLE_COIN)
        {
            // The board hands back a BaseGamepadControls; the usable seed packet is
            // only understood by the concrete GamepadControls, which is what the
            // binary's own __dynamic_cast checks for here.
            GamepadControls *aGamepadControls = mBoard->GetGamepadControlsByPlayerIndex(thePlayerIndex);
            aGamepadControls->PickupUsableSeedPacket(this);
            mFadeCount = 0;
            mGroundY = (int)mPosY;
        }
    }
    else
    {
        if (IsMoney() && mBoard != nullptr)
            mBoard->ShowCoinBank();

        mFadeCount = 0;

        if ((IsSun() || IsDoubleSun()) && mBoard != nullptr)
        {
            if (!mBoard->HasConveyorBeltSeedBank(false))
            {
                // Flash every packet that the sun about to arrive would make
                // affordable for its owner.
                SeedBank *aSeedBank = mBoard->mSeedBank[0];
                for (int i = 0; i < aSeedBank->mNumPackets; ++i)
                {
                    SeedPacket *aSeedPacket = &aSeedBank->mSeedPackets[i];
                    int aCost = mBoard->GetCurrentPlantCost(aSeedPacket->mPacketType, aSeedPacket->mImitaterType);
                    int aPlayerIndex = aSeedPacket->GetPlayerIndex();
                    int aSunMoney = aPlayerIndex == 0 ? mBoard->mSunMoney1 : mBoard->mSunMoney2;
                    int aSunAfter = mBoard->CountSunBeingCollected(aPlayerIndex) + aSunMoney - aCost;
                    if (aSunAfter >= 0 && aSunAfter < GetSunValue())
                        aSeedPacket->FlashIfReady();
                }

                // The fog of a fog level hides the coin bank, so the sun is drawn above
                // everything instead.
                if (mBoard->StageHasFog() != 0)
                    mRenderOrder = Board::MakeRenderOrder(RenderLayer(800000), 0, 2);
            }
        }

        AttachmentDetachCrossFadeParticleType(mAttachmentID[0], PARTICLE_COIN_PICKUP_ARROW, nullptr);

        // The first coin of level 1-2 explains itself.
        if (mApp->IsFirstTimeAdventureMode() != 0 && mBoard != nullptr && mBoard->mLevel == 11 &&
            (unsigned int)(mType - COIN_SILVER) <= 1)
        {
            Sexy::String aText;
            TodStringTranslate(&aText, "[ADVICE_CLICKED_ON_COIN]");
            mBoard->DisplayAdvice(aText, MESSAGE_STYLE_HINT_FAST, ADVICE_ID_CLICKED_ON_COIN);
        }
    }
}

// ==========================================================================
// Coin::TryAutoCollectAfterLevelAward(void)
// @00197814  60 bytes
// ==========================================================================
void Coin::TryAutoCollectAfterLevelAward()
{
    // Sun is collected through player input, including at the end of a level.
    if ((IsMoney() && mCoinMotion != COIN_MOTION_FROM_PRESENT) || mType == COIN_PRESENT_PLANT ||
        mType == COIN_CHOCOLATE || IsPresentWithAdvice())
    {
        Collect(0);
    }
}

// ==========================================================================
// Coin::MouseDown(int,int,int)
// @00197850  164 bytes
// ==========================================================================
void Coin::MouseDown(int theX, int theY, int theClickCount)
{
    // theX and theY are the click position; the binary never reads them.
    // A click collects the coin while the level is running and the board is live.
    if (mBoard != nullptr && !mBoard->mPaused && mApp->mGameScene == SCENE_PLAYING && !mIsBeingCollected &&
        theClickCount >= 0)
    {
        // The binary re-reads the flag it has just tested, so this click is always
        // attributed to player 0.
        if (!mIsBeingCollected)
        {
            PlayCollectSound();
            Collect(0);
            if (mApp->IsFirstTimeAdventureMode() != 0 && mBoard->mLevel == 1)
            {
                Sexy::String aText;
                TodStringTranslate(&aText, "[ADVICE_CLICKED_ON_SUN]");
                mBoard->DisplayAdvice(aText, MESSAGE_STYLE_TUTORIAL_LEVEL1_STAY, ADVICE_ID_CLICKED_ON_SUN);
            }
        }
    }
}

// ==========================================================================
// Coin::GamepadCursorOver(int)
// @00197910  248 bytes
// ==========================================================================
void Coin::GamepadCursorOver(int thePlayerIndex)
{
    // The gamepad cursor only picks a coin up while the level is running.
    if (mBoard != nullptr && !mBoard->mPaused && mApp->mGameScene == SCENE_PLAYING && !mDead)
    {
        if (mType == COIN_USABLE_SEED_PACKET)
        {
            Collect(thePlayerIndex);
        }
        else if (mType != COIN_COOP_DOUBLE_SUN && !mIsBeingCollected && mCoinMotion != COIN_MOTION_FROM_NEAR_CURSOR &&
            (!IsSun() || mScale >= GetSunScale()))
        {
            // The coin leaves the board and starts flying towards the cursor.
            mPlayerIndex = thePlayerIndex;
            mCoinMotion = COIN_MOTION_FROM_NEAR_CURSOR;
            mCursorChaseSpeed = 0.0f;
            if (IsSun())
                mScale = GetSunScale();
            if (mApp->IsFirstTimeAdventureMode() != 0 && mBoard->mLevel == 1)
            {
                Sexy::String aText;
                TodStringTranslate(&aText, "[ADVICE_CLICKED_ON_SUN]");
                mBoard->DisplayAdvice(aText, MESSAGE_STYLE_TUTORIAL_LEVEL1_STAY, ADVICE_ID_CLICKED_ON_SUN);
            }
        }
    }
}

// ==========================================================================
// Coin::CoinGetsBouncyArrow(void)
// @00197A24  70 bytes
// ==========================================================================
bool Coin::CoinGetsBouncyArrow()
{
    if (IsLevelAward())
        return true;

    // Only the silver and gold coins of level 1-2 get the bouncing arrow, and
    // only the first one of the level.
    return ((unsigned int)(mType - COIN_SILVER) <= 1 && mApp->IsFirstTimeAdventureMode() != 0 && mBoard != nullptr &&
               mBoard->mLevel == 11 && !mBoard->mDroppedFirstCoin) ||
        IsPresentWithAdvice();
}

// ==========================================================================
// Coin::CoinInitialize(int,int,CoinType,CoinMotion)
// @00197A6C  2514 bytes
// ==========================================================================
void Coin::CoinInitialize(int theX, int theY, CoinType theCoinType, CoinMotion theCoinMotion)
{
    // Board's coin factory only allocates a slot; every field the coin is born
    // with is set here, in the binary's own order.
    mType = theCoinType;
    mPosX = (float)theX;
    mPrevPosX = (float)theX;
    mPosY = (float)theY;
    mPrevPosY = (float)theY;
    mWidth = 60;
    mHeight = 60;
    mCoinMotion = theCoinMotion;
    mDead = false;
    mDisappearCounter = 0;
    mIsBeingCollected = false;
    mCollectedByPlayerIndex = 0;
    mFadeCount = 0;
    mCoinAge = 0;
    mAttachmentID[0] = 0;
    mAttachmentID[1] = 0;
    mAttachmentID[2] = 0;
    mCollectionDistance = 0.0f;
    mRenderOrder = Board::MakeRenderOrder(RenderLayer(600000), 0, 1);
    mUsableSeedType = (SeedType)-1;
    mNeedsBouncyArrow = false;
    mHasBouncyArrow = false;
    mHitGround = false;
    mTimesDropped = 0;
    mScale = 1.0f;
    mPottedPlantSpec.InitializePottedPlant(SEED_NONE);
    mPlayerIndex = -1;
    mDoubleSunGlowing = false;
    mDoubleSunHighlight0 = false;
    mDoubleSunHighlight1 = false;
    mCustomSunValue = 0;
    mAutoCollect = false;
    mAutoCollectAge = 80;

    // A sun and a VS brain coin wear a reanimation the app owns instead of a
    // sprite; the allied double sun is two tinted sun reanimations.
    if (IsSun())
    {
        Reanimation *aReanim = mApp->AddReanimation(0.0f, 0.0f, 0, REANIM_SUN);
        aReanim->SetPosition(mPosX + mWidth * 0.5f, mPosY + mHeight * 0.5f);
        aReanim->mLoopType = REANIM_LOOP;
        aReanim->SetAnimRate(6.0f);
        AttachReanim(mAttachmentID[0], aReanim, mWidth * 0.5f, mHeight * 0.5f);
    }
    else if (IsDeath())
    {
        Reanimation *aReanim = mApp->AddReanimation(0.0f, 0.0f, 0, REANIM_ZOMBIESUN);
        aReanim->SetPosition(mPosX + mWidth * 0.5f, mPosY + mHeight * 0.5f);
        aReanim->mLoopType = REANIM_LOOP;
        aReanim->SetAnimRate(6.0f);
        AttachReanim(mAttachmentID[0], aReanim, mWidth * 0.5f, mHeight * 0.5f);
    }
    else if (IsDoubleSun())
    {
        float aOffsetX = mWidth * 0.5f;
        float aOffsetY = mHeight * 0.5f;
        Reanimation *aReanim = mApp->AddReanimation(0.0f, 0.0f, 0, REANIM_SUN);
        aReanim->SetPosition(mPosX + aOffsetX, mPosY + aOffsetY);
        aReanim->mLoopType = REANIM_LOOP;
        aReanim->SetAnimRate(6.0f);
        aReanim->mColorOverride = Sexy::Color(249, 132, 6, 255);
        AttachReanim(mAttachmentID[0], aReanim, aOffsetX, aOffsetY);

        // The second sun trails the first by 30 to the right and 6 down.
        float aSecondX = aOffsetX + 30.0f;
        float aSecondY = aOffsetY + 6.0f;
        Reanimation *aSecondReanim = mApp->AddReanimation(0.0f, 0.0f, 0, REANIM_SUN);
        aSecondReanim->SetPosition(mPosX + aSecondX, mPosY + aSecondY);
        aSecondReanim->mLoopType = REANIM_LOOP;
        aSecondReanim->SetAnimRate(6.0f);
        aSecondReanim->mColorOverride = Sexy::Color(249, 132, 6, 255);
        AttachReanim(mAttachmentID[1], aSecondReanim, aSecondX, aSecondY);
    }
    else
    {
        // Every other coin is drawn by a reanimation of its own denomination,
        // shrunk by 10 or 15 and started at a random phase.
        switch (mType)
        {
            case COIN_SILVER:
            case COIN_GOLD:
            {
                mPosX -= 10.0f;
                mPosY -= 8.0f;
                Reanimation *aReanim =
                    mApp->AddReanimation(0.0f, 0.0f, 0, mType == COIN_SILVER ? REANIM_COIN_SILVER : REANIM_COIN_GOLD);
                aReanim->SetPosition(mPosX + 9.0f, mPosY + 9.0f);
                aReanim->mLoopType = REANIM_LOOP;
                aReanim->mAnimTime = Rand(0.99f);
                aReanim->SetAnimRate(RandRangeFloat(0.6f, 1.0f) * aReanim->mAnimRate);
                AttachReanim(mAttachmentID[0], aReanim, 9.0f, 9.0f);
                break;
            }
            case COIN_DIAMOND:
            {
                mPosX -= 15.0f;
                mPosY -= 15.0f;
                Reanimation *aReanim = mApp->AddReanimation(0.0f, 0.0f, 0, REANIM_DIAMOND);
                aReanim->SetPosition(mPosX - 3.0f, mPosY + 4.0f);
                aReanim->mLoopType = REANIM_LOOP;
                aReanim->mAnimTime = Rand(0.99f);
                aReanim->SetAnimRate(RandRangeFloat(50.0f, 80.0f));
                AttachReanim(mAttachmentID[0], aReanim, -3.0f, 4.0f);
                break;
            }
            default:
                break;
        }
    }

    // A stormy night draws every coin above the fog.
    if (mApp->IsStormyNightLevel() != 0)
        mRenderOrder = Board::MakeRenderOrder(RenderLayer(800000), 0, 0);

    // Every coin then takes the sprite (and the cel size) it is drawn with. The
    // plain cases all converge on the shared tail below the switch.
    Sexy::Image *aImage = nullptr;
    switch (mType)
    {
        case COIN_FINAL_SEED_PACKET:
            aImage = IMAGE_SEEDS;
            break;

        case COIN_TROPHY:
            aImage = IMAGE_TROPHY;
            break;
        case COIN_VS_PLANT_TROPHY:
            // The two multiplayer trophies are drawn at half the cel size.
            mWidth = (IMAGE_MP_PLANT_TROPHY_HI_RES)->GetCelWidth() / 2;
            mHeight = (IMAGE_MP_PLANT_TROPHY_HI_RES)->GetCelHeight() / 2;
            mRenderOrder = Board::MakeRenderOrder(RenderLayer(800000), 0, 1);
            break;

        case COIN_VS_ZOMBIE_TROPHY:
            mWidth = (IMAGE_MP_ZOMBIE_TROPHY_HI_RES)->GetCelWidth() / 2;
            mHeight = (IMAGE_MP_ZOMBIE_TROPHY_HI_RES)->GetCelHeight() / 2;
            mRenderOrder = Board::MakeRenderOrder(RenderLayer(800000), 0, 0);
            break;

        case COIN_AWARD_SILVER_SUNFLOWER:
        case COIN_AWARD_GOLD_SUNFLOWER:
            aImage = IMAGE_SUNFLOWER_TROPHY;
            break;

        case COIN_SHOVEL:
            aImage = IMAGE_SHOVEL;
            break;

        case COIN_CARKEYS:
            aImage = IMAGE_CARKEYS;
            break;

        case COIN_ALMANAC:
            aImage = IMAGE_ALMANAC;
            break;

        case COIN_VASE:
            aImage = IMAGE_SCARY_POT;
            break;

        case COIN_WATERING_CAN:
            aImage = IMAGE_WATERINGCAN;
            break;

        case COIN_TACO:
            aImage = IMAGE_TACO;
            break;

        case COIN_NOTE:
            aImage = IMAGE_ZOMBIE_NOTE_SMALL;
            break;

        case COIN_AWARD_MONEY_BAG:
        case COIN_AWARD_BAG_DIAMOND:
            aImage = IMAGE_MONEYBAG;
            break;

        case COIN_CHOCOLATE:
        case COIN_AWARD_CHOCOLATE:
            aImage = IMAGE_CHOCOLATE;
            break;
        case COIN_PRESENT_MINIGAMES:
        case COIN_PRESENT_PUZZLE_MODE:
        case Present32:
        case Present1024:
            aImage = IMAGE_PRESENT;
            break;

        case COIN_PRESENT_PLANT:
        case COIN_AWARD_PRESENT:
            // The present is the size of its own sprite and carries a potted plant.
            mWidth = (IMAGE_PRESENT)->GetCelWidth();
            mHeight = (IMAGE_PRESENT)->GetCelHeight();
            if (mApp->IsSurvivalEndless(mApp->mGameMode) != 0 || mApp->IsEndlessIZombie(mApp->mGameMode) != 0 ||
                mApp->IsEndlessScaryPotter(mApp->mGameMode) != 0 || mBoard->mBackground > 4)
            {
                // An endless level always takes a random seed.
                mPottedPlantSpec.InitializePottedPlant(mApp->mZenGarden->PickRandomSeedType());
            }
            else
            {
                // Otherwise the board's background picks the seed set: backgrounds 0..3
                // own an eight-seed set each and background 4 the six special seeds.
                static const int aBackgroundSeeds[4][8] = {
                    {0, 1, 2, 3, 7, 4, 5, 6},
                    {8, 9, 10, 11, 12, 13, 14, 15},
                    {16, 17, 18, 19, 20, 21, 22, 23},
                    {24, 25, 26, 27, 28, 29, 30, 31},
                };
                static const int aSpecialSeeds[6] = {32, 34, 35, 36, 37, 39};
                int aSeed = mBoard->mBackground == 4 ? TodPickFromArray(aSpecialSeeds, 6)
                                                     : TodPickFromArray(aBackgroundSeeds[mBoard->mBackground], 8);
                mPottedPlantSpec.InitializePottedPlant((SeedType)aSeed);
            }
            break;

        case COIN_USABLE_SEED_PACKET:
            // The usable seed packet keeps the seed sprite's cel size and is drawn in
            // the effect layer instead of the UI layer.
            mWidth = (IMAGE_SEEDS)->GetCelWidth();
            mHeight = (IMAGE_SEEDS)->GetCelHeight();
            mRenderOrder = Board::MakeRenderOrder(RenderLayer(800000), 0, 0);
            break;
    }

    // Every plain sprite case shares one tail: the cel size of the image it chose
    // and the effect layer it is drawn in.
    if (aImage != nullptr)
    {
        mWidth = aImage->GetCelWidth();
        mHeight = aImage->GetCelHeight();
        mRenderOrder = Board::MakeRenderOrder(RenderLayer(800000), 0, 0);
    }

    // The motion the coin was created with fixes its launch: where it starts,
    // how fast it travels and which line it comes to rest on.
    switch (mCoinMotion)
    {
        case COIN_MOTION_FROM_SKY:
            // A sun falls straight down onto a random line, except in Heavy Weapon
            // where everything lands at the bottom of the screen.
            mVelX = 0.0f;
            mVelY = 0.67f;
            mGroundY = Rand(250) + 300;
            if (mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
                mGroundY = 800;
            break;

        case COIN_MOTION_FROM_SKY_SLOW:
            mVelY = 0.33f;
            mVelX = 0.0f;
            mGroundY = Rand(250) + 300;
            break;
        case COIN_MOTION_FROM_PLANT:
        case COIN_MOTION_FROM_GRAVE_STONE:
            // The sun pops out of the plant and lands a little way below it; it keeps
            // growing from this small scale up to GetSunScale() as it falls.
            mVelY = -1.7f - Rand(1.7f);
            mVelX = Rand(0.8f) - 0.4f;
            mScale = 0.4f;
            mGroundY = (int)((mPosY + 15.0f) + (float)Rand(20));
            break;
        case COIN_MOTION_COIN:
            // A coin thrown by the player bounces and rolls: it comes to rest between
            // y=80 and y=521, and the drops that start higher up stop 30 short.
            mVelY = -3.0f - Rand(2.0f);
            mVelX = Rand(1.0f) - 0.5f;
            {
                int aGroundY = (int)((mPosY + 45.0f) + (float)Rand(20));
                if (aGroundY <= 521)
                {
                    if (aGroundY <= 79)
                        aGroundY = 80;
                    mGroundY = aGroundY;
                }
                else
                {
                    mGroundY = 521;
                }
            }
            if ((unsigned int)(mType - COIN_AWARD_SILVER_SUNFLOWER) <= 1)
            {
                mPosY -= 100.0f;
                mGroundY = (int)(mPosY + 45.0f);
                if (mGroundY > 400)
                    mGroundY = 400;
            }
            if ((unsigned int)(mType - COIN_FINAL_SEED_PACKET) <= 9)
                mGroundY -= 30;
            break;
        case COIN_MOTION_LAWNMOWER_COIN:
            // A coin the lawnmower throws is collected the moment it is created.
            mGroundY = 600;
            mVelY = 0.0f;
            mVelX = 0.0f;
            Collect(0);
            break;

        case COIN_MOTION_FROM_PRESENT:
            mGroundY = 600;
            mVelY = 0.0f;
            mVelX = 0.0f;
            mRenderOrder = Board::MakeRenderOrder(RenderLayer(800000), 0, 1);
            break;

        case COIN_MOTION_FROM_BOSS:
            // The boss throws its coin in from the right edge of the screen: the
            // silver sunflower trophy flies higher than the other awards do.
            mGroundY = mType == COIN_AWARD_SILVER_SUNFLOWER ? 170 : 285;
            mPosY = mType == COIN_AWARD_SILVER_SUNFLOWER ? 130.0f : 245.0f;
            mPosX = 750.0f;
            mVelY = -5.0f;
            mVelX = -3.0f;
            break;
        case COIN_MOTION_FROM_FROM_VS_WON:
            // The brain of the losing side drops in from the middle of the screen and
            // is blown inwards from the edge the winning side did not use.
            mVelY = -10.0f;
            mVelX = mApp->mGameScene == SCENE_PLANTS_WON ? -10.0f : 10.0f;
            {
                int aStartY = (int)mPosY;
                if (aStartY <= 50)
                    aStartY = 50;
                else if (aStartY >= 450)
                    aStartY = 450;
                mGroundY = aStartY;
                mPosY = (float)aStartY;
            }
            break;

        default:
            break;
    }

    mScale *= GetSunScale();
    if (CoinGetsBouncyArrow())
        mNeedsBouncyArrow = true;
    if (mCoinMotion != COIN_MOTION_FROM_PRESENT)
        PlayLaunchSound();
}

// ==========================================================================
// Coin::GetDisappearTime(void)
// @00198460  86 bytes
// ==========================================================================
int Coin::GetDisappearTime()
{
    // A present, a diamond or a chocolate sits on the lawn twice as long as the
    // other drops, and a bouncing arrow pins one down for that whole time.
    int aDisappearTime = 750;
    if (mType == COIN_PRESENT_PLANT || mType == COIN_DIAMOND || mType == COIN_CHOCOLATE || mHasBouncyArrow)
    {
        aDisappearTime = 1500;
    }

    // Vasebreaker and the slot machine keep a dropped usable seed packet for 3000.
    if ((mApp->IsScaryPotterLevel() || mApp->IsSlotMachineLevel()) && mType == COIN_USABLE_SEED_PACKET)
    {
        aDisappearTime = 3000;
    }

    // Nothing in the zen garden ever disappears.
    if (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN)
        return 6000;
    return aDisappearTime;
}

// ==========================================================================
// Coin::Draw(Sexy::Graphics *)
// @001984B8  2298 bytes
// ==========================================================================
void Coin::Draw(Sexy::Graphics *g)
{
    // The pickup glow is drawn behind a coin that is about to be picked up, and
    // every glow is tinted with the coin's own colour.
    g->SetColor(GetColor());

    if (mType == COIN_DIAMOND)
    {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, (int)(mPosX - 56.0f), (int)(mPosY - 66.0f));
        g->SetColorizeImages(false);
    }
    if (mType == COIN_PRESENT_PLANT)
    {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, (int)(mPosX - 50.0f), (int)(mPosY - 64.0f));
        g->SetColorizeImages(false);
    }
    if (mType == COIN_AWARD_PRESENT && mIsBeingCollected)
    {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, (int)(mPosX - 50.0f), (int)(mPosY - 64.0f));
        g->SetColorizeImages(false);
    }
    if (mType == COIN_CHOCOLATE || mType == COIN_AWARD_CHOCOLATE)
    {
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_AWARDPICKUPGLOW, (int)(mPosX - 56.0f), (int)(mPosY - 50.0f));
        g->SetColorizeImages(false);
    }

    // The two pulsing highlights of the allied double sun. Each one saves the
    // four scale words it is about to overwrite and puts them back afterwards,
    // which is what the binary does instead of a PushState.
    if (mDoubleSunHighlight0)
    {
        float aCenterX = mPosX + (float)mWidth * 0.5f;
        float aCenterY = mPosY + (float)mHeight * 0.5f;
        float aPulse = sinf((float)mCoinAge * 0.02f);
        float aOldScaleX = g->mScaleX;
        float aOldScaleY = g->mScaleY;
        float aOldScaleOrigX = g->mScaleOrigX;
        float aOldScaleOrigY = g->mScaleOrigY;
        g->SetScale(aPulse * 0.2f + 0.9f, aPulse * 0.2f + 0.9f, aCenterX, aCenterY);
        g->DrawImage(IMAGE_COOP_SUN_HIGHLIGHT_1, (int)(aCenterX - 65.0f), (int)(aCenterY - 65.0f));
        g->mScaleX = aOldScaleX;
        g->mScaleY = aOldScaleY;
        g->mScaleOrigX = aOldScaleOrigX;
        g->mScaleOrigY = aOldScaleOrigY;
    }
    if (mDoubleSunHighlight1)
    {
        float aCenterX = mPosX + 30.0f + (float)mWidth * 0.5f;
        float aCenterY = mPosY + 6.0f + (float)mHeight * 0.5f;
        float aPulse = sinf((float)mCoinAge * 0.02f);
        float aOldScaleX = g->mScaleX;
        float aOldScaleY = g->mScaleY;
        float aOldScaleOrigX = g->mScaleOrigX;
        float aOldScaleOrigY = g->mScaleOrigY;
        g->SetScale(aPulse * 0.2f + 0.9f, aPulse * 0.2f + 0.9f, aCenterX, aCenterY);
        g->DrawImage(IMAGE_COOP_SUN_HIGHLIGHT_2, (int)(aCenterX - 65.0f), (int)(aCenterY - 65.0f));
        g->mScaleX = aOldScaleX;
        g->mScaleY = aOldScaleY;
        g->mScaleOrigX = aOldScaleOrigX;
        g->mScaleOrigY = aOldScaleOrigY;
    }

    // The first two reanimation attachments are drawn in the coin's own frame.
    if (mAttachmentID[0] != 0)
    {
        g->PushState();
        BeginDraw(g);
        AttachmentDraw(mAttachmentID[0], g, false);
        g->PopState();
    }
    if (mAttachmentID[1] != 0)
    {
        g->PushState();
        BeginDraw(g);
        AttachmentDraw(mAttachmentID[1], g, false);
        g->PopState();
    }

    // A silver or gold coin that has already landed is not drawn again, and a
    // diamond is only ever its glow.
    if ((mType == COIN_SILVER || mType == COIN_GOLD) && mHitGround && !mIsBeingCollected)
        return;
    if (mType == COIN_DIAMOND)
        return;

    if (IsLevelAward() && !mIsBeingCollected)
    {
        Sexy::Color aFlashColor;
        GetFlashingColor(&aFlashColor, mCoinAge);
        g->SetColor(aFlashColor);
    }

    if (mType == COIN_SILVER || mType == COIN_GOLD)
    {
        g->SetColorizeImages(true);
        TodDrawImageCenterScaledF(g, IMAGE_REANIM_COINGLOW, mPosX - 14.0f, mPosY - 12.0f, mScale, mScale);
        g->SetColorizeImages(false);
    }

    Sexy::Image *aImage = nullptr;
    int aCelCol = 0;
    float aDrawScale = mScale;
    float aOffsetX = 0.0f;
    float aOffsetY = 0.0f;

    switch (mType)
    {
        case COIN_SILVER:
            aImage = IMAGE_REANIM_COIN_SILVER_DOLLAR;
            aOffsetX = 8.0f;
            aOffsetY = 10.0f;
            break;
        case COIN_GOLD:
            aImage = IMAGE_REANIM_COIN_GOLD_DOLLAR;
            aOffsetX = 8.0f;
            aOffsetY = 10.0f;
            break;

        case COIN_SUN:
        case COIN_SMALLSUN:
        case COIN_LARGESUN:
        case COIN_COOP_DOUBLE_SUN:
        case COIN_VS_ZOMBIE_BRAIN:
            // A sun and a brain are their reanimation alone.
            return;

        case COIN_FINAL_SEED_PACKET:
            // The award packet is scaled around its own top-left corner so it grows
            // out of the middle of its cell.
            g->SetScale(mScale, mScale, 0.0f, 0.0f);
            DrawSeedPacket(g,
                mPosX + (1.0f - mScale) * (float)mWidth * 0.5f,
                mPosY + (1.0f - mScale) * (float)mHeight * 0.5f,
                (SeedType)GetFinalSeedPacketType(),
                (SeedType)-1,
                0.0f,
                255,
                1,
                0,
                0,
                1);
            g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
            return;

        case COIN_PRESENT_PLANT:
        case COIN_AWARD_PRESENT:
            if (mIsBeingCollected)
            {
                mApp->mZenGarden->DrawPottedPlantIcon(g, mPosX + 10.0f, mPosY - 20.0f, &mPottedPlantSpec);
                return;
            }
            aImage = IMAGE_PRESENT;
            aOffsetY = -20.0f;
            break;

        case COIN_CHOCOLATE:
        case COIN_AWARD_CHOCOLATE:
            aImage = IMAGE_CHOCOLATE;
            break;

        case COIN_TROPHY:
            aImage = IMAGE_TROPHY_HI_RES;
            aOffsetX = -(float)(mWidth / 2);
            aOffsetY = -(float)(mHeight / 2);
            aDrawScale *= 0.5f;
            break;

        case COIN_AWARD_SILVER_SUNFLOWER:
            aImage = IMAGE_SUNFLOWER_TROPHY;
            aOffsetY = -5.0f;
            aDrawScale *= 0.6f;
            break;

        case COIN_AWARD_GOLD_SUNFLOWER:
            aImage = IMAGE_SUNFLOWER_TROPHY;
            aCelCol = 1;
            aOffsetY = -5.0f;
            aDrawScale *= 0.6f;
            break;

        case COIN_SHOVEL:
            aImage = IMAGE_SHOVEL_HI_RES;
            aOffsetX = -20.0f;
            aOffsetY = -20.0f;
            aDrawScale *= 0.5f;
            break;

        case COIN_ALMANAC:
            aImage = IMAGE_ALMANAC;
            break;
        case COIN_CARKEYS:
            aImage = IMAGE_CARKEYS;
            break;
        case COIN_VASE:
            aImage = IMAGE_SCARY_POT;
            break;
        case COIN_WATERING_CAN:
            aImage = IMAGE_WATERINGCAN;
            break;
        case COIN_TACO:
            aImage = IMAGE_TACO;
            break;
        case COIN_NOTE:
            aImage = IMAGE_ZOMBIE_NOTE_SMALL;
            break;

        case COIN_AWARD_MONEY_BAG:
        case COIN_AWARD_BAG_DIAMOND:
            aImage = IMAGE_MONEYBAG_HI_RES;
            aOffsetX = -(float)(mWidth / 2);
            aOffsetY = -(float)(mHeight / 2);
            aDrawScale *= 0.5f;
            break;

        case COIN_VS_PLANT_TROPHY:
            aImage = IMAGE_MP_PLANT_TROPHY_HI_RES;
            aOffsetX = -(float)(mWidth / 2);
            aOffsetY = -(float)(mHeight / 2);
            aDrawScale *= 0.5f;
            break;

        case COIN_VS_ZOMBIE_TROPHY:
            aImage = IMAGE_MP_ZOMBIE_TROPHY_HI_RES;
            aOffsetX = -(float)(mWidth / 2);
            aOffsetY = -(float)(mHeight / 2);
            aDrawScale *= 0.5f;
            break;

        case COIN_USABLE_SEED_PACKET:
            // A dropped usable seed packet blinks before it disappears; while it is
            // being carried it is drawn faded instead.
            {
                int aGrayness = 255;
                if (mIsBeingCollected)
                {
                    aGrayness = 128;
                }
                else if (GetDisappearTime() - 299 > mDisappearCounter || mDisappearCounter % 60 > 29)
                {
                    aGrayness = 255;
                }
                else
                {
                    aGrayness = 192;
                }
                g->SetColorizeImages(true);
                DrawSeedPacket(g,
                    (float)(int)mPosX,
                    (float)(int)mPosY,
                    (SeedType)mUsableSeedType,
                    (SeedType)-1,
                    0.0f,
                    aGrayness,
                    0,
                    0,
                    0,
                    1);
                g->SetColorizeImages(false);
                return;
            }

        default:
            break;
    }

    if (aImage == nullptr)
        return;

    // Only the two multiplayer trophies cast a shadow, and it shortens as the
    // trophy closes on the ground.
    if ((mType == COIN_VS_PLANT_TROPHY || mType == COIN_VS_ZOMBIE_TROPHY) && !mIsBeingCollected)
    {
        float aShadowScale = (mPosY - (float)mGroundY) * 0.01f + 1.0f;
        if (aShadowScale < 0.2f)
            aShadowScale = 0.2f;
        if (aShadowScale > 1.0f)
            aShadowScale = 1.0f;
        TodDrawImageCelCenterScaledF(
            g, IMAGE_PLANTSHADOW, mPosX + 18.0f, (float)(mGroundY + 96), 0, aShadowScale, aShadowScale);
    }

    g->SetColorizeImages(true);
    TodDrawImageCelCenterScaledF(g, aImage, aOffsetX + mPosX, aOffsetY + mPosY, aCelCol, aDrawScale, aDrawScale);
    g->SetColorizeImages(false);

    if (mAttachmentID[2] != 0)
    {
        g->PushState();
        BeginDraw(g);
        AttachmentDraw(mAttachmentID[2], g, false);
        g->PopState();
    }
}

// ==========================================================================
// Coin::UpdateFall(void)
// @00198DD4  1390 bytes
// ==========================================================================
void Coin::UpdateFall()
{
    // A coin that is already on the ground does not fall again.
    if (mDoubleSunGlowing)
        return;

    float aVelX = mVelX;
    float aVelY = mVelY;

    if (mCoinMotion == COIN_MOTION_FROM_PRESENT)
    {
        // A present coin simply drifts and is collected once it has aged out.
        mPosX += aVelX;
        mPosY += aVelY;
        mVelX = aVelX * 0.95f;
        mVelY = aVelY * 0.95f;
        if (mCoinAge > 79 && !IsSun() && !IsDoubleSun())
            Collect(0);
        return;
    }

    // A seed packet and every award drop are taken the moment they are created.
    if (mType == COIN_FINAL_SEED_PACKET || (unsigned int)(mType - COIN_AWARD_MONEY_BAG) <= 4)
        Collect(0);
    // Keep TV auto-collection for money and awards; sun requires player input.
    else if (!IsSun() && !IsDoubleSun() && mCoinAge >= mAutoCollectAge &&
        (mAutoCollect || (unsigned int)(mType - COIN_SILVER) <= 5 ||
            (unsigned int)(mType - COIN_AWARD_SILVER_SUNFLOWER) <= 1))
        Collect(0);

    // The falling and landing code is shared by every motion that is not already
    // handled above: while the coin is above its ground line it keeps falling,
    // and once it reaches it the coin lands.
    auto aFallOrLand = [&]()
    {
        if (mPosY + aVelY < (float)mGroundY)
        {
            // Gravity grows with the kind of throw.
            mPosY += aVelY;
            if (mCoinMotion == COIN_MOTION_FROM_PLANT || mCoinMotion == COIN_MOTION_FROM_GRAVE_STONE)
            {
                mVelY += 0.09f;
            }
            else if (mCoinMotion == COIN_MOTION_COIN || mCoinMotion == COIN_MOTION_FROM_BOSS)
            {
                mVelY += 0.15f;
            }
            mPosX += aVelX;
            if (mPosX > (float)(800 - mWidth) && mCoinMotion != COIN_MOTION_FROM_BOSS)
            {
                mPosX = (float)(800 - mWidth);
            }
            else if (mPosX < 0.0f)
            {
                mPosX = 0.0f;
            }
            return;
        }

        // The coin has landed: drop the bouncing arrow, play the ground sound once
        // and let the disappear counter run.
        if (mNeedsBouncyArrow && !mHasBouncyArrow)
        {
            float aParticleX = (float)(mWidth / 2);
            float aParticleY = (float)(mHeight / 2 - 60);
            if (mType == COIN_TROPHY)
            {
                aParticleX += 2.0f;
            }
            else if ((unsigned int)(mType - COIN_VS_PLANT_TROPHY) <= 1)
            {
                aParticleY -= 20.0f;
                int aRenderOrder = Board::MakeRenderOrder(RenderLayer(700000), mRow, mHasBouncyArrow);
                TodParticleSystem *aParticle =
                    mApp->AddTodParticle(mPosX, mPosY, aRenderOrder, PARTICLE_TROPHY_SPARKLE);
                AttachParticle(mAttachmentID[2], aParticle, 0.0f, 0.0f);
            }
            else if (mType == COIN_AWARD_MONEY_BAG || mType == COIN_AWARD_BAG_DIAMOND)
            {
                aParticleX += 2.0f;
                aParticleY -= 2.0f;
            }
            else if (mType == COIN_AWARD_PRESENT || IsPresentWithAdvice())
            {
                aParticleY -= 20.0f;
            }
            else if ((unsigned int)(mType - COIN_AWARD_SILVER_SUNFLOWER) <= 1)
            {
                aParticleX -= 6.0f;
                aParticleY -= 40.0f;
            }
            else if (IsMoney())
            {
                aParticleX += 12.0f;
                aParticleY += 21.0f;
            }

            ParticleEffect aEffect = IsMoney() ? PARTICLE_COIN_PICKUP_ARROW : PARTICLE_AWARD_PICKUP_ARROW;
            TodParticleSystem *aParticle = mApp->AddTodParticle(mPosX + aParticleX, mPosY + aParticleY, 0, aEffect);
            AttachParticle(mAttachmentID[0], aParticle, aParticleX, aParticleY);
            mHasBouncyArrow = true;
        }

        if (!mHitGround)
        {
            mHitGround = true;
            PlayGroundSound();
        }

        // The coin comes to rest on its line and its x is rounded to a whole pixel.
        float aRoundedX = mPosX;
        mPosY = (float)mGroundY;
        if (aRoundedX <= 0.0f)
            aRoundedX -= 0.5f;
        else
            aRoundedX += 0.5f;
        mPosX = (float)(int)aRoundedX;

        // Heavy Weapon and the last stand keep the coin alive until the onslaught
        // starts; the award drops never age out at all.
        if ((mApp->mGameMode != GAMEMODE_CHALLENGE_LAST_STAND || mBoard == nullptr ||
                mBoard->mChallenge->mChallengeState == STATECHALLENGE_LAST_STAND_ONSLAUGHT) &&
            !IsLevelAward() && !IsPresentWithAdvice())
        {
            ++mDisappearCounter;
            if (GetDisappearTime() <= mDisappearCounter)
                StartFade();
        }
    };

    if (mCoinMotion == COIN_MOTION_FROM_NEAR_CURSOR)
    {
        // The coin chases the cursor of the player who picked it up: it speeds up
        // to 600 and is collected as soon as it is within 35 of the cursor.
        if (mPlayerIndex >= 0)
        {
            GamepadControls *aControls = mBoard->GetGamepadControlsByPlayerIndex(mPlayerIndex);
            float aCursorX = 0.0f;
            float aCursorY = 0.0f;
            aControls->GetCursorPosition(aCursorX, aCursorY);
            float aHalfWidth = (float)(mWidth / 2);
            float aHalfHeight = (float)(mHeight / 2);
            float aDeltaX = (aCursorX - aHalfWidth) - mPosX;
            float aDeltaY = (aCursorY - aHalfHeight) - mPosY;
            aCursorX -= aHalfWidth;
            aCursorY -= aHalfHeight;
            if (aDeltaX * aDeltaX + aDeltaY * aDeltaY < 1225.0f)
            {
                Collect(mPlayerIndex);
                return;
            }
            float aScaledX = aDeltaX * 400.0f;
            float aScaledY = aDeltaY * 400.0f;
            float aDistance = sqrtf(aScaledX * aScaledX + aScaledY * aScaledY);
            float aSpeed = mCursorChaseSpeed + (6.4f / (aDistance * 0.01f)) * (aDistance * 0.01f);
            if (aSpeed > 600.0f)
                aSpeed = 600.0f;
            if (aDistance != 0.0f)
            {
                aScaledY /= aDistance;
                aScaledX /= aDistance;
            }
            mCursorChaseSpeed = aSpeed;
            float aStep = aSpeed * 0.016f;
            mPosY = mPosY + aScaledY * aStep;
            mPosX = mPosX + aScaledX * aStep;
        }
        else
        {
            aFallOrLand();
        }
    }
    else if (mCoinMotion == COIN_MOTION_FROM_FROM_VS_WON)
    {
        // The VS brain falls in from above and bounces once before it settles.
        float aNewVelY = aVelY + 0.2f;
        float aNewPosY = aNewVelY + mPosY;
        mVelX = aVelX * 0.95f;
        mVelY = aNewVelY * 0.95f;
        mPosX = mPosX + aVelX;
        mPosY = aNewPosY;
        if (aNewPosY >= (float)mGroundY)
        {
            if (sqrtf(aVelX * aVelX + aVelY * aVelY) <= 0.5f)
            {
                mPosY = (float)mGroundY;
                mVelY = 0.0f;
            }
            else
            {
                mApp->PlayFoley(FOLEY_GRASSSTEP);
                mVelY = -mVelY;
            }
        }
        if (mCoinAge > 199 && !IsSun() && !IsDoubleSun())
            Collect(0);
        // The brain is a sun-sized coin and still grows on the way down.
        float aFullScale = GetSunScale();
        if (aFullScale > mScale)
            mScale += 0.02f;
        else
            mScale = aFullScale;
    }
    else
    {
        aFallOrLand();

        // A plant's sun grows to its full size as it flies.
        if (mCoinMotion == COIN_MOTION_FROM_PLANT || mCoinMotion == COIN_MOTION_FROM_GRAVE_STONE)
        {
            float aFullScale = GetSunScale();
            if (aFullScale > mScale)
                mScale += 0.02f;
            else
                mScale = aFullScale;
        }
    }
}

// ==========================================================================
// Coin::Update(void)
// @00199344  668 bytes
// ==========================================================================
void Coin::Update()
{
    // The coin is only stepped while the level runs or the award screen is up.
    bool aStepIt = mApp->mGameScene == SCENE_AWARD || mApp->mGameScene == SCENE_PLAYING;
    ++mCoinAge;
    if (!aStepIt && mBoard != nullptr && !mBoard->mCutScene->ShouldRunUpsellBoard())
        return;

    if (mFadeCount != 0)
    {
        UpdateFade();
    }
    else if (mIsBeingCollected)
    {
        UpdateCollected();
    }
    else
    {
        static const bool aAutoCollect = Sexy::GetEnvOption("LAWN_AUTO_COLLECT", false) != 0;
        // Keep the normal delay and collection animation. Cooperative sun still
        // follows the two-player pickup rules so it is not assigned to player one.
        if (aAutoCollect && mCoinAge >= mAutoCollectAge && (IsMoney() || IsDeath() || (IsSun() && !mApp->IsCoopMode())))
        {
            Collect(0);
            // Automatic sun pickup follows the same first-level advice as input.
            if (IsSun() && mBoard != nullptr && mApp->IsFirstTimeAdventureMode() && mBoard->mLevel == 1)
            {
                Sexy::String aText;
                TodStringTranslate(&aText, "[ADVICE_CLICKED_ON_SUN]");
                mBoard->DisplayAdvice(aText, MESSAGE_STYLE_TUTORIAL_LEVEL1_STAY, ADVICE_ID_CLICKED_ON_SUN);
            }
        }
        else
            UpdateFall();
    }

    // Each attachment is moved to follow the coin and tinted with its colour. The
    // diamond offsets its first attachment by 18/13 as the coin shrinks, and the
    // VS brain attachments are greyed out until they land.
    if (mAttachmentID[0] != 0)
    {
        float aOffsetX = 0.0f;
        float aOffsetY = 0.0f;
        if (mType == COIN_DIAMOND)
        {
            aOffsetX = (1.0f - mScale) * 18.0f;
            aOffsetY = (1.0f - mScale) * 13.0f;
        }
        AttachmentUpdateAndMove(mAttachmentID[0], aOffsetX + mPosX, aOffsetY + mPosY);
        AttachmentOverrideColor(mAttachmentID[0], GetColor());
        AttachmentOverrideScale(mAttachmentID[0], mScale);

        // A silver or gold coin that is on the ground or already taken loses its
        // glow.
        if ((mHitGround || mIsBeingCollected) && (unsigned int)(mType - COIN_SILVER) <= 1)
            AttachmentOverrideColor(mAttachmentID[0], Sexy::Color(0, 0, 0, 0));
    }

    if (mAttachmentID[1] != 0)
    {
        // The second sun of the allied double sun sits where the first sun was
        // until the pair starts flying, when it follows the collected position.
        float aSecondX;
        float aSecondY;
        if (mIsBeingCollected)
        {
            aSecondX = mCollectX;
            aSecondY = mCollectY;
        }
        else
        {
            aSecondX = mPosX;
            aSecondY = mPosY;
            mCollectX = aSecondX;
            mCollectY = aSecondY;
        }
        AttachmentUpdateAndMove(mAttachmentID[1], aSecondX, aSecondY);
        AttachmentOverrideColor(mAttachmentID[1], GetColor());
        AttachmentOverrideScale(mAttachmentID[1], mScale);
    }

    if (mAttachmentID[2] != 0)
    {
        AttachmentUpdateAndMove(mAttachmentID[2],
            mPosX + (float)(mWidth / 2) * (mScale * 0.5f),
            mPosY + (float)(mHeight / 2) * (mScale * 0.5f));
        AttachmentOverrideColor(mAttachmentID[2], GetColor());
        AttachmentOverrideScale(mAttachmentID[2], mScale);
    }

    if (mType == COIN_COOP_DOUBLE_SUN)
    {
        // Each half of the allied double sun highlights while a gamepad cursor is
        // within 125 of it; when both halves are covered the sun is collected.
        float aSunX = mPosX;
        float aSunY = mPosY;
        bool aBothClose = true;
        bool aGlowing = false;
        for (int i = 0; i < 2; ++i)
        {
            int aAttachmentID = i == 0 ? mAttachmentID[0] : mAttachmentID[1];
            Reanimation *aReanim = FindReanimAttachment(aAttachmentID);
            BaseGamepadControls *aControls = mBoard->mGamepadControls[i];
            float aDeltaX = aControls->mCursorPositionX - aSunX;
            float aDeltaY = aControls->mCursorPositionY - aSunY;
            bool aClose = aDeltaX * aDeltaX + aDeltaY * aDeltaY <= 15625.0f;
            if (aClose)
            {
                AttachmentOverrideScale(aAttachmentID, mScale * 1.1f);
                if (aReanim != nullptr)
                {
                    aGlowing = true;
                    aReanim->SetAnimRate(22.0f);
                }
            }
            else
            {
                AttachmentOverrideScale(aAttachmentID, mScale);
                if (aReanim != nullptr)
                    aReanim->SetAnimRate(6.0f);
                aBothClose = false;
            }
            if (i == 0)
                mDoubleSunHighlight0 = aClose;
            else
                mDoubleSunHighlight1 = aClose;
        }
        mDoubleSunGlowing = aGlowing;
        if (aBothClose)
            Collect(0);
    }
}

// ==========================================================================
// Coin::DroppedUsableSeed(void)
// @001995EC  34 bytes
// ==========================================================================
void Coin::DroppedUsableSeed()
{
    // The first time a usable seed packet is dropped the coin is pulled back to
    // 1200 frames, so a packet that keeps being dropped stays a little longer.
    int aTimesDropped = mTimesDropped;
    mIsBeingCollected = false;
    if (aTimesDropped == 0)
    {
        if (mDisappearCounter >= 1200)
            mDisappearCounter = 1200;
    }
    mTimesDropped = aTimesDropped + 1;
}

// ==========================================================================
// Coin::Coin(void)
// @00199610  98 bytes
// ==========================================================================
Coin::Coin()
{
    // The base constructor registers GameObject's own state block; this one adds
    // the coin's whole derived area (+0x34..+0xC5, 146 bytes) and zeroes it.
    SyncBlockInfo aBlock = MakeSyncBlock(mPosX, mDoubleSunHighlight1);
    mSyncBlocks.push_back(aBlock);
    memset(aBlock.mData, 0, aBlock.mSize);
}
