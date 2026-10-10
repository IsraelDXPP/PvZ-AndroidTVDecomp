// CreditScreen -- reconstructed from libGameMain.so.

#include "CreditScreen.h"
#include "SexyString.h"
#include "ReconData.h"
#include "EffectSystem.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "Image.h"
#include "LawnApp.h"
#include "PerfTimer.h"
#include "PoolEffect.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "ReanimatorDefinition.h"
#include "ReanimatorTransform.h"
#include "ReconTypes.h"
#include "NativeDisplay.h"
#include "SexyAppBase.h"
#include "TodFoley.h"
#include "TodParticleSystem.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include "TodParticleHolder.h"
#include <cstring>
#include <cmath>

namespace
{

// The original CreditsOverlay @0x684898 forwards drawing to its owner.
class CreditsOverlay : public Sexy::Widget
{
public:
    explicit CreditsOverlay(CreditScreen *theScreen) :
        mCreditScreen(theScreen)
    {
        mMouseVisible = false;
        mFocusable = false;
        mClip = false;
        mHasAlpha = true;
        Resize(0, 0, 800, 600);
    }

    void Draw(Sexy::Graphics *theGraphics) override
    {
        mCreditScreen->DrawOverlay(theGraphics);
    }

private:
    CreditScreen *mCreditScreen;
};

} // namespace

// The holder stores each particle system as the system plus the id word that
// follows it. A free slot has a zero high half; a dead system is skipped.
bool IterateParticles(TodParticleSystem *&theParticle, TodParticleHolder *theHolder)
{
    DataArray<TodParticleSystem, ParticleSystemSlot> *aArray = &theHolder->mParticleSystems;
    ParticleSystemSlot *aSlots = aArray->mBlock;
    ParticleSystemSlot *aEnd = aSlots + aArray->mMaxUsedCount;
    ParticleSystemSlot *aSlot = aSlots;
    if (theParticle != nullptr)
    {
        for (; aSlot < aEnd; ++aSlot)
        {
            if (&aSlot->mItem == theParticle)
            {
                ++aSlot;
                break;
            }
        }
    }
    while (aSlot < aEnd)
    {
        if ((aSlot->mID >> 16) != 0 && !aSlot->mItem.mDead)
        {
            theParticle = &aSlot->mItem;
            return true;
        }
        ++aSlot;
    }
    theParticle = nullptr;
    return false;
}

// The number of frames in a reanimation: every credits body derives it as
// `mDefinition->mTracks[0].mTransformCount`, which is exactly what
// `Reanimation::mFrameCount - 1` means elsewhere.
static int CreditsFrameCount(Reanimation *theReanim)
{
    ReanimatorTrack *aTracks = theReanim->mDefinition->mTracks;
    return aTracks->mTransformCount;
}

// ==========================================================================
// CreditScreen::~CreditScreen()
CreditScreen::~CreditScreen()
{
    delete mOverlayWidget;
}

// CreditScreen::MouseUp(int,int,int)
// @001B9070  2 bytes
// ==========================================================================
void CreditScreen::MouseUp(int theX, int theY, int theClickCount)
{
    ;
}

// ==========================================================================
// CreditScreen::ButtonPress(int)
// @001B9074  2 bytes
// ==========================================================================
void CreditScreen::ButtonPress(int theId)
{
    ;
}

// ==========================================================================
// CreditScreen::ButtonDepress(int)
// @001B9080  2 bytes
// ==========================================================================
void CreditScreen::ButtonDepress(int theId)
{
    ;
}

// ==========================================================================
// CreditScreen::AddedToManager(Sexy::WidgetManager *)
// @001B90D4  22 bytes
// ==========================================================================
void CreditScreen::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    Sexy::WidgetContainer::AddedToManager(theWidgetManager);
    AddWidget(mOverlayWidget);
}

// ==========================================================================
// CreditScreen::RemovedFromManager(Sexy::WidgetManager *)
// @001B90EC  38 bytes
// ==========================================================================
void CreditScreen::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    Sexy::WidgetContainer::RemovedFromManager(theWidgetManager);
    RemoveWidget(mOverlayWidget);
    mApp->SetMusicVolume(mSavedMusicVolume);
}

// ==========================================================================
// CreditScreen::GotFocus()
// @001B9114  4 bytes
// ==========================================================================
void CreditScreen::GotFocus()
{
    Sexy::Widget::GotFocus();
}

// ==========================================================================
// CreditScreen::DrawOverlay(Sexy::Graphics *)
// @001B9118  86 bytes
// ==========================================================================
void CreditScreen::DrawOverlay(Sexy::Graphics *theGraphics)
{
    if (mCreditsPhase == CREDITS_END)
    {
        int anAlpha = TodAnimateCurve(50, 100, mCreditsPhaseCounter, 255, 0, CURVE_LINEAR);
        if (anAlpha > 0)
        {
            Sexy::Color aBlack(0, 0, 0, anAlpha);
            theGraphics->SetColor(aBlack);
            theGraphics->FillRect(LawnApp::FULLSCREEN_RECT);
        }
    }
}

// ==========================================================================
// CreditScreen::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @001B9178  94 bytes
// ==========================================================================
void CreditScreen::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    if (theButton == GAMEPAD_BUTTON_B)
    {
        if (mApp->GamepadToPlayerIndex(theFlags) == 0)
        {
            mApp->KillCreditScreen();
            mApp->DoBackToMain();
        }
    }
    else if (theButton == GAMEPAD_BUTTON_X && mCreditsPhase == CREDITS_END &&
        (!mIsFromMainMenu || mApp->HasFinishedAdventure()))
    {
        mApp->KillCreditScreen();
        mApp->ShowCreditScreen(false);
    }
}

