// AlmanacDialog -- reconstructed from libGameMain.so.
//
// The almanac has three pages: an index that shows one plant and one zombie
// side by side, a plant page with the selected plant's seed packet and card,
// and a zombie page with the selected zombie's window and card. Each page owns
// a scratch Plant/Zombie used only for the preview.
//
// See AlmanacDialog.h for the member map and the vtable slots.

#include "AlmanacDialog.h"
#include "game/Board.h"
#include "CustomScrollbarWidget.h"
#include "Font.h"
#include "GameButton.h"
#include "Gamepad.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "IInputDriver.h"
#include "Image.h"
#include "InputManager.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "Music2.h"
#include "Plant.h"
#include "PoolEffect.h"
#include "ReanimatorCache.h"
#include "ReconData.h"
#include "ReconHelpers.h"
#include "ReconTypes.h"
#include "ScrollbarWidget.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include "Zombie.h"
#include <cmath>
#include <cstring>

namespace
{

// The two page sizes. The plant page shows the 48 seeds plus the imitater; the
// zombie page shows every ZombieType the almanac knows.
const int NUM_ALMANAC_SEEDS = 49;
const int NUM_ALMANAC_ZOMBIES = 26;

// DrawStringJustification -- the tree does not declare the enum, so the four
// values the almanac passes to the TodDrawString family are named here.
const int DS_ALIGN_LEFT = 0;
const int DS_ALIGN_RIGHT = 1;
const int DS_ALIGN_CENTER = 2;
const int DS_ALIGN_CENTER_MIDDLE = 5;

// Sexy::GamepadAxis 5 is the vertical axis of the right stick.
const int GAMEPAD_AXIS_RIGHT_Y = 5;

const unsigned char *ZombieDefeatedBytes()
{
    return gZombieDefeated;
}

bool ZombieDefeated(ZombieType theZombieType)
{
    return ZombieDefeatedBytes()[theZombieType] != 0;
}

// ZombieDefinition is 28 bytes in the binary and GetZombieDefinition returns a
// pointer to the entry; the tree declares the accessor untyped, so the starting
// level is read at its own +0x0C slot.
int ZombieStartLevel(ZombieType theZombieType)
{
    return GetZombieDefinition(theZombieType)->mStartingLevel;
}

PlantDefinition *PlantDef(SeedType theSeedType)
{
    return GetPlantDefinition(theSeedType);
}

} // namespace

// ==========================================================================
// AlmanacDialog::~AlmanacDialog()
// @00138428  396 bytes
// ==========================================================================
AlmanacDialog::~AlmanacDialog()
{
    mApp->SafeDeleteWidget(mPlantButton);
    mApp->SafeDeleteWidget(mZombieButton);
    mApp->SafeDeleteWidget(mScrollTextView);
    ClearPlantsAndZombies();

    Sexy::String aResources("DelayLoad_Almanac");
    TodDeleteResources(aResources);
}

// ==========================================================================
// AlmanacDialog::MouseWheel(int)
// @00138258  130 bytes
// ==========================================================================
void AlmanacDialog::MouseWheel(int theDelta)
{
    // The wheel only scrolls the description while the pointer is inside it.
    if (mScrollTextView == nullptr || !mScrollTextView->mVisible)
        return;

    const Sexy::Point position = GetAbsPos();
    int aMouseX = mApp->mWidgetManager->mLastMouseX - position.mX;
    int aMouseY = mApp->mWidgetManager->mLastMouseY - position.mY;
    if (aMouseX < mDescriptionRect.mX || aMouseX >= mDescriptionRect.mX + mDescriptionRect.mWidth)
        return;
    if (aMouseY < mDescriptionRect.mY || aMouseY >= mDescriptionRect.mY + mDescriptionRect.mHeight)
        return;

    mMouseScrollActive = true;
    if (theDelta > 0)
        mScrollTextView->SetValue(mScrollTextView->mValue - 5.0);
    else if (theDelta != 0)
        mScrollTextView->SetValue(mScrollTextView->mValue + 5.0);
}

// ==========================================================================
// AlmanacDialog::KeyChar(char)
// @001382DC  4 bytes
// ==========================================================================
int AlmanacDialog::KeyChar(char theChar)
{
    // The almanac consumes the character without acting on it.
    return 1;
}

// ==========================================================================
// AlmanacDialog::ButtonPress(int)
// @001382E0  2 bytes
// ==========================================================================
void AlmanacDialog::ButtonPress(int theId) {}

// ==========================================================================
// AlmanacDialog::AxisMoved(Sexy::Event const&)
// @00138310  208 bytes
// ==========================================================================
void AlmanacDialog::AxisMoved(const Sexy::Event &theEvent)
{
    // Only an axis event that continues the scroll Update started is handled
    // here; everything else belongs to the focused child.
    if (theEvent.eventCodeMask != 2)
    {
        Widget::AxisMoved(theEvent);
        return;
    }
    if (mAxisScrollActive && mAxisDeviceIndex != (float)(int)theEvent.lateModifiers)
    {
        Widget::AxisMoved(theEvent);
        return;
    }

    // +0x2C is the saved event-code mask. AxisMoved reads it as the axis value
    // the driver stored there; the field's own type is the packed card.
    float aAxisValue = 0.0f;
    std::memcpy(&aAxisValue, &theEvent.mSavedEventCodeMask, sizeof(aAxisValue));
    if (fabsf(aAxisValue) >= 0.2f)
    {
        mAxisDeviceIndex = (float)(int)theEvent.lateModifiers;
        mAxisScrollActive = true;
        if (aAxisValue < 0.0f)
            mScrollTextView->SetValue(mScrollTextView->mValue - 1.0);
        else
            mScrollTextView->SetValue(mScrollTextView->mValue + 1.0);
        return;
    }

    mAxisScrollActive = false;
    mAxisDeviceIndex = -1.0f;
}

// ==========================================================================
// AlmanacDialog::ClearPlantsAndZombies()
// @001383E8  62 bytes
// ==========================================================================
void AlmanacDialog::ClearPlantsAndZombies()
{
    if (mPlant != nullptr)
    {
        mPlant->Die();
        mPlant = nullptr;
    }
    if (mZombie != nullptr)
    {
        mZombie->DieNoLoot();
        mZombie = nullptr;
    }
}

// ==========================================================================
// AlmanacDialog::SetupHelpbar(bool)
// ==========================================================================
void AlmanacDialog::SetupHelpbar(bool theIsZombie)
{
    mHelpBarWidget->ClearButtons(3);
    if (theIsZombie)
        mHelpBarWidget->AddButton(GAMEPAD_BUTTON_THUMBR, Sexy::String("[SCROLL]"), HelpBarWidget::HELP_ALIGN_RIGHT);
}

// ==========================================================================
// AlmanacDialog::SetupPlant()
// @00138718  130 bytes
// ==========================================================================
void AlmanacDialog::SetupPlant()
{
    ClearPlantsAndZombies();

    // Seeds 6..47 have a hand-placed preview position; the rest sit at the
    // default spot below the card.
    float aPosX = 578.0f;
    float aPosY = 175.0f;
    unsigned int aIndex = (unsigned int)mSelectedSeed - 6;
    if (aIndex <= 41)
    {
        aPosX = gAlmanacSeedPos.mSeedX[aIndex];
        aPosY = gAlmanacSeedPos.mSeedY[aIndex];
    }

    mPlant = new Plant();
    mPlant->mBoard = nullptr;
    mPlant->mIsOnBoard = false;
    mPlant->PlantInitialize(0, 0, mSelectedSeed, SEED_NONE, -1);
    mPlant->mX = (int)aPosX;
    mPlant->mY = (int)aPosY;
    mSetupFinished = false;
}

