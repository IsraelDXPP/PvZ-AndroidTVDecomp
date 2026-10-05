#include "HouseChooserDialog.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "LawnPlayerInfo.h"
#include "WidgetManager.h"
#include <cmath>

HouseChooserDialog::~HouseChooserDialog()
{
}

void HouseChooserDialog::GotFocus()
{
    mApp->mHelpBarWidget->ClearButtons(0);
    mApp->mHelpBarWidget->AddButton(GAMEPAD_BUTTON_B, Sexy::String("[CHOOSE_HOUSE_CANCEL]"), HelpBarWidget::HELP_ALIGN_NONE);
    mApp->mHelpBarWidget->AddButton(GAMEPAD_BUTTON_A, Sexy::String("[CHOOSE_HOUSE_CONFIRM]"), HelpBarWidget::HELP_ALIGN_NONE);
    mApp->mHelpBarWidget->AddButton(GAMEPAD_BUTTON_LEFT, Sexy::String("[CHOOSE_HOUSE]"), HelpBarWidget::HELP_ALIGN_NONE);
    mApp->ShowHelpBarWidget();
}

// A house is selectable only when the player owns it. Ownership is the purchase
// slot BLUEPRINT_BLING + house; LEFT reads that table from the other end.
void HouseChooserDialog::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    (void)thePlayerIndex;
    if ((theFlags & 1) != 0)
        return;

    if (theButton == GAMEPAD_BUTTON_LEFT)
    {
        int house = mSelectedHouseType;
        for (;;)
        {
            bool wasBling = (house == BLUEPRINT_SCARY);
            --house;
            int purchase;
            if (house >= 0)
            {
                if (wasBling)
                    break;
                purchase = house + 30;
            }
            else
            {
                house = BLUEPRINT_CLOWN;
                purchase = 35;
            }
            if (mApp->mPlayerInfo->mPurchases[purchase] != 0)
                break;
        }
        mSelectedHouseType = (HouseType)house;
        return;
    }

    if (theButton == GAMEPAD_BUTTON_RIGHT)
    {
        int house = mSelectedHouseType;
        mGrowTime = 0.0f;
        do
            house = (house + 1) % 5;
        while (house != 0 && mApp->mPlayerInfo->mPurchases[house] == 0);
        mSelectedHouseType = (HouseType)house;
        return;
    }

    if (theButton == GAMEPAD_BUTTON_A)
    {
        int house = mSelectedHouseType;
        LawnPlayerInfo* playerInfo = mApp->mPlayerInfo;
        int delta = (house != 0) ? -1 : 2;
        playerInfo->mGameStats[33] = house;
        playerInfo->mGameStats[34] = playerInfo->mPurchases[house] + delta;
        ButtonDepress(1000);
        return;
    }

    if (theButton == GAMEPAD_BUTTON_B)
        ButtonDepress(1001);
}

void HouseChooserDialog::Draw(Sexy::Graphics* g)
{
    static Sexy::Image* const kBlueprintImages[5] =
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
        Sexy::Image* image = kBlueprintImages[house];

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

        if (mApp->mPlayerInfo->mPurchases[house] == 0 && house != 0)
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
    mGrowTime = 0.0f;
    mSelectedHouseType = (HouseType)(mApp->mPlayerInfo->mGameStats[33]);
    Resize(100, 100, 600, 355);
}

void HouseChooserDialog::RemovedFromManager(Sexy::WidgetManager* theWidgetManager)
{
    LawnDialog::RemovedFromManager(theWidgetManager);
    mApp->HideHelpBarWidget();
}