// ==========================================================================
// CreditScreen::CreditScreen(LawnApp *,bool)
// @001B9710  868 bytes
// ==========================================================================
CreditScreen::CreditScreen(LawnApp *theApp, bool isFromMainMenu)
{
    mApp = theApp;
    mClip = false;
    mUpdateCount = 0;
    mDrawCount = 0;
    mLastDrawCount = 0;
    mDontSync = false;
    mCreditsPaused = false;
    mPreloaded = false;
    mBlinkCountdown = 700;
    mBrainPosX = 0.0f;
    mBrainPosY = 0.0f;

    mApp->mEffectSystem->EffectSystemFreeAll();
    mApp->mMusic->StopAllMusic();

    // One resources group at a time: the binary builds a String per group and
    // releases it before the next.
    Sexy::String aGroup("DelayLoad_Credits");
    TodLoadResources(aGroup);

    mCreditsPhase = CREDITS_MAIN1;
    mCreditsPhaseCounter = 0;
    mCreditsReanimID = REANIMATIONID_NULL;
    mFogParticleID = ParticleSystemID(0);
    mDrawBrain = false;

    mOverlayWidget = new CreditsOverlay(this);

    // Constructor @0x1B982A..0x1B984C keeps volume separate from scrolling.
    mSavedMusicVolume = mApp->GetMusicVolume();
    if (mSavedMusicVolume < 0.1)
        mApp->SetMusicVolume(0.85);
    mScrollPositionY1 = mApp->mHeight;
    mScrollPositionY2 = -10000.0;
    mIsFromMainMenu = isFromMainMenu;
    if (isFromMainMenu)
    {
        mApp->KillMainMenu();
        mApp->KillNewOptionsDialog();
        mApp->KillDialog(3);
        // The menu's credits entry plays the unlocked MV for completed profiles.
        // Profiles that have not finished adventure still get the staff roll.
        if (!mApp->HasFinishedAdventure())
            mCreditsPhase = CREDITS_END;
    }

    // Count the localised credit-name lines the string table actually carries.
    Sexy::String aKey;
    Sexy::String aDefault("");
    Sexy::String aText;
    gXblaLast = 0;
    for (int anIndex = 1; anIndex <= 20; anIndex++)
    {
        StrFormat(&aKey, "CREDITS_NAMES_XBLA%d", anIndex);
        aText = mApp->GetString(aKey, aDefault);
        if (aText.empty())
            break;
        gXblaLast = anIndex;
    }
    // Each screen owns its hints. Discard both sets after closing the old page.
    mApp->ForceHideHelpBarWidget();
    mApp->mHelpBarWidget->AddButton(GAMEPAD_BUTTON_B, Sexy::String("[BACK]"), HelpBarWidget::HELP_ALIGN_RIGHT);
    mApp->ShowHelpBarWidget();
}
// ==========================================================================
// CreditScreen::PreLoadCredits(void)
// @001B9ACC  984 bytes
// ==========================================================================
int CreditScreen::PreLoadCredits()
{
    mPreloaded = true;
    mApp->mMusic->MusicLoadCreditsSong();

    // The six background layers, one resources group each.
    for (int aLayer = 1; aLayer <= 6; aLayer++)
    {
        Sexy::String aGroup;
        StrFormat(&aGroup, "DelayLoad_Background%d", aLayer);
        TodLoadResources(aGroup);
    }

    PreLoadReanimForCredits(REANIM_CREDITS_MAIN);
    PreLoadReanimForCredits(REANIM_CREDITS_MAIN2);
    PreLoadReanimForCredits(REANIM_CREDITS_MAIN3);
    PreLoadReanimForCredits(REANIM_ZOMBIE_CREDITS_DANCE);
    PreLoadReanimForCredits(REANIM_CREDITS_BIGBRAIN);
    PreLoadReanimForCredits(REANIM_CREDITS_FLOWER_PETALS);
    PreLoadReanimForCredits(REANIM_CREDITS_INFANTRY);
    PreLoadReanimForCredits(REANIM_CREDITS_STAGE);
    PreLoadReanimForCredits(REANIM_CREDITS_THROAT);
    PreLoadReanimForCredits(REANIM_SUNFLOWER);
    PreLoadReanimForCredits(REANIM_SUN);
    PreLoadReanimForCredits(REANIM_CREDITS_CRAZYDAVE);
    PreLoadReanimForCredits(REANIM_ZOMBIE);
    PreLoadReanimForCredits(REANIM_DIGGER);
    PreLoadReanimForCredits(REANIM_ZOMBIE_FOOTBALL);
    PreLoadReanimForCredits(REANIM_ZOMBIE_CREDITS_CONEHEAD);
    PreLoadReanimForCredits(REANIM_ZOMBIE_CREDITS_SCREEN_DOOR);
    PreLoadReanimForCredits(REANIM_CREDITS_ZOMBIEARMY1);
    PreLoadReanimForCredits(REANIM_CREDITS_ZOMBIEARMY2);
    PreLoadReanimForCredits(REANIM_CREDITS_TOMBSTONES);
    PreLoadReanimForCredits(REANIM_CREDITS_SOLARPOWER);
    PreLoadReanimForCredits(REANIM_CREDITS_ANYHOUR);
    PreLoadReanimForCredits(REANIM_CREDITS_WEARETHEUNDEAD);
    PreLoadReanimForCredits(REANIM_CREDITS_DISCOLIGHTS);
    PreLoadReanimForCredits(REANIM_CREDITS_BOSSDANCE);
    PreLoadReanimForCredits(REANIM_TALLNUT);
    PreLoadReanimForCredits(REANIM_SNOWPEA);
    PreLoadReanimForCredits(REANIM_PEASHOOTER);
    PreLoadReanimForCredits(REANIM_REPEATER);
    PreLoadReanimForCredits(REANIM_KERNELPULT);
    PreLoadReanimForCredits(REANIM_WALLNUT);
    PreLoadReanimForCredits(REANIM_FUMESHROOM);
    PreLoadReanimForCredits(REANIM_PUFFSHROOM);
    int result = PreLoadReanimForCredits(REANIM_SUNSHROOM);

    // @0x1B9ACC: TV offsets 1952, 1968 and 2224 select definitions 122, 123
    // and 139. Access the native records by animation type on both architectures.
    const ReanimationType aTextAnimations[] = {REANIM_CREDITS_MAIN2, REANIM_CREDITS_MAIN3};
    for (ReanimationType aType : aTextAnimations)
    {
        ReanimatorDefinition &aDefinition = gReanimatorDefArray[aType];
        for (int aTrackIndex = 0; aTrackIndex < aDefinition.mTrackCount; ++aTrackIndex)
        {
            ReanimatorTrack &aTrack = aDefinition.mTracks[aTrackIndex];
            const char *aName = aTrack.mName;
            if (strcmp(aName, "Words") != 0 && strcmp(aName, "Words2") != 0)
                continue;
            for (int aFrame = 0; aFrame < aTrack.mTransformCount && aFrame <= 123; ++aFrame)
                aTrack.mTransforms[aFrame].mFont = FONT_BRIANNETOD32BLACK;
        }
    }
    ReanimatorDefinition &anUndeadDefinition = gReanimatorDefArray[REANIM_CREDITS_WEARETHEUNDEAD];
    for (int aTrackIndex = 0; aTrackIndex < anUndeadDefinition.mTrackCount; ++aTrackIndex)
    {
        ReanimatorTrack &aTrack = anUndeadDefinition.mTracks[aTrackIndex];
        for (int aFrame = 0; aFrame < aTrack.mTransformCount; ++aFrame)
        {
            if (aTrack.mTransforms[aFrame].mFont != nullptr)
                aTrack.mTransforms[aFrame].mFont = FONT_BRIANNETOD32BLACK;
        }
    }
    return result;
}