// ==========================================================================
// AlmanacDialog::SetupZombie()
// @001387B0  278 bytes
// ==========================================================================
void AlmanacDialog::SetupZombie()
{
    ClearPlantsAndZombies();

    mZombie = new Zombie();
    mZombie->mBoard = nullptr;
    mZombie->ZombieInitialize(0, mSelectedZombie, false, nullptr, -3, true);

    // Each zombie type gets its own preview position so the sprite lines up
    // with the window it is drawn into.
    float aPosX;
    float aPosY;
    switch (mSelectedZombie)
    {
        case ZOMBIE_POLEVAULTER:
            aPosX = 567.0f;
            aPosY = 175.0f;
            break;
        case ZOMBIE_NEWSPAPER:
        case ZOMBIE_DIGGER:
        case ZOMBIE_LADDER:
            aPosX = 559.0f;
            aPosY = 170.0f;
            break;
        case ZOMBIE_DOOR:
        case ZOMBIE_BOBSLED:
        case ZOMBIE_DOLPHIN_RIDER:
        case ZOMBIE_IMP:
            aPosX = 559.0f;
            aPosY = 165.0f;
            break;
        case ZOMBIE_FOOTBALL:
            aPosX = 559.0f;
            aPosY = 183.0f;
            break;
        case ZOMBIE_DANCER:
            aPosX = 559.0f;
            aPosY = 205.0f;
            break;
        case ZOMBIE_BACKUP_DANCER:
            aPosX = 559.0f;
            aPosY = 190.0f;
            break;
        case ZOMBIE_ZAMBONI:
        case ZOMBIE_BOSS:
            aPosX = 559.0f;
            aPosY = 190.0f;
            break;
        case ZOMBIE_YETI:
            aPosX = 559.0f;
            aPosY = 195.0f;
            break;
        case ZOMBIE_BUNGEE:
        case ZOMBIE_CATAPULT:
            aPosX = 559.0f;
            aPosY = 185.0f;
            break;
        case ZOMBIE_GARGANTUAR:
            aPosX = 559.0f;
            aPosY = 200.0f;
            break;
        default:
            aPosX = 559.0f;
            aPosY = 175.0f;
            break;
    }

    mZombie->mPosX = aPosX;
    mZombie->mPosY = aPosY;
    mZombie->Update();
    mSetupFinished = false;
}

// ==========================================================================
// AlmanacDialog::SetPage(AlmanacPage)
// @001388D0  300 bytes
// ==========================================================================
void AlmanacDialog::SetPage(AlmanacPage thePage)
{
    mOpenPage = thePage;
    ClearPlantsAndZombies();

    switch (mOpenPage)
    {
        case ALMANAC_PAGE_PLANTS:
            SetupHelpbar(true);
            SetupPlant();
            mPlantButton->mBtnNoDraw = true;
            mZombieButton->mBtnNoDraw = true;
            break;

        case ALMANAC_PAGE_ZOMBIES:
            SetupHelpbar(true);
            SetupZombie();
            mPlantButton->mBtnNoDraw = true;
            mZombieButton->mBtnNoDraw = true;
            break;

        default:
            SetupHelpbar(false);

            mPlant = new Plant();
            mPlant->mBoard = nullptr;
            mPlant->mIsOnBoard = false;
            mPlant->PlantInitialize(0, 0, SEED_SUNFLOWER, SEED_NONE, -1);
            mPlant->mX = 167;
            mPlant->mY = 225;

            mZombie = new Zombie();
            mZombie->mBoard = nullptr;
            mZombie->ZombieInitialize(0, ZOMBIE_NORMAL, false, nullptr, -3, true);
            mZombie->mPosX = 535.0f;
            mZombie->mPosY = 215.0f;

            mPlantButton->mBtnNoDraw = false;
            mZombieButton->mBtnNoDraw = false;
            mPlantButton->SetFocusLinks(nullptr, nullptr, nullptr, mZombieButton);
            mScrollTextView->SetVisible(false);
            mScrollTextView->SetValue(0.0);
            mDescriptionScrollY = 0.0f;
            break;
    }
}

// ==========================================================================
// AlmanacDialog::MouseUp(int,int,int)
// @00138A10  50 bytes
// ==========================================================================
void AlmanacDialog::MouseUp(int x, int y, int theClickCount)
{
    mMouseDraggingDescription = false;
}

void AlmanacDialog::MouseDrag(int x, int y)
{
    if (!mMouseDraggingDescription || mScrollTextView == nullptr)
        return;
    if (mDescriptionTextHeight > 0)
        mScrollTextView->SetValue(mScrollTextView->mValue + double(mMouseLastY - y) * 100.0 / mDescriptionTextHeight);
    mMouseLastY = y;
}

// ==========================================================================
// AlmanacDialog::ButtonDepress(int)
// @00138A44  20 bytes
// ==========================================================================
void AlmanacDialog::ButtonDepress(int theId)
{
    mMouseScrollActive = true;
    if (theId == 0)
        SetPage(ALMANAC_PAGE_PLANTS);
    else if (theId == 1)
        SetPage(ALMANAC_PAGE_ZOMBIES);
}

