// ZenGardenControls -- transcribed from libGameMain.so
// Rewritten by hand in stage 2; see build/stage2-brief.md.

#include "ZenGardenControls.h"

#include "game/Board.h"
#include "Coin.h"
#include "CursorObject.h"
#include "CursorPreview.h"
#include "GameStats.h"
#include "GridItem.h"
#include "HelpBarWidget.h"
#include "LawnApp.h"
#include "LawnPlayerInfo.h"
#include "ReconData.h"
#include "ReconFreeFunctions.h"
#include "ReconHelpers.h"
#include "ZenGarden.h"
#include <cmath>
#include <ctime>

// ==========================================================================
// ZenGardenControls::OnButtonUp(int,int,unsigned int)
// @001BCEA4  2 bytes
// ==========================================================================
// The dump's body is a bare return; the base's own handler is what runs.
void ZenGardenControls::OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags)
{
    (void)theButton;
    (void)thePlayerIndex;
    (void)theFlags;
}

// ==========================================================================
// ZenGardenControls::OnGameAxisMove(int,int,int)
// @001BCEA8  2 bytes
// ==========================================================================
void ZenGardenControls::OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex) {}

// ==========================================================================
// ZenGardenControls::~ZenGardenControls()
// @001BCF9C  30 bytes
// ==========================================================================
// The dump stores the two vptrs and then calls ~GamepadControls on itself; a
// real virtual destructor with a real base makes the compiler emit all three.
ZenGardenControls::~ZenGardenControls() {}

// ==========================================================================
// ZenGardenControls::Update(float)
// @001BD840  532 bytes
// ==========================================================================
int ZenGardenControls::Update(float theDeltaTime)
{
    // The dump's 2208 read off the app is LawnApp::mBoard (+0x8A0), and the byte
    // it then loads at +119 is Sexy::Widget::mHasFocus at Board's +0x77.
    if (mGamepadIndex == -1 || !mApp->mBoard->mHasFocus)
        return 0;

    // Nothing selected yet: start on the watering can.
    if (mBoard->mMouseInputActive[mPlayerIndex] &&
        mBoard->mCursorObject[mPlayerIndex]->mCursorType == CURSOR_TYPE_NORMAL)
        mSelectedTool = OBJECT_TYPE_NONE;
    if (mSelectedTool == OBJECT_TYPE_NONE && !mBoard->mMouseInputActive[mPlayerIndex])
    {
        mSelectedTool = OBJECT_TYPE_WATERING_CAN;
        mBoard->PickUpTool(OBJECT_TYPE_WATERING_CAN, mPlayerIndex);
    }
    BaseGamepadControls::Update(theDeltaTime);

    // Stinky lights up while the cursor sits on his own tile (state 23 is
    // GRIDITEM_STINKY; the dump compares the surrogate enum against the literal).
    GridItem *aStinky = mApp->mZenGarden->GetStinky();
    if (aStinky != nullptr)
        aStinky->mHighlighted = aStinky->mGridItemState == 23 &&
            mBoard->PixelToGridX((int)mCursorPositionX, (int)mCursorPositionY) ==
                mBoard->PixelToGridX((int)aStinky->mPosX, (int)aStinky->mPosY) &&
            mBoard->PixelToGridY((int)mCursorPositionX, (int)mCursorPositionY) ==
                mBoard->PixelToGridY((int)aStinky->mPosX, (int)aStinky->mPosY);

    // Which cursor types light the plants under them up: the shovel and the seven
    // tools from the watering can to the money sign always, the wheelbarrow only
    // while it is empty. The dump's `(unsigned)(type - 9) > 6` test is exactly
    // `type < WATERING_CAN || type > MONEY_SIGN`.
    CursorType aCursorType = mBoard->mCursorObject[mPlayerIndex]->mCursorType;
    if (aCursorType == CURSOR_TYPE_SHOVEL ||
        (aCursorType >= CURSOR_TYPE_WATERING_CAN && aCursorType <= CURSOR_TYPE_MONEY_SIGN) ||
        (aCursorType == CURSOR_TYPE_WHEEELBARROW && mApp->mZenGarden->GetPottedPlantInWheelbarrow() == 0))
        mBoard->HighlightPlantsForMouse((int)mCursorPositionX, (int)mCursorPositionY);

    // Board::MouseHitTest fills a HitResult; the recon declaration still types the
    // out parameter `GridItem **`, so the address travels through a cast.
    HitResult aHitResult;
    mBoard->MouseHitTest((int)mCursorPositionX, (int)mCursorPositionY, &aHitResult, 0);
    if (!mBoard->mMouseInputActive[mPlayerIndex] && aHitResult.mObjectType == OBJECT_TYPE_COIN)
        static_cast<Coin *>(aHitResult.mObject)->GamepadCursorOver(mPlayerIndex);

    // The preview cursor is switched on just before the state machine runs.
    mBoard->mCursorPreview[mPlayerIndex]->mVisible = true;
    int aResult = UpdateStates(theDeltaTime);

    // Gamepad velocity drives the cursor, clamped to the zen garden's play area.
    float aX = mCursorPositionX + theDeltaTime * mGamepadVelocityLeftX;
    float aY = mCursorPositionY + theDeltaTime * mGamepadVelocityLeftY;
    if (aX > 770.0f)
        aX = 770.0f;
    if (aX < 40.0f)
        aX = 40.0f;
    if (aY > 580.0f)
        aY = 580.0f;
    if (aY < 80.0f)
        aY = 80.0f;
    if (!mBoard->mMouseInputActive[mPlayerIndex])
    {
        mCursorPositionX = aX;
        mCursorPositionY = aY;
    }

    return aResult;
}

