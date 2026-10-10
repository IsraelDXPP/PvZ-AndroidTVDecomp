// DaveHelp -- reconstructed from libGameMain.so.
//
// The seven words Game\DaveHelp.h names at +0x100..+0x11B carry every offset the
// transcribed bodies spelled out; each one is now a plain member read.
//
// Transcription bugs corrected here:
//   * `DaveHelp::Update` and `RemovedFromManager` were defined `int` while the
//     base (Sexy\WidgetContainer.h) declares both `void`. A mismatched override
//     makes MSVC demote the declaration and mis-size the class, so both are
//     `void` now, and the tail call into WidgetContainer::Update -- which returns
//     void -- is a plain `Sexy::WidgetContainer::Update()`.
//   * `DaveHelp::DrawDave` was defined `int`; the ARM returns the Reanimation's
//     Draw result in r0 as a float-shaped word and no caller reads it, so it is
//     `void` and the trailing `return Reanimation::Draw(...)` is a statement.
//   * the constructor's `*reinterpret_cast<_DWORD *>(this) = &off_6864F8` and the
//     destructor's matching store are the compiler's own vptr writes -- Sexy::
//     Widget is a real base now -- so both are gone, and so is the explicit
//     `~Widget()` call.
//   * `(char*)this + 1072` in the constructor was the x4 form of +268: the ARM at
//     0x1ec4bc is `STRB.W R0,[R4,#0x10C]`. DaveHelp is allocated with
//     `operator new(0x11C)`, so the 1072 spelling wrote 788 bytes past the block.
//   * `DaveEnter`'s second parameter was typed `int`; the ARM reads its vtable
//     slots (`LDR R3,[R2]` / `LDR R3,[R3,#0x18]`), so it is a WidgetContainer *.
//   * the two `VTSLOT` calls and the three hand-written raw vtable slot calls
//     become the real virtual calls the slot table names: byte 24 is slot 6
//     AddWidget, byte 32 is slot 8 HasWidget, byte 28 is slot 7 RemoveWidget,
//     byte 60 is slot 15 BringToFront, and byte 100 on the DaveHelp receiver is
//     slot 25 MarkDirty (build\_widget_vtable.md).

#include "DaveHelp.h"

#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "Reanimation.h"
#include "ReconTypes.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"

// TODO(recon): lrand48 is the Android libc's 48-bit pseudo-random generator,
// which the binary calls by name in DrawDave. Windows has no such function and
// no header in the tree declares it, so it is declared file-locally -- never in
// a header -- with its libc signature; it belongs in Runtime/AndroidShims.h
// together with an implementation.
long PlatformRandomLong();

// ==========================================================================
// DaveHelp::~DaveHelp()
// @001EC21C  24 bytes
// ==========================================================================
DaveHelp::~DaveHelp()
{
    // The ARM stores the DaveHelp vtable and then tail-calls ~Widget; with a real
    // base the compiler emits both, so the body is empty.
}

// ==========================================================================
// DaveHelp::Update()
// @001EC370  206 bytes
// ==========================================================================
void DaveHelp::Update()
{
    if (mCounter > 0)
    {
        --mCounter;
        if (mCounter == 0)
        {
            mApp->CrazyDaveEnter();
            if (mIsSingle)
            {
                switch (mSingleType)
                {
                    case 0:
                        mApp->CrazyDaveTalkIndex(7001);
                        break;
                    case 1:
                        mApp->CrazyDaveTalkIndex(7101);
                        break;
                    case 2:
                        mApp->CrazyDaveTalkIndex(5701);
                        break;
                    case 3:
                        mApp->CrazyDaveTalkIndex(7201);
                        break;
                    default:
                        break;
                }
            }
            else if (mHelpType != 0)
            {
                if (mHelpType == 1)
                    mApp->CrazyDaveTalkIndex(4101);
            }
            else
            {
                mApp->CrazyDaveTalkIndex(4001);
            }
        }
    }

    mApp->UpdateCrazyDave();

    if (mCounter == 0 && mApp->mCrazyDaveState == CRAZY_DAVE_OFF)
    {
        Sexy::WidgetContainer *aContainer = mContainer;
        mVisible = false;
        if (aContainer != nullptr)
        {
            mDrawActive = true;
            aContainer->RemoveWidget(this);
            mContainer = nullptr;
        }
    }

    Sexy::WidgetContainer::Update();
}

