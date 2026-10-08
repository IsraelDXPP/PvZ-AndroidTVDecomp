// ChallengeScreen -- reconstructed from libGameMain.so.
//
// Every body below was re-derived from the IDA decompilation. The dump writes
// `this`-relative members as word indices (`*((_DWORD *)this + N)`) or byte
// offsets (`*((_BYTE *)this + N)`); each one that lands on a member is written
// as that member here, and each vtable-slot call is written as the real call.
//
// ---------------------------------------------------------------------------
// Two call shapes this file has to keep, because of how the tree declares them
// ---------------------------------------------------------------------------
//
// * TodLib string helpers. `TodStringTranslate`, `TodReplaceNumberString` and
//   the `StringCtor` string constructor all hand a pvzstl string back through
//   the ARM sret pointer, and Sexy\ReconFreeFunctions.h declares them variadic
//   (see the TODO(recon) notes in Sexy\ReconVariadic.cpp). At those call sites
//   the destination string is therefore passed explicitly, which is the shape
//   the binary uses:
//       Sexy::String aText;
//       TodStringTranslate(&aText, "[MODE_LOCKED]");
//
// * `GetChallengeDefinition` is declared `int GetChallengeDefinition(...)` for
//   the same reason, so a call site gets the record's address back as a plain
//   int. The authoritative PvZ header's record is
//       +0x00  GameMode     mChallengeMode
//       +0x04  int          mChallengeIconIndex
//       +0x08  int          mPage            (a ChallengePage)
//       +0x0C  int          mRow
//       +0x10  int          mCol
//       +0x14  const char  *mChallengeName
//   and the bodies here read exactly those words. ChallengeScreen.h's trailing
//   TODO(recon) records that the tree still has no declaration for the record.

#include "ChallengeScreen.h"
#include "ButtonListener.h"
#include "ButtonWidget.h"
#include "Challenge.h"
#include "Curve1DUtil.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnPlayerInfo.h"
#include "ReconTypes.h"
#include "ToolTipWidget.h"
#include "UnlockDialog.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include <cstdio>
#include <algorithm>

namespace {
// ChallengePage values, from the authoritative PvZ\Lawn\Common\ConstEnums.h.
// Sexy\ReconTypes.h carries ChallengePage as a one-int surrogate struct with no
// enumerators, so the values this file compares `mPage` against are named here.
;

// GetChallengeDefinition() is declared variadic and returns the record's
// address as a plain int (see the file banner), so every call site casts.
const ChallengeDefinition *ChallengeDefOf(int theIndex)
{
    return GetChallengeDefinition(theIndex);
}
} // namespace

// ==========================================================================
// ChallengeScreen::ButtonPress(int)
// @0018A098  28 bytes
// ==========================================================================
void ChallengeScreen::ButtonPress(int theId)
{
    // The click sound is the whole body; the button id is not used.
    mApp->PlaySample(SOUND_BUTTONCLICK, true);
}

// ==========================================================================
// ChallengeScreen::ShowPageButtons()
// @0018A2AC  28 bytes
// ==========================================================================
bool ChallengeScreen::ShowPageButtons()
{
    // 0xFFFFFFFB clears bit 2, i.e. it excludes CHALLENGE_PAGE_PUZZLE (4).
    if (mApp->mTodCheatKeys)
        return (mPage & ~4) != 0;
    return false;
}

// ==========================================================================
// ChallengeScreen::UpdateButtons()
// @0018A2C8  104 bytes
// ==========================================================================
int ChallengeScreen::UpdateButtons()
{
    int aChallengeMode = 0;
    int result = 0;

    do
    {
        const ChallengeDefinition *aChlDef = ChallengeDefOf(aChallengeMode);
        Sexy::ButtonWidget *aButton = mChallengeButtons[aChallengeMode];
        result = aChlDef->mPage; // mPage

        if (result == mPage)
        {
            aButton->mVisible = true;
            if (mSelectedChallengeIndex == GAMEMODE_CHALLENGE_BUTTERED_POPCORN)
            {
                mSelectedChallengeIndex = GetChallengeByRowColumn(result, 0, 0);
                result = GetModeByRowColumn(mPage, 0, 0);
                mSelectedGameMode = (GameMode)result;
            }
        }
        else
        {
            aButton->mVisible = false;
        }

        ++aChallengeMode;
    }
    while (aChallengeMode != NUM_CHALLENGE_MODES);

    return result;
}

// ==========================================================================
// ChallengeScreen::ButtonDepress(int)
// @0018A330  84 bytes
// ==========================================================================
void ChallengeScreen::ButtonDepress(int theId)
{
    if (theId == ChallengeScreen_Back)
    {
        mApp->KillChallengeScreen();
        mApp->DoBackToMain();
        return;
    }

    // ChallengeScreen_Mode + i launches game mode i + 2 (the challenge modes
    // start at GAMEMODE_SURVIVAL_NORMAL_STAGE_1 == 2).
    int aChallengeMode = theId - ChallengeScreen_Mode;
    if ((unsigned int)aChallengeMode <= 0x5D)
    {
        mApp->KillChallengeScreen();
        mApp->PreNewGame((GameMode)(aChallengeMode + 2), true);
    }

    int aPageIndex = theId - ChallengeScreen_Page;
    if ((unsigned int)aPageIndex <= 5)
    {
        mPage = ChallengePage(aPageIndex);
        UpdateButtons();
    }
}

// ==========================================================================
// ChallengeScreen::IsScaryPotterLevel(GameMode)
// @0018A3BC  34 bytes
// ==========================================================================
bool ChallengeScreen::IsScaryPotterLevel(GameMode theGameMode)
{
    // Levels 1 through 9. Endless (70) is a different page and is not included.
    return theGameMode >= GAMEMODE_SCARY_POTTER_1
        && theGameMode <= GAMEMODE_SCARY_POTTER_9
        && (theGameMode % 2) == 0;
}

// ==========================================================================
// ChallengeScreen::IsIZombieLevel(GameMode)
// @0018A3E0  34 bytes
// ==========================================================================
bool ChallengeScreen::IsIZombieLevel(GameMode theGameMode)
{
    return theGameMode >= GAMEMODE_PUZZLE_I_ZOMBIE_1
        && theGameMode <= GAMEMODE_PUZZLE_I_ZOMBIE_9
        && (theGameMode % 2) == 1;
}

// ==========================================================================
// ChallengeScreen::MoreTrophiesNeeded(int)
// @0018A404  688 bytes
// ==========================================================================
namespace {
// The adventure-progress answer for a challenge that sits at `aRowCol`
// (5 * mRow + mCol) inside its page: the first three slots of a page are free,
// the fourth needs one trophy and everything past it needs two.
int TrophiesForRowCol(int aRowCol)
{
    if (aRowCol <= 2)
        return 0;
    if (aRowCol == 3)
        return 1;
    return 2;
}
} // namespace