// ==========================================================================
// AlmanacDialog::AlmanacDialog(LawnApp *)
// @00138A60  1648 bytes
// ==========================================================================
AlmanacDialog::AlmanacDialog(LawnApp *theApp) :
    LawnDialog(theApp, nullptr, 5, 1, Sexy::String("[ALMANAC_HEADER]"), Sexy::String(""), Sexy::String(""), 0)
{
    mApp = theApp;
    mPlantButton = nullptr;
    mZombieButton = nullptr;
    mScrollTextView = nullptr;
    mOpenPage = ALMANAC_PAGE_INDEX;
    mSelectedSeed = SEED_PEASHOOTER;
    mSelectedZombie = ZOMBIE_NORMAL;
    mPlant = nullptr;
    mZombie = nullptr;
    mCardTextRect = Sexy::Rect(0, 0, 0, 0);
    mDescriptionRect = Sexy::Rect(0, 0, 0, 0);
    mCostRect = Sexy::Rect(0, 0, 0, 0);
    mWaitTimeRect = Sexy::Rect(0, 0, 0, 0);
    mDescriptionScrollY = 0.0f;
    mDescriptionTextHeight = 0;
    mWaitTimeJustification = 0;
    mJustification = 0;
    mSetupFinished = false;
    mHelpBarWidget = nullptr;
    mAxisDeviceIndex = -1.0f;
    mAxisScrollActive = false;

    mWidgetManager = mApp->mWidgetManager;
    mDrawStandardBack = false;
    mClip = false;

    Sexy::String aResources("DelayLoad_Almanac");
    TodLoadResources(aResources);

    Resize(0, 0, 800, 600);

    // ---- the two index buttons ----
    mPlantButton = new GameButton(0, this);
    mPlantButton->SetLabel(Sexy::String("[VIEW_PLANTS]"));
    mPlantButton->mButtonImage = IMAGE_SEEDCHOOSER_BUTTON_DISABLED;
    mPlantButton->mDisabledImage = IMAGE_SEEDCHOOSER_BUTTON_DISABLED;
    mPlantButton->mDownImage = nullptr;
    mPlantButton->mOverOverlayImage = nullptr;
    mPlantButton->mOverImage = IMAGE_SEEDCHOOSER_BUTTON;
    mPlantButton->SetFont(FONT_DWARVENTODCRAFT18);
    mPlantButton->mColors[0] = Color(213, 159, 43);
    mPlantButton->mColors[1] = Color(255, 200, 70);
    mPlantButton->Resize(130, 345, 156, 42);
    mPlantButton->mParentWidget = this;
    mPlantButton->mTextOffsetY = -1;

    mZombieButton = new GameButton(1, this);
    mZombieButton->SetLabel(Sexy::String("[VIEW_ZOMBIES]"));
    mZombieButton->Resize(487, 345, 210, 48);
    mZombieButton->mDrawStoneButton = true;
    mZombieButton->mParentWidget = this;
    mPlantButton->mWidgetManager = mWidgetManager;
    mPlantButton->SetFocusLinks(nullptr, nullptr, nullptr, mZombieButton);
    mZombieButton->SetFocusLinks(nullptr, nullptr, mPlantButton, nullptr);

    // ---- the description scrollbar ----
    mScrollTextView = new CustomScrollbarWidget(2, this);
    mScrollTextView->ResizeScrollbar(10, 10, 6, 100);
    mScrollTextView->SetInvisIfNoScroll(false);
    mScrollTextView->SetMaxValue(100.0);
    mScrollTextView->SetPageSize(50.0);
    mScrollTextView->SetValue(0.0);
    mScrollTextView->SetThumbPosition(0);
    mScrollTextView->RemoveScrollButtons();
    mScrollTextView->SetVisible(false);
    mScrollTextView->SetValue(0.0);

    // ---- the help bar ----
    mHelpBarWidget = mApp->mHelpBarWidget;
    mDescriptionScrollY = 0.0f;
    mHelpBarWidget->mDrawShade = false;
    mHelpBarWidget->ClearButtons(0);
    mHelpBarWidget->AddButton((Sexy::GamepadButton)20, Sexy::String(""), HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBarWidget->AddButton((Sexy::GamepadButton)20, Sexy::String(""), HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBarWidget->AddButton((Sexy::GamepadButton)7, Sexy::String("[BACK]"), HelpBarWidget::HELP_ALIGN_NONE);

    SetPage(ALMANAC_PAGE_INDEX);

    if (mApp->mBoard == nullptr || !mApp->mBoard->mPaused)
        mApp->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_CHOOSE_YOUR_SEEDS);
    mNameString = "";
    mDescriptionHeaderString = "";
    mDescriptionStringTmp = "";
    mDescriptionString = "";
    mCostString = "";
    mWaitTimeString = "";

    mSetupFinished = false;
}

// ==========================================================================
// AlmanacDialog::DrawIndex(Sexy::Graphics *)
// @00139204  302 bytes
// ==========================================================================
void AlmanacDialog::DrawIndex(Sexy::Graphics *g)
{
    g->DrawImage(IMAGE_ALMANAC_INDEXBACK, LawnApp::FULLSCREEN_RECT.mX, LawnApp::FULLSCREEN_RECT.mY);
    TodDrawString(g,
        Sexy::String("[SUBURBAN_ALMANAC_INDEX]"),
        400,
        50,
        FONT_HOUSEOFTERROR28,
        Color(220, 220, 220),
        DS_ALIGN_CENTER);

    if (mPlant != nullptr)
    {
        g->PushState();
        g->mTransX += (float)mPlant->mX;
        g->mTransY += (float)mPlant->mY;
        mPlant->Draw(g);
        g->PopState();
    }

    if (mZombie != nullptr)
    {
        g->PushState();
        g->mTransX += (float)mZombie->mX;
        g->mTransY += (float)mZombie->mY;
        mZombie->Draw(g);
        g->PopState();
    }
}

// ==========================================================================
// AlmanacDialog::GetSeedPosition(SeedType,int &,int &)
// @0013936C  66 bytes
// ==========================================================================
void AlmanacDialog::GetSeedPosition(SeedType theSeedType, int &x, int &y)
{
    // The imitater sits on its own, above the eight-column grid.
    if (theSeedType == SEED_IMITATER)
    {
        x = 11;
        y = 6;
        return;
    }

    x = 52 * (theSeedType % 8) + 26;
    y = 78 * (theSeedType / 8) + 92;
}

// ==========================================================================
// AlmanacDialog::DrawPlants(Sexy::Graphics *)
// @001393B0  1538 bytes
// ==========================================================================
void AlmanacDialog::DrawPlants(Sexy::Graphics *g)
{
    g->DrawImage(IMAGE_ALMANAC_PLANTBACK, LawnApp::FULLSCREEN_RECT.mX, LawnApp::FULLSCREEN_RECT.mY);

    int aHeaderY = FONT_HOUSEOFTERROR20->GetHeight() / 2 + 42;
    TodDrawString(g,
        Sexy::String("[SUBURBAN_ALMANAC_PLANTS]"),
        400,
        aHeaderY,
        FONT_HOUSEOFTERROR20,
        Color(213, 159, 43),
        DS_ALIGN_CENTER);

    int aAlpha = (int)(sin((mUpdateCnt % 100) * 0.01f * 3.14159265f) * 255.0f);
    int aSelectedSeed = mSelectedSeed;
    for (int aSeedIndex = 0; aSeedIndex < NUM_ALMANAC_SEEDS; ++aSeedIndex)
    {
        int aPosX = 0;
        int aPosY = 0;
        GetSeedPosition((SeedType)aSeedIndex, aPosX, aPosY);

        if (!mApp->HasSeedType((SeedType)aSeedIndex, false))
        {
            g->DrawImage(IMAGE_ALMANAC_PLANTBLANK, aPosX, aPosY);
            continue;
        }

        if (aSeedIndex == SEED_IMITATER)
        {
            bool aColorize = g->GetColorizeImages();
            g->SetColorizeImages(true);
            if (aSelectedSeed == SEED_IMITATER)
                g->SetColor(Color(255, 255, 0, aAlpha));
            else
                g->SetColor(Color(255, 255, 255, 64));
            g->DrawImage(IMAGE_ALMANAC_IMITATER, 18, 20);
            g->SetColor(Sexy::Color::White);
            g->SetColorizeImages(aColorize);
        }
        else
        {
            DrawSeedPacket(
                g, (float)aPosX, (float)aPosY, (SeedType)aSeedIndex, SEED_NONE, 0.0f, 255, true, false, false, true);
        }
    }

    // The selected packet is redrawn on top, enlarged and flashed.
    if (aSelectedSeed != SEED_IMITATER && aSelectedSeed != SEED_NONE)
    {
        int aPosX = 0;
        int aPosY = 0;
        GetSeedPosition((SeedType)aSelectedSeed, aPosX, aPosY);
        g->SetScale(1.1f, 1.1f, (float)aPosX, (float)aPosY);
        DrawSeedPacket(g,
            (float)(aPosX - 5),
            (float)(aPosY - 5),
            (SeedType)aSelectedSeed,
            SEED_NONE,
            0.0f,
            255,
            true,
            false,
            false,
            true);
        g->SetColorizeImages(true);
        g->SetColor(Color(255, 255, 0, aAlpha));
        g->DrawImage(IMAGE_SEEDPACKETFLASH, aPosX - 6, aPosY - 6);
        g->SetColor(Sexy::Color::White);
        g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
    }

    // The preview ground follows the selected seed's habitat.
    if (Plant::IsAquatic(mSelectedSeed))
    {
        if (Plant::IsNocturnal(mSelectedSeed))
        {
            g->DrawImage(IMAGE_ALMANAC_GROUNDNIGHTPOOL, 521, 107);
            if (mApp->Is3DAccelerated())
            {
                g->SetClipRect(475, 0, 397, 500);
                g->ClearClipRect();
            }
        }
        else
        {
            g->DrawImage(IMAGE_ALMANAC_GROUNDPOOL, 521, 107);
            if (mApp->Is3DAccelerated())
            {
                g->SetClipRect(475, 0, 397, 500);
                g->ClearClipRect();
            }
        }
    }
    else if (Plant::IsNocturnal(mSelectedSeed) || mSelectedSeed == SEED_GRAVEBUSTER || mSelectedSeed == SEED_PLANTERN)
    {
        g->DrawImage(IMAGE_ALMANAC_GROUNDNIGHT, 521, 107);
    }
    else if (mSelectedSeed == SEED_FLOWERPOT)
    {
        g->DrawImage(IMAGE_ALMANAC_GROUNDROOF, 521, 107);
    }
    else
    {
        g->DrawImage(IMAGE_ALMANAC_GROUNDDAY, 521, 107);
    }

    if (mPlant != nullptr)
    {
        g->PushState();
        g->mTransX += (float)mPlant->mX;
        g->mTransY += (float)mPlant->mY;
        mPlant->Draw(g);
        g->PopState();
    }

    g->DrawImage(IMAGE_ALMANAC_PLANTCARD, 459, 80);
    TodDrawString(g, mNameString, 617, 108, FONT_DWARVENTODCRAFT18, Color(213, 159, 43), DS_ALIGN_CENTER);

    if (mSelectedSeed != SEED_IMITATER)
    {
        TodDrawStringWrapped(g, mCostString, mCostRect, FONT_BRIANNETOD16, Sexy::Color::White, DS_ALIGN_LEFT, false);
        TodDrawStringWrapped(
            g, mWaitTimeString, mWaitTimeRect, FONT_BRIANNETOD16, Sexy::Color::White, mWaitTimeJustification, false);
    }

    // The description is clipped to its rect and scrolled by the scrollbar.
    g->PushState();
    g->ClipRect(mDescriptionRect.mX, mDescriptionRect.mY - 14, mDescriptionRect.mWidth, mDescriptionRect.mHeight + 8);
    float aScroll = (float)(mScrollTextView->mValue * 0.01 * mDescriptionRect.mY);
    g->mTransY = g->mTransY + 2.0f - aScroll;
    mDescriptionScrollY = -aScroll;
    TodDrawStringWrappedHelper(
        g, mDescriptionString, mDescriptionRect, FONT_BRIANNETOD16, Color(143, 67, 27), DS_ALIGN_LEFT, true, true);
    g->PopState();
}

// ==========================================================================
// AlmanacDialog::GetZombiePosition(ZombieType,int &,int &)
// @001399D0  66 bytes
// ==========================================================================
void AlmanacDialog::GetZombiePosition(ZombieType theZombieType, int &x, int &y)
{
    // The boss gets its own oversized window at the bottom of the page.
    if (theZombieType == ZOMBIE_BOSS)
    {
        x = 192;
        y = 486;
        return;
    }

    x = 85 * (theZombieType % 5) + 22;
    y = 80 * (theZombieType / 5) + 86;
}

// ==========================================================================
// AlmanacDialog::GetZombieType(int)
// @00139A14  12 bytes
// ==========================================================================
ZombieType AlmanacDialog::GetZombieType(int theIndex)
{
    if (theIndex > 33)
        return ZOMBIE_INVALID;
    return (ZombieType)theIndex;
}

// ==========================================================================
// AlmanacDialog::SeedHitTest(int,int)
// @00139A20  120 bytes
// ==========================================================================
SeedType AlmanacDialog::SeedHitTest(int x, int y)
{
    if (!mMouseVisible || mOpenPage != ALMANAC_PAGE_PLANTS)
        return SEED_NONE;

    for (int aSeedIndex = 0; aSeedIndex < NUM_ALMANAC_SEEDS; ++aSeedIndex)
    {
        if (!mApp->HasSeedType((SeedType)aSeedIndex, false))
            continue;

        int aPosX = 0;
        int aPosY = 0;
        GetSeedPosition((SeedType)aSeedIndex, aPosX, aPosY);

        int aWidth = 70;
        int aHeight = 50;
        if (aSeedIndex == SEED_IMITATER)
        {
            aWidth = 46;
            aHeight = 36;
        }

        if (x >= aPosX && y >= aPosY && x < aPosX + aWidth && y < aPosY + aHeight)
            return (SeedType)aSeedIndex;
    }
    return SEED_NONE;
}

// ==========================================================================
// AlmanacDialog::NextSeedInDir(AlmanacDialog::SeedDir)
// @00139A98  284 bytes
// ==========================================================================
SeedType AlmanacDialog::NextSeedInDir(AlmanacDialog_SeedDir theDir)
{
    SeedType aSeedType = mSelectedSeed;

    // The imitater sits outside the grid, so up/down wrap onto it.
    if (theDir == 0 && aSeedType == SEED_PEASHOOTER)
        aSeedType = SEED_IMITATER;
    else if (theDir == 1 && aSeedType == SEED_IMITATER)
        aSeedType = SEED_PEASHOOTER;
    else
    {
        switch (theDir)
        {
            case 0:
                aSeedType = (SeedType)(aSeedType - 8);
                break;
            case 1:
                aSeedType = (SeedType)(aSeedType + 8);
                break;
            case 2:
                aSeedType = (SeedType)(aSeedType - 1);
                break;
            case 3:
                aSeedType = (SeedType)(aSeedType + 1);
                break;
            default:
                break;
        }
    }

    if (mApp->HasSeedType(aSeedType, false))
        return (unsigned int)aSeedType > SEED_IMITATER ? mSelectedSeed : aSeedType;

    // Walk in the requested direction until an unlocked seed shows up, staying
    // inside the grid row (left/right) or column (up/down).
    int aStep = 0;
    int aMin = 0;
    int aMax = 0;
    bool aInRange = false;
    switch (theDir)
    {
        case 0:
            aStep = -8;
            aMin = 0;
            aMax = aSeedType;
            aInRange = aSeedType >= 0;
            break;
        case 1:
            aStep = 8;
            aMin = aSeedType;
            aMax = NUM_ALMANAC_SEEDS;
            aInRange = aSeedType <= NUM_ALMANAC_SEEDS;
            break;
        case 2:
            aStep = -1;
            aMin = aSeedType & ~7;
            aMax = aSeedType;
            aInRange = aSeedType >= aMin;
            break;
        case 3:
            aStep = 1;
            aMin = aSeedType;
            aMax = (aSeedType & ~7) + 8;
            if (aMax >= NUM_ALMANAC_SEEDS)
                aMax = NUM_ALMANAC_SEEDS;
            aInRange = aSeedType <= aMax;
            break;
        default:
            aMin = 0;
            aMax = 0;
            aInRange = aSeedType == 0;
            break;
    }

    if (!aInRange)
        return mSelectedSeed;

    while (!mApp->HasSeedType(aSeedType, false))
    {
        aSeedType = (SeedType)(aSeedType + aStep);
        if (aSeedType < aMin || aSeedType > aMax)
            return mSelectedSeed;
    }
    return aSeedType;
}

// ==========================================================================
// AlmanacDialog::ZombieHasSilhouette(ZombieType)
// @00139C04  70 bytes
// ==========================================================================
bool AlmanacDialog::ZombieHasSilhouette(ZombieType theZombieType) const
{
    // Only the yeti is ever hidden, and only until it can actually spawn.
    if (theZombieType != ZOMBIE_YETI || mApp->CanSpawnYetis())
        return false;

    return mApp->HasFinishedAdventure() || mApp->mPlayerInfo->GetLevel() > ZombieStartLevel(ZOMBIE_YETI);
}

// ==========================================================================
// AlmanacDialog::ZombieIsShown(ZombieType)
// @00139C4C  138 bytes
// ==========================================================================
bool AlmanacDialog::ZombieIsShown(ZombieType theZombieType)
{
    int aStartLevel = ZombieStartLevel(theZombieType);
    int aLevel = mApp->mPlayerInfo->GetLevel();

    if (!mApp->IsTrialStageLocked())
    {
        if (theZombieType == ZOMBIE_YETI)
            return mApp->CanSpawnYetis() || ZombieHasSilhouette(ZOMBIE_YETI);
        if (theZombieType > ZOMBIE_BOSS)
            return false;
    }
    else if (theZombieType > ZOMBIE_SNORKEL)
    {
        // The trial only shows the zombies up to the snorkel.
        return false;
    }

    if (mApp->HasFinishedAdventure())
        return true;

    // A zombie appears once its first level is reached; the three types that
    // cannot spawn naturally also have to have been defeated there.
    return aLevel >= aStartLevel &&
        (aLevel != aStartLevel ||
            (theZombieType != ZOMBIE_IMP && theZombieType != ZOMBIE_BACKUP_DANCER && theZombieType != ZOMBIE_BOBSLED) ||
            ZombieDefeated(theZombieType));
}

// ==========================================================================
// AlmanacDialog::DrawZombies(Sexy::Graphics *)
// @00139CE0  2126 bytes
// ==========================================================================
void AlmanacDialog::DrawZombies(Sexy::Graphics *g)
{
    g->DrawImage(IMAGE_ALMANAC_ZOMBIEBACK, LawnApp::FULLSCREEN_RECT.mX, LawnApp::FULLSCREEN_RECT.mY);

    int aHeaderY = FONT_DWARVENTODCRAFT24->GetHeight() / 2 + 42;
    TodDrawString(g,
        Sexy::String("[SUBURBAN_ALMANAC_ZOMBIES]"),
        400,
        aHeaderY,
        FONT_DWARVENTODCRAFT24,
        Color(0, 196, 0),
        DS_ALIGN_CENTER);

    ZombieType aSelectedZombie = mSelectedZombie;
    for (int aIndex = 0; aIndex < NUM_ALMANAC_ZOMBIES; ++aIndex)
    {
        ZombieType aZombieType = GetZombieType(aIndex);
        int aPosX = 0;
        int aPosY = 0;
        GetZombiePosition(aZombieType, aPosX, aPosY);

        if (aZombieType == ZOMBIE_INVALID)
            continue;

        if (!ZombieIsShown(aZombieType))
        {
            g->DrawImage(IMAGE_ALMANAC_ZOMBIEBLANK, aPosX, aPosY);
            continue;
        }

        if (aSelectedZombie == aZombieType)
        {
            g->mTransX -= 4.0f;
            g->mTransY -= 4.0f;
            g->SetScale(1.1f, 1.1f, (float)aPosX, (float)aPosY);
        }

        g->DrawImage(IMAGE_ALMANAC_ZOMBIEWINDOW, aPosX, aPosY);
        g->PushState();
        g->SetClipRect(aPosX + 2, aPosY + 2, 72, 72);

        // The cached zombie is drawn at half scale inside the window, nudged so
        // the sprite lines up with the window's frame.
        g->mScaleX = 0.5f;
        g->mScaleY = 0.5f;
        g->mTransX += (float)(aPosX + 1);
        g->mTransY += (float)(aPosY - 6);

        ZombieType aDrawZombie = aZombieType;
        switch (aZombieType)
        {
            case ZOMBIE_POLEVAULTER:
                aDrawZombie = ZOMBIE_CACHED_POLEVAULTER_WITH_POLE;
                g->mTransX += 2.0f;
                g->mTransY -= 3.0f;
                break;
            case ZOMBIE_FLAG:
                g->mTransX += 2.0f;
                g->mTransY += 10.0f;
                break;
            case ZOMBIE_TRAFFIC_CONE:
                g->mTransY += 12.0f;
                break;
            case ZOMBIE_PAIL:
                g->mTransY += 9.0f;
                break;
            case ZOMBIE_FOOTBALL:
                g->mTransX -= 15.0f;
                g->mTransY -= 1.0f;
                break;
            case ZOMBIE_ZAMBONI:
                g->mTransX -= 54.0f;
                g->mTransY += 3.0f;
                break;
            case ZOMBIE_DOLPHIN_RIDER:
                g->mTransX -= 2.0f;
                g->mTransY -= 10.0f;
                break;
            case ZOMBIE_POGO:
                g->mTransY -= 3.0f;
                break;
            case ZOMBIE_DANCER:
                g->mTransY += 15.0f;
                break;
            case ZOMBIE_BACKUP_DANCER:
                g->mTransX -= 5.0f;
                g->mTransY += 17.0f;
                break;
            case ZOMBIE_GARGANTUAR:
                g->mTransX += 15.0f;
                g->mTransY += 17.0f;
                break;
            case ZOMBIE_IMP:
                g->mTransX -= 8.0f;
                g->mTransY -= 7.0f;
                break;
            case ZOMBIE_BUNGEE:
                g->mTransX -= 4.0f;
                g->mTransY += 3.0f;
                break;
            case ZOMBIE_SNORKEL:
                g->mTransX -= 10.0f;
                break;
            case ZOMBIE_YETI:
                g->mTransY += 4.0f;
                break;
            case ZOMBIE_CATAPULT:
                g->mTransX -= 24.0f;
                g->mTransY -= 1.0f;
                break;
            case ZOMBIE_BOBSLED:
                g->mTransY -= 8.0f;
                break;
            case ZOMBIE_LADDER:
                g->mTransY -= 3.0f;
                break;
            default:
                break;
        }

        if (ZombieHasSilhouette(aZombieType))
        {
            g->SetColor(Color(0, 0, 0, 64));
            g->SetColorizeImages(true);
        }

        if (aSelectedZombie != aZombieType)
        {
            mApp->mReanimatorCache->DrawCachedZombie(g, 0.0f, 0.0f, aDrawZombie);
            g->PopState();
            g->DrawImage(IMAGE_ALMANAC_ZOMBIEWINDOW2, aPosX, aPosY);
            continue;
        }

        // The selected window gets a second, brighter pass so it reads as lit.
        g->mTransX -= 4.0f;
        g->mTransY -= 4.0f;
        mApp->mReanimatorCache->DrawCachedZombie(g, 0.0f, 0.0f, aDrawZombie);
        g->PopState();

        g->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
        g->SetColor(Color(255, 255, 255, 100));
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_ALMANAC_ZOMBIEWINDOW, aPosX, aPosY);
        g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
        g->SetColorizeImages(false);

        g->DrawImage(IMAGE_ALMANAC_ZOMBIEWINDOW2, aPosX, aPosY);

        g->SetDrawMode(Sexy::Graphics::DRAWMODE_ADDITIVE);
        g->SetColor(Color(255, 255, 255, 48));
        g->SetColorizeImages(true);
        g->DrawImage(IMAGE_ALMANAC_ZOMBIEWINDOW2, aPosX, aPosY);
        g->SetDrawMode(Sexy::Graphics::DRAWMODE_NORMAL);
        g->SetColorizeImages(false);

        g->mTransX += 4.0f;
        g->mTransY += 4.0f;
        g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
    }

    // The big preview at the bottom, on ice for the two sliding zombies.
    bool aOnIce =
        mZombie != nullptr && (mZombie->mZombieType == ZOMBIE_ZAMBONI || mZombie->mZombieType == ZOMBIE_BOBSLED);
    g->DrawImage(aOnIce ? IMAGE_ALMANAC_GROUNDICE : IMAGE_ALMANAC_GROUNDDAY, 518, 110);

    if (mZombie != nullptr && !ZombieHasSilhouette(mZombie->mZombieType))
    {
        g->PushState();
        g->mTransX += (float)mZombie->mX;
        g->mTransY += (float)mZombie->mY;
        g->SetClipRect(-42, -51, 197, 187);

        bool aDrawShadow = false;
        switch (mZombie->mZombieType)
        {
            case ZOMBIE_ZAMBONI:
                g->mTransX -= 30.0f;
                g->mTransY += 5.0f;
                break;
            case ZOMBIE_GARGANTUAR:
                g->mTransY += 30.0f;
                break;
            case ZOMBIE_FOOTBALL:
                g->mTransX -= 10.0f;
                aDrawShadow = true;
                break;
            case ZOMBIE_BALLOON:
                g->mTransY -= 20.0f;
                aDrawShadow = true;
                break;
            case ZOMBIE_BUNGEE:
                g->mTransX += 15.0f;
                break;
            case ZOMBIE_CATAPULT:
                g->mTransX -= 10.0f;
                break;
            case ZOMBIE_BOSS:
                g->mTransX -= 540.0f;
                g->mTransY -= 175.0f;
                break;
            default:
                if (mZombie->mZombieType != ZOMBIE_DANCER && mZombie->mZombieType != ZOMBIE_BACKUP_DANCER)
                    aDrawShadow = true;
                break;
        }

        if (aDrawShadow)
            mZombie->DrawShadow(g);
        mZombie->Draw(g);
        g->PopState();
    }

    g->DrawImage(IMAGE_ALMANAC_ZOMBIECARD, 455, 78);
    TodDrawString(g, mNameString, 613, 112, FONT_DWARVENTODCRAFT18, Color(213, 159, 43), DS_ALIGN_CENTER);

    // The magnetshroom-gated formats stay hidden until that seed is unlocked.
    for (int aIndex = 0; aIndex < 14; ++aIndex)
    {
        TodStringListFormat &aFormat = gLawnStringFormats[aIndex];
        if ((aFormat.mFlags & 2) == 0)
            continue;
        if (mApp->HasSeedType(SEED_MAGNETSHROOM, false))
        {
            aFormat.mColor.mAlpha = 255;
            aFormat.mOffset = 0;
        }
        else
        {
            aFormat.mColor.mAlpha = 0;
            aFormat.mOffset = -17;
        }
    }

    g->PushState();
    g->ClipRect(mDescriptionRect);
    float aScroll = (float)(mScrollTextView->mValue * 0.01 * mDescriptionTextHeight);
    g->mTransY -= aScroll;
    mDescriptionScrollY = -aScroll;
    mDescriptionTextHeight = TodDrawStringWrappedHelper(
        g, mDescriptionString, mDescriptionRect, FONT_BRIANNETOD16, Color(40, 50, 90), mJustification, true, true);
    g->PopState();
}