// ==========================================================================
// ZenGardenControls::Draw(Sexy::Graphics *)
// @001BDA64  88 bytes
// ==========================================================================
void ZenGardenControls::Draw(Sexy::Graphics *g)
{
    // The dump's 2376 read off the app is LawnApp::mCrazyDaveState (+0x948).
    if (mGamepadIndex == -1 || mApp->mCrazyDaveState > 2)
        return;

    CursorObject *aCursor = mBoard->mCursorObject[mPlayerIndex];
    if (!aCursor->BeginDraw(g))
        return;

    aCursor->Draw(g);
    aCursor->EndDraw(g);
}

// ==========================================================================
// ZenGardenControls::OnButtonDown(int,int,unsigned int)
// @001BDC70  1120 bytes
// ==========================================================================
void ZenGardenControls::OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags)
{
    if (mGamepadIndex != thePlayerIndex || mGamepadIndex == -1)
        return;

    switch (theButton)
    {
        case 5:
            // The dump reaches the app through Board::mApp (Board + 276).
            mBoard->mApp->PlaySample(SOUND_PAUSE, true);
            mBoard->mApp->DoNewOptions(false, mGamepadIndex);
            return;

        case 6:
            mBoard->MouseDownWithTool(
                (int)mCursorPositionX, (int)mCursorPositionY, 0, mBoard->mCursorObject[mPlayerIndex]->mCursorType, 0);
            return;

        case 7:
            mApp->DoConfirmBackToMain(true);
            return;

        case 8:
            // The store only opens in a main-menu garden whose own byte at +0x14
            // (ZenGarden::mShowShopButton) is set.
            if (mApp->CanShowStore() && mApp->mZenGarden->mShowShopButton)
                mApp->mZenGarden->OpenStore();
            return;

        case 9:
            if (mApp->mZenGarden->mGardenType == 3)
                mApp->PlaySample(SOUND_TAPGLASS, true);

            if (mBoard->mTutorialState == 27)
            {
                mApp->KillBoard();
                mApp->mHelpBarWidget->ClearButtons(0);
                mApp->PreNewGame(GAMEMODE_ADVENTURE, false);
            }
            else if (mApp->mZenGarden->CanFeedStinkyChocolate())
            {
                GridItem *aStinky = mApp->mZenGarden->GetStinky();
                if (aStinky != nullptr)
                {
                    mApp->mZenGarden->WakeStinky();
                    mApp->AddTodParticle(aStinky->mPosX + 40.0f,
                        aStinky->mPosY + 40.0f,
                        aStinky->mRenderOrder + 1,
                        PARTICLE_PRESENT_PICKUP);

                    // The dump's 2348 read off the app is LawnApp::mPlayerInfo (+0x92C);
                    // its +0x304 is PlayerInfo::mLastStinkyChocolateTime and its +0x224
                    // falls inside PlayerInfo::mPurchases, the counter feeding uses.
                    LawnPlayerInfo *aPlayerInfo = mApp->mPlayerInfo;
                    aPlayerInfo->mLastStinkyChocolateTime = (int)time(nullptr);
                    int aOldCount = aPlayerInfo->mPurchases[26]--;

                    // Above 1001 the stat is credited straight away; otherwise the tool
                    // state is re-entered first and the stat only counts if the profile
                    // survived that.
                    bool aCredit = aOldCount > 1001;
                    if (!aCredit)
                    {
                        OnButtonDown(10, mGamepadIndex, 0);
                        aCredit = mApp->mPlayerInfo != nullptr;
                    }
                    if (aCredit)
                        mApp->mPlayerInfo->mGameStats.ChangeMiscStat(STAT_STINKY_CHOCOLATES, 1);

                    mApp->PlayFoley(FOLEY_WAKEUP);
                    mApp->PlaySample(SOUND_MINDCONTROLLED, true);
                }
            }
            return;

        case 10:
            // Step the selected tool backwards through the wheelbarrow's tool ring.
            if (mApp->mGameScene == SCENE_PLAYING)
            {
                int aTool = mSelectedTool;
                do
                {
                    if (aTool == OBJECT_TYPE_NONE)
                        aTool = OBJECT_TYPE_WATERING_CAN;
                    else if (aTool <= OBJECT_TYPE_WATERING_CAN)
                        aTool = OBJECT_TYPE_TREE_OF_WISDOM_GARDEN;
                    else
                        --aTool;
                } while (!mBoard->CanUseGameObject((GameObjectType)aTool) ||
                    !mBoard->PickUpTool((GameObjectType)aTool, mPlayerIndex));
                mSelectedTool = (GameObjectType)aTool;
            }
            return;

        case 11:
            // ... and forwards, logging the tool it settled on.
            if (mApp->mGameScene == SCENE_PLAYING)
            {
                int aTool = mSelectedTool;
                int aLoggedTool = 0;
                do
                {
                    if (aTool == OBJECT_TYPE_TREE_OF_WISDOM_GARDEN || aTool == OBJECT_TYPE_NONE)
                    {
                        aTool = OBJECT_TYPE_WATERING_CAN;
                        aLoggedTool = OBJECT_TYPE_WATERING_CAN;
                    }
                    else
                    {
                        aLoggedTool = ++aTool;
                        if (aTool > OBJECT_TYPE_TREE_OF_WISDOM_GARDEN)
                            break;
                    }
                } while (!mBoard->CanUseGameObject((GameObjectType)aTool) ||
                    !mBoard->PickUpTool((GameObjectType)aTool, mPlayerIndex));
                mSelectedTool = (GameObjectType)aTool;

                Sexy::String aLog;
                StrFormat(&aLog, "mSelectedTool = %d\n", aLoggedTool);
                OutputDebug(aLog.c_str());
            }
            return;

        case 16:
        case 17:
        case 18:
        case 19:
        {
            // The four dpad buttons. A grid step in progress snaps the cursor back to
            // the cell centre first, and the seed-select state swallows the press.
            if (mGamepadState == MOVEMENT_STATE_GRID_STEP)
            {
                float aDeltaY = mGridCenterPositionY - mCursorPositionY;
                float aDeltaX = mGridCenterPositionX - mCursorPositionX;
                if (sqrtf(aDeltaX * aDeltaX + aDeltaY * aDeltaY) >= 3.0f)
                    return;
                mCursorPositionX = mGridCenterPositionX;
                mCursorPositionY = mGridCenterPositionY;
            }
            else if (mGamepadState == MOVEMENT_STATE_SELECT_SEED)
            {
                return;
            }

            // dword_5A1F54 is the eight-word table of grid deltas the four dpad
            // buttons index: [0..3] are the X steps, [4..7] the Y steps.
            int aDeltaIndex = theButton - 16;
            int aGridDeltaX = gZenGridDelta[aDeltaIndex];
            int aGridDeltaY = gZenGridDelta[aDeltaIndex + 4];

            GotoState(MOVEMENT_STATE_GRID_STEP);

            int aGridX = aGridDeltaX + mApp->mZenGarden->PixelToGridX((int)mCursorPositionX, (int)mCursorPositionY);
            int aGridY = aGridDeltaY + mApp->mZenGarden->PixelToGridY((int)mCursorPositionX, (int)mCursorPositionY);

            // GetGridSize takes its two results as (Y size, X size) -- the binary
            // bounds aGridX with the second and aGridY with the first.
            int aGridSizeY;
            int aGridSizeX;
            mApp->mZenGarden->GetGridSize(aGridSizeY, aGridSizeX);
            if (aGridX < 0 || aGridY < 0 || aGridX >= aGridSizeX || aGridY >= aGridSizeY)
                break;

            int aCellX = mBoard->GridToPixelX(aGridX, aGridY);
            float aCenterX = (float)(mBoard->GridCellWidth(aGridX, aGridY) / 2) + (float)aCellX;
            int aCellY = mBoard->GridToPixelY(aGridX, aGridY);
            int aCellHeight = mBoard->GridCellHeight(aGridX, aGridY);
            float aOldX = mCursorPositionX;
            float aOldY = mCursorPositionY;
            mGridCenterPositionX = aCenterX;
            float aCenterDeltaX = aCenterX - aOldX;
            float aCenterY = (float)(aCellHeight / 2) + (float)aCellY;
            float aCenterDeltaY = aCenterY - aOldY;
            mGridCenterPositionY = aCenterY;

            // The velocity is the unit vector towards the new cell centre, scaled.
            float aLength = sqrtf(aCenterDeltaX * aCenterDeltaX + aCenterDeltaY * aCenterDeltaY);
            if (aLength != 0.0f)
            {
                aCenterDeltaY = aCenterDeltaY / aLength;
                aCenterDeltaX = aCenterDeltaX / aLength;
            }
            mGamepadVelocityLeftY = aCenterDeltaY * 50.0f;
            mGamepadVelocityLeftX = aCenterDeltaX * 50.0f;
            break;
        }

        default:
            return;
    }
}

