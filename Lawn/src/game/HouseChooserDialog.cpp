#include "HouseChooserDialog.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "LeaderboardsWidget.h"
#include "WidgetManager.h"
#include <cmath>

HouseChooserDialog::~HouseChooserDialog()
{
}

bool HouseChooserDialog::IsHouseAvaliable(HouseType houseType)
{
    return houseType == BLUEPRINT_BLING
        || (houseType >= BLUEPRINT_SCARY && houseType <= BLUEPRINT_CLOWN
            && gLawnApp->mPlayerInfo->mPurchases[STORE_ITEM_BLUEPRINT_BLING + int(houseType)] > 0);
}

// Touch selection adapted from PvZ TV Touch Team; original layout @0x192990.
void HouseChooserDialog::MouseDown(int x, int y, int theClickCount)
{
    if (theClickCount <= 0 || theClickCount == 3)
        return;
    for (int house = BLUEPRINT_BLING; house <= BLUEPRINT_CLOWN; ++house)
    {
        if (!Sexy::Rect(75 + 90 * house, 125, IMAGE_STORE_BLUEPRINT_CLOWN->mWidth,
                        IMAGE_STORE_BLUEPRINT_CLOWN->mHeight).Contains(x, y)
            || !IsHouseAvaliable(HouseType(house)))
            continue;
        if (mSelectedHouseType == house)
            GameButtonDown(GAMEPAD_BUTTON_A, 0, 0);
        else
        {
            mSelectedHouseType = HouseType(house);
            mGrowTime = 0.0f;
            mApp->PlaySample(SOUND_BLEEP, true);
        }
        return;
    }
}

void HouseChooserDialog::GotFocus()
{
    mApp->mHelpBarWidget->ClearButtons(0);
    mApp->mHelpBarWidget->AddButton(GAMEPAD_BUTTON_B, Sexy::String("[CHOOSE_HOUSE_CANCEL]"), HelpBarWidget::HELP_ALIGN_NONE);
    mApp->mHelpBarWidget->AddButton(GAMEPAD_BUTTON_A, Sexy::String("[CHOOSE_HOUSE_CONFIRM]"), HelpBarWidget::HELP_ALIGN_NONE);
    mApp->mHelpBarWidget->AddButton(GAMEPAD_BUTTON_LEFT, Sexy::String("[CHOOSE_HOUSE]"), HelpBarWidget::HELP_ALIGN_NONE);
    mApp->ShowHelpBarWidget();
}

int HouseChooserDialog::KeyDown(Sexy::KeyCode theKey)
{
    int button = GAMEPAD_BUTTON_NONE;
    int player = 0;
    unsigned int flags = 0;
    if (mEvent != nullptr && mApp->MapToButtonEvent(mEvent, button, player, flags))
    {
        GameButtonDown(Sexy::GamepadButton(button), player, flags);
        return 1;
    }
    switch (theKey)
    {
    case Sexy::KEYCODE_ESCAPE:
    case Sexy::KEYCODE_GAMEPAD_B: GameButtonDown(GAMEPAD_BUTTON_B, 0, 0); return 1;
    case Sexy::KEYCODE_RETURN:
    case Sexy::KEYCODE_GAMEPAD_A: GameButtonDown(GAMEPAD_BUTTON_A, 0, 0); return 1;
    case Sexy::KEYCODE_LEFT: GameButtonDown(GAMEPAD_BUTTON_LEFT, 0, 0); return 1;
    case Sexy::KEYCODE_RIGHT: GameButtonDown(GAMEPAD_BUTTON_RIGHT, 0, 0); return 1;
    default: return LawnDialog::KeyDown(theKey);
    }
}

void HouseChooserDialog::ButtonDepress(int theId)
{
    if (theId == 1000 || theId == 1001)
    {
        LawnApp* app = mApp;
        app->KillDialog(mId);
        if (theId == 1000)
            app->WriteHouse(true);
        if (app->mLeaderboardScreen != nullptr)
            app->mWidgetManager->SetFocus(app->mLeaderboardScreen);
        return;
    }
    LawnDialog::ButtonDepress(theId);
}