// ==========================================================================
// AlmanacDialog::ZombieHasDescription(ZombieType)
// @0013A550  106 bytes
// ==========================================================================
bool AlmanacDialog::ZombieHasDescription(ZombieType theZombieType)
{
    int aStartLevel = ZombieStartLevel(theZombieType);
    int aLevel = mApp->mPlayerInfo->GetLevel();

    if (theZombieType == ZOMBIE_YETI)
    {
        // The yeti keeps its description until it is both spawnable and seen.
        if (!mApp->CanSpawnYetis())
            return false;
        if (mApp->mPlayerInfo->GetFlag(PlayerFlags(0x8000)))
            return true;
    }
    else if (mApp->HasFinishedAdventure())
    {
        return true;
    }

    if (aLevel < aStartLevel)
        return false;
    if (aLevel != aStartLevel)
        return true;
    return ZombieDefeated(theZombieType);
}

// ==========================================================================
// AlmanacDialog::ZombieHitTest(int,int)
// @0013A5C4  118 bytes
// ==========================================================================
ZombieType AlmanacDialog::ZombieHitTest(int x, int y)
{
    if (!mMouseVisible || mOpenPage != ALMANAC_PAGE_ZOMBIES)
        return ZOMBIE_INVALID;

    for (int aIndex = 0; aIndex < NUM_ALMANAC_ZOMBIES; ++aIndex)
    {
        ZombieType aZombieType = GetZombieType(aIndex);
        if (aZombieType == ZOMBIE_INVALID || !ZombieIsShown(aZombieType))
            continue;

        int aPosX = 0;
        int aPosY = 0;
        GetZombiePosition(aZombieType, aPosX, aPosY);
        if (x >= aPosX && y >= aPosY && x <= aPosX + 75 && y <= aPosY + 75)
            return aZombieType;
    }
    return ZOMBIE_INVALID;
}