// ==========================================================================
// CreditScreen::GetTiming(CreditsTiming **,CreditsTiming **,float *)
// @001B9EDC  258 bytes
// ==========================================================================
int CreditScreen::GetTiming(const CreditsTiming **theStart, const CreditsTiming **theEnd, float *theFraction)
{
    const CreditsTiming *aTable = gCreditsTiming;
    Reanimation *aReanim = mApp->ReanimationGet(mCreditsReanimID);

    // How far the sequence has scrolled, in frames: the reanimation's frame count
    // times its current animation time, less the one frame of lead-in. The three
    // constants below place each phase on that timeline.
    float aTime = (float)CreditsFrameCount(aReanim) * aReanim->mAnimTime - 1.0f;
    float aStart = aTime + 2.0f;
    if (mCreditsPhase != CREDITS_MAIN1)
    {
        if (mCreditsPhase == CREDITS_MAIN2)
        {
            aStart = aTime + 400.0f;
        }
        else
        {
            if (mCreditsPhase != CREDITS_MAIN3)
            {
                *theStart = nullptr;
                *theEnd = nullptr;
                *theFraction = 0.0f;
                return 0;
            }
            aStart = aTime + 785.0f;
        }
    }

    if (aStart < aTable[0].mFrame)
    {
        *theStart = nullptr;
        *theEnd = aTable;
        *theFraction = aStart / aTable[0].mFrame;
    }
    else
    {
        int anIndex = 0;
        int aLastIndex = gCreditsTimingCount - 1;
        for (;;)
        {
            if (anIndex >= aLastIndex)
            {
                *theStart = &aTable[aLastIndex];
                *theEnd = nullptr;
                *theFraction = 0.0f;
                return anIndex;
            }
            if (aStart < aTable[anIndex + 1].mFrame)
                break;
            ++anIndex;
        }
        // theStart is the entry the scroll is inside and theEnd the one it is heading for; theFraction
        // is how far between the two it has come.
        *theStart = &aTable[anIndex];
        *theEnd = &aTable[anIndex + 1];
        *theFraction = (aStart - aTable[anIndex].mFrame) / (aTable[anIndex + 1].mFrame - aTable[anIndex].mFrame);
    }
    return 0;
}

// ==========================================================================
// CreditScreen::PlayReanim(int)
// @001B9FF4  332 bytes
// ==========================================================================
Reanimation *CreditScreen::PlayReanim(int thePhase)
{
    Reanimation *aOldReanim = mApp->ReanimationTryToGet(mCreditsReanimID);
    if (aOldReanim != nullptr)
        aOldReanim->ReanimationDie();

    Reanimation *aReanim = nullptr;
    switch (thePhase)
    {
        case 1:
            aReanim = mApp->AddReanimation(0, 0, 0, REANIM_CREDITS_MAIN);
            aReanim->AssignRenderGroupToPrefix("Background", 1);
            aReanim->AssignRenderGroupToPrefix("attacher__Zombie", 2);
            aReanim->AssignRenderGroupToPrefix("Words", 3);
            aReanim->AssignRenderGroupToPrefix("SpotFront", 3);
            break;
        case 2:
            aReanim = mApp->AddReanimation(0, 0, 0, REANIM_CREDITS_MAIN2);
            aReanim->AssignRenderGroupToPrefix("Background", 1);
            aReanim->AssignRenderGroupToPrefix("attacher__Zombie", 2);
            aReanim->AssignRenderGroupToPrefix("Words", 3);
            aReanim->AssignRenderGroupToPrefix("SpotFront", 3);
            aReanim->AssignRenderGroupToPrefix("attacher__undead", 2);
            break;
        case 3:
            aReanim = mApp->AddReanimation(0, 0, 0, REANIM_CREDITS_MAIN3);
            aReanim->AssignRenderGroupToPrefix("Background", 1);
            aReanim->AssignRenderGroupToPrefix("attacher__Zombie", 2);
            aReanim->AssignRenderGroupToPrefix("attacher__DiscoLights", 2);
            aReanim->AssignRenderGroupToPrefix("Words", 3);
            aReanim->AssignRenderGroupToPrefix("attacher__cattail", 3);
            aReanim->AssignRenderGroupToPrefix("SpotFront", 3);
            aReanim->AssignRenderGroupToPrefix("attacher__undead", 2);
            aReanim->AssignRenderGroupToPrefix("black", 3);
            break;
        default:
            break;
    }
    aReanim->mIsAttachment = true;
    aReanim->mLoopType = REANIM_PLAY_ONCE_AND_HOLD;
    mCreditsReanimID = mApp->ReanimationGetID(aReanim);
    return aReanim;
}

// ==========================================================================
// CreditScreen::DrawFogEffect(Sexy::Graphics *,float)
// @001BA184  592 bytes
// ==========================================================================
int CreditScreen::DrawFogEffect(Sexy::Graphics *theGraphics, float theOffset)
{
    Reanimation *aReanim = mApp->ReanimationGet(mCreditsReanimID);
    Sexy::Image *aFogImage = IMAGE_FOG;
    if (mApp->Is3DAccelerated() == 0)
        aFogImage = IMAGE_FOG_SOFTWARE;

    int aRowOffset = 0;
    int aColumn = 0;
    int aY = -240;
    int anAlpha = (int)(theOffset * 255.0f);
    int result = 0;
    bool aLastColumn = false;
    do
    {
        int aCel = 0;
        int aX = aRowOffset;
        do
        {
            int aBlue = 255;
            int aCelIndex = aX & 7;
            int aRedGreen = 255;
            float aDrawX = (float)aY - 15.0f;
            float aDrawY = (float)(85 * aCel) + 200.0f;
            // The fog drifts with the credits reanimation's own progress.
            float aScroll = (float)CreditsFrameCount(aReanim) * aReanim->mAnimTime / aReanim->mAnimRate * 100.0f;
            double aWave = sin((float)((float)(aScroll * 0.0069813) + (float)((float)aCel * 2.6928)));
            double aWave2 = sin((float)((float)((float)aColumn * 1.3464) + (float)(aScroll * 0.012566))) * 8.0;
            if (mApp->Is3DAccelerated() != 0)
            {
                float aFade = aWave2 + aWave * 4.0 + 13.0;
                aBlue = (int)((float)(255 - aX % 20) - aFade);
                aRedGreen = (int)((float)(int)(255.0f - (float)(aX % 20) * 1.5f) - aFade * 1.5f);
            }
            else
            {
                aDrawX = (float)aY - 15.0f + 10.0f;
                aCelIndex = aX % IMAGE_FOG_SOFTWARE->mNumCols;
                aDrawY = aDrawY + 23.0f;
            }
            theGraphics->SetColorizeImages(true);
            ++aCel;
            Sexy::Color aColor(aRedGreen, aRedGreen, aBlue, anAlpha);
            theGraphics->SetColor(aColor);
            aX += aColumn + 17;
            theGraphics->DrawImageCel(aFogImage, (int)aDrawX, (int)aDrawY, aCelIndex, 0);
            theGraphics->SetColorizeImages(false);
        } while (aCel != 7);
        aLastColumn = (aColumn++ == 13);
        aY += 80;
        aRowOffset += 13;
    } while (!aLastColumn);
    return result;
}