int ChallengeScreen::MoreTrophiesNeeded(int theChallengeIndex)
{
    const ChallengeDefinition *aChlDef = ChallengeDefOf(theChallengeIndex);
    GameMode aMode = aChlDef->mChallengeMode;
    int aPage = aChlDef->mPage;
    int aRowCol = 5 * aChlDef->mRow + aChlDef->mCol;

    // The versus page is never gated.
    if (aMode == GAMEMODE_MP_VS)
        return 0;

    // The co-op pages unlock with the player's adventure level instead: one
    // co-op stage per ten adventure levels.
    if ((unsigned int)(aMode - GAMEMODE_TWO_PLAYER_COOP_DAY) <= 0xA)
    {
        // Flag 1 is the "co-op unlocked" player flag.
        if (mApp->mPlayerInfo->GetFlag(PlayerFlags(1)))
            return 0;
        if (aMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS)
            return !mApp->mPlayerInfo->GetFlag(PlayerFlags(1));

        int aLevelsAhead = mApp->mPlayerInfo->mLevel / 10 + GAMEMODE_TWO_PLAYER_COOP_DAY - aMode;
        if (aLevelsAhead >= 0)
            return 0;
        return (aLevelsAhead != -1) ? 2 : 1;
    }

    // The upsell screen shows the challenge list as a teaser; only the final
    // boss is playable there.
    if (mApp->mGameMode == GAMEMODE_UPSELL && mApp->mGameScene == SCENE_LEVEL_INTRO)
        return aMode == GAMEMODE_CHALLENGE_FINAL_BOSS;

    bool aTrialLocked = mApp->IsTrialStageLocked() != 0;

    if (aTrialLocked)
    {
        // The trial build only opens the first few stages of each page.
        if (aPage == CHALLENGE_PAGE_CHALLENGE && aMode > GAMEMODE_TWO_PLAYER_COOP_ENDLESS)
            return (aMode != GAMEMODE_CHALLENGE_RAINING_SEEDS) ? 2 : 1;
        if (aPage == CHALLENGE_PAGE_SURVIVAL && aMode > GAMEMODE_SURVIVAL_NORMAL_STAGE_3)
            return (aMode != GAMEMODE_SURVIVAL_NORMAL_STAGE_4) ? 2 : 1;
    }

    // The two puzzle pages gate on how many of their own challenges are beaten.
    if (aPage == CHALLENGE_PAGE_PUZZLE)
    {
        if (aTrialLocked && aMode > GAMEMODE_PUZZLE_I_ZOMBIE_3)
            return (aMode != GAMEMODE_SCARY_POTTER_4) ? 2 : 1;

        if (IsScaryPotterLevel(aMode) || IsIZombieLevel(aMode))
        {
            bool aIsIZombie = !IsScaryPotterLevel(aMode);

            int aBeaten = 0;
            for (int i = 0; i < NUM_CHALLENGE_MODES; ++i)
            {
                GameMode aDefMode = ChallengeDefOf(i)->mChallengeMode;
                bool aOnThisPage = aIsIZombie ? IsIZombieLevel(aDefMode) : IsScaryPotterLevel(aDefMode);
                if (aOnThisPage && mApp->HasBeatenChallenge(aDefMode))
                    ++aBeaten;
            }

            int aFirst = aIsIZombie ? GAMEMODE_PUZZLE_I_ZOMBIE_1 : GAMEMODE_SCARY_POTTER_1;
            int aFourth = aIsIZombie ? GAMEMODE_PUZZLE_I_ZOMBIE_4 : GAMEMODE_SCARY_POTTER_4;
            // Past this stage the page needs adventure finished and three wins.
            int aLastFree = aIsIZombie ? GAMEMODE_PUZZLE_I_ZOMBIE_3 : GAMEMODE_SCARY_POTTER_3;

            if (aMode > aLastFree && !mApp->HasFinishedAdventure() && aBeaten > 2)
                return (aMode != aFourth) ? 2 : 1;

            int aResult = (aMode - aFirst) / 2 - aBeaten;
            if (aResult <= 0)
                return 0;
            return (aResult > 8) ? 9 : aResult;
        }
    }

    // Everything else: the first three slots of a page are free, and past that
    // the page has to be three trophies ahead of what the player already has.
    if ((aPage == CHALLENGE_PAGE_CHALLENGE || aPage == CHALLENGE_PAGE_SURVIVAL)
        && !mApp->HasFinishedAdventure())
        return TrophiesForRowCol(aRowCol);

    int aTrophies = mApp->GetNumTrophies(ChallengePage(aPage));
    if (aPage == CHALLENGE_PAGE_LIMBO)
        return 0;
    if (!mApp->IsSurvivalEndless(aMode))
    {
        int aNeeded = aTrophies + 3;
        return (aNeeded <= aRowCol) ? (aRowCol - aNeeded + 1) : 0;
    }
    return 10 - aTrophies;
}

// ==========================================================================
// ChallengeScreen::AccomplishmentsNeeded(int)
// @0018A6B4  90 bytes
// ==========================================================================
int ChallengeScreen::AccomplishmentsNeeded(int theChallengeIndex)
{
    int aTrophiesNeeded = MoreTrophiesNeeded(theChallengeIndex);
    const ChallengeDefinition *aChlDef = ChallengeDefOf(theChallengeIndex);

    // An endless survival page counts as a single accomplishment once the
    // player is on the last trophy and has finished adventure.
    if (mApp->IsSurvivalEndless(aChlDef->mChallengeMode)
        && aTrophiesNeeded <= 3
        && mApp->GetNumTrophies(CHALLENGE_PAGE_SURVIVAL) <= 9
        && mApp->HasFinishedAdventure()
        && !mApp->IsTrialStageLocked())
    {
        aTrophiesNeeded = 1;
    }

    if (mCheatEnableChallenges)
        return 0;
    return aTrophiesNeeded;
}