// ==========================================================================
// AlmanacDialog::MouseDown(int,int,int)
// @0013A63C  214 bytes
// ==========================================================================
void AlmanacDialog::MouseDown(int x, int y, int theClickCount)
{
    if (theClickCount <= 0 || theClickCount == 3 || mOpenPage == ALMANAC_PAGE_INDEX)
        return;
    mMouseScrollActive = true;
    mMouseDraggingDescription =
        mScrollTextView != nullptr && mScrollTextView->mVisible && mDescriptionRect.Contains(x, y);
    mMouseLastY = y;

    SeedType aSeedType = mOpenPage == ALMANAC_PAGE_PLANTS ? SeedHitTest(x, y) : SEED_NONE;
    if (aSeedType != SEED_NONE && aSeedType != mSelectedSeed)
    {
        mSelectedSeed = aSeedType;
        SetupPlant();
        mApp->PlaySample(SOUND_TAP, true);
    }

    ZombieType aZombieType = mOpenPage == ALMANAC_PAGE_ZOMBIES ? ZombieHitTest(x, y) : ZOMBIE_INVALID;
    if (aZombieType != ZOMBIE_INVALID && aZombieType != mSelectedZombie)
    {
        mSelectedZombie = aZombieType;
        SetupZombie();
        mApp->PlaySample(SOUND_TAP, true);
    }
}