// ==========================================================================
// CreditScreen::DrawFinalCredits(Sexy::Graphics *)
// @001BA400  1732 bytes
// ==========================================================================
void CreditScreen::DrawFinalCredits(Sexy::Graphics *theGraphics)
{
    double aScroll = mScrollPositionY1;
    TodDrawString(theGraphics,
        Sexy::String("[CREDITS_GAMENAME]"),
        400,
        (int)(aScroll + 60.0),
        FONT_HOUSEOFTERROR28,
        Sexy::Color::White,
        2);

    TodDrawStringWrapped(theGraphics,
        Sexy::String("[CREDITS_NAMES1]"),
        Sexy::TRect<int>(405, (int)(aScroll + 90.0), 200, 200),
        FONT_HOUSEOFTERROR16,
        Sexy::Color::White,
        0,
        0);
    TodDrawStringWrapped(theGraphics,
        Sexy::String("[CREDITS_ROLES1]"),
        Sexy::TRect<int>(190, (int)(aScroll + 90.0), 200, 200),
        FONT_HOUSEOFTERROR16,
        Sexy::Color::White,
        1,
        0);
    TodDrawStringWrapped(theGraphics,
        Sexy::String("[CREDITS_NAMES2]"),
        Sexy::TRect<int>(340, (int)(aScroll + 280.0), 450, 250),
        FONT_HOUSEOFTERROR16,
        Sexy::Color::White,
        0,
        0);
    TodDrawStringWrapped(theGraphics,
        Sexy::String("[CREDITS_ROLES2]"),
        Sexy::TRect<int>(30, (int)(aScroll + 280.0), 300, 250),
        FONT_HOUSEOFTERROR16,
        Sexy::Color::White,
        1,
        0);

    TodDrawString(theGraphics,
        Sexy::String("[CREDITS_XBLA]"),
        400,
        (int)(aScroll + 600.0),
        FONT_HOUSEOFTERROR28,
        Sexy::Color::White,
        2);

    // Each XBLA block stacks under the last. The wrapped height decides the next Y.
    int aBlockY = 640;
    if (gXblaLast > 0)
    {
        for (int i = 1; i <= gXblaLast; i++)
        {
            int aY = (int)((double)aBlockY + aScroll);
            Sexy::String aRoles;
            Sexy::String aNames;
            StrFormat(&aRoles, "[CREDITS_ROLES_XBLA%d]", i);
            StrFormat(&aNames, "[CREDITS_NAMES_XBLA%d]", i);
            TodDrawStringWrapped(theGraphics,
                aRoles,
                Sexy::TRect<int>(-100, aY, 430, 250),
                FONT_HOUSEOFTERROR16,
                Sexy::Color::White,
                1,
                0);
            int aHeight = TodDrawStringWrapped(theGraphics,
                aNames,
                Sexy::TRect<int>(340, aY, 500, 250),
                FONT_HOUSEOFTERROR16,
                Sexy::Color::White,
                0,
                0);
            aBlockY += aHeight + 2;
        }
        aBlockY += 20;
    }
    else
    {
        aBlockY = 660;
    }

    for (int i = 1; i <= 2; i++)
    {
        Sexy::String aLegal;
        StrFormat(&aLegal, "[CREDITS_LEGAL%d]", i);
        int aHeight = TodDrawStringWrapped(theGraphics,
            aLegal,
            Sexy::TRect<int>(0, (int)((double)aBlockY + aScroll), 800, 250),
            FONT_HOUSEOFTERROR16,
            Sexy::Color::White,
            2,
            0);
        aBlockY += aHeight + 20;
    }

    aBlockY += 400;
    int aDisclaimer = TodDrawStringWrapped(theGraphics,
        Sexy::String("[CREDITS_DISCLAIMER]"),
        Sexy::TRect<int>(0, (int)((double)aBlockY + aScroll), 800, 250),
        FONT_HOUSEOFTERROR16,
        Sexy::Color::White,
        2,
        0);
    aBlockY += aDisclaimer + 400;
    TodDrawString(theGraphics,
        Sexy::String("[CREDITS_THANKS]"),
        400,
        (int)((double)aBlockY + aScroll),
        FONT_HOUSEOFTERROR16,
        Sexy::Color::White,
        2);

    // The scroll stops when the thanks line reaches the middle of the screen.
    int aScreenHeight = mApp->mPlatformDriverOrQueue->mScreenHeight;
    mScrollPositionY2 = (double)(aScreenHeight / 2 - aBlockY);
}

// ==========================================================================
// CreditScreen::UpdateBlink()
// @001BAAE0  172 bytes
// ==========================================================================
int CreditScreen::UpdateBlink()
{
    if (--mBlinkCountdown <= 0)
    {
        Reanimation *aReanim = mApp->ReanimationGet(mCreditsReanimID);
        mBlinkCountdown = 700;
        Reanimation *aHead = aReanim->FindSubReanim(REANIM_SUNFLOWER);
        if (aHead != nullptr &&
            (mCreditsPhase != CREDITS_MAIN3 || aReanim->mAnimTime <= 200.0f / (float)(CreditsFrameCount(aReanim) - 1)))
        {
            Reanimation *aBlink = mApp->mEffectSystem->mReanimationHolder->AllocReanimation(0, 0, 0, REANIM_SUNFLOWER);
            aBlink->SetFramesForLayer("anim_blink");
            aBlink->mLoopType = (ReanimLoopType)4;
            aBlink->SetAnimRate(15.0);
            aBlink->AttachToAnotherReanimation(aHead, "anim_idle");
            return 0;
        }
    }
    return 0;
}

// ==========================================================================
// CreditScreen::ExportMovieFrame(int)
// @001BAB98  2 bytes
// ==========================================================================
void CreditScreen::ExportMovieFrame(int theFrame)
{
    ;
}

// ==========================================================================
// CreditScreen::TurnOffTongues(Reanimation *,int)
// @001BAB9C  126 bytes
// ==========================================================================
void CreditScreen::TurnOffTongues(Reanimation *theReanim, int theTrack)
{
    // @0x1BAB9C uses distinct strides for definition and runtime tracks.
    int aTonguePhase = theTrack % 4;
    for (int aIndex = 0; aIndex < theReanim->mDefinition->mTrackCount; ++aIndex)
    {
        ReanimatorTrackInstance &anInstance = theReanim->mTrackInstances[aIndex];
        ReanimatorTrack &aTrack = theReanim->mDefinition->mTracks[aIndex];
        if (theReanim->mReanimationType == REANIM_ZOMBIE_CREDITS_DANCE && aTonguePhase != 1 &&
            strcasecmp(aTrack.mName, "anim_tongue") == 0)
            anInstance.mRenderGroup = -1;
        Reanimation *anAttached = FindReanimAttachment(anInstance.mAttachmentID);
        if (anAttached != nullptr)
            TurnOffTongues(anAttached, aIndex);
    }
}

