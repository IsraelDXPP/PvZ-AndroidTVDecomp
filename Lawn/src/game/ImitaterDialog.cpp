// ImitaterDialog -- reconstructed from libGameMain.so
//
// The "choose a seed to copy" dialog. The bodies below are the IDA
// decompilation rewritten against Game\ImitaterDialog.h: every raw
// `((char *)this + N)` is the member the header names at that offset, and every
// hand-written vtable slot call is a real call on `mApp` or on
// `mApp->mSeedChooserScreen`.
//
// The offsets this class's own members sit at are +0x2FC..+0x30F; everything
// below that is LawnDialog (0x2E0..0x2FB) or Dialog/Widget.

#include "ImitaterDialog.h"

#include "GamepadApp.h"
#include "Graphics.h"
#include "HelpBarWidget.h"
#include "LawnApp.h"
#include "LawnDialog.h"
#include "Plant.h"
#include "ReconTypes.h"
#include "SeedChooserScreen.h"
#include "SexyAppBase.h"
#include "ToolTipWidget.h"
#include "Widget.h"
#include "WidgetManager.h"
// ==========================================================================
// ImitaterDialog::MouseUp(int,int,int)
// @0014B98C  2 bytes
// ==========================================================================
void ImitaterDialog::MouseUp(int theX, int theY, int theClickCount)
{
    (void)theX;
    (void)theY;
    (void)theClickCount;
    // Two bytes: `BX LR`. The dialog does not handle button release.
}

// ==========================================================================
// ImitaterDialog::DrawOverlay(Sexy::Graphics *)
// @0014B9E4  8 bytes
// ==========================================================================
void ImitaterDialog::DrawOverlay(Sexy::Graphics *theGraphics)
{
    // The binary tail-calls ToolTipWidget::Draw, which is why this override is
    // `void` like the Sexy::Widget slot it replaces.
    mToolTipWidget->Draw(theGraphics, true);
}

// ==========================================================================
// ImitaterDialog::~ImitaterDialog()
// @0014BBBC  194 bytes
// ==========================================================================
ImitaterDialog::~ImitaterDialog()
{
    // Compiler-generated shape: the ARM body inline-expands the three Sexy::String
    // destructors of the owned ToolTipWidget and then frees it, i.e. the source
    // said `delete mToolTipWidget`. The base destructor runs after it.
    delete mToolTipWidget;
}

// ==========================================================================
// ImitaterDialog::ImitaterDialog(int)
// @0014BF3C  568 bytes
// ==========================================================================
ImitaterDialog::ImitaterDialog(int thePlayerIndex) :
    LawnDialog(gLawnApp, nullptr, 52, true, "[CHOOSE_SEED_TO_COPY]", "", "[DIALOG_BUTTON_OK]", 1)
{
    // Before anything else the dialog is sized to the seed chooser's own
    // rectangle: 550x600, adjusted by however much CalcSize grows the frame.
    CalcSize(550 - mWidth, 600 - mHeight, true);
    // CalcSize subtracts the base dialog's already rounded height. Keep the
    // original 600-pixel target while retaining whole frame tiles.
    if (mHeight < 600)
    {
        const int aFrameHeight = IMAGE_DIALOG_TOPLEFT->mHeight + 45 + IMAGE_DIALOG_BOTTOMLEFT->mHeight;
        const int aHeight = aFrameHeight + MakeMultiple(600 - aFrameHeight, IMAGE_DIALOG_CENTERLEFT->mHeight);
        Resize(mX, mY, mWidth, aHeight);
    }

    // The tool tip is owned by this object; the destructor deletes it.
    mToolTipSeed = -1;
    mToolTipWidget = new ToolTipWidget();

    // The OK/Cancel buttons the base created are drawn by this dialog's own
    // Draw, so they must not draw themselves.
    mClip = false;
    mLawnYesButton->mMouseVisible = false;
    mLawnYesButton->mBtnNoDraw = true;
    mLawnNoButton->mBtnNoDraw = true;
    mLawnNoButton->mMouseVisible = false;

    mSelectedSeed = 0;
    mPlayerIndex = thePlayerIndex;

    // Take over the shared help bar for as long as the dialog is up.
    mHelpBar = mApp->mHelpBarWidget;
    mHelpBar->ClearButtons(0);
    mHelpBar->AddButton(GAMEPAD_BUTTON_A, "[SELECT]", HelpBarWidget::HELP_ALIGN_NONE);
    mHelpBar->AddButton(GAMEPAD_BUTTON_B, "[BACK]", HelpBarWidget::HELP_ALIGN_NONE);

    // HelpBarWidget's own "the intro animation already ran" flag, then the slide-in.
    mHelpBar->mDrawShade = false;
    mApp->ShowHelpBarWidget();
}