// ==========================================================================
// AlmanacDialog::NextZombieInDir(AlmanacDialog::SeedDir)
// @0013A720  90 bytes
// ==========================================================================
ZombieType AlmanacDialog::NextZombieInDir(AlmanacDialog_SeedDir theDir)
{
    // The zombie grid is five columns wide, so up/down step by five and the
    // boss -- which sits alone below it -- wraps onto the catapult.
    ZombieType aZombieType = mSelectedZombie;
    switch (theDir)
    {
        case 0:
            if (aZombieType == ZOMBIE_BOSS)
                aZombieType = ZOMBIE_CATAPULT;
            else
                aZombieType = (ZombieType)(aZombieType - 5);
            break;
        case 1:
            aZombieType = (ZombieType)(aZombieType + 5);
            break;
        case 2:
            aZombieType = (ZombieType)(aZombieType - 1);
            break;
        case 3:
            aZombieType = (ZombieType)(aZombieType + 1);
            break;
        default:
            break;
    }

    if (aZombieType > ZOMBIE_BOSS)
        aZombieType = ZOMBIE_BOSS;
    else if (aZombieType < ZOMBIE_NORMAL)
        return mSelectedZombie;

    return ZombieIsShown(aZombieType) ? aZombieType : mSelectedZombie;
}

// ==========================================================================
// AlmanacDialog::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @0013A77C  166 bytes
// ==========================================================================
void AlmanacDialog::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    switch (theButton)
    {
        case GAMEPAD_BUTTON_UP:
        case GAMEPAD_BUTTON_DOWN:
        case GAMEPAD_BUTTON_LEFT:
        case GAMEPAD_BUTTON_RIGHT:
            if (mOpenPage == ALMANAC_PAGE_PLANTS)
            {
                SeedType aSeedType = NextSeedInDir((AlmanacDialog_SeedDir)theButton);
                if (aSeedType != mSelectedSeed)
                {
                    mSelectedSeed = aSeedType;
                    mApp->PlaySample(SOUND_BLEEP, true);
                    SetupPlant();
                }
            }
            else if (mOpenPage == ALMANAC_PAGE_ZOMBIES)
            {
                ZombieType aZombieType = NextZombieInDir((AlmanacDialog_SeedDir)theButton);
                if (aZombieType != mSelectedZombie)
                {
                    mSelectedZombie = aZombieType;
                    mApp->PlaySample(SOUND_GRAVEBUTTON, true);
                    SetupZombie();
                }
            }
            break;

        case GAMEPAD_BUTTON_SELECT:
        case GAMEPAD_BUTTON_B:
            // Back returns to the index from a sub-page, and closes from there.
            if ((unsigned int)(mOpenPage - 1) <= 1)
                SetPage(ALMANAC_PAGE_INDEX);
            else
                mApp->KillAlmanacDialog();
            break;

        default:
            break;
    }
}

// ==========================================================================
// AlmanacDialog::GameAxisMove(Sexy::GamepadAxis,int,int)
// @0013A830  104 bytes
// ==========================================================================
void AlmanacDialog::GameAxisMove(Sexy::GamepadAxis theAxis, int theValue, int thePlayerIndex)
{
    // Only the active player's vertical axis scrolls the description.
    if (mApp->mPlayerInfo->GetId() != thePlayerIndex || theAxis != GAMEPAD_AXIS_RIGHT_Y)
        return;

    if (theValue > 0)
        mScrollTextView->SetValue(mScrollTextView->mValue - 1.0);
    else if (theValue != 0)
        mScrollTextView->SetValue(mScrollTextView->mValue + 1.0);
}

// ==========================================================================
// AlmanacDialog::ShowZombie(ZombieType)
// @0013A898  10 bytes
// ==========================================================================
void AlmanacDialog::ShowZombie(ZombieType theZombieType)
{
    mSelectedZombie = theZombieType;
    SetPage(ALMANAC_PAGE_ZOMBIES);
}

// ==========================================================================
// AlmanacDialog::ShowPlant(SeedType)
// @0013A8A4  10 bytes
// ==========================================================================
void AlmanacDialog::ShowPlant(SeedType theSeedType)
{
    mSelectedSeed = theSeedType;
    SetPage(ALMANAC_PAGE_PLANTS);
}