// ==========================================================================
// CreditScreen::UpdateMovie()
// @001BAC20  4036 bytes
// ==========================================================================
int CreditScreen::UpdateMovie()
{
    if (mCreditsPhase == CREDITS_END)
    {
        mScrollPositionY1 = mScrollPositionY1 > mScrollPositionY2 ? mScrollPositionY1 - 0.4 : mScrollPositionY1;
        ++mCreditsPhaseCounter;
        if (mCreditsPhaseCounter != 50)
            return 0;

        // The help bar offers replay and back once the final scroll has settled.
        HelpBarWidget *aHelp = mApp->mHelpBarWidget;
        aHelp->mDrawShade = false;
        aHelp->ClearButtons(0);
        if (!mIsFromMainMenu || mApp->HasFinishedAdventure())
            aHelp->AddButton(GAMEPAD_BUTTON_X, Sexy::String("[CREDITS_REPLAY_BUTTON]"), HelpBarWidget::HELP_ALIGN_LEFT);
        aHelp->AddButton(GAMEPAD_BUTTON_B, Sexy::String("[BACK]"), HelpBarWidget::HELP_ALIGN_RIGHT);
        return mApp->ShowHelpBarWidget();
    }

    UpdateBlink();
    Reanimation *aCredits = mApp->ReanimationGet(mCreditsReanimID);
    float aFrame = 1.0f / (float)(aCredits->mFrameCount - 1);
    aCredits->Update();
    mApp->mEffectSystem->Update();
    mApp->mPoolEffect->PoolEffectUpdate();
    TurnOffTongues(aCredits, 0);

    if (mCreditsPhase == CREDITS_MAIN1 && aCredits->mLoopCount > 0)
    {
        aCredits = PlayReanim(2);
        mCreditsPhase = CREDITS_MAIN2;
    }
    else if (mCreditsPhase == CREDITS_MAIN2 && aCredits->mLoopCount > 0)
    {
        aCredits = PlayReanim(3);
        mCreditsPhase = CREDITS_MAIN3;
    }
    else if (mCreditsPhase == CREDITS_MAIN3 && aCredits->mLoopCount > 0)
    {
        mCreditsPhase = CREDITS_END;
    }

    if (mCreditsPhase == CREDITS_MAIN2 || mCreditsPhase == CREDITS_MAIN3)
    {
        ReanimatorTrackInstance *aWords = aCredits->GetTrackInstanceByName("Words");
        ReanimatorTrackInstance *aWords2 = aCredits->GetTrackInstanceByName("Words2");
        Sexy::Color aWordColor = aCredits->mAnimTime < aFrame * 124.0f ? Sexy::Color::Black : Sexy::Color::White;
        aWords->mTrackColor = aWordColor;
        aWords2->mTrackColor = aWordColor;

        Reanimation *aUndead = aCredits->FindSubReanim(REANIM_CREDITS_WEARETHEUNDEAD);
        if (aUndead != nullptr)
        {
            aUndead->GetTrackInstanceByName("bubbletext1")->mTrackColor = Sexy::Color::Black;
            aUndead->GetTrackInstanceByName("bubbletext2")->mTrackColor = Sexy::Color::Black;
            aUndead->GetTrackInstanceByName("bubbletext3")->mTrackColor = Sexy::Color::Black;
            aUndead->GetTrackInstanceByName("ShakyText")->mShakeOverride =
                aCredits->mAnimTime > aFrame * 112.0f ? 2.0f : 0.0f;
        }
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 120.0f))
            mApp->AddTodParticle(400.0f, 300.0f, 400000, ParticleEffect(103));
    }

    if (mCreditsPhase == CREDITS_MAIN1)
    {
        float aStrobe[] = {128, 130, 132, 134, 136, 138, 140, 142};
        for (int i = 0; i < 8; i++)
        {
            if (aCredits->ShouldTriggerTimedEvent(aFrame * aStrobe[i]))
                mApp->AddTodParticle(400.0f, 300.0f, 400000, ParticleEffect(101));
        }
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 136.5f))
            mApp->AddTodParticle(400.0f, 300.0f, 400000, ParticleEffect(102));
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 330.0f))
            mApp->PlayFoley((FoleyType)85);
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 336.0f))
            mApp->mSoundSystem->StopFoley((FoleyType)85);
    }

    if (mCreditsPhase == CREDITS_MAIN2)
    {
        float aStrobe[] = {111.5f,
            115.5f,
            119.5f,
            121.5f,
            123.5f,
            125.5f,
            127.5f,
            131.5f,
            135.5f,
            139.5f,
            143.5f,
            147.5f,
            151.5f,
            155.5f,
            159.5f,
            163.5f,
            167.5f,
            171.5f,
            175.5f,
            179.5f,
            183.5f,
            187.5f,
            191.5f,
            195.5f,
            199.5f,
            203.5f,
            207.5f,
            211.5f,
            215.5f,
            219.5f,
            223.5f,
            227.5f,
            231.5f,
            235.5f,
            239.5f,
            243.5f};
        for (int i = 0; i < 36; i++)
        {
            if (aCredits->ShouldTriggerTimedEvent(aFrame * aStrobe[i]))
                mApp->AddTodParticle(400.0f, 300.0f, 400000, ParticleEffect(101));
        }
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 332.75f))
            mApp->AddTodParticle(678.0f, 352.0f, 400000, PARTICLE_MELONSPLASH);
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 336.0f))
        {
            TodParticleSystem *aSplash = nullptr;
            while (IterateParticles(aSplash, mApp->mEffectSystem->mParticleHolder))
            {
                if (aSplash->mEffectType == 0)
                    aSplash->ParticleSystemDie();
            }
        }
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 342.0f))
            mApp->AddTodParticle(400.0f, 300.0f, 400000, ParticleEffect(101));

        ReanimatorTransform aBackground;
        aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background2"), &aBackground);
        float aFogX = aBackground.mTransX + 856.0f;
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 188.0f))
        {
            TodParticleSystem *aFog = mApp->AddTodParticle(aFogX, 230.0f, 400000, PARTICLE_STARBURST);
            mFogParticleID = mApp->ParticleGetID(aFog);
        }
        TodParticleSystem *aFog = mApp->ParticleTryToGet(mFogParticleID);
        if (aFog != nullptr)
        {
            if (aCredits->ShouldTriggerTimedEvent(aFrame * 248.0f))
            {
                aFog->ParticleSystemDie();
                mFogParticleID = ParticleSystemID(0);
            }
            else
            {
                aFog->SystemMove(aFogX, 230.0f);
            }
        }
    }

    if (mCreditsPhase == CREDITS_MAIN3)
    {
        if (aCredits->ShouldTriggerTimedEvent(aFrame * 65.0f))
            mApp->PlayFoley((FoleyType)51);
        float aStrobe[] = {111, 115, 119, 121, 123, 219, 223, 227, 231, 235, 239, 243, 247};
        for (int i = 0; i < 13; i++)
        {
            if (aCredits->ShouldTriggerTimedEvent(aFrame * aStrobe[i]))
                mApp->AddTodParticle(400.0f, 300.0f, 400000, ParticleEffect(101));
        }
    }

    const CreditsTiming *aBefore = nullptr;
    const CreditsTiming *aAfter = nullptr;
    float aFraction = 0.0f;
    GetTiming(&aBefore, &aAfter, &aFraction);
    mDrawBrain = false;
    int aMouth = 0;
    if (aBefore != nullptr && aAfter != nullptr)
    {
        CreditBrainType aMotion = aAfter->mBrainType;
        if (aMotion == BRAIN_FLY_ON)
        {
            mBrainPosX = TodAnimateCurveFloatTime(
                0.0f, 1.0f, aFraction, -50.0f, (float)aAfter->mWordX - 15.0f, CURVE_EASE_IN_OUT);
            mBrainPosY = TodAnimateCurveFloatTime(0.0f, 1.0f, aFraction, 505.0f, 485.0f, CURVE_BOUNCE_FAST_MIDDLE);
            mDrawBrain = true;
        }
        else if (aMotion == BRAIN_FAST_ON)
        {
            mBrainPosX = TodAnimateCurveFloatTime(
                0.0f, 1.0f, aFraction, (float)aAfter->mWordX - 50.0f, (float)aAfter->mWordX - 15.0f, CURVE_EASE_IN_OUT);
            mBrainPosY = TodAnimateCurveFloatTime(0.0f, 1.0f, aFraction, 485.0f, 505.0f, CURVE_EASE_IN_OUT);
            mDrawBrain = true;
        }
        else if (aMotion == BRAIN_FLY_OFF)
        {
            mBrainPosX = TodAnimateCurveFloatTime(
                0.0f, 1.0f, aFraction, (float)aBefore->mWordX - 15.0f, 850.0f, CURVE_EASE_IN_OUT);
            mBrainPosY = TodAnimateCurveFloatTime(0.0f, 1.0f, aFraction, 505.0f, 485.0f, CURVE_BOUNCE_FAST_MIDDLE);
            mDrawBrain = true;
        }
        else if (aMotion == BRAIN_FAST_OFF)
        {
            mBrainPosX = TodAnimateCurveFloatTime(0.0f,
                1.0f,
                aFraction,
                (float)aBefore->mWordX - 15.0f,
                (float)aBefore->mWordX + 25.0f,
                CURVE_EASE_IN_OUT);
            mBrainPosY = TodAnimateCurveFloatTime(0.0f, 1.0f, aFraction, 505.0f, 485.0f, CURVE_EASE_IN_OUT);
            mDrawBrain = true;
        }
        else if (aBefore->mBrainType == BRAIN_FLY_ON || aBefore->mBrainType == BRAIN_FAST_ON ||
            aBefore->mBrainType == BRAIN_NEXT_WORD)
        {
            mBrainPosX = TodAnimateCurveFloatTime(0.0f,
                1.0f,
                aFraction,
                (float)aBefore->mWordX - 15.0f,
                (float)aAfter->mWordX - 15.0f,
                CURVE_EASE_IN_OUT);
            mBrainPosY = TodAnimateCurveFloatTime(0.0f, 1.0f, aFraction, 505.0f, 485.0f, CURVE_BOUNCE_FAST_MIDDLE);
            mDrawBrain = true;
        }

        float aSpan = aAfter->mFrame - aBefore->mFrame;
        if (aBefore->mWordType != WORD_OFF && (aSpan * aFraction < 0.2f || (1.0f - aFraction) * aSpan < 0.4f))
            aMouth = 1;
        else if (aBefore->mWordType == WORD_AA)
            aMouth = 2;
        else if (aBefore->mWordType == WORD_EE)
            aMouth = 3;
        else if (aBefore->mWordType == WORD_AW)
            aMouth = 4;
        else if (aBefore->mWordType == WORD_OH)
            aMouth = 5;
    }

    Reanimation *aFlower = aCredits->FindSubReanim(REANIM_SUNFLOWER);
    if (aFlower == nullptr)
        return 0;
    if (mCreditsPhase == CREDITS_MAIN3 && aCredits->mAnimTime > aFrame * 255.0f)
    {
        aFlower->SetImageOverride("anim_idle", IMAGE_REANIM_SUNFLOWER_HEAD_WINK);
        Reanimation *aStage = aCredits->FindSubReanim(REANIM_CREDITS_STAGE);
        aFlower->mAnimTime -= aFlower->mAnimRate * 0.01f / (float)aFlower->mFrameCount;
        aStage->mAnimTime -= aStage->mAnimRate * 0.01f / (float)aStage->mFrameCount;
        return 0;
    }
    Sexy::Image *aHead = nullptr;
    if (aMouth == 1)
        aHead = IMAGE_REANIM_SUNFLOWER_HEAD_SING1;
    else if (aMouth == 2)
        aHead = IMAGE_REANIM_SUNFLOWER_HEAD_SING2;
    else if (aMouth == 3)
        aHead = IMAGE_REANIM_SUNFLOWER_HEAD_SING3;
    else if (aMouth == 4)
        aHead = IMAGE_REANIM_SUNFLOWER_HEAD_SING4;
    else if (aMouth == 5)
        aHead = IMAGE_REANIM_SUNFLOWER_HEAD_SING5;
    aFlower->SetImageOverride("anim_idle", aHead);
    return 0;
}

