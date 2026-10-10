#include "ShopSeedPacket.h"
#include "GameObject.h"
#include "GamepadControls.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnUser.h"
#include "SexyTransform2D.h"
#include <cmath>

ShopSeedPacket::~ShopSeedPacket() {}

ShopSeedPacket::ShopSeedPacket()
{
    mStoreItemType = -1;
    mPacketOffsetX = 0;
    mSelectAnimCounter = 0.15f;
    mSelectArrowAnimTime = 0.0f;
    mSelectScale = 0.0f;
    mSeedType = SEED_NONE;
    mSelectedPlayerIndex = false;
    mIsSelected = false;
    mShowBothPlayerCursors = false;
}

void ShopSeedPacket::UpdateSelected()
{
    Board *board = mBoard;
    SeedBank *seedBank1 = board->mSeedBank[0];
    SeedBank *seedBank2 = board->mSeedBank[1];
    GamepadControls *gamepad1 = board->mGamepadControls[0];
    GamepadControls *gamepad2 = board->mGamepadControls[1];
    int shopItemIndex = mStoreItemType - 10;
    bool playerIndex;

    // The first player's cursor shows only while the second player's gamepad is
    // idle, not sitting on a zombie, and not already on this very shop item.
    if (mSeedBank != seedBank1 || gamepad2->mSelectedShopSeedIndex != shopItemIndex || gamepad2->mGamepadIndex == -1 ||
        gamepad2->mIsZombie)
    {
        playerIndex = false;
    }
    else
    {
        playerIndex = !mApp->IsCoopMode();
    }

    mSelectedPlayerIndex = playerIndex;
    if (seedBank2 == mSeedBank && gamepad2 != nullptr)
    {
        if (gamepad2->mIsZombie || mApp->IsCoopMode())
        {
            if (gamepad2->mSelectedShopSeedIndex == shopItemIndex)
                mSelectedPlayerIndex = true;
        }
    }

    if (mApp->mGameMode == GAMEMODE_MP_VS)
    {
        mSelectedPlayerIndex = (seedBank1 == mSeedBank && gamepad1->mPlayerIndex == 1 &&
                                   gamepad1->mSelectedShopSeedIndex == shopItemIndex) ||
            (seedBank2 == mSeedBank && gamepad2->mPlayerIndex == 1 &&
                gamepad2->mSelectedShopSeedIndex == shopItemIndex);
    }

    bool isSelected;
    if (seedBank1 == mSeedBank && gamepad1->mSelectedShopSeedIndex == shopItemIndex)
        isSelected = true;
    else if (seedBank2 == mSeedBank && gamepad2->mSelectedShopSeedIndex == shopItemIndex)
        isSelected = true;
    else
        isSelected = mSelectedPlayerIndex;

    mIsSelected = isSelected;
    bool bothCursors = false;
    if (gamepad2->mGamepadIndex != -1 && gamepad1->mSelectedShopSeedIndex == gamepad2->mSelectedShopSeedIndex)
    {
        bothCursors = !mApp->IsCoopMode() && mApp->mGameMode != GAMEMODE_MP_VS;
    }

    mShowBothPlayerCursors = bothCursors;
    if (gamepad1->mIsInShopSeedBank)
    {
        bool onThisItem = (gamepad1->mSelectedShopSeedIndex == shopItemIndex);
        mIsSelected = onThisItem;
        if (!onThisItem && mApp->IsTwoPlayerGame())
        {
            GamepadControls *otherGamepad = board->mGamepadControls[1];
            if (otherGamepad != nullptr && otherGamepad->mIsInShopSeedBank && otherGamepad->mGamepadIndex != -1)
            {
                mIsSelected = (otherGamepad->mSelectedShopSeedIndex == shopItemIndex);
            }
        }
    }
    else
    {
        mIsSelected = false;
    }
}

void ShopSeedPacket::Update()
{
    UpdateSelected();
    mSelectArrowAnimTime += 0.006f;
    mSelectAnimCounter += 0.006f;
}

void ShopSeedPacket::DrawMiddle() {}