// ==========================================================================
// AlmanacDialog::SetupLayoutPlants(Sexy::Graphics *)
// @0013A940  2860 bytes
// ==========================================================================
void AlmanacDialog::SetupLayoutPlants(Sexy::Graphics *g)
{
    PlantDefinition *aDefinition = PlantDef(mSelectedSeed);

    // ---- the four text rects, in the card's own coordinates ----
    mCardTextRect = Sexy::Rect(485, 309, 258, 230);
    mDescriptionRect = Sexy::Rect(485, 363, 266, 178);
    mCostRect = Sexy::Rect(485, 314, 125, 50);
    mWaitTimeRect = Sexy::Rect(585, 314, 165, 50);

    mNameString = "";
    mDescriptionHeaderString = "";
    mDescriptionStringTmp = "";
    mDescriptionString = "";
    mCostString = "";
    mWaitTimeString = "";

    mNameString = Plant::GetNameString(mSelectedSeed, SEED_NONE);

    StrFormat(&mDescriptionHeaderString, "[%s_DESCRIPTION_HEADER]", aDefinition->mPlantName);
    TodStringTranslate(&mDescriptionHeaderString, mDescriptionHeaderString.c_str());

    StrFormat(&mDescriptionStringTmp, "[%s_DESCRIPTION]", aDefinition->mPlantName);
    TodStringTranslate(&mDescriptionStringTmp, mDescriptionStringTmp.c_str());

    TodStringTranslate(&mDescriptionString, mDescriptionStringTmp.c_str());

    // The description box shows the header line above the description body.
    Sexy::String aDescription = Sexy::String("{NORMAL}") + mDescriptionHeaderString;
    aDescription += "\n";
    aDescription += mDescriptionString;
    aDescription += "\n";
    mDescriptionString = aDescription;

    if (mSelectedSeed != SEED_IMITATER)
    {
        StrFormat(&mCostString, "{YELLOW}{COST}:{GREEN} %d", aDefinition->mPlantCost);
        Sexy::String aCost("[COST]");
        Sexy::String aCostText;
        TodReplaceString(&aCostText, mCostString.c_str(), "{COST}", &aCost);
        mCostString = aCostText;

        int aRefreshTime = aDefinition->mRefreshTime;
        if (aRefreshTime == 750)
        {
            mWaitTimeString = "{YELLOW}{WAIT_TIME}:{GREEN} {WAIT_TIME_LENGTH}";
            Sexy::String aShort("[WAIT_TIME_SHORT]");
            Sexy::String aWaitText;
            TodReplaceString(&aWaitText, mWaitTimeString.c_str(), "{WAIT_TIME_LENGTH}", &aShort);
            mWaitTimeString = aWaitText;
        }
        else if (aRefreshTime == 3000)
        {
            mWaitTimeString = "{YELLOW}{WAIT_TIME}:{GREEN} {WAIT_TIME_LENGTH}";
            Sexy::String aLong("[WAIT_TIME_LONG]");
            Sexy::String aWaitText;
            TodReplaceString(&aWaitText, mWaitTimeString.c_str(), "{WAIT_TIME_LENGTH}", &aLong);
            mWaitTimeString = aWaitText;
        }
        else
        {
            mWaitTimeString = "{YELLOW}{WAIT_TIME}:{GREEN} {WAIT_TIME_LENGTH}";
            Sexy::String aVeryLong("[WAIT_TIME_VERY_LONG]");
            Sexy::String aWaitText;
            TodReplaceString(&aWaitText, mWaitTimeString.c_str(), "{WAIT_TIME_LENGTH}", &aVeryLong);
            mWaitTimeString = aWaitText;
        }

        Sexy::String aWaitTime("[WAIT_TIME]");
        Sexy::String aWaitText;
        TodReplaceString(&aWaitText, mWaitTimeString.c_str(), "{WAIT_TIME}", &aWaitTime);
        mWaitTimeString = aWaitText;

        // The two colour tags are stripped again now that the text is final.
        Sexy::String aEmpty("");
        Sexy::String aYellowTag;
        TodReplaceString(&aYellowTag, mCostString.c_str(), "{YELLOW}", &aEmpty);
        Sexy::String aGreenTag;
        TodReplaceString(&aGreenTag, aYellowTag.c_str(), "{GREEN}", &aEmpty);
        mCostString = aGreenTag;

        Sexy::String aWaitNoColor;
        TodReplaceString(&aWaitNoColor, mWaitTimeString.c_str(), "{YELLOW}", &aEmpty);
        Sexy::String aWaitNoColor2;
        TodReplaceString(&aWaitNoColor2, aWaitNoColor.c_str(), "{GREEN}", &aEmpty);
        mWaitTimeString = aWaitNoColor2;

        mWaitTimeJustification = 1;
    }

    // ---- measure the description so the scrollbar can size itself ----
    g->PushState();
    g->ClipRect(mDescriptionRect);

    Sexy::Rect aMeasureRect = mDescriptionRect;
    aMeasureRect.mHeight = mDescriptionRect.mHeight - 4;

    g->mTransY = mDescriptionScrollY;
    mDescriptionTextHeight = TodDrawStringWrappedHelper(
        g, mDescriptionString, aMeasureRect, FONT_BRIANNETOD16, Color(40, 50, 90), DS_ALIGN_LEFT, false, true);
    g->PopState();

    mScrollTextView->SetValue(0.0);
    mDescriptionScrollY = 0.0f;

    if (mDescriptionTextHeight <= mDescriptionRect.mHeight)
    {
        mScrollTextView->SetVisible(false);
    }
    else
    {
        mDescriptionRect.mHeight -= 4;
        // unk_694004 is the plant page's seven scrollbar colours, four ints
        // each. ReconData.h types the symbol as one word.
        mScrollTextView->SetColors(gAlmanacPlantScrollColors, 7);
        mScrollTextView->SetVisible(true);
        mScrollTextView->ResizeScrollbar(
            mDescriptionRect.mX + mDescriptionRect.mWidth - 6, mDescriptionRect.mY - 6, 8, mDescriptionRect.mHeight);
        mDescriptionRect.mWidth -= 7;
        mScrollTextView->SetPageSize((double)mDescriptionRect.mHeight / (double)mDescriptionTextHeight * 85.0);
    }

    mSetupFinished = true;
}

// ==========================================================================
// AlmanacDialog::SetupLayoutZombies(Sexy::Graphics *)
// @0013B550  1262 bytes
// ==========================================================================
void AlmanacDialog::SetupLayoutZombies(Sexy::Graphics *g)
{
    ZombieDefinition *aDefinition = GetZombieDefinition(mSelectedZombie);

    // ---- the two text rects, in the card's own coordinates ----
    mCardTextRect = Sexy::Rect(485, 365, 263, 230);
    mDescriptionRect = Sexy::Rect(485, 371, 263, 160);

    mNameString = "";
    mDescriptionHeaderString = "";
    mDescriptionStringTmp = "";
    mDescriptionString = "";
    mCostString = "";
    mWaitTimeString = "";

    if (ZombieHasDescription(mSelectedZombie))
    {
        Sexy::String aHeader;
        StrFormat(&aHeader, "[%s_DESCRIPTION_HEADER]", aDefinition->mZombieName);
        Sexy::String aBody;
        StrFormat(&aBody, "[%s_DESCRIPTION]", aDefinition->mZombieName);

        mDescriptionStringTmp = aBody;
        TodStringTranslate(&mDescriptionHeaderString, aHeader.c_str());
        TodStringTranslate(&mDescriptionString, mDescriptionStringTmp.c_str());
        mJustification = DS_ALIGN_LEFT;
    }
    else
    {
        TodStringTranslate(&mDescriptionString, "[NOT_ENCOUNTERED_YET_DESCRIPTION]");
        mJustification = DS_ALIGN_CENTER_MIDDLE;
    }

    // The description box shows the header line above the description body.
    Sexy::String aDescription = Sexy::String("{NORMAL}") + mDescriptionHeaderString;
    aDescription += "\n";
    aDescription += mDescriptionString;
    aDescription += "\n";
    mDescriptionString = aDescription;

    if (ZombieHasSilhouette(mSelectedZombie))
        mNameString = "[NOT_ENCOUNTERED_YET]";
    else
        StrFormat(&mNameString, "[%s]", aDefinition->mZombieName);

    // ---- measure the description so the scrollbar can size itself ----
    g->PushState();
    g->ClipRect(mDescriptionRect);
    g->mTransY = mDescriptionScrollY;
    mDescriptionTextHeight = TodDrawStringWrappedHelper(
        g, mDescriptionString, mDescriptionRect, FONT_BRIANNETOD16, Color(40, 50, 90), mJustification, false, true);
    g->PopState();

    mScrollTextView->SetValue(0.0);
    mDescriptionScrollY = 0.0f;

    if (mDescriptionTextHeight > mDescriptionRect.mHeight)
    {
        // unk_694064 is the zombie page's seven scrollbar colours.
        mScrollTextView->SetColors(gAlmanacZombieScrollColors, 7);
        mScrollTextView->SetVisible(true);
        mScrollTextView->ResizeScrollbar(
            mDescriptionRect.mX + mDescriptionRect.mWidth - 6, mDescriptionRect.mY, 8, mDescriptionRect.mHeight);
        mDescriptionRect.mWidth -= 7;
        mScrollTextView->SetPageSize((double)mDescriptionRect.mHeight / (double)mDescriptionTextHeight * 85.0);
    }
    else
    {
        mScrollTextView->SetVisible(false);
    }

    mSetupFinished = true;
}

