// ReanimTrackWidgetAnimator -- transcribed from libGameMain.so
// Rewritten by hand in stage 2; see build/stage2-brief.md.

#include "ReanimTrackWidgetAnimator.h"

#include "GameButton.h"
#include "LawnApp.h"
#include "LawnMenuWidget.h"
#include "Reanimation.h"
#include "ReanimatorTransform.h"
#include "Widget.h"

// ==========================================================================
// ReanimTrackWidgetAnimator::~ReanimTrackWidgetAnimator()
// @001B8A5C  60 bytes
// ==========================================================================
ReanimTrackWidgetAnimator::~ReanimTrackWidgetAnimator()
{
    // The binary releases the three track-name strings by hand -- the three
    // refcount decrements the dump spells out at words 8, 9 and 10. They are
    // Sexy::String members here, so their own destructors do exactly that and
    // there is nothing left to write. The vptr stores around them are the
    // compiler's.
}

// ==========================================================================
// ReanimTrackWidgetAnimator::IsCompleted()
// @001B8A1C  4 bytes
// ==========================================================================
bool ReanimTrackWidgetAnimator::IsCompleted()
{
    return false;
}

// ==========================================================================
// ReanimTrackWidgetAnimator::Init(Sexy::WidgetAnimationState)
// @001B8A20  60 bytes
// ==========================================================================
// The base declares the state parameter; this override ignores it and simply
// points the three named tracks at their render groups.
void ReanimTrackWidgetAnimator::Init(Sexy::WidgetAnimationState theState)
{
    LawnMenuWidget *aMenu = (LawnMenuWidget *)mLawnMenuWidget;

    Reanimation *aReanim = mApp->ReanimationTryToGet(aMenu->mBackdropReanimID);
    if (aReanim == nullptr)
        return;

    aReanim->AssignRenderGroupToTrack(mTrackName1.c_str(), -1);
    aReanim->AssignRenderGroupToTrack(mTrackName2.c_str(), -1);
    aReanim->AssignRenderGroupToTrack(mTrackName3.c_str(), -1);
}

// ==========================================================================
// ReanimTrackWidgetAnimator::Update()
// @001B8BE0  396 bytes
// ==========================================================================
// The widget follows one of three named reanim tracks -- the third while it is
// held down, the second while the pointer is over it, the first otherwise --
// and the current transform of that track is added to the menu's own offset
// and to this animator's stored offset to place the widget.
void ReanimTrackWidgetAnimator::Update()
{
    if (mWidget == nullptr)
        return;

    if (mLawnMenuWidget == nullptr)
        return;

    LawnMenuWidget *aMenu = (LawnMenuWidget *)mLawnMenuWidget;
    Reanimation *aReanim = mApp->ReanimationTryToGet(aMenu->mBackdropReanimID);
    if (aReanim == nullptr)
        return;

    Sexy::String aTrackName = mTrackName1;

    GameButton *aButton = dynamic_cast<GameButton *>(mWidget);
    if (aButton != nullptr)
    {
        if (!mTrackName2.empty() && aButton->mIsOver)
            aTrackName = mTrackName2;
        else if (!mTrackName3.empty() && mWidget->mIsDown)
            aTrackName = mTrackName3;
    }

    int aTrackIndex = aReanim->FindTrackIndex(aTrackName.c_str());
    if (aTrackIndex == -1)
        return;

    ReanimatorTransform aTransform;
    aReanim->GetCurrentTransform(aTrackIndex, &aTransform);

    float aX = (aMenu->mCameraOffsetX + aTransform.mTransX) + mOffsetX;
    float aY = (aMenu->mCameraOffsetY + aTransform.mTransY) + mOffsetY;

    mWidget->Move((int)aX, (int)aY);

    if (aButton != nullptr)
    {
        aButton->mAnchorX = aX;
        aButton->mAnchorY = aY;
        aButton->mUseFixedOrigin = true;
    }

    // The binary stores these two OUTSIDE the guard above (0x1b8cc0 branches over
    // the three stores but rejoins before these), so a widget that is not a
    // GameButton dereferences null here exactly as the original does.
    aButton->mScaleX = aTransform.mScaleX;
    aButton->mScaleY = aTransform.mScaleY;
}

// ==========================================================================
// ReanimTrackWidgetAnimator::ReanimTrackWidgetAnimator(Sexy::Widget *,LawnMenuWidget *,Sexy::String,Sexy::String,Sexy::String)
// @001B8D8C  120 bytes
// ==========================================================================
// The dump also stores the class vptr, seeds the three strings with the shared
// empty rep and releases the by-value arguments; the compiler emits all of that
// from the declarations. mHideOnCompletion (+0x14) is the one base field the
// binary leaves untouched.
ReanimTrackWidgetAnimator::ReanimTrackWidgetAnimator(Sexy::Widget *theWidget, LawnMenuWidget *theMenu,
    Sexy::String theTrackName1, Sexy::String theTrackName2, Sexy::String theTrackName3)
{
    mWidget = theWidget;
    mRefCount = 0;
    mUpdateCounter = 0;
    mLawnMenuWidget = theMenu;
    mOffsetX = 0.0f;
    mOffsetY = 0.0f;
    mTrackName1 = theTrackName1;
    mTrackName2 = theTrackName2;
    mTrackName3 = theTrackName3;
    mApp = (LawnApp *)gSexyAppBase;
}