// ==========================================================================
// ChallengeScreen::DrawButton(Sexy::Graphics *,int,int)
// @0018A710  1960 bytes
// ==========================================================================
void ChallengeScreen::DrawButton(Sexy::Graphics * theGraphics, int theIndex, int thePageIndex)
{
    // theIndex is the challenge mode (the index into mChallengeButtons) and thePageIndex its
    // slot on the page; the button's position and its animation time come from
    // those two arrays.
    const ChallengeDefinition *aChlDef = ChallengeDefOf(theIndex);
    Sexy::ButtonWidget *aButton = mChallengeButtons[theIndex];
    GameMode aMode = aChlDef->mChallengeMode;

    theGraphics->PushState();
    theGraphics->Translate(
        (int)TodAnimateCurveFloatTime(0.0f, 0.25f, mPageChallengeAnimTime[thePageIndex], 0.0f, -25.0f, CURVE_EASE_OUT),
        0);

    int aX = aButton->mX;
    int aY = aButton->mY;
    if (aButton->mIsDown)
    {
        aX += 4;
        aY += 4;
    }

    // A hidden button leaves the state pushed, which is what the binary does.
    if (!aButton->mVisible)
        return;

    // Grey out anything the player has not collected enough trophies for.
    if (aButton->mDisabled || AccomplishmentsNeeded(theIndex) > 1)
    {
        theGraphics->SetColor(Sexy::Color(92, 92, 92));
        theGraphics->SetColorizeImages(true);
    }

    // The challenge that is being unlocked right now fades in from black.
    if (mUnlockChallengeIndex == theIndex)
    {
        if (mUnlockState == UNLOCK_SHAKING)
        {
            theGraphics->SetColor(Sexy::Color(92, 92, 92));
        }
        else if (mUnlockState == UNLOCK_FADING)
        {
            int aShade = TodAnimateCurve(50, 25, mUnlockStateCounter, 92, 255, CURVE_LINEAR);
            theGraphics->SetColor(Sexy::Color(aShade, aShade, aShade));
        }
        theGraphics->SetColorizeImages(true);
    }

    // ---- the thumbnail, the name plate and the label ---------------------
    // The survival and versus pages share one thumbnail sheet; the co-op page
    // uses it too, except for the bowling and boss stages.
    bool aUseSurvivalThumbnails = mPage == CHALLENGE_PAGE_SURVIVAL || mPage == CHALLENGE_PAGE_VS
        || (mPage == CHALLENGE_PAGE_COOP && aMode != GAMEMODE_TWO_PLAYER_COOP_BOWLING
            && aMode != GAMEMODE_TWO_PLAYER_COOP_BOSS);

    Sexy::Image *aThumbnails = aUseSurvivalThumbnails ? IMAGE_SURVIVAL_THUMBNAILS
                                                      : IMAGE_CHALLENGE_THUMBNAILS;
    theGraphics->DrawImageCel(aThumbnails, Sexy::TRect<int>(aX + 6, aY + 2, 104, 104), aChlDef->mChallengeIconIndex);
    theGraphics->SetColorizeImages(false);
    theGraphics->DrawImage(IMAGE_CHALLENGE_NAME_BACK, aX - 6, aY - 2, 720, 118);

    Sexy::Font *aFont = FONT_DWARVENTODCRAFT24;
    Sexy::Color aNameColor(255, 255, 255);
    if (mSelectedChallengeIndex == theIndex && theIndex != mUnlockChallengeIndex)
        aNameColor = Sexy::Color(22, 221, 45);

    Sexy::String aLabel;
    TodStringTranslate(&aLabel, aChlDef->mChallengeName);

    int aTextLeftOffset = 0;
    int aTextHeight = 0;
    int aJustification = 0;
    if (mUnlockChallengeIndex == theIndex && mUnlockState == UNLOCK_SHAKING)
    {
        Sexy::String aUnlocked;
        TodStringTranslate(&aUnlocked, "[MODE_LOCKED]");
        aLabel = aUnlocked;
        aJustification = 2;
        aTextHeight = 20;
        aTextLeftOffset = -40;
        aFont = FONT_HOUSEOFTERROR28;
        aNameColor = Sexy::Color::White;
    }
    else if (aButton->mDisabled)
    {
        Sexy::String aLocked;
        TodStringTranslate(&aLocked, "[MODE_LOCKED]");
        aLabel = aLocked;
        aJustification = 2;
        aTextLeftOffset = -40;
        aTextHeight = 20;
        aFont = FONT_HOUSEOFTERROR28;
    }
    // ---- the wrapped label ----------------------------------------------
    theGraphics->PushState();
    int aFontHeight = aFont->GetHeight();
    Sexy::TRect<int> aLabelRect(
        aTextLeftOffset + aX + 150,
        (int)((float)aY + 60.0f + (float)aTextHeight - (float)aFontHeight),
        550,
        100);
    TodDrawStringWrappedHelper(theGraphics, aLabel, aLabelRect, aFont, aNameColor, aJustification, 1, 0);
    theGraphics->PopState();

    // ---- the trophy / padlock badge --------------------------------------
    int aStreak = mApp->mPlayerInfo->mChallengeRecords[theIndex];
    if (mUnlockChallengeIndex == theIndex)
    {
        if (mUnlockState == UNLOCK_FADING)
        {
            int aAlpha = TodAnimateCurve(25, 0, mUnlockStateCounter, 255, 0, CURVE_LINEAR);
            theGraphics->SetColor(Sexy::Color(255, 255, 255, aAlpha));
            theGraphics->SetColorizeImages(true);
            theGraphics->DrawImage(
                IMAGE_LOCK,
                (int)(mLockShakeX + 10.0f + (float)aX),
                (int)(mLockShakeY + 10.0f + (float)aY));
        }
        else
        {
            theGraphics->DrawImage(IMAGE_LOCK, (int)((float)aX + 10.0f), (int)((float)aY + 10.0f));
        }
        theGraphics->SetColorizeImages(false);
    }
    else if (aStreak <= 0)
    {
        // Never finished: a plain padlock on the locked challenges.
        if (aButton->mDisabled)
            theGraphics->DrawImage(IMAGE_LOCK, (int)((float)aX + 10.0f), (int)((float)aY + 10.0f));
    }
    else if (mApp->HasBeatenChallenge(aMode))
    {
        theGraphics->DrawImage(IMAGE_MINIGAME_TROPHY, aX - 6, aY - 2);
    }
    else if (mApp->IsEndlessScaryPotter(aMode) || mApp->IsEndlessIZombie(aMode))
    {
        // The endless puzzle stages show the longest streak instead.
        Sexy::String aStreakLabel("[LONGEST_STREAK]");
        Sexy::String aStreakText;
        TodReplaceNumberString(&aStreakText, &aStreakLabel, "{STREAK}", aStreak);

        Sexy::TRect<int> aBadgeRect(aX, aY + 15, 120, 50);
        theGraphics->SetColor(Sexy::Color(255, 255, 255));
        TodDrawStringWrapped(theGraphics, aStreakText, aBadgeRect, FONT_CONTINUUMBOLD14OUTLINE, aNameColor, 5, 0);
        theGraphics->SetColor(Sexy::Color(255, 0, 0));
        TodDrawStringWrapped(theGraphics, aStreakText, aBadgeRect, FONT_CONTINUUMBOLD14, aNameColor, 5, 0);
    }
    else if (ChallengeModeRecordsTime(aMode) != 0)
    {
        // The timed challenges show how many flags the player has taken.
        Sexy::String aFlagsText = mApp->Pluralize(aStreak, "[ONE_FLAG]", "[COUNT_FLAGS]");
        theGraphics->SetColor(Sexy::Color(255, 255, 255));
        TodDrawString(theGraphics, aFlagsText, aX + 48, aY + 48, FONT_CONTINUUMBOLD14OUTLINE, aNameColor, 2);
        theGraphics->SetColor(Sexy::Color(255, 0, 0));
        TodDrawString(theGraphics, aFlagsText, aX + 48, aY + 48, FONT_CONTINUUMBOLD14, aNameColor, 2);
    }

    theGraphics->PopState();
}