// ==========================================================================
// AlmanacDialog::Draw(Sexy::Graphics *)
// @0013BB10  98 bytes
// ==========================================================================
void AlmanacDialog::Draw(Sexy::Graphics *g)
{
    g->SetLinearBlend(true);

    // Only the index page is drawn for page 0. A page other than the two
    // list pages draws nothing; it does not fall through to the index.
    if (mOpenPage == ALMANAC_PAGE_INDEX)
    {
        DrawIndex(g);
    }
    else if (mOpenPage == ALMANAC_PAGE_PLANTS)
    {
        if (!mSetupFinished)
            SetupLayoutPlants(g);
        DrawPlants(g);
    }
    else if (mOpenPage == ALMANAC_PAGE_ZOMBIES)
    {
        if (!mSetupFinished)
            SetupLayoutZombies(g);
        DrawZombies(g);
    }
}

// ==========================================================================
// AlmanacDialog::AddedToManager(Sexy::WidgetManager *)
// @0013BB74  56 bytes
// ==========================================================================
void AlmanacDialog::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    WidgetContainer::AddedToManager(theWidgetManager);
    AddWidget(mScrollTextView);
    AddWidget(mPlantButton);
    AddWidget(mZombieButton);
    mApp->ShowHelpBarWidget();
}

// ==========================================================================
// AlmanacDialog::RemovedFromManager(Sexy::WidgetManager *)
// @0013BBAC  62 bytes
// ==========================================================================
void AlmanacDialog::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    ClearPlantsAndZombies();
    RemoveWidget(mScrollTextView);
    RemoveWidget(mPlantButton);
    RemoveWidget(mZombieButton);
    mApp->HideHelpBarWidget();
}

// ==========================================================================
// AlmanacDialog::Update()
// @0013BBF0  432 bytes
// ==========================================================================
void AlmanacDialog::Update()
{
    Widget::Update();

    // The description creeps downwards on its own, one step every 50 frames.
    if (!mMouseScrollActive && mScrollTextView != nullptr && ++gAlmanacScrollTick % 50 == 0)
        mScrollTextView->SetValue(mScrollTextView->mValue + 1.0);

    mPlantButton->Update();
    mZombieButton->Update();

    // Windows polls gamepad axes without dispatching the TV's packed axis event.
    // Use the active player's right stick for the same per-frame scroll step.
    int gamepadIndex = mApp->mPlayerInfo->GetId();
    Sexy::Gamepad *gamepad = gamepadIndex >= 0 ? mApp->GetGamepad(gamepadIndex) : nullptr;
    bool hasGamepadScroll = gamepad != nullptr && gamepad->IsConnected();
    if (hasGamepadScroll && mScrollTextView != nullptr &&
        (mOpenPage == ALMANAC_PAGE_PLANTS || mOpenPage == ALMANAC_PAGE_ZOMBIES))
    {
        float axisValue = gamepad->GetRightAxisYPosition();
        if (fabsf(axisValue) >= 0.2f)
            GameAxisMove(GAMEPAD_AXIS_RIGHT_Y, axisValue > 0.0f ? 1 : -1, mApp->mPlayerInfo->GetId());
    }
    else if (mAxisScrollActive && mAxisDeviceIndex >= 0.0f)
    {
        // Re-read the device the scroll came from and keep scrolling while its
        // axis is still past the dead zone. Find returns the driver as an int
        // (InputManager.h); the object it found is an IInputDriver.
        Sexy::IInputDriver *aDriver = mApp->mInputManager->Find((int)mAxisDeviceIndex);

        // The call is vtable +0x48 with (driver, 2, &state, 0). That byte is
        // IInputDriver::GetNumAxes, whose declared signature takes only the
        // device index -- the state pointer and the trailing 0 are what this
        // one site passes and what the header's method does not. IInputDriver
        // is not declared in AlmanacDialog.h, so the method cannot be added or
        // retyped from here; the slot is named on the local vtable view.
        Sexy::DriverAxisSample aState = {};
        if (aDriver != nullptr)
            aDriver->GetAxisSample(2, aState, 0);

        if (aDriver == nullptr || fabsf(aState.mValue) < aState.mDeadZone || fabsf(aState.mValue) < 0.2f)
        {
            mAxisScrollActive = false;
            mAxisDeviceIndex = -1.0f;
        }
        else if (aState.mValue > 0.0f)
        {
            mScrollTextView->SetValue(mScrollTextView->mValue + 1.0);
        }
        else if (aState.mValue < 0.0f)
        {
            mScrollTextView->SetValue(mScrollTextView->mValue - 1.0);
        }
        else
        {
            mAxisScrollActive = false;
            mAxisDeviceIndex = -1.0f;
        }
    }

    if (mPlant != nullptr)
        mPlant->Update();
    if (mZombie != nullptr)
        mZombie->Update();

    int aMouseX = mApp->mWidgetManager->mLastMouseX;
    int aMouseY = mApp->mWidgetManager->mLastMouseY;
    bool aOverSomething = SeedHitTest(aMouseX, aMouseY) != SEED_NONE ||
        ZombieHitTest(aMouseX, aMouseY) != ZOMBIE_INVALID || mPlantButton->IsMouseOver() ||
        mZombieButton->IsMouseOver();
    mApp->SetCursor(aOverSomething ? 1 : 0);

    mApp->mPoolEffect->PoolEffectUpdate();
    MarkDirty();
}

// ==========================================================================
// AlmanacDialog::KeyDown(Sexy::KeyCode)
// @0013BDB0  222 bytes
// ==========================================================================
int AlmanacDialog::KeyDown(Sexy::KeyCode theKey)
{
    if (theKey >= Sexy::KEYCODE_LEFT && theKey <= Sexy::KEYCODE_DOWN)
    {
        // LDR [table + key*4 + 0xBC]. The direction ints start at +0x150 and
        // KEYCODE_LEFT is 0x25, so the index is the key minus that.
        int aDir = gAlmanacSeedPos.mArrowDir[theKey - Sexy::KEYCODE_LEFT];

        if (mOpenPage == ALMANAC_PAGE_PLANTS)
        {
            SeedType aSeedType = NextSeedInDir(AlmanacDialog_SeedDir(aDir));
            if (aSeedType != mSelectedSeed)
            {
                mSelectedSeed = aSeedType;
                mApp->PlaySample(SOUND_BLEEP, true);
                SetupPlant();
            }
        }
        else if (mOpenPage == ALMANAC_PAGE_ZOMBIES)
        {
            ZombieType aZombieType = NextZombieInDir(AlmanacDialog_SeedDir(aDir));
            if (aZombieType != mSelectedZombie)
            {
                mSelectedZombie = aZombieType;
                mApp->PlaySample(SOUND_GRAVEBUTTON, true);
                SetupZombie();
            }
        }
        else
        {
            return LawnDialog::KeyDown(theKey);
        }
        return 1;
    }

    if (theKey != Sexy::KEYCODE_ESCAPE && theKey != 305 && theKey != 314)
        return LawnDialog::KeyDown(theKey);

    // Escape and the two gamepad back buttons return to the index, and close
    // the almanac when it is already showing.
    if ((unsigned int)(mOpenPage - 1) <= 1)
    {
        SetFocus(mOpenPage == ALMANAC_PAGE_ZOMBIES ? mZombieButton : mPlantButton, true);
        SetPage(ALMANAC_PAGE_INDEX);
    }
    else
    {
        mApp->KillAlmanacDialog();
    }
    return 1;
}

// ==========================================================================
// AlmanacDialog::GotFocus()
// @0013BEA0  34 bytes
// ==========================================================================
void AlmanacDialog::GotFocus()
{
    LawnDialog::GotFocus();
    if (mFocusedChildWidget == nullptr)
        SetFocus(mPlantButton, true);
}
