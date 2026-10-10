// StoreScreen -- restored from libGameMain.so.
// Rewritten by hand in stage 2; see build/phase2-kickoff.md.
//
// The stage-1 bodies were the IDA decompilation with syntactic changes only.
// Every raw `this` offset, hand-written vtable call, stored vptr and two-step
// allocation is gone; the members, the virtuals and the return types all come
// from Game\StoreScreen.h.

#include "StoreScreen.h"
#include "RawObject.h"
#include "ReconData.h"
#include "game/Board.h"
#include "DataArrayMethods.h"
#include "Coin.h"
#include "Curve1DUtil.h"
#include "Dialog.h"
#include "Font.h"
#include "GameStats.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "HouseChooserDialog.h"
#include "Image.h"
#include "LawnApp.h"
#include "LawnMower.h"
#include "LawnPlayerInfo.h"
#include "MemoryImage.h"
#include "Plant.h"
#include "PottedPlant.h"
#include "Reanimation.h"
#include "ReconTypes.h"
#include "SeedBank.h"
#include "SeedChooserScreen.h"
#include "SexyAppBase.h"
#include "StoreScreenOverlay.h"
#include "TodFoley.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include "ZenGarden.h"
#include <cstring>
#include <new>
#include <cmath>

namespace
{

// ---------------------------------------------------------------------------
// The coin pool's element type.
//
// StoreScreen::mCoins is the framework's DataArray<Coin>: the record is seven
// words (Game\DataArray_Coin.h lays out exactly the same seven) and one element
// is the Coin followed by the array's packed four-byte id, so the stride is
// 0xCC. The dump reaches an element as `block + 204 * index` because
// DataArray<Coin>::IterateNext was inlined into the DrawOverlay/Update loops.
//
// The id packs the free-list key into the high half and the slot index into the
// low half; the loops test the key, which is why `*(_WORD *)(item + 202) == 0`
// appears in the decompilation.
// ---------------------------------------------------------------------------

} // namespace

// ==========================================================================
// StoreScreen::MouseDown(int,int,int)
// @0018D7B8  2 bytes
// ==========================================================================
// The original mouse entry is empty; desktop shelf gestures are adapted from
// PvZ TV Touch Team's touch extension (GPL-3.0-or-later).
void StoreScreen::MouseDown(int theX, int theY, int theClickCount)
{
    mMousePressedButton = -1;
    if (theClickCount <= 0 || theClickCount == 3)
        return;
    if (mBubbleClickToContinue)
    {
        AdvanceCrazyDaveDialog();
        return;
    }
    if (!CanInteractWithButtons())
        return;
    // Button origins from StoreScreen::Draw @0x19017C.
    Sexy::Image *backImage = Sexy::GetImageById(std::min(mCarKeyImageIndex, 11) + 701);
    if (backImage == nullptr || IMAGE_STORE_PREVBUTTON == nullptr || IMAGE_STORE_NEXTBUTTON == nullptr)
        return;
    const Sexy::Rect back(mShakeX + 305, mShakeY + 510, backImage->mWidth, backImage->mHeight);
    const Sexy::Rect previous(
        mShakeX + 172, mShakeY + 375, IMAGE_STORE_PREVBUTTON->mWidth, IMAGE_STORE_PREVBUTTON->mHeight);
    const Sexy::Rect next(
        mShakeX + 573, mShakeY + 373, IMAGE_STORE_NEXTBUTTON->mWidth, IMAGE_STORE_NEXTBUTTON->mHeight);
    if (back.Contains(theX, theY))
    {
        mMousePressedButton = 100;
        mMousePressedRect = back;
    }
    else if (IsPageShown(STORE_PAGE_PLANT_UPGRADES) && previous.Contains(theX, theY))
    {
        mMousePressedButton = 101;
        mMousePressedRect = previous;
    }
    else if (IsPageShown(STORE_PAGE_PLANT_UPGRADES) && next.Contains(theX, theY))
    {
        mMousePressedButton = 102;
        mMousePressedRect = next;
    }
    if (mMousePressedButton != -1)
        return;
    // PvZ TV Touch Team shelf hit testing, using this build's item layout.
    for (int slot = 0; slot < 8; ++slot)
    {
        const StoreItem item = StoreItem(GetStoreItemType(slot));
        if (item == STORE_ITEM_INVALID)
            continue;
        Sexy::Image *image = nullptr;
        int itemX = 0;
        int itemY = 0;
        int count = 0;
        GetStoreItemInfo(slot, item, &image, itemX, itemY, count);
        if (IsPottedPlant(item))
            image = mPottedPlantImage[slot];
        const int width = image != nullptr ? image->GetWidth() : 80;
        const int height = image != nullptr ? image->GetHeight() : 80;
        if (!Sexy::Rect(itemX - width / 2, itemY - height, width, height).Contains(theX, theY))
            continue;
        if (mSelectedSlot == slot)
            KeyDown(Sexy::KEYCODE_RETURN);
        else
            SetSelectedSlot(slot);
        return;
    }
}

void StoreScreen::MouseUp(int x, int y, int theClickCount)
{
    const int button = mMousePressedButton;
    mMousePressedButton = -1;
    if (button != -1 && theClickCount > 0 && theClickCount != 3 && CanInteractWithButtons() &&
        mMousePressedRect.Contains(x, y))
        ButtonDepress(button);
}

// ==========================================================================
// StoreScreen::DrawOverlay(Sexy::Graphics *)
// @0018D804  90 bytes
// ==========================================================================
// Draw every live coin in the pool. The dump's outer loop plus its "restart from
// the block base" branch is the inlined DataArray<Coin>::IterateNext: it walks
// the block by the 0xCC element stride and skips a slot whose packed id has a
// zero key.
void StoreScreen::DrawOverlay(Sexy::Graphics *g)
{
    CoinSlot *aItems = mCoins.mBlock;

    for (unsigned int i = 0; i < mCoins.mMaxUsedCount; ++i)
    {
        if ((aItems[i].mID >> 16) == 0)
            continue;
        if (!aItems[i].mItem.mDead)
            aItems[i].mItem.Draw(g);
    }
}

// ==========================================================================
// StoreScreen::RemovedFromManager(Sexy::WidgetManager *)
// @0018D860  40 bytes
// ==========================================================================
void StoreScreen::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    WidgetContainer::RemovedFromManager(theWidgetManager);
    // The dump spells this as a call through `*this + 0x1C`, which is virtual slot
    // 7 of WidgetContainer -- RemoveWidget -- dispatched on the object itself.
    RemoveWidget(mOverlayWidget);
    mApp->CrazyDaveDie();
    mApp->HideHelpBarWidget();
}

// ==========================================================================
// StoreScreen::ButtonPress(int)
// @0018D888  34 bytes
// ==========================================================================
void StoreScreen::ButtonPress(int theId)
{
    // The two page-turn buttons stay silent; every other button clicks.
    if ((unsigned int)(theId - 101) > 1)
        mApp->PlaySample(SOUND_BUTTONCLICK, true);
}