// ==========================================================================
// ChallengeScreen::Draw(Sexy::Graphics *)
// @0018AEE4  1418 bytes
// ==========================================================================
void ChallengeScreen::Draw(Sexy::Graphics * theGraphics)
{
    theGraphics->SetLinearBlend(true);
    theGraphics->DrawImage(IMAGE_CHALLENGE_BACKGROUND, LawnApp::FULLSCREEN_RECT.mX, LawnApp::FULLSCREEN_RECT.mY);

    // ---- the page title ---------------------------------------------------
    const char *aTitleText = "[PICK_CHALLENGE]";
    switch ((int)mPage)
    {
    case CHALLENGE_PAGE_SURVIVAL: aTitleText = "[PICK_AREA]"; break;
    case CHALLENGE_PAGE_PUZZLE:   aTitleText = "[SCARY_POTTER]"; break;
    case CHALLENGE_PAGE_VS:       aTitleText = "[VS_MODE]"; break;
    case CHALLENGE_PAGE_COOP:     aTitleText = "[XBOX_COOP]"; break;
    default: break;
    }

    Sexy::String aTitle(aTitleText);
    theGraphics->SetColor(Sexy::Color(220, 220, 220));
    TodDrawString(theGraphics, aTitle, 400, 45, FONT_HOUSEOFTERROR28, Sexy::Color(220, 220, 220), 2);

    // ---- the trophy counter ----------------------------------------------
    int aTrophiesGot = mApp->GetNumTrophies(mPage);
    int aTrophiesTotal = 0;
    if (mPage == CHALLENGE_PAGE_SURVIVAL || mPage == CHALLENGE_PAGE_COOP)
        aTrophiesTotal = 10;
    else if (mPage == CHALLENGE_PAGE_CHALLENGE)
        aTrophiesTotal = 20;
    else if (mPage == CHALLENGE_PAGE_PUZZLE)
        aTrophiesTotal = 18;

    if (aTrophiesTotal > 0)
    {
        Sexy::String aFormat;
        TodStringTranslate(&aFormat, "[NUMBER_OF_TROPHIES]");
        Sexy::String aTrophyText;
        StrFormat(&aTrophyText, aFormat.c_str(), aTrophiesGot, aTrophiesTotal);
        theGraphics->SetColor(Sexy::Color(255, 240, 0));
        TodDrawString(theGraphics, aTrophyText, 711, 62, FONT_BRIANNETOD16, Sexy::Color(255, 240, 0), 2);
    }

    TodDrawImageScaledF(theGraphics, IMAGE_TROPHY, 690.0f, 15.0f, 0.5f, 0.5f);

    // ---- the scroll bar ---------------------------------------------------
    theGraphics->PushState();

    int aScrollBarX = 760;
    int aScrollBarY = 80;
    int aScrollBarWidth = 40;
    int aScrollBarHeight = 460;
    int aThumbRectX = 766;
    int aThumbRectWidth = 28;

    float aScrollPosition = (float)mScrollPosition;
    float aScrollBarHeightF = (float)aScrollBarHeight;

    if (mScrollPosition == mScrollTargetPosition)
    {
        // Not scrolling: the bar is full height.
        aScrollBarHeightF = 448.0f;
        aScrollBarHeight = 448;
        aScrollBarY = 86;
    }
    else
    {
        // Scrolling: animate the position and shrink the bar.
        aScrollPosition = TodAnimateCurveFloatTime(
            0.0f, 0.15f, mScrollAnimationTime, aScrollPosition, (float)mScrollTargetPosition, CURVE_LINEAR);
        aScrollBarHeight = aScrollBarHeight - 12;
        aScrollBarY = aScrollBarY + 6;
        aThumbRectX = aScrollBarX + 6;
        aThumbRectWidth = aScrollBarWidth - 12;
        aScrollBarHeightF = (float)aScrollBarHeight;
    }

    int aThumbPosition = (int)((aScrollPosition / (float)mPageChallengeCount) * aScrollBarHeightF);
    int aThumbHeight = (int)(aScrollBarHeightF * (5.0f / (float)mPageChallengeCount));
    int aThumbY = aScrollBarY + aThumbPosition;
    int aThumbHeightClamped = (aThumbHeight > aScrollBarHeight) ? aScrollBarHeight : aThumbHeight;

    theGraphics->ClipRect(aThumbRectX, aScrollBarY, aThumbRectWidth, aScrollBarHeight);
    theGraphics->SetColor(Sexy::Color(0, 128));
    theGraphics->FillRect(
        aScrollBarX + 6, aScrollBarY + 6, aScrollBarWidth - 12, aScrollBarHeight - 12);
    theGraphics->SetColor(Sexy::Color(140, 140, 140, 255));
    theGraphics->FillRect(Sexy::TRect<int>(aThumbRectX, aThumbY, aThumbRectWidth, aThumbHeightClamped));
    theGraphics->ClearClipRect();
    theGraphics->SetColorizeImages(false);
    theGraphics->DrawImageBox(
        Sexy::TRect<int>(aScrollBarX, aScrollBarY, aScrollBarWidth, aScrollBarHeight),
        IMAGE_DLG_SELECTORFRAME);
    theGraphics->PopState();

    // ---- the scrolling list of challenge buttons --------------------------
    theGraphics->PushState();
    theGraphics->ClipRect(-20, 80, 1000, 475);
    theGraphics->TranslateF(0.0f, -(aScrollPosition * 120.0f));

    for (int i = 0; i < mPageChallengeCount; ++i)
        DrawButton(theGraphics, mPageChallengeIndex[i], i);

    theGraphics->ClearClipRect();
    mToolTip->Draw(theGraphics);
    theGraphics->PopState();
}