void ShopSeedPacket::DrawOverlay(Sexy::Graphics *g)
{
    if (!mApp->IsSlotMachineLevel())
    {
        if (mIsSelected)
        {
            if (mSelectAnimCounter < 0.15f)
                mSelectScale = TodAnimateCurveFloatTime(0, 0.15f, mSelectAnimCounter, 1.0f, 1.1f, CURVE_EASE_OUT);
            else
                mSelectScale = 1.1f;
        }
        else
        {
            mSelectScale = TodAnimateCurveFloatTime(0, 0.15f, mSelectAnimCounter, 1.1f, 1.0f, CURVE_EASE_OUT);
        }
    }

    if (mApp->IsSlotMachineLevel() || !mIsSelected)
        return;

    float bob = sinf(mSelectArrowAnimTime * 5.0f);
    if (!mApp->IsTwoPlayerGame())
    {
        Sexy::Image *arrow = mSelectedPlayerIndex ? IMAGE_CURSOR_ARROW_P2 : IMAGE_CURSOR_ARROW_P1;
        SexyTransform2D transform(true);
        transform.Scale(1.5f, 1.5f);
        transform.Translate((float)(mPacketOffsetX + 23), (float)(bob + bob) - 8.0f);
        g->DrawImageMatrix(arrow, transform, 0.0f, 0.0f, true);
        return;
    }

    Sexy::Image *textImage;
    Sexy::Image *arrow;
    if (mSelectedPlayerIndex)
    {
        textImage = IMAGE_CURSOR_P2_TEXT;
        arrow = IMAGE_CURSOR_ARROW_P2;
        if (mShowBothPlayerCursors)
        {
            Sexy::Image *p1Arrow = IMAGE_CURSOR_ARROW_P1;
            Sexy::Image *p1Text = IMAGE_CURSOR_P1_TEXT;
            float bob2 = bob + bob;
            g->DrawImageRotatedF(
                arrow, (float)(mPacketOffsetX + 23 - arrow->mWidth / 2), bob2 + 63.0f, 3.14159274f, nullptr);
            g->DrawImageF(textImage, (float)(mPacketOffsetX + 23 - textImage->mWidth / 2), 75.0f);
            g->DrawImageF(p1Arrow, (float)(mPacketOffsetX + 23 - p1Arrow->mWidth / 2), bob2 - 8.0f);
            g->DrawImageF(p1Text, (float)(mPacketOffsetX + 23 - p1Text->mWidth / 2), -28.0f);
            return;
        }
    }
    else
    {
        textImage = IMAGE_CURSOR_P1_TEXT;
        arrow = IMAGE_CURSOR_ARROW_P1;
    }
    g->DrawImageF(arrow, (float)(mPacketOffsetX + 23 - arrow->mWidth / 2), (float)(bob + bob) - 8.0f);
    g->DrawImageF(textImage, (float)(mPacketOffsetX + 23 - textImage->mWidth / 2), -28.0f);
}

void ShopSeedPacket::DrawBackground(Sexy::Graphics *g)
{
    bool scaled = false;

    if (!mApp->IsSlotMachineLevel())
    {
        if (mIsSelected)
        {
            if (mSelectAnimCounter >= 0.15f)
                mSelectScale = 1.1f;
            else
                mSelectScale = TodAnimateCurveFloatTime(0, 0.15f, mSelectAnimCounter, 1.0f, 1.1f, CURVE_EASE_OUT);
            scaled = true;
        }
        else
        {
            mSelectScale = TodAnimateCurveFloatTime(0, 0.15f, mSelectAnimCounter, 1.1f, 1.0f, CURVE_EASE_OUT);
            scaled = (mSelectAnimCounter < 0.15f);
        }

        if (scaled)
        {
            g->SetScale(mSelectScale, mSelectScale, (float)(mPacketOffsetX + 25), 35.0f);
            g->TranslateF((float)(1.0f - mSelectScale) * 25.0f, (float)(1.0f - mSelectScale) * 35.0f);
        }
    }

    if (!mApp->IsSlotMachineLevel() && mIsSelected)
    {
        Sexy::Image *selector = mSelectedPlayerIndex ? IMAGE_SEED_SELECTOR_BLUE : IMAGE_SEED_SELECTOR;
        g->DrawImage(selector,
            (int)((float)(mPacketOffsetX - 1) - g->mScaleX * 5.0f),
            (int)-(g->mScaleY * 5.0f + 2.5f),
            (int)(g->mScaleX * 60.0f),
            (int)(g->mScaleY * 80.0f));
    }

    int shopItemIndex = mStoreItemType - 10;
    Sexy::Image *shopItemImage = nullptr;
    switch (mStoreItemType)
    {
        case 0xA:
            shopItemImage = IMAGE_SHOPITEM_BOMB;
            break;
        case 0xB:
            shopItemImage = IMAGE_SHOPITEM_SUN500;
            break;
        case 0xC:
            shopItemImage = IMAGE_SHOPITEM_SUN1000;
            break;
        case 0xD:
            shopItemImage = IMAGE_SHOPITEM_COLDDOWN;
            break;
        case 0xE:
            shopItemImage = IMAGE_SHOPITEM_SHOOTER;
            break;
        case 0xF:
            shopItemImage = IMAGE_SHOPITEM_PEPPER;
            break;
        case 0x10:
            shopItemImage = IMAGE_SHOPITEM_GETMONEY;
            break;
        default:
            break;
    }
    if (shopItemImage != nullptr)
    {
        g->DrawImage(shopItemImage, mPacketOffsetX, 0, (int)(mSelectScale * 48.0f), (int)(mSelectScale * 66.0f));
        if (shopItemIndex != 6)
        {
            LawnUser *user = LawnUser::getCurUser();
            std::map<int, Sexy::String>::iterator found = user->mPrices.find(shopItemIndex);
            if (found != user->mPrices.end())
            {
                int price = atoi(found->second.c_str());
                Sexy::String priceText;
                StrFormat(&priceText, "%d", price);
                Sexy::Color color(0, 0, 0);
                TodDrawString(g,
                    priceText,
                    (int)((float)mPacketOffsetX + mSelectScale * 29.0f),
                    (int)(mSelectScale * 30.0f - 30.0f + 60.0f),
                    FONT_BRIANNETOD12,
                    color,
                    2);
            }
        }
    }

    if (!mApp->IsSlotMachineLevel() && (mIsSelected || mSelectAnimCounter < 0.15f))
    {
        g->SetScale(1.0f, 1.0f, 0.0f, 0.0f);
        g->TranslateF((float)(mSelectScale - 1.0f) * 25.0f, (float)(mSelectScale - 1.0f) * 35.0f);
    }
}

void ShopSeedPacket::Draw(Sexy::Graphics *g)
{
    DrawBackground(g);
    DrawMiddle();
    DrawOverlay(g);
}