// A house is selectable only when the player owns it. Ownership is the purchase
// slot BLUEPRINT_BLING + house; LEFT reads that table from the other end.
void HouseChooserDialog::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    (void)thePlayerIndex;
    if ((theFlags & 1) != 0)
        return;

    if (theButton == GAMEPAD_BUTTON_LEFT || theButton == GAMEPAD_BUTTON_RIGHT)
    {
        int house = mSelectedHouseType;
        mGrowTime = 0.0f;
        do
            house = (house + (theButton == GAMEPAD_BUTTON_LEFT ? 4 : 1)) % 5;
        while (!IsHouseAvaliable(HouseType(house)));
        mSelectedHouseType = HouseType(house);
        return;
    }

    if (theButton == GAMEPAD_BUTTON_A)
    {
        int house = mSelectedHouseType;
        LawnPlayerInfo* playerInfo = mApp->mPlayerInfo;
        int delta = (house != 0) ? -1 : 2;
        playerInfo->mGameStats.mHouseType = mSelectedHouseType;
        playerInfo->mGameStats.mHouseLevel = playerInfo->mPurchases[STORE_ITEM_BLUEPRINT_BLING + house] + delta;
        ButtonDepress(1000);
        return;
    }

    if (theButton == GAMEPAD_BUTTON_B)
        ButtonDepress(1001);
}

void HouseChooserDialog::Draw(Sexy::Graphics* g)
{
    Sexy::Image* const blueprintImages[5] =
    {
        IMAGE_STORE_BLUEPRINT_BLING,
        IMAGE_STORE_BLUEPRINT_SCARY,
        IMAGE_STORE_BLUEPRINT_TRAILER,
        IMAGE_STORE_BLUEPRINT_FUTURE,
        IMAGE_STORE_BLUEPRINT_CLOWN,
    };

    mSpaceAfterHeader = 175;
    LawnDialog::Draw(g);

    int posX = 75;
    for (int house = 0; house != 5; ++house)
    {
        bool selected = (mSelectedHouseType == house);
        Sexy::Image* image = blueprintImages[house];

        if (image != nullptr)
        {
            if (selected)
            {
                float scale = TodAnimateCurveFloatTime(0, 0.2f, mGrowTime, 1.0f, 1.1f, 1);
                g->SetScale(scale, scale, (float)(posX + 33), 159.0f);
            }
            g->DrawImage(image, posX, 125);
            if (selected)
            {
                g->SetDrawMode(0);
                double pulse = sin((float)((mUpdateCnt % 100) * 0.01f) * 3.14159265);
                g->SetColor(Sexy::Color(255, 255, 100, (int)(pulse * 150.0)));
                g->SetColorizeImages(true);
                g->DrawImage(image, posX, 125);
                g->SetColor(Sexy::Color::White);
            }
        }

        if (!IsHouseAvaliable(HouseType(house)))
        {
            Sexy::Rect rect(posX, 125, 67, 68);
            TodDrawStringWrapped(g, TodStringTranslate("[NOT_AVAILABLE]"), rect,
                                 FONT_HOUSEOFTERROR16, Sexy::Color::White, 5, 0);
        }

        if (selected)
            g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
        posX += 90;
    }

    g->PushState();
    g->Translate(-350, 300);
    g->PopState();
}

void HouseChooserDialog::Update()
{
    mGrowTime += 0.016f;
    LawnDialog::Update();
}

void HouseChooserDialog::Resize(int theX, int theY, int theWidth, int theHeight)
{
    LawnDialog::Resize(theX, theY, theWidth, theHeight);
}

HouseChooserDialog::HouseChooserDialog(LawnApp* theApp)
    : LawnDialog(theApp, nullptr, 71, true, Sexy::String("[CHOOSE_HOUSE]"), Sexy::String(), Sexy::String(), 0)
{
    TodLoadResources(Sexy::String("DelayLoad_Store"));
    mGrowTime = 0.0f;
    mSelectedHouseType = mApp->mPlayerInfo->mGameStats.mHouseType;
    Resize(100, 100, 600, 355);
}

void HouseChooserDialog::RemovedFromManager(Sexy::WidgetManager* theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    mApp->HideHelpBarWidget();
}