// ==========================================================================
// ChallengeScreen::UpdateToolTip(void)
// @0018B4B4  808 bytes
// ==========================================================================
void ChallengeScreen::UpdateToolTip()
{
    Sexy::ButtonWidget *aButton = mChallengeButtons[mSelectedChallengeIndex];

    // The tool tip only explains a challenge the player cannot play yet.
    if (!aButton->mVisible || !aButton->mDisabled)
    {
        mToolTip->mVisible = false;
        return;
    }
    if (AccomplishmentsNeeded(mSelectedChallengeIndex) > 1)
    {
        mToolTip->mVisible = false;
        return;
    }

    mToolTip->mX = aButton->mX + aButton->mWidth / 2;
    mToolTip->mY = aButton->mY + 56 - mToolTip->mHeight / 2;

    if (MoreTrophiesNeeded(mSelectedChallengeIndex) <= 0)
    {
        mToolTip->mVisible = false;
        return;
    }

    const ChallengeDefinition *aChlDef = ChallengeDefOf(mSelectedChallengeIndex);
    GameMode aMode = aChlDef->mChallengeMode;

    if (mPage == CHALLENGE_PAGE_PUZZLE)
    {
        if (IsScaryPotterLevel(aMode))
        {
            if (!mApp->HasFinishedAdventure() && aMode == GAMEMODE_SCARY_POTTER_4)
                mToolTip->SetLabel(Sexy::String("[FINISH_ADVENTURE_TOOLTIP]"));
            else
                mToolTip->SetLabel(Sexy::String("[ONE_MORE_SCARY_POTTER_TOOLTIP]"));
            return;
        }
        if (IsIZombieLevel(aMode))
        {
            if (!mApp->HasFinishedAdventure() && aMode == GAMEMODE_PUZZLE_I_ZOMBIE_4)
                mToolTip->SetLabel(Sexy::String("[FINISH_ADVENTURE_TOOLTIP]"));
            else
                mToolTip->SetLabel(Sexy::String("[ONE_MORE_IZOMBIE_TOOLTIP]"));
            return;
        }
    }

    if (mPage == CHALLENGE_PAGE_COOP)
    {
        if (aMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS)
            mToolTip->SetLabel(Sexy::String("[FINISH_ADVENTURE_TOOLTIP]"));
        else
            mToolTip->SetLabel(Sexy::String("[PLAY_MORE_ADVENTURE_TOOLTIP]"));
        mToolTip->mVisible = false;
        return;
    }

    if (mApp->HasFinishedAdventure() && !mApp->IsTrialStageLocked())
    {
        if (mApp->IsSurvivalEndless(aMode))
        {
            mToolTip->SetLabel(Sexy::String("[10_SURVIVAL_TOOLTIP]"));
        }
        else if (mPage == CHALLENGE_PAGE_SURVIVAL)
        {
            mToolTip->SetLabel(Sexy::String("[ONE_MORE_SURVIVAL_TOOLTIP]"));
        }
        else if (mPage == CHALLENGE_PAGE_CHALLENGE)
        {
            mToolTip->SetLabel(Sexy::String("[ONE_MORE_CHALLENGE_TOOLTIP]"));
        }
        else
        {
            mToolTip->mVisible = false;
        }
        return;
    }

    mToolTip->SetLabel(Sexy::String("[FINISH_ADVENTURE_TOOLTIP]"));
}

// ==========================================================================
// ChallengeScreen::GameAxisMove(Sexy::GamepadAxis,int,int)
// @0018B82C  2 bytes
// ==========================================================================
void ChallengeScreen::GameAxisMove()
{
    // Empty in the binary.
}

// ==========================================================================
// ChallengeScreen::GameButtonUp(Sexy::GamepadButton,int,unsigned int)
// @0018B830  2 bytes
// ==========================================================================
void ChallengeScreen::GameButtonUp()
{
    // Empty in the binary.
}

// ==========================================================================
// ChallengeScreen::SetScrollTarget(int)
// @0018B834  34 bytes
// ==========================================================================
ChallengeScreen * ChallengeScreen::SetScrollTarget(int theTarget)
{
    int aTarget = theTarget & ~(theTarget >> 31); // clamp to >= 0
    int aLast = mPageChallengeCount - 1;

    mScrollAnimationTime = 0.0f;
    mScrollTargetPosition = (aLast > aTarget) ? aTarget : aLast;
    return this;
}
// ==========================================================================
// ChallengeScreen::SetUnlockChallengeIndex(ChallengePage,bool)
// @0018B858  156 bytes
// ==========================================================================
int * ChallengeScreen::SetUnlockChallengeIndex(ChallengePage thePage, bool theAnimate)
{
    mUnlockState = UNLOCK_SHAKING;
    mUnlockStateCounter = 100;
    mUnlockChallengeIndex = 0;

    for (int i = 0; i < NUM_CHALLENGE_MODES; ++i)
    {
        const ChallengeDefinition *aChlDef = ChallengeDefOf(i);
        if (aChlDef->mPage != thePage)
            continue;

        if (thePage == CHALLENGE_PAGE_PUZZLE)
        {
            // The two puzzle families alternate on that page, so only the one
            // the caller asked for is eligible.
            GameMode aMode = aChlDef->mChallengeMode;
            if (theAnimate ? !IsIZombieLevel(aMode) : !IsScaryPotterLevel(aMode))
                continue;
        }

        if (AccomplishmentsNeeded(i) > 0)
            continue;

        mUnlockChallengeIndex = i;
        mSelectedChallengeIndex = i;

        if (mPageChallengeCount <= 0)
            continue;

        // Scroll so the newly unlocked challenge lands where the page shows it:
        // slot `j` sits two rows above the top of the list.
        int aScrollTarget = -1;
        for (int j = 0; j < mPageChallengeCount; ++j)
        {
            if (mPageChallengeIndex[j] == i)
            {
                aScrollTarget = j - 2;
                break;
            }
        }
        if (aScrollTarget < 0)
            continue;

        SetScrollTarget(aScrollTarget);
    }

    // The binary leaves a register behind here (the last challenge record it
    // looked at, or SetScrollTarget's result) and no caller reads it.
    return nullptr;
}