// ==========================================================================
// DaveHelp::DaveHelp(LawnApp *)
// @001EC488  66 bytes
// ==========================================================================
DaveHelp::DaveHelp(LawnApp *theApp)
{
    mApp = theApp;
    // TODO(recon): the dumped spelling is LawnApp::FULLSCREEN_RECT, which the
    // header declares private; the global word of the same name declared in
    // ReconData.h is the same address, so the rect at it is what the dump passes.
    Resize(LawnApp::FULLSCREEN_RECT);
    mMousePreservesFocus = true;
    mContainer = nullptr;
    mCounter = 0;
    mIsSingle = false;
    mHelpType = -1;
    mSingleType = -1;
}

// ==========================================================================
// DaveHelp::DaveEnter(E_DAVEHELP,Sexy::WidgetContainer *,bool)
// @001EC4E0  104 bytes
// ==========================================================================
int DaveHelp::DaveEnter(int theHelpType, Sexy::WidgetContainer *theContainer, bool theDrawActive)
{
    mHelpType = theHelpType;
    if (theHelpType >= 0)
    {
        mIsSingle = false;
        mVisible = true;
        if (theHelpType == 0)
            mCounter = 10;
        else if (theHelpType == 1)
            mCounter = 200;
        else
            mCounter = 1;

        mContainer = theContainer;
        if (!theContainer->HasWidget(this))
            theContainer->AddWidget(this);
        BringToFront(this);
        mDrawActive = theDrawActive;
    }
    return 0;
}

// ==========================================================================
// DaveHelp::DaveSingleEnter(E_DAVESINGLEHELP,Sexy::WidgetContainer *,bool)
// @001EC548  84 bytes
// ==========================================================================
int DaveHelp::DaveSingleEnter(E_DAVESINGLEHELP theSingleType, Sexy::WidgetContainer *theContainer, bool theDrawActive)
{
    mSingleType = theSingleType;
    mIsSingle = true;
    mVisible = true;
    switch (theSingleType)
    {
        case 0:
            mCounter = 150;
            break;
        case 1:
        case 2:
        case 3:
            mCounter = 100;
            break;
        default:
            mCounter = 1;
            break;
    }

    mContainer = theContainer;
    theContainer->AddWidget(this);
    BringToFront(this);
    mDrawActive = theDrawActive;
    return 0;
}

// ==========================================================================
// DaveHelp::DaveLeave()
// @001EC59C  8 bytes
// ==========================================================================
void DaveHelp::DaveLeave()
{
    mApp->CrazyDaveLeave();
}

void DaveHelp::MouseDown(int theX, int theY, int theClickCount)
{
    Sexy::Widget::MouseDown(theX, theY, theClickCount);
    // Board::GameButtonDown @0x15DCC4 ignores confirmation during entry/exit.
    if (theClickCount <= 0 || theClickCount == 3 || mCounter > 0 || mApp->mCrazyDaveState == CRAZY_DAVE_OFF ||
        mApp->mCrazyDaveState == CRAZY_DAVE_ENTERING || mApp->mCrazyDaveState == CRAZY_DAVE_LEAVING)
        return;
    DealClick(Sexy::KEYCODE_RETURN);
}

// ==========================================================================
// DaveHelp::DealClick(Sexy::KeyCode)
// @001EC5A4  84 bytes
// ==========================================================================
int DaveHelp::DealClick(Sexy::KeyCode theKey)
{
    if (theKey == Sexy::KEYCODE_RETURN)
    {
        if (mApp->AdvanceCrazyDaveText() == 0)
        {
            if (mApp->mCrazyDaveState != CRAZY_DAVE_OFF && mApp->mCrazyDaveState != CRAZY_DAVE_LEAVING)
            {
                mApp->CrazyDaveLeave();
                if (!mIsSingle)
                {
                    mApp->StartHelp(mHelpType);
                    // mHelpType == 1 is the "passed the shop seed tutorial" hint, so the
                    // profile records it. ARM: `LDR R2,[R4,#0x100] / LDR R2,[R2,#0x92C] /
                    // STRB.W R3,[R2,#0x732]` -- mApp->mPlayerInfo->mPassedShopSeedTutorial.
                    if (mHelpType == 1)
                        mApp->mPlayerInfo->mPassedShopSeedTutorial = true;
                }
            }
        }
    }
    return 0;
}