// ==========================================================================
// ImitaterDialog::RemoveToolTip()
// @0014C1F0  18 bytes
// ==========================================================================
void ImitaterDialog::RemoveToolTip()
{
    mToolTipWidget->mVisible = false;
    mToolTipSeed = -1;
}

// ==========================================================================
// ImitaterDialog::GetSeedPosition(int,int &,int &)
// @0014C374  100 bytes
// ==========================================================================
void ImitaterDialog::GetSeedPosition(int theIndex, int &theX, int &theY)
{
    // Both out-parameters are cleared before the chooser is even looked at, so a
    // missing chooser leaves the caller with (0, 0).
    theX = 0;
    theY = 0;

    SeedChooserScreen *aChooser = mApp->mSeedChooserScreen;
    if (aChooser == nullptr)
        return;

    // The seed grid is laid out relative to the middle of the dialog: 51 pixels
    // per column starting 210 left of centre, 71 pixels per row starting at 114.
    int aNumColumns = aChooser->NumColumns();
    theX = mWidth / 2 - 210 + 51 * (theIndex % aNumColumns);
    theY = 71 * (theIndex / aNumColumns) + 114;
}

// ==========================================================================
// ImitaterDialog::SeedHitTest(int,int)
// @0014C3D8  92 bytes
// ==========================================================================
int ImitaterDialog::SeedHitTest(int theX, int theY)
{
    // The chooser holds 40 pickable packets; index 48 is the imitater slot itself
    // and is therefore not a hit target.
    for (int aSeed = 0; aSeed != 40; ++aSeed)
    {
        if (!mApp->HasSeedType((SeedType)aSeed, false))
            continue;

        int aSeedX;
        int aSeedY;
        GetSeedPosition(aSeed, aSeedX, aSeedY);
        if (aSeedX <= theX && aSeedY <= theY && theX <= aSeedX + 49 && theY <= aSeedY + 69)
            return aSeed;
    }
    return -1;
}

// ==========================================================================
// ImitaterDialog::GetNextSeedInDir(int,SeedChooserScreen::SeedDir)
// @0014C434  134 bytes
// ==========================================================================
int ImitaterDialog::GetNextSeedInDir(int theIndex, SeedChooserScreen_SeedDir theDir)
{
    int aNumColumns = mApp->mSeedChooserScreen->NumColumns();

    int aRow;
    int aCol;
    if (theIndex == SEED_IMITATER)
    {
        // The imitater packet sits below the 5x8 grid of real seeds.
        aRow = 5;
        aCol = 8;
    }
    else
    {
        aRow = theIndex / aNumColumns;
        aCol = theIndex % aNumColumns;
    }

    // Clamp at the edges instead of wrapping: the caller keeps the current
    // selection when the move runs off the grid.
    switch (theDir)
    {
        case SEED_DIR_UP:
            if (aRow > 0)
                --aRow;
            break;
        case SEED_DIR_DOWN:
            if (aRow <= 3)
                ++aRow;
            break;
        case SEED_DIR_LEFT:
            if (aCol > 0)
                --aCol;
            break;
        case SEED_DIR_RIGHT:
            if (aCol <= 6)
                ++aCol;
            break;
    }

    return aCol + aNumColumns * aRow;
}

// ==========================================================================
// ImitaterDialog::UpdateCursor()
// @0014C604  76 bytes
// ==========================================================================
int ImitaterDialog::UpdateCursor()
{
    int aSeedX;
    int aSeedY;
    GetSeedPosition(mSelectedSeed, aSeedX, aSeedY);

    int aSeed = SeedHitTest(aSeedX, aSeedY);
    if (aSeed == -1 || mApp->mSeedChooserScreen->SeedNotAllowedToPick((SeedType)aSeed) != 0)
        return mApp->SetCursor(0);

    return mApp->SetCursor(1);
}