// ==========================================================================
// ChallengeScreen::ChallengeScreen(LawnApp *,ChallengePage,unsigned int)
// @0018B8F4  746 bytes
// ==========================================================================
ChallengeScreen::ChallengeScreen(LawnApp * theApp, ChallengePage thePage, unsigned int theFlags)
{
    // theApp is the app and thePage the page; theFlags is unused. The Sexy::Widget base and the
    // ButtonListener subobject at +0x100 are built by the compiler now, so the
    // vptr stores and the explicit base construction the dump shows are gone.
    mApp = theApp;
    mPage = thePage;
    mLockShakeX = 0.0f;
    mLockShakeY = 0.0f;
    mScrollAnimationTime = 0.0f;
    mClip = false;
    mCheatEnableChallenges = false;
    mUnlockState = UNLOCK_OFF;
    mUnlockStateCounter = 0;
    mScrollPosition = 0;
    mScrollTargetPosition = 0;
    mPageChallengeCount = 0;
    mUnlockChallengeIndex = -1;

    for (int i = 0; i < NUM_CHALLENGE_MODES; ++i)
    {
        const ChallengeDefinition *aChlDef = ChallengeDefOf(i);

        Sexy::ButtonWidget *aButton = new Sexy::ButtonWidget(ChallengeScreen_Mode + i, this);
        mChallengeButtons[i] = aButton;
        aButton->mDoFinger = true;
        aButton->mFrameNoDraw = true;

        if (MoreTrophiesNeeded(i) != 0)
        {
            aButton->mDisabled = true;
            aButton->mDoFinger = false;
        }

        if (aChlDef->mPage == mPage)
        {
            // The challenges of this page are stacked 120 pixels apart.
            aButton->Resize(35, (int)((float)mPageChallengeCount * 120.0f + 80.0f), 112, 65);
            mPageChallengeIndex[mPageChallengeCount] = i;
            ++mPageChallengeCount;
        }

        mPageChallengeAnimTime[i] = 0.0f;
    }

    mToolTip = new ToolTipWidget();
    mToolTip->mCenter = true;
    mToolTip->mVisible = false;

    mSelectedChallengeIndex = GAMEMODE_CHALLENGE_BUTTERED_POPCORN;
    UpdateButtons();

    // The upsell screen keeps whatever music it started with.
    if (mApp->mGameMode != GAMEMODE_UPSELL || mApp->mGameScene != SCENE_LEVEL_INTRO)
    {
        // MusicTune 6 is the "choose your seeds" tune. Sexy\ReconTypes.h carries
        // MusicTune as a one-int surrogate with no enumerators.
        mApp->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
    }

    // A pending unlock request for this page is consumed here.
    if (mPage == CHALLENGE_PAGE_SURVIVAL)
    {
        if (mApp->mPlayerInfo->GetFlag(PlayerFlags(512)))
        {
            SetUnlockChallengeIndex(mPage, false);
            mApp->mPlayerInfo->SetFlag(512, 0);
        }
    }
    else if (mPage == CHALLENGE_PAGE_CHALLENGE)
    {
        if (mApp->mPlayerInfo->GetFlag(PlayerFlags(64)))
        {
            SetUnlockChallengeIndex(mPage, false);
            mApp->mPlayerInfo->SetFlag(64, 0);
        }
    }
    else if (mPage == CHALLENGE_PAGE_PUZZLE)
    {
        if (mApp->mPlayerInfo->GetFlag(PlayerFlags(128)))
        {
            SetUnlockChallengeIndex(mPage, false);
            mApp->mPlayerInfo->SetFlag(128, 0);
        }
        else if (mApp->mPlayerInfo->GetFlag(PlayerFlags(256)))
        {
            SetUnlockChallengeIndex(mPage, true);
            mApp->mPlayerInfo->SetFlag(256, 0);
        }
    }

    mHelpBarWidget = mApp->mHelpBarWidget;
    mHelpBarWidget->ClearButtons(0);
    mHelpBarWidget->AddButton(
        GAMEPAD_BUTTON_B, Sexy::String("[BACK]"), HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBarWidget->mDrawShade = false;
}

// ==========================================================================
// ChallengeScreen::KeyChar(char)
// @0018BC2C  206 bytes
// ==========================================================================
int ChallengeScreen::KeyChar(char theChar)
{
    if (!mApp->mDebugKeysEnabled || gDisableDebugKeys != 0)
        return 1;

    // The shipped build still builds this message and throws it away.
    Sexy::String aDebugText;
    StrFormat(&aDebugText, "Challenge cheat key '%c'", theChar);

    if ((theChar & 0xDF) == 0x43) // 'C'
    {
        mCheatEnableChallenges = !mCheatEnableChallenges;

        // The cheat unlocks every challenge; without it each one goes back to
        // whatever its trophy requirement says.
        for (int i = 0; i < NUM_CHALLENGE_MODES; ++i)
        {
            Sexy::ButtonWidget *aButton = mChallengeButtons[i];
            bool aUnlocked = mCheatEnableChallenges || MoreTrophiesNeeded(i) == 0;
            aButton->mDoFinger = aUnlocked;
            aButton->mDisabled = !aUnlocked;
        }
    }
    else if (theChar == 'u')
    {
        SetUnlockChallengeIndex(mPage, false);
    }

    return 1;
}

// ==========================================================================
// ChallengeScreen::KeyDown(Sexy::KeyCode)
// @0018BD0C  592 bytes
// ==========================================================================
int ChallengeScreen::KeyDown(Sexy::KeyCode theKey)
{
    // Find the slot the currently selected challenge sits in.
    int aSelectedSlot = 0;
    for (int i = 0; i < mPageChallengeCount; ++i)
    {
        if (mPageChallengeIndex[i] == mSelectedChallengeIndex)
            aSelectedSlot = i;
    }

    bool aMoved = false;
    if (theKey == KEYCODE_UP)
    {
        if (mScrollPosition == mScrollTargetPosition)
        {
            if (aSelectedSlot != 0)
            {
                --aSelectedSlot;
                if (mScrollPosition > aSelectedSlot)
                    SetScrollTarget(mScrollPosition - 2);
            }
            else
            {
                // Wrap round to the last full row of four.
                aSelectedSlot = mPageChallengeCount - 1;
                int aCount = mPageChallengeCount;
                SetScrollTarget((aCount % 4 != 0) ? (aCount - aCount % 4) : (aCount - 4));
            }
        }
        aMoved = true;
    }
    else if (theKey == KEYCODE_DOWN)
    {
        if (mScrollPosition == mScrollTargetPosition)
        {
            ++aSelectedSlot;
            if (aSelectedSlot >= mPageChallengeCount)
            {
                aSelectedSlot = 0;
                SetScrollTarget(0);
            }
            else if (aSelectedSlot > mScrollPosition + 3)
            {
                SetScrollTarget(mScrollPosition + 2);
            }
        }
        aMoved = true;
    }

    if (aMoved)
        mApp->PlaySample(SOUND_BLEEP, true);

    mSelectedChallengeIndex = mPageChallengeIndex[aSelectedSlot];
    mSelectedGameMode = ChallengeDefOf(mSelectedChallengeIndex)->mChallengeMode;
    int aAccomplishments = AccomplishmentsNeeded(mSelectedChallengeIndex);
    if (theKey == KEYCODE_RETURN)
    {
        // The return key starts the selected challenge, or explains why it
        // cannot be started yet.
        if (mChallengeButtons[mSelectedChallengeIndex]->mDisabled)
        {
            if (aAccomplishments <= 1)
            {
                mApp->PlaySample(SOUND_CERAMIC, true);
                mApp->DoDialog(
                    75,
                    true,
                    Sexy::String("[MODE_LOCKED]"),
                    mToolTip->mLabel,
                    Sexy::String("[OK]"),
                    3);
            }
            else
            {
                mApp->PlaySample(SOUND_BUZZER, true);
            }
            return 1;
        }

        bool aIsNotVersus = mSelectedGameMode != GAMEMODE_MP_VS;
        mApp->PlaySample(SOUND_CERAMIC, true);
        mApp->KillChallengeScreen();
        mApp->PreNewGame(mSelectedGameMode, aIsNotVersus);
        return 1;
    }

    if (theKey == KEYCODE_ESCAPE)
    {
        mApp->KillChallengeScreen();
        mApp->DoBackToMain();
        return 1;
    }

    // Every other key only moved the selection.
    return 0;
}

// ==========================================================================
// ChallengeScreen::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @0018BF98  616 bytes
// ==========================================================================
void ChallengeScreen::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    // The button is the only argument the body reads; the player index and the
    // modifier flags belong to the dispatch signature.

    // Find the slot the currently selected challenge sits in.
    int aSelectedSlot = 0;
    for (int i = 0; i < mPageChallengeCount; ++i)
    {
        if (mPageChallengeIndex[i] == mSelectedChallengeIndex)
            aSelectedSlot = i;
    }

    bool aMoved = false;
    if (theButton == GAMEPAD_BUTTON_UP)
    {
        if (mScrollPosition == mScrollTargetPosition)
        {
            if (aSelectedSlot != 0)
            {
                --aSelectedSlot;
                if (mScrollPosition > aSelectedSlot)
                    SetScrollTarget(mScrollPosition - 2);
            }
            else
            {
                // Wrap round to the last full row of four.
                aSelectedSlot = mPageChallengeCount - 1;
                int aCount = mPageChallengeCount;
                SetScrollTarget((aCount % 4 != 0) ? (aCount - aCount % 4) : (aCount - 4));
            }
        }
        aMoved = true;
    }
    else if (theButton == GAMEPAD_BUTTON_DOWN)
    {
        if (mScrollPosition == mScrollTargetPosition)
        {
            ++aSelectedSlot;
            if (aSelectedSlot >= mPageChallengeCount)
            {
                aSelectedSlot = 0;
                SetScrollTarget(0);
            }
            else if (aSelectedSlot > mScrollPosition + 3)
            {
                SetScrollTarget(mScrollPosition + 2);
            }
        }
        aMoved = true;
    }

    if (aMoved)
        mApp->PlaySample(SOUND_BLEEP, true);

    mSelectedChallengeIndex = mPageChallengeIndex[aSelectedSlot];
    mSelectedGameMode = ChallengeDefOf(mSelectedChallengeIndex)->mChallengeMode;
    int aAccomplishments = AccomplishmentsNeeded(mSelectedChallengeIndex);

    if (theButton != GAMEPAD_BUTTON_A)
    {
        if (theButton == GAMEPAD_BUTTON_B)
        {
            mApp->KillChallengeScreen();
            mApp->DoBackToMain();
        }
        return;
    }

    // The A button starts the challenge, or explains why it cannot.
    if (mChallengeButtons[mSelectedChallengeIndex]->mDisabled)
    {
        if (aAccomplishments <= 1)
        {
            mApp->PlaySample(SOUND_CERAMIC, true);
            mApp->DoDialog(
                75,
                true,
                Sexy::String("[MODE_LOCKED]"),
                mToolTip->mLabel,
                Sexy::String("[OK]"),
                3);
        }
        else
        {
            mApp->PlaySample(SOUND_BUZZER, true);
        }
        return;
    }

    bool aIsNotVersus = mSelectedGameMode != GAMEMODE_MP_VS;
    mApp->PlaySample(SOUND_CERAMIC, true);
    mApp->KillChallengeScreen();
    mApp->PreNewGame(mSelectedGameMode, aIsNotVersus);
}
// ==========================================================================
// ChallengeScreen::DoUnlockSurvivalDialog(int)
// @0018C23C  76 bytes
// ==========================================================================
int ChallengeScreen::DoUnlockSurvivalDialog(int theChallengeIndex)
{
    // Dialog id 18. If it is already up there is nothing to do.
    if (mApp->GetDialog(18) != nullptr)
        return 1;

    UnlockDialog *aDialog = new UnlockDialog(theChallengeIndex, 3);
    mApp->AddDialogWithId(18, aDialog);
    mApp->mWidgetManager->SetFocus(aDialog);
    return 0;
}