// ==========================================================================
// DaveHelp::DrawDave(Sexy::Graphics *)
// @001EC5F8  862 bytes
// ==========================================================================
void DaveHelp::DrawDave(Sexy::Graphics *theGraphics)
{
    Reanimation *aReanim = mApp->ReanimationTryToGet(mApp->mCrazyDaveReanimID);
    if (aReanim == nullptr)
        return;

    if (!mApp->mCrazyDaveMessageText->empty())
    {
        theGraphics->DrawImage(IMAGE_STORE_SPEECHBUBBLE2, 329, 298);

        Sexy::String aMessage(*mApp->mCrazyDaveMessageText);
        TRect<int> aTextRect;
        aTextRect.mX = 354;
        aTextRect.mY = 304;
        aTextRect.mWidth = 233;
        aTextRect.mHeight = 144;

        if (StringFind(&aMessage, "{SHAKE}", 0, 7) != (unsigned int)-1)
        {
            Sexy::String aEmpty(gEmptyCString);
            Sexy::String aReplaced;
            TodReplaceString(&aReplaced, &aMessage, "{SHAKE}", &aEmpty);
            aMessage = aReplaced;
            aTextRect.mX += PlatformRandomLong() % 2;
            aTextRect.mY += PlatformRandomLong() % 2;
        }

        // ARM at 0x1ec6f8: `CMP R3,#0x48 / BEQ loc_1EC754`, and loc_1EC754 is the
        // `MOV.W R11,#0` that makes the click prompt below skip. So the prompt is
        // shown only when the game mode is NOT 72 AND the "{NO_CLICK}" tag is absent;
        // either condition clears it.
        bool aShowClickToContinue = false;
        if (mApp->mGameMode != GAMEMODE_UPSELL)
        {
            if (StringFind(&aMessage, "{NO_CLICK}", 0, 10) == (unsigned int)-1)
            {
                aShowClickToContinue = true;
            }
            else
            {
                Sexy::String aEmpty(gEmptyCString);
                Sexy::String aReplaced;
                TodReplaceString(&aReplaced, &aMessage, "{NO_CLICK}", &aEmpty);
                aMessage = aReplaced;
            }
        }

        TodDrawStringWrapped(theGraphics, aMessage, aTextRect, FONT_BRIANNETOD16, Sexy::Color::Black, 5, 0);

        // ARM at 0x1ec782: R10 starts as FONT_DWARVENTODCRAFT12, and 0x1ec7ae
        // `CMP.W R12,#0 / BNE` replaces it with FONT_BRIANNETOD16 only when
        // StringEqualCStr(GetLocale(), "zh_CN") returned zero -- and that helper returns
        // zero exactly when the locale string EQUALS "zh_CN". So zh_CN gets
        // BrianneTod16 and every other locale gets DwarvenTodCraft12.
        Sexy::Font *aFont = FONT_DWARVENTODCRAFT12;
        if (mApp->GetLocale().compare("zh_CN") == 0)
            aFont = FONT_BRIANNETOD16;

        if (aShowClickToContinue)
        {
            Sexy::String aContinueText("[CLICK_TO_CONTINUE]");
            TodDrawString(theGraphics, aContinueText, 468, 438, aFont, Sexy::Color::Black, 2);
        }
    }

    aReanim->OverrideScale(0.8, 0.8);
    aReanim->SetPosition(100.0, 240.0);
    aReanim->Draw(theGraphics);
}

// ==========================================================================
// DaveHelp::RemovedFromManager(Sexy::WidgetManager *)
// @001ECA08  18 bytes
// ==========================================================================
void DaveHelp::RemovedFromManager(Sexy::WidgetManager *theManager)
{
    Sexy::WidgetContainer::RemovedFromManager(theManager);
    DaveLeave();
}

// ==========================================================================
// DaveHelp::Draw(Sexy::Graphics *)
// @001ECA1C  100 bytes
// ==========================================================================
void DaveHelp::Draw(Sexy::Graphics *theGraphics)
{
    if (mApp->mCrazyDaveState != CRAZY_DAVE_LEAVING && mDrawActive)
    {
        theGraphics->SetColor(Sexy::Color(0, 0, 0, 178));
        theGraphics->FillRect(0, 0, mWidth, mHeight);
    }

    Sexy::Graphics aDaveGraphics(*theGraphics);
    DrawDave(&aDaveGraphics);
}