// ==========================================================================
// ZenGardenControls::OnKeyDown(Sexy::KeyCode,unsigned int)
// @001BE104  938 bytes
// ==========================================================================
bool ZenGardenControls::OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags)
{
    // The gamepad's own key codes are handled first and end the function.
    bool aHandled = XButtonKeyCodeDown(theKey) != 0;
    if (aHandled)
    {
        mBoard->mApp->PlaySample(SOUND_PAUSE, true);
        mBoard->mApp->DoNewOptions(false, mGamepadIndex);
        return aHandled;
    }

    switch (theKey)
    {
        case 13:
            // Enter: plant whatever is held, or open the seed selection.
            if (mApp->mCrazyDaveMessageIndex == -1)
            {
                aHandled = true;
                if (mGamepadState == MOVEMENT_STATE_SELECT_SEED)
                {
                    mBoard->MouseDownWithTool((int)mCursorPositionX,
                        (int)mCursorPositionY,
                        0,
                        mBoard->mCursorObject[mPlayerIndex]->mCursorType,
                        0);
                    GotoState(MOVEMENT_STATE_NORMAL);
                }
                else
                {
                    GotoState(MOVEMENT_STATE_SELECT_SEED);
                }
            }
            return aHandled;

        case 27:
            // Escape: leave the garden, or close the seed selection.
            if (mBoard->mTutorialState == 27)
            {
                aHandled = true;
                mApp->KillBoard();
                mApp->mHelpBarWidget->ClearButtons(0);
                mApp->PreNewGame(GAMEMODE_ADVENTURE, false);
            }
            else
            {
                if (mGamepadState != MOVEMENT_STATE_SELECT_SEED)
                {
                    mApp->DoConfirmBackToMain(true);
                    return true;
                }
                aHandled = true;
                GotoState(MOVEMENT_STATE_NORMAL);
            }
            return aHandled;

        case 37:
        case 38:
        case 39:
        case 40:
        {
            // The four arrow keys: while the seed selection is open they step the
            // tool ring; otherwise they walk the cursor one grid cell.
            if (mGamepadState == MOVEMENT_STATE_SELECT_SEED)
            {
                if (mApp->mGameScene == SCENE_PLAYING)
                {
                    if (theKey == 37)
                    {
                        // Left steps the tool backwards, retrying while a tool is unusable.
                        int aTool = mSelectedTool;
                        while (true)
                        {
                            if (aTool == OBJECT_TYPE_NONE)
                                aTool = OBJECT_TYPE_WATERING_CAN;
                            else if (aTool <= OBJECT_TYPE_WATERING_CAN)
                                aTool = OBJECT_TYPE_TREE_OF_WISDOM_GARDEN;
                            else
                                --aTool;

                            if (mBoard->CanUseGameObject((GameObjectType)aTool))
                            {
                                aHandled = mBoard->PickUpTool((GameObjectType)aTool, mPlayerIndex) != 0;
                                if (aHandled)
                                    break;
                            }
                        }
                        mSelectedTool = (GameObjectType)aTool;
                    }
                    else if (theKey == 39)
                    {
                        // Right is the mirror image, and logs the tool it settled on.
                        int aTool = mSelectedTool;
                        int aLoggedTool = 0;
                        do
                        {
                            if (aTool == OBJECT_TYPE_TREE_OF_WISDOM_GARDEN || aTool == OBJECT_TYPE_NONE)
                            {
                                aTool = OBJECT_TYPE_WATERING_CAN;
                                aLoggedTool = OBJECT_TYPE_WATERING_CAN;
                            }
                            else
                            {
                                aLoggedTool = ++aTool;
                                if (aTool > OBJECT_TYPE_TREE_OF_WISDOM_GARDEN)
                                    break;
                            }
                        } while (!mBoard->CanUseGameObject((GameObjectType)aTool) ||
                            !mBoard->PickUpTool((GameObjectType)aTool, mPlayerIndex));
                        mSelectedTool = (GameObjectType)aTool;

                        Sexy::String aLog;
                        StrFormat(&aLog, "mSelectedTool = %d\n", aLoggedTool);
                        OutputDebug(aLog.c_str());
                        return true;
                    }
                }
                return aHandled;
            }

            if (mGamepadState == MOVEMENT_STATE_GRID_STEP)
            {
                // A step in progress must have finished before another one starts.
                float aDeltaY = mGridCenterPositionY - mCursorPositionY;
                float aDeltaX = mGridCenterPositionX - mCursorPositionX;
                if (sqrtf(aDeltaX * aDeltaX + aDeltaY * aDeltaY) >= 3.0f)
                    return aHandled;
                mCursorPositionX = mGridCenterPositionX;
                mCursorPositionY = mGridCenterPositionY;
            }

            // dword_5A1F54 holds four delta pairs: [0..3]/[4..7] for the dpad buttons
            // and [8..11]/[12..15] for the arrow keys, which are indexed here.
            int aDeltaIndex = theKey - 37;
            int aGridDeltaX = gZenGridDelta[aDeltaIndex + 8];
            int aGridDeltaY = gZenGridDelta[aDeltaIndex + 12];

            GotoState(MOVEMENT_STATE_GRID_STEP);

            int aGridX = aGridDeltaX + mApp->mZenGarden->PixelToGridX((int)mCursorPositionX, (int)mCursorPositionY);
            int aGridY = aGridDeltaY + mApp->mZenGarden->PixelToGridY((int)mCursorPositionX, (int)mCursorPositionY);

            // GetGridSize takes its two results as (Y size, X size) -- the binary
            // bounds aGridX with the second and aGridY with the first.
            int aGridSizeY;
            int aGridSizeX;
            mApp->mZenGarden->GetGridSize(aGridSizeY, aGridSizeX);
            if (aGridX >= 0 && aGridY >= 0 && aGridX < aGridSizeX && aGridY < aGridSizeY)
            {
                int aCellX = mBoard->GridToPixelX(aGridX, aGridY);
                float aCenterX = (float)(mBoard->GridCellWidth(aGridX, aGridY) / 2) + (float)aCellX;
                int aCellY = mBoard->GridToPixelY(aGridX, aGridY);
                int aCellHeight = mBoard->GridCellHeight(aGridX, aGridY);
                float aOldX = mCursorPositionX;
                float aOldY = mCursorPositionY;
                mGridCenterPositionX = aCenterX;
                float aCenterDeltaX = aCenterX - aOldX;
                float aCenterY = (float)(aCellHeight / 2) + (float)aCellY;
                float aCenterDeltaY = aCenterY - aOldY;
                mGridCenterPositionY = aCenterY;

                // The velocity is the unit vector towards the new cell centre, scaled.
                float aLength = sqrtf(aCenterDeltaX * aCenterDeltaX + aCenterDeltaY * aCenterDeltaY);
                if (aLength != 0.0f)
                {
                    aCenterDeltaY = aCenterDeltaY / aLength;
                    aCenterDeltaX = aCenterDeltaX / aLength;
                }
                mGamepadVelocityLeftX = aCenterDeltaX * 50.0f;
                mGamepadVelocityLeftY = aCenterDeltaY * 50.0f;
            }
            return true;
        }

        default:
            return aHandled;
    }
}

// ==========================================================================
// ZenGardenControls::ZenGardenControls(Board *,int,int)
// @001C30EC  46 bytes
// ==========================================================================
ZenGardenControls::ZenGardenControls(Board *theBoard, int thePlayerIndex1, int thePlayerIndex2) :
    GamepadControls(theBoard, thePlayerIndex1, thePlayerIndex2)
{
    // The dump constructs the GamepadControls base in place and then stores the
    // two vptrs; with a real base the compiler emits both from the declarations.
    mSelectedTool = OBJECT_TYPE_NONE;
    GotoState(MOVEMENT_STATE_NORMAL);
}