// ==========================================================================
// ChallengeScreen::SaveSurvivalCount(void)
// @0018C294  328 bytes
// ==========================================================================
int ChallengeScreen::SaveSurvivalCount()
{
    Sexy::String aFolder;
    GetAppDataFolder(&aFolder);

    // Make sure the userdata directory exists before writing into it.
    Sexy::String aDir = aFolder;
    aDir += "userdata";
    MkDir(aDir.c_str());

    Sexy::String aFilePath;
    GetAppDataFolder(&aFolder);
    aFilePath = aFolder;
    aFilePath += "userdata/survival.dat";

    FILE *aFile = fopen(aFilePath.c_str(), "wb+");
    if (aFile == nullptr)
        printf("error creating file");

    int aCount = mSurvivalCount;
    fwrite(&aCount, 4u, 1u, aFile);
    fclose(aFile);
    printf("write to survival.dat");
    return 0;
}

// ==========================================================================
// ChallengeScreen::LoadSurvivalCount(void)
// @0018C43C  196 bytes
// ==========================================================================
int ChallengeScreen::LoadSurvivalCount()
{
    int aCount = 3; // the default when the file is missing

    Sexy::String aFolder;
    GetAppDataFolder(&aFolder);
    Sexy::String aFilePath = aFolder;
    aFilePath += "userdata/survival.dat";

    FILE *aFile = fopen(aFilePath.c_str(), "rb");
    if (aFile != nullptr)
    {
        fread(&aCount, 4u, 1u, aFile);
        fclose(aFile);
        printf("read survival.dat");
    }
    else
    {
        printf("error reading file");
    }

    return aCount;
}