// ==========================================================================
// CreditScreen::JumpToFrame(CreditsPhase,float)
// @001BBC18  714 bytes
// ==========================================================================
int CreditScreen::JumpToFrame(CreditsPhase thePhase, float theTime)
{
    mFogParticleID = ParticleSystemID(0);
    mApp->mEffectSystem->EffectSystemFreeAll();
    if (thePhase == CREDITS_END)
    {
        Reanimation *aCredits = PlayReanim(3);
        mApp->mMusic->PlayFromOffset(5, 14046238, 1.0);
        aCredits->mAnimTime = 1.0f;
        mCreditsPhase = CREDITS_END;
        return 0;
    }

    Reanimation *aCredits = PlayReanim((int)thePhase + 1);
    // Each phase's music starts at a fixed sample, then advances 12142 samples
    // per credits frame from the nearest cue at or before the requested time.
    float aBase = 0.0f;
    float aCue = 0.0f;
    if (thePhase == CREDITS_MAIN1)
    {
        if (theTime >= 368.0f)
        {
            aCue = 368.0f;
            aBase = 4634500.0f;
        }
        else if (theTime >= 340.0f)
        {
            aCue = 340.0f;
            aBase = 4280700.0f;
        }
        else if (theTime >= 304.0f)
        {
            aCue = 304.0f;
            aBase = 3825700.0f;
        }
        else if (theTime >= 272.0f)
        {
            aCue = 272.0f;
            aBase = 3421800.0f;
        }
        else if (theTime >= 144.0f)
        {
            aCue = 144.0f;
            aBase = 1805700.0f;
        }
        else if (theTime >= 128.0f)
        {
            aCue = 128.0f;
            aBase = 1603700.0f;
        }
    }
    else if (thePhase == CREDITS_MAIN2)
    {
        if (theTime >= 320.0f)
        {
            aCue = 320.0f;
            aBase = 9069100.0f;
        }
        else if (theTime >= 248.0f)
        {
            aCue = 248.0f;
            aBase = 8159800.0f;
        }
        else if (theTime >= 188.0f)
        {
            aCue = 188.0f;
            aBase = 7401500.0f;
        }
        else if (theTime >= 124.0f)
        {
            aCue = 124.0f;
            aBase = 6593500.0f;
        }
        else
        {
            aBase = 5026400.0f;
        }
    }
    else if (thePhase == CREDITS_MAIN3)
    {
        if (theTime >= 240.0f)
        {
            aCue = 240.0f;
            aBase = 12898000.0f;
        }
        else if (theTime >= 216.0f)
        {
            aCue = 216.0f;
            aBase = 12595000.0f;
        }
        else if (theTime >= 124.0f)
        {
            aCue = 124.0f;
            aBase = 11434000.0f;
        }
        else
        {
            aBase = 9864900.0f;
        }
    }
    int aSample = (int)((theTime - aCue) * 12142.0f + aBase);
    mApp->mMusic->PlayFromOffset(5, aSample - 900, 1.0);
    aCredits->mAnimTime = theTime / (float)(aCredits->mFrameCount - 1);
    mCreditsPhase = thePhase;
    return 0;
}