// ==========================================================================
// StoreScreen::GotFocus(void)
// @0018D8BC  372 bytes
// ==========================================================================
void StoreScreen::GotFocus()
{
    // The help bar is cached in mHelpBarWidget so that Update can bring it to the front
    // without a second lookup through mApp.
    Widget::GotFocus();
    mHelpBarWidget = mApp->mHelpBarWidget;
    mHelpBarWidget->ClearButtons(0);
    mHelpBarWidget->mDrawShade = false;
    // Every label below is a temporary Sexy::String the dump builds with
    // StringCtor and then releases by hand; the destructor does that now.
    mHelpBarWidget->AddButton((Sexy::GamepadButton)14, Sexy::String("[USE_DPAD]"), HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBarWidget->AddButton((Sexy::GamepadButton)20, Sexy::String(""), HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBarWidget->AddButton((Sexy::GamepadButton)7, Sexy::String("[BACK]"), HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBarWidget->AddButton((Sexy::GamepadButton)6, Sexy::String("[BUY_ITEM]"), HelpBarWidget::HELP_ALIGN_NONE);
    mApp->ShowHelpBarWidget();
}

// ==========================================================================
// StoreScreen::GetStoreItemInfo(int,int,Sexy::Image **,int &,int &,int &)
// @0018DA5C  488 bytes
// ==========================================================================
// Where on the shelf a slot's item is drawn, and which image stands for it.
//
// The shelf is two banks of fifteen: slots 0..3 and 4..14 are laid out at
// different origins and different row pitches, which is what the two branches
// at the top compute. The final output is the quantity in one purchase.
void StoreScreen::GetStoreItemInfo(
    int theSlot, int theItem, Sexy::Image **theImage, int &theDrawX, int &theDrawY, int &thePurchaseCount)
{
    bool aFirstBank = theSlot <= 3;
    if (!aFirstBank)
        theSlot -= 4;

    int aSlotOffset = 5 * theSlot;
    if (aFirstBank)
        aSlotOffset *= 15;
    thePurchaseCount = 0;
    if (!aFirstBank)
        aSlotOffset *= 15;

    int aOffsetX = 0;
    if (aFirstBank)
    {
        theDrawY = 244;
        theDrawX = aSlotOffset + 435;
    }
    else
    {
        theDrawY = 366;
        theDrawX = aSlotOffset + 355;
    }

    Sexy::Image *anImage = nullptr;
    switch (theItem)
    {
        case 0:
            anImage = IMAGE_STORESEEDPACK_REPEATER;
            break;
        case 1:
            anImage = IMAGE_STORESEEDPACK_DOUBLESUNFLOWER;
            break;
        case 2:
            anImage = IMAGE_STORESEEDPACK_GLOOMSHROOM;
            break;
        case 3:
            anImage = IMAGE_STORESEEDPACK_CATTAIL;
            break;
        case 4:
            anImage = IMAGE_STORESEEDPACK_FROZENWATERMELON;
            break;
        case 5:
            anImage = IMAGE_STORESEEDPACK_GOLDMAGNET;
            break;
        case 6:
            anImage = IMAGE_STORESEEDPACK_SPIKEROCK;
            break;
        case 7:
            anImage = IMAGE_STORESEEDPACK_COBCANNON;
            break;
        case 8:
            anImage = IMAGE_STORESEEDPACK_IMITATER;
            break;
        case 13:
            anImage = IMAGE_WATERINGCANGOLD;
            break;

        case 14:
            anImage = IMAGE_FERTILIZER;
            thePurchaseCount = 5;
            break;
        case 15:
            anImage = IMAGE_BUG_SPRAY;
            thePurchaseCount = 5;
            break;

        case 16:
            anImage = IMAGE_PHONOGRAPH;
            break;
        case 17:
            anImage = IMAGE_ZEN_GARDENGLOVE;
            break;
        case 18:
            anImage = IMAGE_STORE_MUSHROOMGARDENICON;
            break;
        case 19:
            anImage = IMAGE_ZEN_WHEELBARROW;
            aOffsetX = 10;
            break;
        case 20:
            anImage = IMAGE_REANIM_STINKY_TURN3;
            aOffsetX = 15;
            break;
        case 21:
            anImage = IMAGE_STORE_PACKETUPGRADE;
            break;
        case 22:
            anImage = IMAGE_ICON_POOLCLEANER;
            break;
        case 23:
            anImage = IMAGE_ICON_ROOFCLEANER;
            break;
        case 24:
            anImage = IMAGE_ICON_RAKE;
            break;
        case 25:
            anImage = IMAGE_STORE_AQUARIUMGARDENICON;
            break;
        case 27:
            anImage = IMAGE_STORE_TREEOFWISDOMICON;
            break;
        case 28:
            anImage = IMAGE_TREEFOOD;
            aOffsetX = 15;
            break;
        case 29:
            anImage = IMAGE_STORE_FIRSTAIDWALLNUTICON;
            aOffsetX = 15;
            break;
        case 30:
            anImage = IMAGE_STORE_PVZICON;
            break;
        case 31:
            anImage = IMAGE_STORE_BLUEPRINT_BLING;
            break;
        case 32:
            anImage = IMAGE_STORE_BLUEPRINT_SCARY;
            break;
        case 33:
            anImage = IMAGE_STORE_BLUEPRINT_TRAILER;
            break;
        case 34:
            anImage = IMAGE_STORE_BLUEPRINT_FUTURE;
            break;
        case 35:
            anImage = IMAGE_STORE_BLUEPRINT_CLOWN;
            break;
        case 36:
            anImage = IMAGE_STORE_BLUEPRINT_CHANGE;
            break;
        default:
            break;
    }

    theDrawX += aOffsetX;
    if (theImage != nullptr)
        *theImage = anImage;
}
// ==========================================================================
// StoreScreen::GetStoreItemType(int)
// @0018DCC8  296 bytes
// ==========================================================================
int StoreScreen::GetStoreItemType(int theSlot)
{
    // Eight shelf slots per page. Slot 6 of the first page is the full-version
    // icon while the trial is locked, and gloom-shroom otherwise.
    static const int kShelf[6][8] = {
        {21, 22, 24, 23, 0, 1, 2, 3},
        {6, 5, 4, 7, 8, 29, -1, -1},
        {10, 11, 12, 13, 14, 15, 16, 17},
        {18, 25, 19, 20, 27, 28, -1, -1},
        {-1, -1, -1, -1, -1, -1, -1, -1},
        {31, 32, 33, 34, 35, 36, -1, -1},
    };

    if (theSlot < 0 || theSlot >= 8 || (unsigned)mPage > STORE_PAGE_HOUSE)
        return -1;
    if (mPage == STORE_PAGE_SLOT_UPGRADES && theSlot == 6 && mApp->IsTrialStageLocked() != 0)
        return 30;
    return kShelf[mPage][theSlot];
}

// ==========================================================================
// StoreScreen::IsPottedPlant(StoreItem)
// @0018DDF0  12 bytes
// ==========================================================================
bool StoreScreen::IsPottedPlant(StoreItem theItem)
{
    return (unsigned int)(theItem - 10) <= 2;
}

// ==========================================================================
// StoreScreen::IsFullVersionOnly(StoreItem)
// @0018DDFC  48 bytes
// ==========================================================================
int StoreScreen::IsFullVersionOnly(StoreItem theItem)
{
    int result = mApp->IsTrialStageLocked();

    if (result != 0)
    {
        if (theItem == 21)
        {
            if (mApp->mPlayerInfo->mPurchases[21] <= 1)
                return 0;
        }
        else if (theItem != 1)
        {
            return 0;
        }
    }
    return result;
}

// ==========================================================================
// StoreScreen::IsComingSoon(StoreItem)
// @0018DE2C  102 bytes
// ==========================================================================
bool StoreScreen::IsComingSoon(StoreItem theItem)
{
    LawnPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;
    int aFullVersionOnly = IsFullVersionOnly(theItem);

    if (aFullVersionOnly != 0)
        return true;
    if (theItem == 19)
    {
        if (aPlayerInfo->mPurchases[18] == 0 && aPlayerInfo->mPurchases[25] == 0)
            return true;
    }
    else
    {
        if (IsPottedPlant(theItem) && mApp->HasFinishedAdventure() == 0)
            return true;
        if (theItem == 28)
        {
            if (aPlayerInfo->mPurchases[27] == 0)
                return true;
            return aPlayerInfo->mPurchases[28] < 1000;
        }
    }
    return aFullVersionOnly;
}

// ==========================================================================
// StoreScreen::IsItemSoldOut(StoreItem)
// @0018DE94  268 bytes
// ==========================================================================
bool StoreScreen::IsItemSoldOut(StoreItem theItem)
{
    LawnPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;

    if (theItem == STORE_ITEM_INVALID)
        return false;
    if (theItem == STORE_ITEM_PACKET_UPGRADE)
        return aPlayerInfo->mPurchases[STORE_ITEM_PACKET_UPGRADE] >= 4;
    if (theItem == STORE_ITEM_FERTILIZER || theItem == STORE_ITEM_BUG_SPRAY)
        return aPlayerInfo->mPurchases[theItem] >= 1016;
    if (theItem == STORE_ITEM_TREE_FOOD)
        return aPlayerInfo->mPurchases[STORE_ITEM_TREE_FOOD] > 1009;
    if (theItem == STORE_ITEM_BONUS_LAWN_MOWER)
        return aPlayerInfo->mPurchases[STORE_ITEM_BONUS_LAWN_MOWER] > 1;
    if (IsPottedPlant(theItem))
    {
        int aCurrentDay = GetCurrentDaysSince2000();
        return mApp->mZenGarden->IsZenGardenFull(true) || aCurrentDay == aPlayerInfo->mPurchases[theItem];
    }
    if (theItem >= STORE_ITEM_BLUEPRINT_BLING && theItem <= STORE_ITEM_BLUEPRINT_CLOWN)
    {
        if (theItem == STORE_ITEM_BLUEPRINT_BLING)
            return aPlayerInfo->mPurchases[STORE_ITEM_BLUEPRINT_BLING] > 1;
        return aPlayerInfo->mPurchases[theItem] > 4;
    }
    if (theItem == STORE_ITEM_BLUEPRINT_CHANGE)
        return false;
    return aPlayerInfo->mPurchases[theItem] != 0;
}

// ==========================================================================
// StoreScreen::IsItemUnavailable(StoreItem)
// @0018DFA0  228 bytes
// ==========================================================================
int StoreScreen::IsItemUnavailable(StoreItem theItem)
{
    // A locked item stays off the shelf until the adventure or the matching
    // level is done. Roof cleaner and the two night plants also stay locked
    // during the trial. Winter melon, cob cannon, imitater and first aid wait
    // for the adventure to finish.
    if (mEasyBuyingCheat)
        return 0;

    LawnPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;
    bool aAdventureDone = mApp->HasFinishedAdventure() != 0;
    bool aTrialLocked = mApp->IsTrialStageLocked() != 0;

    if (theItem == 23)
        return aTrialLocked || (!aAdventureDone && aPlayerInfo->mLevel <= 41);
    if (theItem == 2 || theItem == 3)
        return aTrialLocked || (!aAdventureDone && aPlayerInfo->mLevel <= 34);
    if (theItem == 6 || theItem == 5)
        return !aAdventureDone && aPlayerInfo->mLevel <= 40;
    if (theItem == 4 || theItem == 7 || theItem == 8 || theItem == 29)
        return !aAdventureDone;
    return 0;
}

// ==========================================================================
// StoreScreen::NextSlotInDir(StoreScreen::ItemDir)
// @0018E084  66 bytes
// ==========================================================================
// The slot four columns away in the page's grid; a slot that holds nothing, or
// holds an item this profile cannot buy, leaves the selection where it was.
unsigned int StoreScreen::NextSlotInDir(StoreScreen_ItemDir theDir)
{
    int aSlot = mSelectedSlot;

    switch (theDir)
    {
        case 0:
            aSlot -= 4;
            break;
        case 1:
            aSlot += 4;
            break;
        case 2:
            --aSlot;
            break;
        case 3:
            ++aSlot;
            break;
        default:
            break;
    }

    int aStoreItemType = GetStoreItemType(aSlot);
    if (aStoreItemType == -1 || IsItemUnavailable(StoreItem(aStoreItemType)) != 0)
        return mSelectedSlot;
    return aSlot;
}
// ==========================================================================
// StoreScreen::DrawItemHighlight(Sexy::Graphics *,Sexy::Image *,int,int)
// @0018E0C8  2 bytes
// ==========================================================================
// Two bytes and no instruction: the store's highlight is drawn by DrawItem, so
// this hook does nothing.
void StoreScreen::DrawItemHighlight() {}

// ==========================================================================
// StoreScreen::SetBubbleText(int,int,bool)
// @0018E0CC  26 bytes
// ==========================================================================
int StoreScreen::SetBubbleText(int theCrazyDaveMessage, int theTime, bool theClickToContinue)
{
    // The talk index goes through the app. Its return type is void in this tree,
    // so the word the binary leaves in r0 is not a result; no caller of
    // SetBubbleText reads one either.
    mApp->CrazyDaveTalkIndex(theCrazyDaveMessage);
    mBubbleCountDown = theTime;
    mBubbleClickToContinue = theClickToContinue;
    return 0;
}
// ==========================================================================
// StoreScreen::CanInteractWithButtons(void)
// @0018E0E8  38 bytes
// ==========================================================================
// The store ignores input for its first two seconds on screen, while a bubble
// is waiting for a click, and while the hatch is animating.
int StoreScreen::CanInteractWithButtons()
{
    if (mStoreTime <= 119)
        return 0;
    if (mBubbleClickToContinue != 0)
        return 0;
    if (mHatchTimer > 0)
        return 0;
    return mWaitForDialog ^ 1;
}
// ==========================================================================
// StoreScreen::UpdateSelection(void)
// @0018E110  496 bytes
// ==========================================================================
// Crazy Dave's line about whatever slot is selected. Each store item has its
// own message id; the three upgrades that come in levels derive theirs from
// how many levels the profile has already bought.
//
// The bubble is only reset when the line actually changed, which is why the
// countdown is set to 100 rather than restarted unconditionally.
int StoreScreen::UpdateSelection()
{
    int result = CanInteractWithButtons();
    if (result == 0)
        return result;

    StoreItem aStoreItem = mSelectedStoreItemType;
    if (aStoreItem == -1)
        return 0;

    if (IsItemUnavailable(aStoreItem) != 0)
        return result;

    int aMessageId;
    switch (aStoreItem)
    {
        case 0:
            aMessageId = 2000;
            break;
        case 1:
            aMessageId = 2001;
            break;
        case 2:
            aMessageId = 2002;
            break;
        case 3:
            aMessageId = 2003;
            break;
        case 4:
            aMessageId = 2004;
            break;
        case 5:
            aMessageId = 2005;
            break;
        case 6:
            aMessageId = 2006;
            break;
        case 7:
            aMessageId = 2007;
            break;
        case 8:
            aMessageId = 2008;
            break;
        case 9:
            aMessageId = 2009;
            break;
        case 10:
        case 11:
        case 12:
            aMessageId = 2010;
            break;
        case 13:
            aMessageId = 2019;
            break;
        case 14:
            aMessageId = 2020;
            break;
        case 15:
            aMessageId = 2022;
            break;
        case 16:
            aMessageId = 2021;
            break;
        case 17:
            aMessageId = 2023;
            break;
        case 18:
            aMessageId = 2032;
            break;
        case 19:
            aMessageId = 2024;
            break;
        case 20:
            aMessageId = 2025;
            break;
        case 21:
            // The seed-slot upgrade: one message per level already bought, and the
            // fourth and later levels all share the last one.
            aMessageId = 2011;
            switch (mApp->mPlayerInfo->mPurchases[21])
            {
                case 0:
                    break;
                case 1:
                    aMessageId = 2012;
                    break;
                case 2:
                    aMessageId = 2013;
                    break;
                default:
                    aMessageId = 2014;
                    break;
            }
            break;
        case 22:
            aMessageId = 2026;
            break;
        case 23:
            aMessageId = 2027;
            break;
        case 24:
            aMessageId = 2028;
            break;
        case 25:
            aMessageId = 2029;
            break;
        case 27:
            aMessageId = 2030;
            break;
        case 28:
            aMessageId = 2031;
            break;
        case 29:
            aMessageId = 2033;
            break;
        case 30:
            aMessageId = 2034;
            break;
        case 31:
            aMessageId = 2036;
            break;
        case 32:
            aMessageId = 2037;
            break;
        case 33:
            aMessageId = 2039;
            break;
        case 34:
            aMessageId = 2038;
            break;
        case 35:
            aMessageId = 2040;
            break;
        case 36:
            aMessageId = 2035;
            break;
        default:
            aMessageId = -1;
            break;
    }

    // Already showing this line: hold it on screen instead of restarting it.
    if (mApp->mCrazyDaveMessageIndex == aMessageId)
    {
        mBubbleCountDown = 100;
        return result;
    }

    return SetBubbleText(aMessageId, 500, false);
}
// ==========================================================================
// StoreScreen::StorePreLoad(void)
// @0018E300  104 bytes
// ==========================================================================
// The three plant reanimations the store can show, plus the seven the Zen
// garden sells once the adventure is finished.
int StoreScreen::StorePreLoad()
{
    ReanimatorEnsureDefinitionLoaded(REANIM_CRAZY_DAVE, true);
    ReanimatorEnsureDefinitionLoaded(REANIM_ZENGARDEN_FERTILIZER, true);
    mApp->CrazyDaveEnter();

    Plant::PreloadPlantResources((SeedType)0x28);
    Plant::PreloadPlantResources((SeedType)0x29);

    int result = mApp->HasFinishedAdventure();
    if (result != 0)
    {
        Plant::PreloadPlantResources((SeedType)0x29);
        Plant::PreloadPlantResources((SeedType)0x2A);
        Plant::PreloadPlantResources((SeedType)0x2B);
        Plant::PreloadPlantResources((SeedType)0x2C);
        Plant::PreloadPlantResources((SeedType)0x2D);
        Plant::PreloadPlantResources((SeedType)0x2E);
        Plant::PreloadPlantResources((SeedType)0x2F);
        return Plant::PreloadPlantResources((SeedType)0x30);
    }
    return result;
}
// ==========================================================================
// StoreScreen::IsPageShown(StorePage)
// @0018E368  88 bytes
// ==========================================================================
// Native unlock levels @0x18E368, with the reference port's house page enabled
// after adventure. NUM_STORE_PAGES is the original sentinel, not a shelf page.
bool StoreScreen::IsPageShown(StorePages thePage)
{
    if (thePage == NUM_STORE_PAGES || thePage < STORE_PAGE_SLOT_UPGRADES || thePage > STORE_PAGE_HOUSE)
        return false;
    if (mApp->IsTrialStageLocked() != 0)
        return thePage == STORE_PAGE_SLOT_UPGRADES;
    const bool finishedAdventure = mApp->HasFinishedAdventure();
    switch (thePage)
    {
        case STORE_PAGE_PLANT_UPGRADES:
            return finishedAdventure || mApp->mPlayerInfo->mLevel >= 42;
        case STORE_PAGE_ZEN1:
            return finishedAdventure || mApp->mPlayerInfo->mLevel >= 45;
        case STORE_PAGE_ZEN2:
        case STORE_PAGE_HOUSE:
            return finishedAdventure;
        default:
            return true;
    }
}
// ==========================================================================
// StoreScreen::GetItemCost(StoreItem)
// @0018E3C0  272 bytes
// ==========================================================================
int StoreScreen::GetItemCost(StoreItem theItem)
{
    LawnPlayerInfo *aPlayer = ((LawnApp *)gLawnApp)->mPlayerInfo;

    // The second lawn mower costs more than the first. Each packet-upgrade
    // step costs more than the last. Each blueprint costs more the more times
    // that house style has already been bought.
    if (theItem == 9)
        return aPlayer->mPurchases[9] != 0 ? 500 : 200;

    switch (theItem)
    {
        case 0:
        case 1:
            return 500;
        case 2:
        case 6:
            return 750;
        case 3:
        case 4:
        case 13:
        case 27:
            return 1000;
        case 5:
        case 20:
        case 23:
            return 300;
        case 7:
            return 2000;
        case 8:
        case 18:
        case 25:
            return 3000;
        case 10:
        case 11:
        case 12:
        case 28:
            return 250;
        case 14:
            return 75;
        case 15:
        case 17:
        case 22:
            return 100;
        case 16:
            return 1500;
        case 19:
        case 24:
            return 20;
        case 21:
        {
            int aBought = aPlayer->mPurchases[21];
            if (aBought == 0)
                return 75;
            if (aBought == 1)
                return 500;
            if (aBought == 2)
                return 2000;
            return 8000;
        }
        case 29:
            return 200;
        case 31:
            return 800 * aPlayer->mPurchases[31] + 2400;
        case 32:
            return 800 * aPlayer->mPurchases[32] + 800;
        case 33:
            return 800 * aPlayer->mPurchases[33] + 800;
        case 34:
            return 800 * aPlayer->mPurchases[34] + 800;
        case 35:
            return 800 * aPlayer->mPurchases[35] + 800;
        case 36:
            return 30;
        default:
            return theItem;
    }
}

// ==========================================================================
// StoreScreen::DrawItem(Sexy::Graphics *,int,StoreItem)
// @0018E4D8  3060 bytes
// ==========================================================================
// One shelf slot: the item's image, the pulse the selected slot gets, the
// price tag, and the coming-soon / sold-out overlay on top.
//
// a3 is the slot index, which also picks the row the price tag sits on; a4 is
// the item itself.
int StoreScreen::DrawItem(Sexy::Graphics *g, int theSlot, StoreItem theItem)
{
    if (IsItemUnavailable(theItem) != 0)
        return 0;

    bool aSelected = (mSelectedStoreItemType == theItem);
    Sexy::Image *anItemImage = nullptr;
    int aDrawX = 0;
    int aDrawY = 0;
    int aPurchaseCount = 0;
    GetStoreItemInfo(theSlot, theItem, &anItemImage, aDrawX, aDrawY, aPurchaseCount);

    if (IsPottedPlant(theItem))
        anItemImage = mPottedPlantImage[theSlot];

    if (aSelected)
    {
        // The selected item breathes: the pulse is a sine of the highlight
        // timer clamped into 0.3..0.5, and the image is scaled about its own
        // centre.
        float aPhase = 0.5f;
        if (mHighlightPhase <= 0.5f)
            aPhase = mHighlightPhase;
        if (aPhase < 0.3f)
            aPhase = 0.3f;

        float aScale = sin(aPhase * 3.14159265f) + 0.1f;
        int aHalfWidth;
        int aHalfHeight;
        if (anItemImage != nullptr)
        {
            aHalfWidth = anItemImage->GetWidth() / 2;
            aHalfHeight = anItemImage->GetHeight() / 2;
        }
        else
        {
            aHalfWidth = 20;
            aHalfHeight = 15;
            aDrawX = (int)((float)aDrawX - aScale * 20.0f);
            aDrawY = (int)((float)aDrawY - aScale * 15.0f);
        }

        g->SetScale(aScale, aScale, (float)(aDrawX + aHalfWidth), (float)(aDrawY + aHalfHeight));
    }

    if (anItemImage == nullptr)
        return 0;

    g->DrawImage(anItemImage, aDrawX - anItemImage->GetWidth() / 2, aDrawY - anItemImage->GetHeight());

    if (aSelected)
    {
        // A second, additive pass over the same image gives the selection its
        // glow; the tint's alpha follows the same sine.
        g->SetDrawMode(0);
        float aPulse = (float)(mUpdateCnt % 100) * 0.01f;
        g->SetDrawMode(1);
        double aGlow = sin(aPulse * 3.14159265f);
        Sexy::Color aGlowColor(255, 255, 100, (int)(aGlow * 150.0));
        g->SetColor(aGlowColor);
        g->SetColorizeImages(true);
        g->DrawImage(anItemImage, aDrawX - anItemImage->GetWidth() / 2, aDrawY - anItemImage->GetHeight());
        g->SetColor(Sexy::Color::White);
        g->SetDrawMode(0);
    }

    int anImageWidth = anItemImage->GetWidth();
    int anImageHeight = anItemImage->GetHeight();
    int aLabelX = aDrawX - anImageWidth / 2;
    int aLabelY = aDrawY - anImageHeight;

    if (theItem == 21)
    {
        // The seed-slot upgrade prints how many slots the purchase is worth.
        Sexy::String aText("[STORE_UPGRADE_SLOTS]");
        Sexy::String aSlotsText;
        TodReplaceNumberString(&aSlotsText, &aText, "{SLOTS}", mApp->mPlayerInfo->mPurchases[21] + 7);

        aLabelY = aDrawY - anImageHeight;
        if (mApp->GetBoolean(Sexy::String("STORE_USE_EXTRA_SLOT_IMAGE_LABEL")) != 0)
        {
            g->DrawImage(
                IMAGE_STORE_EXTRA_SLOT, aLabelX + anImageWidth / 2 - IMAGE_STORE_EXTRA_SLOT->mWidth / 2, aLabelY);
        }
        else
        {
            constexpr int DS_ALIGN_CENTER_MIDDLE = 5;
            Sexy::TRect<int> aSlotsRect(aLabelX, aLabelY, anImageWidth, anImageHeight);
            TodDrawStringWrapped(
                g, aSlotsText, aSlotsRect, FONT_HOUSEOFTERROR16, Sexy::Color::White, DS_ALIGN_CENTER_MIDDLE, false);
        }
    }
    else if ((unsigned int)(theItem - 31) <= 4)
    {
        // The five blueprint items print the level they unlock, one past the
        // level already bought.
        Sexy::String aLevelText;
        if (theItem == 31)
        {
            if (mApp->mPlayerInfo->mPurchases[31] > 1)
            {
                aLevelText = "";
            }
            else
            {
                Sexy::String aFormat("[STORE_BLUEPRINT_LEVEL]");
                TodReplaceNumberString(&aLevelText, &aFormat, "{LEVEL}", mApp->mPlayerInfo->mPurchases[31] + 4);
            }
        }
        else if (mApp->mPlayerInfo->mPurchases[theItem] > 4)
        {
            aLevelText = "";
        }
        else
        {
            Sexy::String aFormat("[STORE_BLUEPRINT_LEVEL]");
            TodReplaceNumberString(&aLevelText, &aFormat, "{LEVEL}", mApp->mPlayerInfo->mPurchases[theItem] + 1);
        }

        if (aLevelText.size() != 0)
        {
            Sexy::TRect<int> aLevelRect(
                aDrawX - anImageWidth / 2, aDrawY - anImageHeight - 30, anImageWidth, anImageHeight);
            TodDrawStringWrapped(g, aLevelText, aLevelRect, FONT_HOUSEOFTERROR16, Sexy::Color::White, 2, 0);
        }
    }
    else if (theItem != 36)
    {
        // Everything else that has been bought at least once prints its count.
        if (aPurchaseCount != 0)
        {
            Sexy::String aCountFormat("[STORE_ITEM_PURCHASE_COUNT]");
            Sexy::String aCountText;
            TodReplaceNumberString(&aCountText, &aCountFormat, "{COUNT}", aPurchaseCount);
            Sexy::TRect<int> aCountRect(aDrawX - anImageWidth / 2, aDrawY - anImageHeight, anImageWidth, anImageHeight);
            TodDrawStringWrapped(g, aCountText, aCountRect, FONT_HOUSEOFTERROR16, Sexy::Color::White, 5, 0);
        }
    }
    else
    {
        // The house item prints its own name.
        Sexy::String aHouseText;
        TodStringTranslate(&aHouseText, "[STORE_HOUSE]");
        Sexy::TRect<int> aHouseRect(aDrawX - anImageWidth / 2, aDrawY - anImageHeight, anImageWidth, anImageHeight);
        TodDrawStringWrapped(g, aHouseText, aHouseRect, FONT_HOUSEOFTERROR16, Sexy::Color::White, 5, 0);
    }

    g->SetColorizeImages(false);
    g->SetDrawMode(0);

    if (theItem != 30)
    {
        // The price tag sits on the slot's row, and the amount is drawn onto
        // it in black.
        int aRow = (theSlot < 0) ? (theSlot + 3) : theSlot;
        int aTagY = gStoreRowY[aRow >> 2];
        int aTagX = aDrawX - IMAGE_STORE_PRICETAG->mWidth / 2;
        g->DrawImage(IMAGE_STORE_PRICETAG, aTagX - 15, aTagY);

        Sexy::String aMoney = mApp->GetMoneyString(GetItemCost(theItem));
        Sexy::Color aBlack(0, 0, 0);
        TodDrawString(g, aMoney, aTagX + 11, aTagY + 14, FONT_BRIANNETOD12, aBlack, 2);
    }

    if (IsComingSoon(theItem))
    {
        // The coming-soon banner: a centred image, or the label when the
        // platform config asks for text.
        int aBadgeWidth;
        int aBadgeHeight;
        int aBadgeX;
        int aBadgeY = aDrawY;
        if (anItemImage != nullptr)
        {
            aBadgeWidth = (int)((float)anItemImage->GetWidth() * 0.8f);
            aBadgeHeight = anItemImage->GetHeight();
            aBadgeX = aDrawX - aBadgeWidth / 2;
        }
        else
        {
            aBadgeWidth = 70;
            aBadgeHeight = 60;
            aBadgeX = aDrawX;
        }
        if (theItem == 1 || theItem == 21)
            aBadgeX -= 4;

        if (mApp->GetBoolean(Sexy::String("STORE_USE_COMING_SOON_IMAGE_LABEL")) != 0)
        {
            g->DrawImage(IMAGE_STORE_COMING_SOON,
                aBadgeX + aBadgeWidth / 2 - IMAGE_STORE_COMING_SOON->mWidth / 2,
                aBadgeY - aBadgeHeight);
        }
        else
        {
            Sexy::String aComingSoon("[COMING_SOON]");
            Sexy::Color aRed(255, 0, 0);
            Sexy::TRect<int> aBadgeRect(aBadgeX, aBadgeY - aBadgeHeight, aBadgeWidth, aBadgeHeight);
            TodDrawStringWrapped(g, aComingSoon, aBadgeRect, FONT_HOUSEOFTERROR16, aRed, 5, 0);
        }
    }
    else if (IsItemSoldOut(theItem) != 0)
    {
        // The same shape for the sold-out banner, but the badge is at least
        // fifty wide.
        int aBadgeWidth;
        int aBadgeHeight;
        int aBadgeX;
        int aBadgeY = aDrawY;
        if (anItemImage != nullptr)
        {
            aBadgeHeight = anItemImage->GetHeight();
            aBadgeWidth = (anItemImage->GetWidth() < 50) ? 50 : anItemImage->GetWidth();
            aBadgeX = aDrawX - aBadgeWidth / 2;
        }
        else
        {
            aBadgeWidth = 50;
            aBadgeHeight = 60;
            aBadgeX = aDrawX - 25;
        }

        if (mApp->GetBoolean(Sexy::String("STORE_USE_SOLD_OUT_IMAGE_LABEL")) != 0)
        {
            g->DrawImage(IMAGE_STORE_SOLD_OUT,
                aBadgeX + aBadgeWidth / 2 - IMAGE_STORE_SOLD_OUT->mWidth / 2,
                aBadgeY - aBadgeHeight);
        }
        else
        {
            Sexy::String aSoldOut("[SOLD_OUT]");
            Sexy::Color aRed(255, 0, 0);
            Sexy::TRect<int> aBadgeRect(aBadgeX, aBadgeY - aBadgeHeight, aBadgeWidth, aBadgeHeight);
            TodDrawStringWrapped(g, aSoldOut, aBadgeRect, FONT_HOUSEOFTERROR16, aRed, 5, 0);
        }
    }

    g->SetScale(1.0, 1.0, 0.0, 0.0);
    return 0;
}
// ==========================================================================
// StoreScreen::CanAffordItem(StoreItem)
// @0018F108  30 bytes
// ==========================================================================
bool StoreScreen::CanAffordItem(StoreItem theItem)
{
    return GetItemCost(theItem) <= mApp->mPlayerInfo->mCoins;
}
// ==========================================================================
// StoreScreen::PurchaseItem(StoreItem)
// @0018F128  1432 bytes
// ==========================================================================
// Buy the selected item. The confirmation dialog is built by the app and its
// two buttons are relabelled first; the purchase only happens when it comes
// back with 1000.
//
// What the purchase then does to the profile depends on the item: the seed-slot
// upgrade opens a second dialog, the rake/mower upgrades raise the Zen garden
// counters, the four blueprints record which level was unlocked, the house
// hands over to HouseChooserDialog, and the mower item walks the board's lawn
// mowers and respawns each one as a super mower.
void StoreScreen::PurchaseItem(StoreItem theItem)
{
    mApp->SetCursor(0);
    mBubbleCountDown = 0;
    mApp->CrazyDaveStopTalking();

    if (!CanAffordItem(theItem))
    {
        // The app builds the "not enough money" dialog and the store waits on
        // it; the wait flag keeps Update from touching the screen meanwhile.
        mWaitForDialog = true;
        mApp->DoNotEnoughMoneyDialog()->WaitForResult(true);
        mWaitForDialog = false;
        return;
    }

    LawnDialog *aDialog = mApp->DoStoreItemDialog();
    aDialog->mLawnYesButton->SetLabel(Sexy::String("[DIALOG_BUTTON_YES]"));
    aDialog->mLawnNoButton->SetLabel(Sexy::String("[DIALOG_BUTTON_NO]"));

    mWaitForDialog = true;
    int aResult = aDialog->WaitForResult(true);
    mWaitForDialog = false;

    if (aResult != 1000)
        return;

    int anItemCost = GetItemCost(theItem);
    if (theItem != STORE_ITEM_BLUEPRINT_CHANGE)
        mApp->mPlayerInfo->AddCoins(-anItemCost);
    // AddCoins records the accumulated spending and checks the 25000-coin achievement.

    // TV's special purchases jump directly to the shared save/refresh path.
    // Keep them in this chain so a generic one-time purchase cannot overwrite them.
    if (theItem == STORE_ITEM_PACKET_UPGRADE)
    {
        // The seed-slot upgrade announces itself and then widens the seed bank.
        ++mApp->mPlayerInfo->mPurchases[STORE_ITEM_PACKET_UPGRADE];
        mWaitForDialog = true;
        mApp->DoUpgradeDialog()->WaitForResult(true);
        mWaitForDialog = false;
        if (mApp->mBoard != nullptr && mApp->mBoard->mSeedBank[0] != nullptr)
            mApp->mBoard->mSeedBank[0]->UpdateWidth();
    }
    else if (theItem == STORE_ITEM_BONUS_LAWN_MOWER)
    {
        ++mApp->mPlayerInfo->mPurchases[STORE_ITEM_BONUS_LAWN_MOWER];
    }
    else if (theItem == STORE_ITEM_RAKE)
    {
        mApp->mPlayerInfo->mPurchases[STORE_ITEM_RAKE] = 3;
    }
    else if (theItem == STORE_ITEM_STINKY_THE_SNAIL)
    {
        mApp->mPlayerInfo->mPurchases[STORE_ITEM_STINKY_THE_SNAIL] = (int)time(nullptr);
    }
    else if (theItem == STORE_ITEM_FERTILIZER || theItem == STORE_ITEM_BUG_SPRAY)
    {
        // The fertiliser and the bug spray: five more uses, from a floor of
        // 1000 so the count can never be mistaken for "not bought".
        int &aCount = mApp->mPlayerInfo->mPurchases[theItem];
        aCount = (aCount < 1000) ? 1005 : (aCount + 5);
    }
    else if (theItem == STORE_ITEM_TREE_FOOD)
    {
        int &aCount = mApp->mPlayerInfo->mPurchases[STORE_ITEM_TREE_FOOD];
        aCount = (aCount < 1000) ? 1001 : (aCount + 1);
    }
    else if (theItem == STORE_ITEM_TREE_OF_WISDOM)
    {
        // The tree of wisdom: ask for its own dialog, then remember the choice.
        mApp->mPlayerInfo->mPurchases[STORE_ITEM_TREE_OF_WISDOM] = 1;
        mApp->mPlayerInfo->mChallengeRecords[GAMEMODE_TREE_OF_WISDOM - GAMEMODE_SURVIVAL_NORMAL_STAGE_1] = 1;

        LawnDialog *aTreeDialog = mApp->DoTreeItemDialog();
        aTreeDialog->mLawnYesButton->SetLabel(Sexy::String("[DIALOG_BUTTON_YES]"));
        aTreeDialog->mLawnNoButton->SetLabel(Sexy::String("[DIALOG_BUTTON_NO]"));

        mWaitForDialog = true;
        int aTreeResult = aTreeDialog->WaitForResult(true);
        mWaitForDialog = false;

        if (aTreeResult == 1000)
        {
            mGoToTreeNow = true;
            mResult = 1000;
        }
    }
    else if (IsPottedPlant(theItem))
    {
        // A potted plant goes straight into the Zen garden, with a random
        // variant, and the purchase date is stamped on the profile.
        mApp->mZenGarden->AddPottedPlant(&mPottedPlantSpecs);
        mPottedPlantSpecs.InitializePottedPlant(SEED_MARIGOLD);
        mPottedPlantSpecs.mDrawVariation = RandRangeInt(VARIATION_MARIGOLD_WHITE, VARIATION_MARIGOLD_LIGHT_GREEN);
        mApp->mPlayerInfo->mPurchases[theItem] = GetCurrentDaysSince2000();
    }
    else if (theItem == STORE_ITEM_POOL_CLEANER)
    {
        // The pool cleaner: every pool-row mower on the board is replaced.
        ++mApp->mPlayerInfo->mPurchases[STORE_ITEM_POOL_CLEANER];

        LawnMower *aMower = nullptr;
        while (mApp->mBoard != nullptr && mApp->mBoard->IterateLawnMowers(aMower))
        {
            if (aMower == nullptr)
                continue;

            if (mApp->mBoard->mPlantRow[aMower->mRow] == PLANTROW_POOL)
            {
                Reanimation *aReanim = mApp->AddReanimation(0.0f, 18.0f, aMower->mRenderOrder, REANIM_POOL_CLEANER);
                if (aReanim == nullptr)
                    continue;

                aMower->mMowerType = LAWNMOWER_POOL;
                Reanimation *anOldReanim = mApp->ReanimationTryToGet(aMower->mReanimID);
                anOldReanim->ReanimationDie();

                aReanim->mLoopType = REANIM_LOOP;
                aReanim->SetAnimRate(0.0f);
                aReanim->mIsAttachment = true;
                aReanim->OverrideScale(0.85f, 0.85f);
                aMower->mReanimID = mApp->ReanimationGetID(aReanim);
                aReanim->OverrideScale(0.8f, 0.8f);
                aReanim->SetFramesForLayer("anim_land");
                aReanim->SetTruncateDisappearingFrames(nullptr, false);

                if (mApp->mBoard->mSuperMowerMode)
                    aMower->EnableSuperMower(true);
            }
        }
    }
    else if (theItem == STORE_ITEM_BLUEPRINT_CHANGE)
    {
        // Charge a house switch only after the chooser confirms a different house.
        HouseType anOldHouseType = mApp->mPlayerInfo->mGameStats.mHouseType;
        HouseChooserDialog *aHouseDialog = new HouseChooserDialog(mApp);
        mApp->AddDialog(aHouseDialog);
        mApp->mWidgetManager->SetFocus(aHouseDialog);
        mWaitForDialog = true;
        const int houseResult = aHouseDialog->WaitForResult(true);
        mWaitForDialog = false;
        if (houseResult == 1000 && mApp->mPlayerInfo->mGameStats.mHouseType != anOldHouseType)
            mApp->mPlayerInfo->AddCoins(-anItemCost);

        mApp->WriteHouse(true);
        mApp->mWidgetManager->SetFocus(this);
    }
    else if (theItem < STORE_ITEM_BLUEPRINT_BLING || theItem > STORE_ITEM_BLUEPRINT_CLOWN)
    {
        mApp->mPlayerInfo->mPurchases[theItem] = 1;
    }
    else
    {
        // The five blueprints: one more level bought, and the profile records
        // which one so the store can price the next.
        int anOldCount = mApp->mPlayerInfo->mPurchases[theItem];
        int aNewCount = anOldCount + 1;
        if (theItem == STORE_ITEM_BLUEPRINT_BLING)
            anOldCount += 3;
        mApp->mPlayerInfo->mPurchases[theItem] = aNewCount;
        mApp->mPlayerInfo->mGameStats.mHouseLevel = anOldCount;
        mApp->mPlayerInfo->mGameStats.mHouseType = HouseType(theItem - STORE_ITEM_BLUEPRINT_BLING);
        mApp->WriteHouse(true);
    }

    if (theItem == STORE_ITEM_FIRSTAID)
        SetBubbleText(3400, 800, false);

    if (mApp->mSeedChooserScreen != nullptr)
        mApp->mSeedChooserScreen->UpdateAfterPurchase();
    if (mApp->HasAllAchievementPlants())
        mApp->GrantAchievement(ACHIEVEMENT_MORTICULTURALIST);
    mApp->TryToSaveProfile();
}
// ==========================================================================
// StoreScreen::EnableButtons(bool)
// @0018F6E0  14 bytes
// ==========================================================================
// The buttons stay live while the store is waiting for a dialog, and otherwise
// follow whether the second page can be reached.
int StoreScreen::EnableButtons(bool theEnabled)
{
    if (mEasyBuyingCheat == 0)
        return IsPageShown(STORE_PAGE_PLANT_UPGRADES);
    return 1;
}
// ==========================================================================
// StoreScreen::ButtonDepress(int)
// @0018F6F0  212 bytes
// ==========================================================================
// The two page-turn buttons. Each one closes the Crazy Dave bubble, plays the
// hatch and then steps the page in its direction, wrapping at the ends and
// keeping on until it lands on a page the profile is allowed to see.
void StoreScreen::ButtonDepress(int theId)
{
    switch (theId)
    {
        case 100:
            mResult = 1000;
            break;

        case 101:
            mApp->PlaySample(SOUND_HATCHBACK_CLOSE, true);
            mHatchTimer = 50;
            mHatchDirection = 1;
            mBubbleCountDown = 0;
            mApp->CrazyDaveStopTalking();
            EnableButtons(false);
            do
            {
                int aPage = (int)mPage - 1;
                if (aPage == NUM_STORE_PAGES)
                    --aPage;
                if (aPage < 0)
                    aPage = STORE_PAGE_HOUSE;
                mPage = (StorePages)aPage;
            } while (!IsPageShown(mPage));
            break;

        case 102:
            mApp->PlaySample(SOUND_HATCHBACK_CLOSE, true);
            mHatchTimer = 50;
            mHatchDirection = 2;
            mBubbleCountDown = 0;
            mApp->CrazyDaveStopTalking();
            EnableButtons(false);
            do
            {
                int aPage = (int)mPage + 1;
                if (aPage == NUM_STORE_PAGES)
                    ++aPage;
                if (aPage > STORE_PAGE_HOUSE)
                    aPage = 0;
                mPage = (StorePages)aPage;
            } while (!IsPageShown(mPage));
            break;

        default:
            break;
    }
}
// ==========================================================================
// StoreScreen::AdvanceCrazyDaveDialog(void)
// @0018F7D4  462 bytes
// ==========================================================================
// Step Crazy Dave's script on by one line, and act on the message ids that are
// commands rather than speech:
//   3100              open the hatch before the line is shown
//   606 / 303 / 2601  open the hatch
//   603               drop a diamond coin
//   902               a hundred coins
//   1002              a hundred coins, and stop
int StoreScreen::AdvanceCrazyDaveDialog()
{
    if (mBubbleClickToContinue == 0)
        return 0;

    if (mApp->mCrazyDaveMessageIndex == 3100)
    {
        // The line that asks for the hatch to open.
        mHatchTimer = 150;
        mHatchOpen = true;
        mApp->PlaySample(SOUND_HATCHBACK_OPEN, true);
    }

    if (mApp->AdvanceCrazyDaveText() != 0)
    {
        // Still talking: the new line becomes the bubble, click-to-continue.
        SetBubbleText(mApp->mCrazyDaveMessageIndex, 0, true);
    }
    else
    {
        // Out of lines: put the bubble back on its ambient timer.
        mApp->CrazyDaveStopTalking();
        int anOldHatchTimer = mHatchTimer;
        mBubbleClickToContinue = false;
        mBubbleCountDown = 500;
        if (anOldHatchTimer == 0)
            EnableButtons(true);
    }

    int aMessageIndex = mApp->mCrazyDaveMessageIndex;
    if (aMessageIndex == 606 || aMessageIndex == 303 || aMessageIndex == 2601)
    {
        mHatchTimer = 150;
        mHatchOpen = true;
        mApp->PlaySample(SOUND_HATCHBACK_OPEN, true);
        aMessageIndex = mApp->mCrazyDaveMessageIndex;
    }

    if (aMessageIndex == 603)
    {
        // A diamond coin, dropped into the pool at (80, 520).
        mApp->PlaySample(SOUND_DIAMOND, true);

        Coin *aCoin = mCoins.DataArrayAlloc();
        aCoin->CoinInitialize(80, 520, COIN_DIAMOND, (CoinMotion)3);
        aCoin->mVelX = 0.0f;
        aCoin->mVelY = -5.0f;

        mApp->mPlayerInfo->SetFlag((PlayerFlags)0x1000, false);
        mApp->TryToSaveProfile();
        aMessageIndex = mApp->mCrazyDaveMessageIndex;
    }

    if (aMessageIndex == 902)
    {
        mApp->mPlayerInfo->AddCoins(100);
        aMessageIndex = mApp->mCrazyDaveMessageIndex;
    }

    if (aMessageIndex == 1002)
        return mApp->mPlayerInfo->AddCoins(100);

    return 0;
}
// ==========================================================================
// StoreScreen::KeyChar(char)
// @0018F9B0  192 bytes
// ==========================================================================
// Space or Return advances Crazy Dave while a bubble is up. Otherwise this is
// the debug cheat entry: C toggles the easy-buying flag, '0' and '$' hand out
// coins.
int StoreScreen::KeyChar(char theChar)
{
    if (mBubbleClickToContinue != 0 && (theChar == ' ' || theChar == 13))
    {
        AdvanceCrazyDaveDialog();
        return 1;
    }

    // The cheat keys only work with debug keys enabled and not disabled.
    if (mApp->mDebugKeysEnabled == 0 || gDisableDebugKeys != 0)
        return 1;

    Sexy::String aDebugText;
    StrFormat(&aDebugText, "Store cheat key '%c'", theChar);

    if ((theChar & 0xDF) == 'C')
    {
        mEasyBuyingCheat = true;
        return 1;
    }

    if (theChar == '0')
        mApp->mPlayerInfo->AddCoins(50000);
    else if (theChar == '$')
        mApp->mPlayerInfo->AddCoins(100);
    else
        return 1;

    mApp->PlaySample(SOUND_BUTTONCLICK, true);
    return 1;
}
// ==========================================================================
// StoreScreen::SetupForIntro(int)
// @0018FA84  16 bytes
// ==========================================================================
int StoreScreen::SetupForIntro(int theDialog)
{
    mStartDialog = theDialog;
    mHatchOpen = false;
    return EnableButtons(false);
}
// ==========================================================================
// StoreScreen::SetSelectedSlot(int)
// @0018FA94  32 bytes
// ==========================================================================
int StoreScreen::SetSelectedSlot(int theSlot)
{
    mSelectedSlot = theSlot;
    mSelectedStoreItemType = (StoreItem)GetStoreItemType(theSlot);
    mHighlightPhase = 0.0f;
    return UpdateSelection();
}
// ==========================================================================
// StoreScreen::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @0018FAB4  460 bytes
// ==========================================================================
// The gamepad's store controls. The incoming button is only acted on when the
// profile's own gamepad mode matches, which is what the first call reads.
int StoreScreen::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theFlags)
{
    // vtable byte 8 on the player info is DefaultPlayerInfo::GetId; the store
    // only responds to the pad that owns this profile.
    int aProfileId = mApp->mPlayerInfo->GetId();
    if (thePlayerIndex != aProfileId)
        return aProfileId;

    mInputGamepadIndex = thePlayerIndex;
    switch (theButton)
    {
        case GAMEPAD_BUTTON_UP:
        case GAMEPAD_BUTTON_DOWN:
        case GAMEPAD_BUTTON_LEFT:
        case GAMEPAD_BUTTON_RIGHT:
            // The d-pad moves the selection, one slot at a time.
            {
                int aSlot = NextSlotInDir((StoreScreen_ItemDir)theButton);
                if (aSlot != mSelectedSlot)
                {
                    SetSelectedSlot(aSlot);
                    mApp->PlaySample(SOUND_BLEEP, true);
                }
            }
            break;

        case GAMEPAD_BUTTON_SELECT:
        case GAMEPAD_BUTTON_B:
            ButtonDepress(100);
            break;

        case GAMEPAD_BUTTON_A:
            // The action button: advance the bubble, or buy what is selected.
            if (mBubbleClickToContinue != 0)
            {
                AdvanceCrazyDaveDialog();
                break;
            }

            if (CanInteractWithButtons() == 0)
                break;

            {
                int aStoreItemType = GetStoreItemType(mSelectedSlot);
                if (aStoreItemType == -1)
                    break;

                if (IsFullVersionOnly(StoreItem(aStoreItemType)) != 0)
                {
                    mWaitForDialog = true;
                    mApp->LawnMessageBox(51,
                        "[GET_FULL_VERSION_TITLE]",
                        "[FULL_VERSION_TO_BUY]",
                        "[DIALOG_BUTTON_OK]",
                        gEmptyCString,
                        3);
                    mWaitForDialog = false;
                }
                else if (aStoreItemType == 30)
                {
                    // The full-version item: the yes answer buys it.
                    mWaitForDialog = true;
                    int aResult = mApp->LawnMessageBox(51,
                        "[BUY_PVZ_TITLE]",
                        "[BUY_PVZ_BODY]",
                        "[GET_FULL_VERSION_YES_BUTTON]",
                        "[GET_FULL_VERSION_NO_BUTTON]",
                        1);
                    mWaitForDialog = false;
                    if (aResult == 1000)
                        mApp->BuyFullVersion();
                }
                else if (IsItemSoldOut(StoreItem(aStoreItemType)) == 0 &&
                    IsItemUnavailable(StoreItem(aStoreItemType)) == 0 && !IsComingSoon(StoreItem(aStoreItemType)))
                {
                    PurchaseItem((StoreItem)aStoreItemType);
                }
            }
            break;

        case GAMEPAD_BUTTON_TL:
            if (CanInteractWithButtons() != 0 && IsPageShown(STORE_PAGE_PLANT_UPGRADES) != 0)
                ButtonDepress(101);
            break;

        case GAMEPAD_BUTTON_TR:
            if (CanInteractWithButtons() != 0 && IsPageShown(STORE_PAGE_PLANT_UPGRADES) != 0)
                ButtonDepress(102);
            break;

        default:
            break;
    }

    return 0;
}
// ==========================================================================
// StoreScreen::StoreScreen(LawnApp *)
// @0018FCA8  778 bytes
// ==========================================================================
StoreScreen::StoreScreen(LawnApp *theApp) :
    Sexy::Dialog(nullptr, nullptr, 6, 1, Sexy::String("[STORE]"), Sexy::String(""), Sexy::String(""), 0)
{
    // The base Dialog is constructed by the initialiser above; what is left is
    // the field setup, kept in the binary's order.
    mBubbleText = "";

    // The eight floats are the highlight pulse control points. Nothing reads
    // them back, so they are assigned below instead of constructed in place.

    mApp = theApp;
    mClip = false;
    mStoreTime = 0;
    mBubbleCountDown = 0;
    mBubbleClickToContinue = false;
    mAmbientSpeechCountDown = 0;
    mPage = STORE_PAGE_SLOT_UPGRADES;
    mSelectedSlot = 0;
    mHatchTimer = 0;
    mHatchDirection = 0;
    mShakeX = 0;
    mShakeY = 0;
    mInputGamepadIndex = 0;
    mEasyBuyingCheat = false;
    mWaitForDialog = false;
    mBubbleCountDown = 200;
    mPreviousAmbientSpeechIndex = -1;
    mSelectedStoreItemType = (StoreItem)-1;
    mStartDialog = -1;
    mHatchOpen = true;

    mCoins.DataArrayInitialize(1024, "coins");
    mCoins.mNextKey = 57193;

    Sexy::String aDelayLoad("DelayLoad_Store");
    TodLoadResources(aDelayLoad);

    Resize(0, 0, 800, 600);
    mPottedPlantSpecs.InitializePottedPlant(SEED_MARIGOLD);
    mPottedPlantSpecs.mDrawVariation = (unsigned int)(RandRangeInt(2, 12) & 0xF);

    // The overlay widget the store forwards its drawing to.
    StoreScreenOverlay *anOverlay = new StoreScreenOverlay();
    anOverlay->mStoreScreen = this;
    anOverlay->mMouseVisible = false;
    anOverlay->mHasAlpha = true;
    mOverlayWidget = anOverlay;
    anOverlay->Resize(0, 0, 800, 600);

    mDrawnOnce = false;
    mGoToTreeNow = false;
    mPurchasedFullVersion = false;
    mTrialLockedWhenStoreOpened = (mApp->IsTrialStageLocked() != 0);
    mHighlightPhase = 0.0f;

    // The Curve1DUtil's eight control points: two at 1.0, six intermediate
    // values and a final 2.0.
    mHighlightCurve[0] = 1.0f;
    mHighlightCurve[1] = 1.0f;
    mHighlightCurve[2] = 0.98f;
    mHighlightCurve[3] = 0.9f;
    mHighlightCurve[4] = 1.15f;
    mHighlightCurve[5] = 1.12f;
    mHighlightCurve[6] = 0.95f;
    mHighlightCurve[7] = 1.25f;

    mIntroStep = 0;
    for (int i = 0; i < 8; ++i)
        mPottedPlantImage[i] = nullptr;
    mCarKeyImageIndex = 0;
}
// ==========================================================================
// StoreScreen::Draw(Sexy::Graphics *)
// @0019017C  1296 bytes
// ==========================================================================
// The store itself: the night or day background, the car and its hatch, the
// eight shelf slots, the coin bank with the running total, and the page
// counter underneath.
void StoreScreen::Draw(Sexy::Graphics *g)
{
    g->SetLinearBlend(true);
    mDrawnOnce = true;

    // The sign slides in over the first sixty frames of the store's life.
    int aSignY = TodAnimateCurve(50, 110, mStoreTime, -250, LawnApp::FULLSCREEN_RECT.mY, 4);

    bool aNight = mApp->IsNight() != 0;
    g->DrawImage(aNight ? IMAGE_STORE_BACKGROUNDNIGHT : IMAGE_STORE_BACKGROUND,
        LawnApp::FULLSCREEN_RECT.mX,
        LawnApp::FULLSCREEN_RECT.mY);

    // The car sits on the shake offsets the hatch animation drives.
    int aCarX = LawnApp::FULLSCREEN_RECT.mX + 359 + mShakeX;
    int aCarY = LawnApp::FULLSCREEN_RECT.mY + 142 + mShakeY;
    g->DrawImage(aNight ? IMAGE_STORE_CAR_NIGHT : IMAGE_STORE_CAR, aCarX, aCarY);

    // The car key on the shelf is one of twelve images, clamped at the last.
    int aKeyIndex = (mCarKeyImageIndex >= 11) ? 11 : mCarKeyImageIndex;
    g->DrawImage(Sexy::GetImageById(aKeyIndex + 701), mShakeX + 305, mShakeY + 510);

    g->DrawImage(IMAGE_STORE_PREVBUTTON, mShakeX + 172, mShakeY + 375);
    g->DrawImage(IMAGE_STORE_NEXTBUTTON, mShakeX + 573, mShakeY + 373);

    if (mHatchTimer == 0 && mHatchOpen)
    {
        // Fully open: one wide image, and no closed-car pass at all.
        g->DrawImage(IMAGE_STORE_HATCHBACKOPEN,
            mShakeX + LawnApp::FULLSCREEN_RECT.mX + 458,
            mShakeY + LawnApp::FULLSCREEN_RECT.mY - 34);
    }
    else
    {
        g->DrawImage(IMAGE_STORE_CARCLOSED,
            mShakeX + LawnApp::FULLSCREEN_RECT.mX + 440,
            mShakeY + LawnApp::FULLSCREEN_RECT.mY + 144);
        if (aNight)
            g->DrawImage(IMAGE_STORE_CARCLOSED_NIGHT,
                mShakeX + LawnApp::FULLSCREEN_RECT.mX + 501,
                mShakeY + LawnApp::FULLSCREEN_RECT.mY + 169);

        // The page-turn button the hatch is heading for lights up.
        if (mHatchDirection == 1)
            g->DrawImage(IMAGE_STORE_PREVBUTTONHIGHLIGHT, mShakeX + 172, mShakeY + 375);
        else if (mHatchDirection == 2)
            g->DrawImage(IMAGE_STORE_NEXTBUTTONHIGHLIGHT, mShakeX + 573, mShakeY + 373);
    }

    g->DrawImage(IMAGE_STORE_SIGN, 285, aSignY);

    // Crazy Dave is drawn through a copy of the Graphics whose origin is moved
    // so he lands beside the car.
    Sexy::Graphics aDaveGraphics(*g);
    aDaveGraphics.mTransX -= 72.0f;
    aDaveGraphics.mTransY += 68.0f;
    mApp->DrawCrazyDave(&aDaveGraphics);

    if (mHatchTimer == 0 && mHatchOpen)
    {
        for (int i = 0; i < 8; ++i)
        {
            int aStoreItemType = GetStoreItemType(i);
            if (aStoreItemType != -1)
                DrawItem(g, i, (StoreItem)aStoreItemType);
        }
    }

    // The coin bank and the running total in its corner.
    g->DrawImage(IMAGE_COINBANK, 575, 500);
    g->SetColor(Sexy::Color(180, 255, 90));
    g->SetFont(FONT_CONTINUUMBOLD14);

    Sexy::String aMoney = mApp->GetMoneyString(mApp->mPlayerInfo->mCoins);
    int aMoneyWidth = (FONT_CONTINUUMBOLD14)->StringWidth(aMoney);
    g->DrawString(aMoney, 725 - aMoneyWidth, 531);

    // The page counter, which only appears when more than one page is open.
    int aPageCount = 0;
    int currentPage = 0;
    for (int i = STORE_PAGE_SLOT_UPGRADES; i <= STORE_PAGE_HOUSE; ++i)
    {
        if (IsPageShown(StorePages(i)))
        {
            ++aPageCount;
            if (i <= mPage)
                ++currentPage;
        }
    }

    if (aPageCount > 1)
    {
        Sexy::String aPageFormat("[STORE_PAGE]");
        Sexy::String aPageText;
        TodReplaceNumberString(&aPageText, &aPageFormat, "{PAGE}", currentPage);

        Sexy::String aPagesText;
        TodReplaceNumberString(&aPagesText, &aPageText, "{NUM_PAGES}", aPageCount);

        Sexy::Color aGray(200, 200, 200);
        TodDrawString(g, aPagesText, 410, 512, FONT_BRIANNETOD16, aGray, 2);
    }
}
// ==========================================================================
// StoreScreen::SetupPage(void)
// @001906DC  206 bytes
// ==========================================================================
// Rebuild the eight per-spot potted-plant icons for the page now showing. A
// spot holding a potted plant gets a freshly rendered 100x70 thumbnail of the
// plant standing in its pot; every other spot's icon is dropped.
int StoreScreen::SetupPage()
{
    for (unsigned int i = 0; i < 8; ++i)
    {
        if (mPottedPlantImage[i] != nullptr)
        {
            delete mPottedPlantImage[i];
            mPottedPlantImage[i] = nullptr;
        }

        int aStoreItemType = GetStoreItemType(i);
        if (!IsPottedPlant((StoreItem)aStoreItemType))
            continue;

        Sexy::MemoryImage *anImage = new Sexy::MemoryImage();
        mPottedPlantImage[i] = anImage;
        anImage->Create(100, 70);

        // The pot and its plant are drawn into the fresh image through a
        // Graphics built on it; its destructor is what flushes the drawing.
        Sexy::Graphics aGraphics(anImage);
        aGraphics.SetColor(Sexy::Color::White);
        aGraphics.Translate(10, -30);
        mApp->mZenGarden->DrawPottedPlant(&aGraphics, 0.0f, 0.0f, &mPottedPlantSpecs, 1.0f, true);
    }

    return SetSelectedSlot(0);
}
// ==========================================================================
// StoreScreen::Update(void)
// @001907C8  590 bytes
// ==========================================================================
// One frame: advance the highlight pulse, tick the coins, run Crazy Dave's
// countdowns, and otherwise let the base class update the children.
void StoreScreen::Update()
{
    mHighlightPhase = mHighlightPhase + 0.028f;
    // vtable byte 56 on the app's music object is Music2::MakeSureMusicIsPlaying.
    mApp->mMusic->MakeSureMusicIsPlaying(MUSIC_TUNE_TITLE_CRAZY_DAVE_MAIN_THEME);
    mApp->UpdateCrazyDave();

    // Every live coin in the pool: the block is walked by the 0xCC element
    // stride and a slot whose packed id has a zero key is skipped.
    CoinSlot *aItems = mCoins.mBlock;
    for (unsigned int i = 0; i < mCoins.mMaxUsedCount; ++i)
    {
        if ((aItems[i].mID >> 16) == 0)
            continue;
        if (!aItems[i].mItem.mDead)
            aItems[i].mItem.Update();
    }

    if (mWaitForDialog != 0)
        return;

    // Dave's own state machine gates everything below: nothing runs while he is
    // off screen, and the bubble and ambient-speech countdowns only advance
    // once he is fully in.
    if (mApp->mCrazyDaveState != CRAZY_DAVE_OFF)
    {
        ++mStoreTime;

        if (mApp->mCrazyDaveState > CRAZY_DAVE_ENTERING)
        {
            if (mHatchTimer > 0)
            {
                --mHatchTimer;
                if (mHatchTimer == 0)
                {
                    // The hatch has finished opening: rebuild the page and let
                    // the buttons back in.
                    mHatchDirection = 0;
                    SetupPage();
                    if (mBubbleClickToContinue == 0)
                        EnableButtons(true);
                    mShakeX = 0;
                    mShakeY = 0;
                }
                else
                {
                    // While it is opening the whole store shakes; the vertical
                    // part only starts once the hatch is mostly up.
                    mShakeX = 0;
                    if (mHatchTimer > 35)
                        mShakeY = Rand(3) - 1;
                    else
                        mShakeY = 0;
                }
            }
            else if (mStartDialog != -1)
            {
                // The dialog the intro asked for, once the app is free.
                SetBubbleText(mStartDialog, 0, true);
                mStartDialog = -1;
            }
            else if (mBubbleClickToContinue == 0)
            {
                if (mBubbleCountDown > 0)
                {
                    --mBubbleCountDown;
                    if (mBubbleCountDown == 0)
                    {
                        // Hold the bubble while Dave is still speaking.
                        if (mApp->mSoundSystem->IsFoleyPlaying(FOLEY_CRAZY_DAVE_SHORT) ||
                            mApp->mSoundSystem->IsFoleyPlaying(FOLEY_CRAZY_DAVE_LONG) ||
                            mApp->mSoundSystem->IsFoleyPlaying(FOLEY_CRAZY_DAVE_EXTRA_LONG))
                            mBubbleCountDown = 1;
                        else
                            mApp->CrazyDaveStopTalking();
                    }
                }
                else
                {
                    // Out of scripted lines: pick an ambient one from the
                    // weighted table. The entry already showing gets weight 0,
                    // and the adventure-only line gets 20 once the adventure is
                    // finished and 0 before that.
                    int aPreviousIndex = mPreviousAmbientSpeechIndex;
                    bool aWasOne = (mAmbientSpeechCountDown == 1);
                    --mAmbientSpeechCountDown;
                    if (mAmbientSpeechCountDown >= 0 && !aWasOne)
                        return;

                    TodWeightedArray aTable[4];
                    int aMessageId = 2015;
                    int aTableIndex = 0;
                    for (;;)
                    {
                        aTable[aTableIndex].mItem = aMessageId;
                        if (aMessageId == aPreviousIndex)
                            aTable[aTableIndex].mWeight = 0;
                        else if (aMessageId == 2018)
                            aTable[aTableIndex].mWeight = mApp->HasFinishedAdventure() ? 20 : 0;
                        else
                            aTable[aTableIndex].mWeight = 100;

                        ++aMessageId;
                        ++aTableIndex;
                        if (aMessageId == 2019)
                        {
                            int aPicked = TodPickFromWeightedArray(aTable, 4);
                            mPreviousAmbientSpeechIndex = aPicked;
                            SetBubbleText(aPicked, 800, false);
                            mAmbientSpeechCountDown = RandRangeInt(500, 1000);
                            break;
                        }
                    }
                }
            }
        }

        if (CanInteractWithButtons() != 0 && mPurchasedFullVersion != 0 && mApp->IsTrialStageLocked() == 0)
        {
            // The full version has been bought: the store hands over to the
            // tree of wisdom.
            mGoToTreeNow = true;
            mResult = 1000;
        }
        else
        {
            // vtable byte 60 on the widget manager is WidgetContainer::BringToFront.
            mWidgetManager->BringToFront((Sexy::Widget *)mHelpBarWidget);
            Sexy::Widget::Update();
        }
    }
    else if (mDrawnOnce != 0)
    {
        StorePreLoad();
    }
}
// ==========================================================================
// StoreScreen::AddedToManager(Sexy::WidgetManager *)
// @00190A1C  50 bytes
// ==========================================================================
void StoreScreen::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    Sexy::WidgetContainer::AddedToManager(theWidgetManager);
    // vtable byte 24 -- WidgetContainer::AddWidget -- for the overlay this
    // screen keeps in mOverlayWidget.
    AddWidget(mOverlayWidget);
    SetSelectedSlot(0);
    mCarKeyImageIndex = RandRangeInt(0, 11);
    SetupPage();
}
// ==========================================================================
// StoreScreen::KeyDown(Sexy::KeyCode)
// @00190A50  490 bytes
// ==========================================================================
// The keyboard equivalent of GameButtonDown. The four arrows move the
// selection; when Left or Right finds the selection already at the edge it
// turns the page instead. Return and the gamepad's confirm key 304 act on the
// selected item, Escape and the key codes 305/314 press the back button, and
// 310/311 are the gamepad's page-turn pair.
int StoreScreen::KeyDown(Sexy::KeyCode theKey)
{
    if (theKey == 304 || theKey == KEYCODE_RETURN)
    {
        // Confirm: the same body the two share in the binary.
        if (mBubbleClickToContinue != 0)
        {
            AdvanceCrazyDaveDialog();
            return 1;
        }

        if (CanInteractWithButtons() == 0)
            return 1;

        int aStoreItemType = GetStoreItemType(mSelectedSlot);
        if (aStoreItemType == -1)
            return Sexy::Dialog::KeyDown(theKey);

        if (IsFullVersionOnly(StoreItem(aStoreItemType)) != 0)
        {
            mWaitForDialog = true;
            mApp->LawnMessageBox(
                51, "[GET_FULL_VERSION_TITLE]", "[FULL_VERSION_TO_BUY]", "[DIALOG_BUTTON_OK]", gEmptyCString, 3);
            mWaitForDialog = false;
            return 1;
        }

        if (aStoreItemType == 30)
        {
            // The full-version item: the yes answer buys it.
            mWaitForDialog = true;
            int aResult = mApp->LawnMessageBox(51,
                "[BUY_PVZ_TITLE]",
                "[BUY_PVZ_BODY]",
                "[GET_FULL_VERSION_YES_BUTTON]",
                "[GET_FULL_VERSION_NO_BUTTON]",
                1);
            mWaitForDialog = false;
            if (aResult == 1000)
                mApp->BuyFullVersion();
            return 1;
        }

        if (IsItemSoldOut(StoreItem(aStoreItemType)) == 0 && IsItemUnavailable(StoreItem(aStoreItemType)) == 0 &&
            !IsComingSoon(StoreItem(aStoreItemType)))
            PurchaseItem((StoreItem)aStoreItemType);

        return 1;
    }

    if (theKey == KEYCODE_ESCAPE || theKey == 305 || theKey == 314)
    {
        ButtonDepress(100);
        return 1;
    }

    if (theKey == 310)
    {
        // The gamepad's page-back button.
        if (CanInteractWithButtons() != 0 && IsPageShown(STORE_PAGE_PLANT_UPGRADES) != 0)
        {
            ButtonDepress(101);
            return 1;
        }
        return 1;
    }

    if (theKey == 311)
    {
        // The gamepad's page-forward button.
        if (CanInteractWithButtons() != 0 && IsPageShown(STORE_PAGE_PLANT_UPGRADES) != 0)
        {
            ButtonDepress(102);
            return 1;
        }
        return 1;
    }

    // Only the four arrows are left; anything else belongs to the base dialog.
    if ((unsigned int)(theKey - KEYCODE_LEFT) > 3)
        return Sexy::Dialog::KeyDown(theKey);

    // The direction table is indexed from KEYCODE_UP, two below Left.
    unsigned int aSlot = NextSlotInDir((StoreScreen_ItemDir)gStoreDpadDir[theKey - 37]);
    if (aSlot != (unsigned int)mSelectedSlot)
    {
        SetSelectedSlot(aSlot);
        mApp->PlaySample(SOUND_BLEEP, true);
        return 1;
    }

    // The selection did not move. Up and Down have nothing else to do; Left and
    // Right turn the page when the profile may see the one next to it.
    if (CanInteractWithButtons() == 0)
        return 1;

    if (theKey == KEYCODE_RIGHT)
    {
        if (IsPageShown(STORE_PAGE_PLANT_UPGRADES) != 0)
        {
            ButtonDepress(102);
            return 1;
        }
        return 1;
    }

    if (theKey != KEYCODE_LEFT)
        return 1;

    if (IsPageShown(STORE_PAGE_PLANT_UPGRADES) != 0)
    {
        ButtonDepress(101);
        return 1;
    }
    return 1;
}

// ==========================================================================
// StoreScreen::~StoreScreen()
// @00190034  292 bytes
// ==========================================================================
StoreScreen::~StoreScreen()
{
    // The two vptr stores are the compiler's -- this class and the
    // ButtonListener base Sexy::Dialog carries at +0x100 -- and the trailing
    // `Sexy::Dialog::~Dialog(this)` is the base, which C++ runs.
    //
    // DataArray<Coin>::DataArrayDispose: Coin::~Coin on every live slot, then
    // operator delete[] of the raw block. A second call is a no-op.
    mCoins.DataArrayDispose();

    // The word at +0x2E4 is mOverlayWidget.
    delete mOverlayWidget;

    // Eight per-spot potted-plant icons at +0x364..+0x383. The dump loops eight
    // times from +0x364, deleting each non-null entry and clearing it.
    for (int i = 0; i < 8; ++i)
        delete mPottedPlantImage[i];

    // The resources group the store screen loaded.
    Sexy::String aDelayLoad("DelayLoad_Store");
    TodDeleteResources(aDelayLoad);

    // The word at +0x2EC is mBubbleText, a one-word COW string: the dump drops
    // its rep's refcount and frees the rep at zero, which is exactly
    // Sexy::String's own destructor, so it is not written here.
}