// ==========================================================================
// ChallengeScreen::Update(void)
// @0018C540  310 bytes
// ==========================================================================
void ChallengeScreen::Update()
{
    Sexy::Widget::Update();
    UpdateToolTip();

    if (mUnlockStateCounter > 0)
        --mUnlockStateCounter;

    if (mUnlockState == UNLOCK_SHAKING)
    {
        if (mUnlockStateCounter != 0)
        {
            mLockShakeX = RandRangeFloat(-2.0f, 2.0f);
            mLockShakeY = RandRangeFloat(-2.0f, 2.0f);
        }
        else
        {
            // The padlock stops shaking and starts fading out.
            mApp->PlayFoley(FOLEY_PAPER);
            mUnlockState = UNLOCK_FADING;
            mUnlockStateCounter = 50;
            mLockShakeX = 0.0f;
            mLockShakeY = 0.0f;
        }
    }
    else if (mUnlockState == UNLOCK_FADING && mUnlockStateCounter == 0)
    {
        mUnlockState = UNLOCK_OFF;
        mUnlockChallengeIndex = -1;
    }

    MarkDirty();

    // Grow the selected challenge's thumbnail and shrink the others back.
    for (int i = 0; i < mPageChallengeCount; ++i)
    {
        if (mPageChallengeIndex[i] == mSelectedChallengeIndex)
        {
            mPageChallengeAnimTime[i] += 0.016f;
            if (mPageChallengeAnimTime[i] > 0.25f)
                mPageChallengeAnimTime[i] = 0.25f;
        }
        else
        {
            mPageChallengeAnimTime[i] -= 0.016f;
            if (mPageChallengeAnimTime[i] < 0.0f)
                mPageChallengeAnimTime[i] = 0.0f;
        }
    }

    mScrollAnimationTime += 0.016f;
    if (mScrollAnimationTime > 0.15f)
        mScrollPosition = mScrollTargetPosition;
}

// ==========================================================================
// ChallengeScreen::AddedToManager(Sexy::WidgetManager *)
// @0018C684  44 bytes
// ==========================================================================
void ChallengeScreen::AddedToManager(Sexy::WidgetManager * theWidgetManager)
{
    Sexy::WidgetContainer::AddedToManager(theWidgetManager);

    for (int i = 0; i < NUM_CHALLENGE_MODES; ++i)
    {
        // Draw() scrolls these decorative rows; the screen owns hit testing.
        mChallengeButtons[i]->mMouseVisible = false;
        AddWidget(mChallengeButtons[i]);
    }

    mApp->ShowHelpBarWidget();
}

// ==========================================================================
// ChallengeScreen::RemovedFromManager(Sexy::WidgetManager *)
// @0018C6B0  44 bytes
// ==========================================================================
void ChallengeScreen::RemovedFromManager(Sexy::WidgetManager * theWidgetManager)
{
    Sexy::WidgetContainer::RemovedFromManager(theWidgetManager);

    for (int i = 0; i < NUM_CHALLENGE_MODES; ++i)
        RemoveWidget(mChallengeButtons[i]);

    mApp->HideHelpBarWidget();
}

// ==========================================================================
// ChallengeScreen::GotFocus()
// @0018C6DC  4 bytes
// ==========================================================================
void ChallengeScreen::GotFocus()
{
    Sexy::Widget::GotFocus();
}

// ==========================================================================
// ChallengeScreen::~ChallengeScreen()
// @0018A0C4  296 bytes
// ==========================================================================
ChallengeScreen::~ChallengeScreen()
{
    // The two vptr stores are the compiler's -- this class and the
    // ButtonListener base at +0x100 -- and the trailing
    // `Sexy::Widget::~Widget(this)` is the base, which C++ runs.
    //
    // The first walk steps 4 bytes from +0x104 over 376 bytes, which is exactly
    // the 94 entries of mChallengeButtons; each non-null one is dispatched
    // through vtable byte 4, i.e. `delete`.
    for ( int i = 0; i < NUM_CHALLENGE_MODES; ++i )
        delete mChallengeButtons[i];

    // The word at +0x280 is mToolTip. The dump releases its three COW string
    // reps (+0x00 mTitle, +0x04 mLabel, +0x08 mWarningText) and then frees the
    // object -- which is exactly `delete` on a ToolTipWidget, whose three
    // Sexy::String members release themselves.
    delete mToolTip;
}

// Adapted from PvZ TV Touch Team's scrolling challenge list (GPL-3.0-or-later).
// Row origins/spacing and clipping are from Draw @0x18AEE4 and DrawButton @0x18A710.
void ChallengeScreen::MouseDown(int x, int y, int theClickCount)
{
    mMousePressed = theClickCount > 0 && theClickCount != 3 && x >= -20 && x < 760 && y >= 80 && y < 555;
    mMouseScrolled = false;
    mMouseDownY = y;
    mMouseScrollStart = mScrollTargetPosition;
    if (theClickCount < 0)
        GameButtonDown(GAMEPAD_BUTTON_B, 0, 0);
}

void ChallengeScreen::MouseDrag(int x, int y)
{
    (void)x;
    if (!mMousePressed)
        return;
    const int rows = (mMouseDownY - y) / 60;
    if (rows != 0)
    {
        mMouseScrolled = true;
        SetScrollTarget(std::clamp(mMouseScrollStart + rows, 0, std::max(0, mPageChallengeCount - 4)));
    }
}

void ChallengeScreen::MouseUp(int x, int y, int theClickCount)
{
    const bool click = mMousePressed && !mMouseScrolled;
    mMousePressed = false;
    if (!click || theClickCount <= 0 || theClickCount == 3 || x < -20 || x >= 760 || y < 80 || y >= 555)
        return;
    const int row = mScrollPosition + (y - 80) / 120;
    if (row < 0 || row >= mPageChallengeCount || mScrollPosition != mScrollTargetPosition)
        return;
    const int index = mPageChallengeIndex[row];
    const bool activate = mSelectedChallengeIndex == index;
    mSelectedChallengeIndex = index;
    mSelectedGameMode = ChallengeDefOf(index)->mChallengeMode;
    UpdateToolTip();
    if (activate)
        GameButtonDown(GAMEPAD_BUTTON_A, 0, 0);
    else
        mApp->PlaySample(SOUND_BUTTONCLICK, true);
}

void ChallengeScreen::MouseWheel(int theDelta)
{
    SetScrollTarget(std::clamp(mScrollTargetPosition - theDelta, 0, std::max(0, mPageChallengeCount - 4)));
}