// ==========================================================================
// CreditScreen::KeyChar(char)
// @001BBF84  342 bytes
// ==========================================================================
int CreditScreen::KeyChar(char theChar)
{
    // The debug-keys gate is SexyAppBase::mDebugKeysEnabled at +0x5E8, which
    // the ARM reads as `LDRB R3,[R3,#0x5E8]`.
    if (!mCreditsPaused && mApp->mDebugKeysEnabled && gDisableDebugKeys == 0)
    {
        switch (theChar)
        {
            case '1':
                JumpToFrame(CREDITS_MAIN1, 0.0);
                break;
            case '2':
                JumpToFrame(CREDITS_MAIN1, 128.0);
                break;
            case '3':
                JumpToFrame(CREDITS_MAIN1, 144.0);
                break;
            case '4':
                JumpToFrame(CREDITS_MAIN1, 272.0);
                break;
            case '5':
                JumpToFrame(CREDITS_MAIN1, 304.0);
                break;
            case '6':
                JumpToFrame(CREDITS_MAIN1, 340.0);
                break;
            case '7':
                JumpToFrame(CREDITS_MAIN1, 368.0);
                break;
            case 'q':
                JumpToFrame(CREDITS_MAIN2, 0.0);
                break;
            case 'w':
                JumpToFrame(CREDITS_MAIN2, 124.0);
                break;
            case 'e':
                JumpToFrame(CREDITS_MAIN2, 188.0);
                break;
            case 'r':
                JumpToFrame(CREDITS_MAIN2, 248.0);
                break;
            case 't':
                JumpToFrame(CREDITS_MAIN2, 320.0);
                break;
            case 'a':
                JumpToFrame(CREDITS_MAIN3, 0.0);
                break;
            case 's':
                JumpToFrame(CREDITS_MAIN3, 124.0);
                break;
            case 'd':
                JumpToFrame(CREDITS_MAIN3, 216.0);
                break;
            case 'f':
                JumpToFrame(CREDITS_MAIN3, 240.0);
                break;
            case 'g':
                JumpToFrame(CREDITS_MAIN3, 324.0);
                break;
            case 'n':
                mDontSync = !mDontSync;
                break;
            default:
                break;
        }
    }
    return 1;
}

// ==========================================================================
// CreditScreen::Update()
// @001BC0E4  314 bytes
// ==========================================================================
void CreditScreen::Update()
{
    Sexy::Widget::Update();
    // @0x1BC0F2 skips movie loading and draw-count gating for the final roll.
    if (mCreditsPhase == CREDITS_END)
    {
        if (!mCreditsPaused)
            UpdateMovie();
        mLastDrawCount = mDrawCount;
        MarkDirty();
        return;
    }
    if (mDrawCount == 0 || mCreditsPaused)
        return;

    ++mUpdateCount;
    if (mUpdateCount == 1)
    {
        PreLoadCredits();
        PlayReanim(1);
        // 13 is the credits tune, "Zombies on Your Lawn".
        mApp->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CREDITS_ZOMBIES_ON_YOUR_LAWN);
        mApp->ClearUpdateBacklog(false);
        mTimerSinceStart.Start();
    }
    else if (mDontSync)
    {
        UpdateMovie();
    }
    else
    {
        // The movie should sit on the wall clock. Later phases add the time the
        // earlier phases already spent. More than ten seconds behind jumps a phase;
        // a smaller gap catches up a tenth of the missing time.
        Reanimation *aReanim = mApp->ReanimationGet(mCreditsReanimID);
        int aElapsed = (int)mTimerSinceStart.GetDuration();
        int aMovieTime = (int)((aReanim->mFrameCount * aReanim->mAnimTime / aReanim->mAnimRate) * 1000.0f);
        if (mCreditsPhase == CREDITS_MAIN2)
            aMovieTime += 57142;
        else if (mCreditsPhase == CREDITS_MAIN3)
            aMovieTime += 112000;

        int aBehind = aElapsed - aMovieTime;
        if (aBehind > 10000)
            JumpToFrame((CreditsPhase)((int)mCreditsPhase + 1), 0.0f);
        else
        {
            int aSteps = (aBehind + 5) / 10;
            for (int i = 0; i < aSteps; i++)
                UpdateMovie();
        }
    }
    mLastDrawCount = mDrawCount;
    MarkDirty();
}

// ==========================================================================
// CreditScreen::PauseCredits()
// @001BC224  164 bytes
// ==========================================================================
void CreditScreen::PauseCredits()
{
    if (mCreditsPaused)
        return;

    mApp->mSoundSystem->StopFoley((FoleyType)85);
    mApp->PlaySample(SOUND_PAUSE, true);
    mCreditsPaused = true;
    mTimerSinceStart.GetDuration();
    mApp->mMusic->GameMusicPause(true);
    if (mApp->LawnMessageBox(
            51, "[CREDITS_PAUSE_HEADER]", "[CREDITS_PAUSE_BODY]", "[CREDITS_RESUME_BUTTON]", "[MAIN_MENU_BUTTON]", 1) ==
        1001)
    {
        mApp->KillCreditScreen();
        mApp->DoBackToMain();
    }
    mCreditsPaused = false;
    mApp->mMusic->GameMusicPause(false);
}

// ==========================================================================
// CreditScreen::KeyDown(Sexy::KeyCode)
// @001BC2E0  112 bytes
// ==========================================================================
int CreditScreen::KeyDown(Sexy::KeyCode theKey)
{
    if (mCreditsPaused)
        return 0;

    // Enter, space and escape pause. X replays the MV after the final roll.
    if (theKey == 13 || theKey == 32 || theKey == 27)
    {
        PauseCredits();
        return 1;
    }
    if (theKey != 307 && theKey != 88)
        return Sexy::Widget::KeyDown(theKey);
    if (mCreditsPhase == CREDITS_END && (!mIsFromMainMenu || mApp->HasFinishedAdventure()))
    {
        mApp->KillCreditScreen();
        mApp->ShowCreditScreen(false);
    }
    return 1;
}