// ==========================================================================
// ImitaterDialog::ShowToolTip()
// @0014CBF0  630 bytes
// ==========================================================================
void ImitaterDialog::ShowToolTip()
{
    int aSeedX;
    int aSeedY;
    GetSeedPosition(mSelectedSeed, aSeedX, aSeedY);

    int aSeed = SeedHitTest(aSeedX, aSeedY);
    if (aSeed == -1)
    {
        RemoveToolTip();
        return;
    }

    // Rebuilding the tool tip is only worth it when the highlighted seed changed.
    if (aSeed == mToolTipSeed)
        return;

    RemoveToolTip();

    SeedChooserScreen *aChooser = mApp->mSeedChooserScreen;
    unsigned int aNotRecommended = aChooser->SeedNotRecommendedToPick((SeedType)aSeed);
    if (aChooser->SeedNotAllowedToPick((SeedType)aSeed) != 0)
        mToolTipWidget->SetWarningText("[NOT_ALLOWED_ON_THIS_LEVEL]");
    else if (aNotRecommended != 0)
    {
        // Bit 0 of the "not recommended" flag is the nocturnal-level warning.
        if ((aNotRecommended & 1) == 0)
            mToolTipWidget->SetWarningText("[NOT_RECOMMENDED_FOR_LEVEL]");
        else
            mToolTipWidget->SetWarningText("[NOCTURNAL_WARNING]");
    }
    else
        mToolTipWidget->SetWarningText("");

    mToolTipWidget->SetTitle(Plant::GetNameString(SEED_IMITATER, (SeedType)aSeed));
    mToolTipWidget->SetLabel(Plant::GetToolTip((SeedType)aSeed));

    int aToolTipX;
    int aToolTipY;
    GetSeedPosition(aSeed, aToolTipX, aToolTipY);

    // Centre the tool tip over the packet and put it 72 pixels below the packet's
    // top-left corner, in the dialog's own coordinate space.
    mToolTipWidget->mX = aToolTipX + (50 - mToolTipWidget->mWidth) / 2;
    mToolTipWidget->mY = aToolTipY + 72;
    mToolTipWidget->mVisible = true;
    mToolTipSeed = aSeed;
}

// ==========================================================================
// ImitaterDialog::MouseDown(int,int,int)
// @0014EAC0  144 bytes
// ==========================================================================
void ImitaterDialog::MouseDown(int theX, int theY, int theClickCount)
{
    if (theClickCount <= 0 || theClickCount == 3)
        return;
    int aSeed = SeedHitTest(theX, theY);
    if (aSeed == -1)
    {
        Sexy::Widget::MouseDown(theX, theY, theClickCount);
        return;
    }

    SeedChooserScreen *aChooser = mApp->mSeedChooserScreen;
    if (aChooser->SeedNotAllowedToPick((SeedType)aSeed) != 0)
        return;

    // Slot 48 of the chooser is the imitater packet. Note the store order: the
    // copy target is written before the state, then the packet is animated from
    // the chooser's own layout coordinates.
    ChosenSeedData &aImitater = aChooser->mChosenSeeds[SEED_IMITATER];
    aImitater.mImitaterType = (SeedType)aSeed;
    aImitater.mSeedState = SEED_IN_CHOOSER;
    aChooser->GetSeedPositionInChooser(SEED_IMITATER, aImitater.mX, aImitater.mY);
    aChooser->ClickedSeedInChooser(aImitater, aChooser->mMouseChoosingPlayer);
    aChooser->UpdateImitaterButton();

    // The chooser owns this child window; it is not in the app's dialog map.
    aChooser->RemoveWidget(this);
    aChooser->mImitaterDialog = nullptr;
    mApp->SafeDeleteWidget(this);
}

// ==========================================================================
// ImitaterDialog::OnKeyDown(Sexy::KeyCode,unsigned int)
// @0014EB50  314 bytes
// ==========================================================================
int ImitaterDialog::OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags)
{
    SeedChooserScreen *aChooser = mApp->mSeedChooserScreen;

    switch (theKey)
    {
        case Sexy::KEYCODE_RETURN:
            if (mSelectedSeed == -1 || aChooser->SeedNotAllowedToPick((SeedType)mSelectedSeed) != 0)
                return 0;

            {
                ChosenSeedData &aImitater = aChooser->mChosenSeeds[SEED_IMITATER];
                aImitater.mSeedState = SEED_IN_CHOOSER;
                aImitater.mImitaterType = (SeedType)mSelectedSeed;
                aChooser->GetSeedPositionInChooser(SEED_IMITATER, aImitater.mX, aImitater.mY);
                aChooser->ClickedSeedInChooser(aImitater, mApp->GamepadToPlayerIndex(mPlayerIndex));
                aChooser->UpdateImitaterButton();

                // Slot 7 of SeedChooserScreen (RemoveWidget) and slot 47 of LawnApp
                // (SafeDeleteWidget), then the chooser forgets this dialog.
                aChooser->RemoveWidget(this);
                mApp->SafeDeleteWidget(this);
                aChooser->mImitaterDialog = nullptr;
            }
            return 1;

        case Sexy::KEYCODE_ESCAPE:
            aChooser->RemoveWidget(this);
            aChooser->mImitaterDialog = nullptr;
            mApp->SafeDeleteWidget(this);
            return 1;

        case Sexy::KEYCODE_LEFT:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_LEFT);
            return 1;
        case Sexy::KEYCODE_UP:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_UP);
            return 1;
        case Sexy::KEYCODE_RIGHT:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_RIGHT);
            return 1;
        case Sexy::KEYCODE_DOWN:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_DOWN);
            return 1;

        default:
            return 0;
    }
}

// ==========================================================================
// ImitaterDialog::GameButtonDown(Sexy::GamepadButton,int,unsigned int)
// @0014EC8C  304 bytes
// ==========================================================================
void ImitaterDialog::GameButtonDown(Sexy::GamepadButton theButton, int thePlayerIndex, unsigned int theButtonFlags)
{
    // The decompiler treated the receiver as the first argument, which made the
    // body look as if it switched on the argument it compares against
    // mPlayerIndex. The disassembly of 0x14ec8c settles it: `LDR R3, [R0,#0x308]`
    // / `CMP R3, R2` compares mPlayerIndex with the SECOND argument, and
    // `CMP R1, #7` switches on the FIRST. The third (the button flags
    // MapToButtonEvent filled in) is never read -- which is exactly how the mod
    // source calls it: GameButtonDown(aButtonCode, aPlayerIndex, aButtonFlags).
    (void)theButtonFlags;

    if (mPlayerIndex != thePlayerIndex)
        return;

    SeedChooserScreen *aChooser = mApp->mSeedChooserScreen;

    switch (theButton)
    {
        case GAMEPAD_BUTTON_UP:
        case GAMEPAD_BUTTON_DPAD_UP:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_UP);
            break;
        case GAMEPAD_BUTTON_DOWN:
        case GAMEPAD_BUTTON_DPAD_DOWN:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_DOWN);
            break;
        case GAMEPAD_BUTTON_LEFT:
        case GAMEPAD_BUTTON_DPAD_LEFT:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_LEFT);
            break;
        case GAMEPAD_BUTTON_RIGHT:
        case GAMEPAD_BUTTON_DPAD_RIGHT:
            mSelectedSeed = GetNextSeedInDir(mSelectedSeed, SEED_DIR_RIGHT);
            break;

        case GAMEPAD_BUTTON_SELECT:
        case GAMEPAD_BUTTON_B:
            aChooser->RemoveWidget(this);
            aChooser->mImitaterDialog = nullptr;
            mApp->SafeDeleteWidget(this);
            break;

        case GAMEPAD_BUTTON_A:
            if (mSelectedSeed == -1 || aChooser->SeedNotAllowedToPick((SeedType)mSelectedSeed) != 0)
                break;

            {
                ChosenSeedData &aImitater = aChooser->mChosenSeeds[SEED_IMITATER];
                aImitater.mSeedState = SEED_IN_CHOOSER;
                aImitater.mImitaterType = (SeedType)mSelectedSeed;
                aChooser->GetSeedPositionInChooser(SEED_IMITATER, aImitater.mX, aImitater.mY);
                aChooser->ClickedSeedInChooser(aImitater, mApp->GamepadToPlayerIndex(mPlayerIndex));
                aChooser->UpdateImitaterButton();
                aChooser->RemoveWidget(this);
                mApp->SafeDeleteWidget(this);
                aChooser->mImitaterDialog = nullptr;
            }
            break;

        default:
            break;
    }
}