// ==========================================================================
// CreditScreen::Draw(Sexy::Graphics *)
// @001BC390  2636 bytes
// ==========================================================================
void CreditScreen::Draw(Sexy::Graphics *theGraphics)
{
    theGraphics->SetLinearBlend(true);
    theGraphics->SetColor(Sexy::Color::Black);
    theGraphics->FillRect(LawnApp::FULLSCREEN_RECT);
    if (mCreditsPhase == CREDITS_END)
    {
        mApp->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
        DrawFinalCredits(theGraphics);
        return;
    }
    if (!mPreloaded)
    {
        mDrawCount = 1;
        return;
    }
    ++mDrawCount;

    Reanimation *aCredits = mApp->ReanimationGet(mCreditsReanimID);
    float aFrame = 1.0f / (float)(aCredits->mFrameCount - 1);
    ReanimatorTransform aBack;
    ReanimatorTransform aBack2;
    aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background"), &aBack);
    aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background2"), &aBack2);

    // The first moments of a phase show the previous lawn through the windows
    // the credits tracks open. A missing frame means that window is shut.
    bool aEarly = aCredits->mAnimTime < aFrame * 125.0f;
    if (mCreditsPhase == CREDITS_MAIN2 && aEarly)
    {
        ReanimatorTransform aBack3;
        ReanimatorTransform aBack4;
        aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background3"), &aBack3);
        aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background4"), &aBack4);
        if (aBack2.mFrame != -1.0f && aBack2.mImage != nullptr)
        {
            theGraphics->PushState();
            theGraphics->ClipRect(
                (int)aBack2.mTransX, (int)aBack2.mTransY, aBack2.mImage->mWidth - 1, aBack2.mImage->mHeight - 1);
            theGraphics->Translate(LawnApp::FULLSCREEN_RECT.mX, 0);
            theGraphics->DrawImageF(IMAGE_BACKGROUND1, aBack2.mTransX - 400.0f, aBack2.mTransY - 300.0f);
            theGraphics->PopState();
        }
        if (aBack3.mFrame != -1.0f && aBack3.mImage != nullptr)
        {
            theGraphics->PushState();
            theGraphics->ClipRect(
                (int)aBack3.mTransX, (int)aBack3.mTransY, aBack3.mImage->mWidth - 1, aBack3.mImage->mHeight - 1);
            theGraphics->Translate(LawnApp::FULLSCREEN_RECT.mX, 0);
            theGraphics->DrawImageF(IMAGE_BACKGROUND1, aBack3.mTransX - 600.0f, aBack3.mTransY - 300.0f);
            theGraphics->PopState();
        }
        if (aBack4.mFrame != -1.0f && aBack4.mImage != nullptr)
        {
            theGraphics->PushState();
            theGraphics->ClipRect(
                (int)aBack4.mTransX, (int)aBack4.mTransY, aBack4.mImage->mWidth - 1, aBack4.mImage->mHeight - 1);
            theGraphics->Translate(LawnApp::FULLSCREEN_RECT.mX, 0);
            theGraphics->DrawImageF(IMAGE_BACKGROUND2, aBack4.mTransX - 400.0f, aBack4.mTransY - 300.0f);
            theGraphics->PopState();
        }
    }
    else if (mCreditsPhase == CREDITS_MAIN3 && aEarly)
    {
        ReanimatorTransform aBack1;
        ReanimatorTransform aBack3;
        ReanimatorTransform aBack4;
        aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background"), &aBack1);
        aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background3"), &aBack3);
        aCredits->GetCurrentTransform(aCredits->FindTrackIndex("Background4"), &aBack4);
        if (aBack1.mFrame != -1.0f && aBack1.mImage != nullptr)
        {
            theGraphics->PushState();
            theGraphics->ClipRect(
                (int)aBack1.mTransX, (int)aBack1.mTransY, aBack1.mImage->mWidth - 1, aBack1.mImage->mHeight - 1);
            theGraphics->Translate(LawnApp::FULLSCREEN_RECT.mX, 0);
            theGraphics->DrawImageF(IMAGE_BACKGROUND1, aBack1.mTransX - 80.0f, aBack1.mTransY - 360.0f);
            theGraphics->PopState();
        }
        if (aBack3.mFrame != -1.0f && aBack3.mImage != nullptr)
        {
            theGraphics->PushState();
            theGraphics->mTransX += aBack3.mTransX - 20.0f;
            theGraphics->mTransY += aBack3.mTransY - 260.0f;
            theGraphics->ClipRect(20, 260, aBack3.mImage->mWidth - 1, aBack3.mImage->mHeight - 1);
            theGraphics->Translate(LawnApp::FULLSCREEN_RECT.mX, 0);
            theGraphics->DrawImageF(IMAGE_BACKGROUND3, -220.0f, 0.0f);
            theGraphics->PopState();
        }
        if (aBack4.mFrame != -1.0f && aBack4.mImage != nullptr)
        {
            theGraphics->PushState();
            theGraphics->ClipRect(
                (int)aBack4.mTransX, (int)aBack4.mTransY, aBack4.mImage->mWidth - 1, aBack4.mImage->mHeight - 1);
            theGraphics->Translate(LawnApp::FULLSCREEN_RECT.mX, 0);
            theGraphics->DrawImageF(IMAGE_BACKGROUND2, aBack4.mTransX - 400.0f, aBack4.mTransY - 300.0f);
            theGraphics->PopState();
        }
    }

    aCredits->DrawRenderGroup(theGraphics, 1);

    float aTime = aCredits->mAnimTime;
    bool aDrawPool = false;
    bool aDrawDoor = false;
    bool aDrawFog = false;
    bool aDrawChimney = false;
    if (mCreditsPhase == CREDITS_MAIN1)
    {
        aDrawPool = aTime > aFrame * 203.0f && aTime < aFrame * 339.0f;
        aDrawDoor = aTime > aFrame * 305.0f && aTime < aFrame * 339.0f;
    }
    else if (mCreditsPhase == CREDITS_MAIN2)
    {
        aDrawPool = aTime > aFrame * 187.0f && aTime < aFrame * 249.0f;
        aDrawFog = aBack2.mFrame != -1.0f && aTime > aFrame * 189.0f && aTime < aFrame * 249.0f;
    }
    else if (mCreditsPhase == CREDITS_MAIN3)
    {
        aDrawChimney = aTime > aFrame * 123.0f && aTime < aFrame * 218.0f;
    }
    if (aBack2.mFrame == -1.0f)
    {
        aDrawDoor = false;
        aDrawChimney = false;
    }

    Sexy::Graphics aWindow(*theGraphics);
    aWindow.mTransX += aBack2.mTransX + 220.0f;
    if (aDrawPool)
        mApp->mPoolEffect->PoolEffectDraw(&aWindow, false);
    if (aDrawDoor)
        aWindow.DrawImage(IMAGE_BACKGROUND3_GAMEOVER_INTERIOR_OVERLAY, -171, 241);

    aCredits->DrawRenderGroup(theGraphics, 0);
    if (aDrawDoor)
        theGraphics->ClipRect(48, 0, 800, 600);
    if (aDrawFog)
        aWindow.DrawImage(IMAGE_REANIM_CREDITS_FOGMACHINE, 600, 200);
    aCredits->DrawRenderGroup(theGraphics, 2);
    if (aDrawDoor)
    {
        theGraphics->ClearClipRect();
        aWindow.DrawImage(IMAGE_BACKGROUND3_GAMEOVER_MASK, -172, 234);
    }
    if (aDrawChimney)
        aWindow.DrawImage(IMAGE_BACKGROUND5_GAMEOVER_MASK, -220, 81);

    TodParticleSystem *aParticle = nullptr;
    while (IterateParticles(aParticle, mApp->mEffectSystem->mParticleHolder))
    {
        if (!aParticle->mIsAttachment && !aParticle->mDead)
            aParticle->Draw(theGraphics);
    }
    if (aDrawFog)
    {
        float aPercent = TodAnimateCurveFloatTime(aFrame * 189.0f, aFrame * 249.0f, aTime, 0.0f, 1.0f, CURVE_LINEAR);
        DrawFogEffect(&aWindow, aPercent);
    }
    aCredits->DrawRenderGroup(theGraphics, 3);
    if (mDrawBrain)
        theGraphics->DrawImageF(IMAGE_BRAIN, mBrainPosX, mBrainPosY);
}