// ==========================================================================
// ImitaterDialog::KeyDown(Sexy::KeyCode)
// @0014EDBC  128 bytes
// ==========================================================================
int ImitaterDialog::KeyDown(Sexy::KeyCode theKey)
{
    // A focused child receives its parent's key event. Prefer a directly
    // delivered event, otherwise use the chooser's current event.
    Sexy::Widget *aTopLevelWidget = GetTopLevel();
    const Sexy::Event *aEvent = mEvent != nullptr ? mEvent : aTopLevelWidget->mEvent;

    if (aEvent == nullptr || aEvent->lateModifiers == 0 ||
        (mApp->HasGamepad() == 0 && (mApp->mGamePad1IsOn == 0 || mApp->mGamePad2IsOn == 0)))
    {
        // Keyboard input remains on its own path with a gamepad connected;
        // direct KeyDown calls without an event also use the raw key.
        unsigned int aModifiers = 0;
        if (aEvent != nullptr)
            aModifiers = (unsigned int)aEvent->modifierMask;

        if (OnKeyDown(theKey, aModifiers) == 0)
            return Sexy::Widget::KeyDown(theKey);
        return 1;
    }

    // Map controller input while preserving the dialog owner's gamepad index.
    int aButtonCode;
    int aPlayerIndex;
    unsigned int aButtonFlags;
    if (mApp->MapToButtonEvent(aEvent, aButtonCode, aPlayerIndex, aButtonFlags) == 0)
        return Sexy::Widget::KeyDown(theKey);

    // MapToButtonEvent hands the button code back through an `int&` (the mangled
    // name is ...ERiS4_Rj), which is also what `Sexy::GamepadButton` resolves to
    // in this tree, so no cast is needed. The dump returns 1 on this path.
    GameButtonDown(aButtonCode, aPlayerIndex, aButtonFlags);
    return 1;
}

// ==========================================================================
// ImitaterDialog::Update()
// @00151254  24 bytes
// ==========================================================================
void ImitaterDialog::Update()
{
    LawnDialog::Update();
    ShowToolTip();
    UpdateCursor();
}

// ==========================================================================
// ImitaterDialog::Draw(Sexy::Graphics *)
// @0015126C  482 bytes
// ==========================================================================
void ImitaterDialog::Draw(Sexy::Graphics *theGraphics)
{
    LawnDialog::Draw(theGraphics);
    theGraphics->SetLinearBlend(true);

    // Two passes over the 40 packets: the first draws each one unselected, the
    // second draws the selected variant. The per-pass flags are the last two
    // arguments of DrawSeedPacket.
    for (int aPass = 0; aPass != 2; ++aPass)
    {
        for (int aSeed = 0; aSeed != 40; ++aSeed)
        {
            if (!mApp->HasSeedType((SeedType)aSeed, false))
                continue;

            int aSeedX;
            int aSeedY;
            GetSeedPosition(aSeed, aSeedX, aSeedY);

            int aGrayness;
            if (mApp->mSeedChooserScreen->SeedNotAllowedToPick((SeedType)aSeed) != 0 ||
                mApp->mSeedChooserScreen->SeedNotRecommendedToPick((SeedType)aSeed) != 0)
                aGrayness = 115;
            else
                aGrayness = 255;

            DrawSeedPacket(theGraphics,
                (float)aSeedX,
                (float)aSeedY,
                SEED_IMITATER,
                (SeedType)aSeed,
                0.0f,
                aGrayness,
                true,
                false,
                aPass != 0,
                aPass == 0);
        }
    }

    if (mSelectedSeed == -1)
    {
        DeferOverlay(0);
        return;
    }

    int aSeedX;
    int aSeedY;
    GetSeedPosition(mSelectedSeed, aSeedX, aSeedY);

    // The selector frame is dimmed when the highlighted seed cannot be picked, and
    // blue for the second player.
    bool aDimmed = true;
    if (mApp->mSeedChooserScreen->SeedNotAllowedToPick((SeedType)mSelectedSeed) == 0)
        aDimmed = mApp->mSeedChooserScreen->SeedNotRecommendedToPick((SeedType)mSelectedSeed) != 0;

    Sexy::Image *aSelectorImage =
        (mPlayerIndex == mApp->mSecondPlayerGamepadIndex) ? IMAGE_SEED_SELECTOR_BLUE : IMAGE_SEED_SELECTOR;

    theGraphics->DrawImage(aSelectorImage, aSeedX - 8, aSeedY - 4, 64, 85);

    // The packet itself is drawn once more, five pixels lower, inside the frame.
    int aPacketX;
    int aPacketY;
    GetSeedPosition(mSelectedSeed, aPacketX, aPacketY);

    DrawSeedPacket(theGraphics,
        (float)aPacketX,
        (float)(aPacketY + 5),
        SEED_IMITATER,
        (SeedType)mSelectedSeed,
        0.0f,
        aDimmed ? 115 : 255,
        true,
        false,
        false,
        true);

    DeferOverlay(0);
}
