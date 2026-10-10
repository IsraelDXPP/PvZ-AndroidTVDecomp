// WidgetManager owns the widget tree, mouse capture, focus and the touch map.

#include "WidgetManager.h"

#include "BaseAppDriver.h"
#include "Graphics.h"
#include "Graphics3D.h"
#include "Image.h"
#include "MemoryImage.h"
#include "NativeDisplay.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include <map>
#include <vector>

namespace
{

int EventWord(const TMLongCard &theCard)
{
    return theCard.mValue;
}

void CopyEventToSample(Sexy::TouchSample *theSample, const Sexy::Event &theEvent)
{
    theSample->mModifiers = theEvent.modifiers;
    theSample->mModifierMask = EventWord(theEvent.modifierMask);
    theSample->mDevice = theEvent.lateModifiers;
    theSample->mEventType = EventWord(theEvent.eventType);
    theSample->mEventCode = EventWord(theEvent.eventCode);
    theSample->mEventCodeMask = EventWord(theEvent.eventCodeMask);
    theSample->mMatchEvent = theEvent.matchEvent;
    theSample->mStandard = theEvent.standard;
    theSample->mScaleDivisorX = EventWord(theEvent.mScaleDivisorX);
    theSample->mScaleDivisorY = EventWord(theEvent.mScaleDivisorY);
    theSample->mSavedModifierMask = EventWord(theEvent.mSavedModifierMask);
    theSample->mSavedEventCodeMask = EventWord(theEvent.mSavedEventCodeMask);
    theSample->mSavedMatchEvent = theEvent.mSavedMatchEvent;
}

void CopySample(Sexy::TouchSample *theDest, const Sexy::TouchSample &theSource)
{
    *theDest = theSource;
}

// Touch position lives in the previous sample's scale-Y and saved-mask words.
int TouchPointX(const Sexy::TouchInfo &theInfo)
{
    return theInfo.mPrevious.mScaleDivisorY;
}

int TouchPointY(const Sexy::TouchInfo &theInfo)
{
    return theInfo.mPrevious.mSavedModifierMask;
}

struct TouchRelease
{
    int mId;
    int mX;
    int mY;
    bool mStillLive;
};

// Shared by TouchUp and TouchCancel. A finger that is no longer down is adopted
// as the live id; when none remains, the held widget is released.
TouchRelease AdoptReleasedTouch(Sexy::WidgetManager *theManager)
{
    TouchRelease aRelease;
    aRelease.mId = theManager->mActiveTouchId;
    aRelease.mX = theManager->mLastMouseX;
    aRelease.mY = theManager->mLastMouseY;
    aRelease.mStillLive = false;

    Sexy::TouchInfo *aInfo = theManager->GetTouchInfo(theManager->mActiveTouchId);
    if (aInfo != nullptr && aInfo->State() > 1)
    {
        std::map<int, Sexy::TouchInfo>::iterator aTouch = theManager->mTouchInfo.mInfo.begin();
        std::map<int, Sexy::TouchInfo>::iterator aEnd = theManager->mTouchInfo.mInfo.end();
        while (aTouch != aEnd && aTouch->second.State() > 1)
            ++aTouch;
        if (aTouch != aEnd)
        {
            int aOldId = theManager->mActiveTouchId;
            theManager->mActiveTouchId = aTouch->first;
            theManager->mLastMouseX = TouchPointX(aTouch->second);
            theManager->mLastMouseY = TouchPointY(aTouch->second);
            aRelease.mStillLive = aOldId == theManager->mActiveTouchId;
            aRelease.mId = theManager->mActiveTouchId;
            aRelease.mX = theManager->mLastMouseX;
            aRelease.mY = theManager->mLastMouseY;
        }
    }

    if (!aRelease.mStillLive)
    {
        theManager->mActiveTouchId = -1;
        theManager->mActualDownButtons &= ~1;
    }
    return aRelease;
}

void FinishTouchRelease(Sexy::WidgetManager *theManager)
{
    if (theManager->mActiveTouchId >= 0)
        return;

    if (theManager->mLastDownWidget != nullptr && (theManager->mDownButtons & 1) != 0)
    {
        Sexy::Widget *aOver = theManager->mOverWidget;
        theManager->mDownButtons &= ~1;
        if (aOver != nullptr)
        {
            theManager->TouchLeave(aOver);
            theManager->mOverWidget = nullptr;
            if (theManager->mLastDownWidget == nullptr)
                return;
        }
        else
        {
            theManager->mOverWidget = nullptr;
        }
        theManager->mLastDownWidget->mIsDown = false;
        theManager->mLastDownWidget = nullptr;
        return;
    }
    theManager->mDownButtons &= ~1;
}

Sexy::Widget *FirstChild(Sexy::WidgetContainer *theContainer)
{
    if (theContainer->mWidgets.mNext == &theContainer->mWidgets)
        return nullptr;
    return static_cast<Sexy::WidgetListNode *>(theContainer->mWidgets.mNext)->mWidget;
}

Sexy::Widget *FirstFocusableChild(Sexy::WidgetContainer *theContainer)
{
    for (Sexy::WidgetListLink *aNode = theContainer->mWidgets.mNext; aNode != &theContainer->mWidgets;
        aNode = aNode->mNext)
    {
        Sexy::Widget *aWidget = static_cast<Sexy::WidgetListNode *>(aNode)->mWidget;
        if (aWidget->IsFocusable())
            return aWidget;
    }
    return nullptr;
}

// Focus, else the modal widget, else the first focusable child after focusing it.
Sexy::Widget *KeyTarget(Sexy::WidgetManager *theManager)
{
    if (theManager->mFocusWidget != nullptr)
        return theManager->mFocusWidget;
    if (theManager->mBaseModalWidget != nullptr)
        return theManager->mBaseModalWidget;

    Sexy::Widget *aChild = FirstFocusableChild(theManager);
    if (aChild != nullptr)
        theManager->SetFocus(aChild);
    if (theManager->mFocusWidget != nullptr)
        return theManager->mFocusWidget;
    return theManager->mBaseModalWidget;
}

} // namespace

Sexy::TouchSample::TouchSample() :
    mModifiers(0),
    mModifierMask(0),
    mDevice(0),
    mEventType(0),
    mEventCode(-1),
    mEventCodeMask(0),
    mMatchEvent(2),
    mStandard(0),
    mScaleDivisorX(0),
    mScaleDivisorY(0),
    mSavedModifierMask(0),
    mSavedEventCodeMask(0),
    mSavedMatchEvent(0)
{
}

Sexy::TouchInfo::TouchInfo() :
    mCurrent(),
    mPrevious(),
    mBegin()
{
}

Sexy::KeyState *Sexy::KeyDownMap::Find(int theKey)
{
    std::map<int, KeyState>::iterator aFound = mDown.find(theKey);
    if (aFound == mDown.end())
        return nullptr;
    return &aFound->second;
}

const Sexy::KeyState *Sexy::KeyDownMap::Find(int theKey) const
{
    std::map<int, KeyState>::const_iterator aFound = mDown.find(theKey);
    if (aFound == mDown.end())
        return nullptr;
    return &aFound->second;
}

Sexy::KeyState *Sexy::KeyDownMap::Get(int theKey)
{
    return &mDown[theKey];
}

Sexy::TouchInfo *Sexy::TouchInfoMap::Find(int theKey)
{
    std::map<int, TouchInfo>::iterator aFound = mInfo.find(theKey);
    if (aFound == mInfo.end())
        return nullptr;
    return &aFound->second;
}

const Sexy::TouchInfo *Sexy::TouchInfoMap::Find(int theKey) const
{
    std::map<int, TouchInfo>::const_iterator aFound = mInfo.find(theKey);
    if (aFound == mInfo.end())
        return nullptr;
    return &aFound->second;
}

Sexy::TouchInfo &Sexy::TouchInfoMap::Get(int theKey)
{
    return mInfo[theKey];
}

int *Sexy::GamepadKeyMap::Find(int theKey)
{
    std::map<int, int>::iterator aFound = mKeys.find(theKey);
    if (aFound == mKeys.end())
        return nullptr;
    return &aFound->second;
}

const int *Sexy::GamepadKeyMap::Find(int theKey) const
{
    std::map<int, int>::const_iterator aFound = mKeys.find(theKey);
    if (aFound == mKeys.end())
        return nullptr;
    return &aFound->second;
}

int &Sexy::GamepadKeyMap::Get(int theKey)
{
    return mKeys[theKey];
}

void Sexy::WidgetManager::SetFocus(Sexy::Widget *theWidget)
{
    Sexy::Widget *aFocusWidget = mFocusWidget;
    if (aFocusWidget == theWidget)
        return;

    if (theWidget == nullptr || theWidget->mWidgetManager != this)
    {
        if (aFocusWidget != nullptr)
            aFocusWidget->SetWindowFocus(false);
        mFocusWidget = nullptr;
        return;
    }

    Sexy::Widget *aTopLevelWidget = theWidget->GetTopLevel();
    mFocusWidget = aTopLevelWidget;

    if (aFocusWidget != nullptr)
    {
        if (mHasFocus && aFocusWidget == aTopLevelWidget)
        {
            aFocusWidget->SetWindowFocus(true);
            mFocusWidget->SetFocus(theWidget, false);
            return;
        }
        aFocusWidget->SetWindowFocus(false);
    }

    if (!mHasFocus)
        return;

    Sexy::Widget *aWidget = mFocusWidget;
    if (aWidget == nullptr)
        return;

    aWidget->SetWindowFocus(true);
    if (theWidget != aTopLevelWidget)
    {
        mFocusWidget->SetFocus(theWidget, false);
        return;
    }

    aWidget = mFocusWidget;
    if (!aWidget->mHasFocus)
        aWidget->GotFocus();
}

void Sexy::WidgetManager::FreeResources() {}

int Sexy::WidgetManager::GetWidgetFlags()
{
    if (mHasFocus)
        return mWidgetFlags;
    return (mLostFocusFlagsMod.mAddFlags | mWidgetFlags) & ~mLostFocusFlagsMod.mRemoveFlags;
}

Widget *Sexy::WidgetManager::GetAnyWidgetAt(int x, int y, int *theWidgetX, int *theWidgetY)
{
    bool aFound = false;
    return GetWidgetAtHelper(x, y, GetWidgetFlags(), &aFound, theWidgetX, theWidgetY);
}

Widget *Sexy::WidgetManager::GetWidgetAt(int x, int y, int *theWidgetX, int *theWidgetY)
{
    Widget *aWidget = GetAnyWidgetAt(x, y, theWidgetX, theWidgetY);
    if (aWidget != nullptr && aWidget->mDisabled)
        return nullptr;
    return aWidget;
}

int Sexy::WidgetManager::IsLeftButtonDown()
{
    return mActualDownButtons & 1;
}

bool Sexy::WidgetManager::IsMiddleButtonDown()
{
    return (mActualDownButtons & 4) != 0;
}

bool Sexy::WidgetManager::IsRightButtonDown()
{
    return (mActualDownButtons & 2) != 0;
}

int Sexy::WidgetManager::DoMouseUps(Sexy::Widget *theWidget, unsigned long theDownCode)
{
    int aClickCounts[3];
    aClickCounts[0] = 1;
    aClickCounts[1] = -1;
    aClickCounts[2] = 3;
    for (int aButton = 0; aButton != 3; ++aButton)
    {
        if (((1u << aButton) & theDownCode) == 0)
            continue;
        theWidget->mIsDown = false;
        theWidget->MouseUp(mLastMouseX - theWidget->mX, mLastMouseY - theWidget->mY, aClickCounts[aButton]);
    }
    return 1;
}

void Sexy::WidgetManager::DoMouseUps()
{
    Sexy::Widget *aWidget = mLastDownWidget;
    if (aWidget == nullptr || mDownButtons == 0)
        return;
    DoMouseUps(aWidget, (unsigned long)mDownButtons);
    mDownButtons = 0;
    mLastDownWidget = nullptr;
}

int Sexy::WidgetManager::RemapMouse(int &theX, int &theY)
{
    theX = mMouseDestRect.mWidth * (theX - mMouseSourceRect.mX) / mMouseSourceRect.mWidth + mMouseDestRect.mX;
    theY = mMouseDestRect.mHeight * (theY - mMouseSourceRect.mY) / mMouseSourceRect.mHeight + mMouseDestRect.mY;
    return theY;
}

Sexy::WidgetManager *Sexy::WidgetManager::RemapMouse(float &theX, float &theY)
{
    theX = ((theX - (float)mMouseSourceRect.mX) * (float)mMouseDestRect.mWidth) / (float)mMouseSourceRect.mWidth +
        (float)mMouseDestRect.mX;
    theY = ((theY - (float)mMouseSourceRect.mY) * (float)mMouseDestRect.mHeight) / (float)mMouseSourceRect.mHeight +
        (float)mMouseDestRect.mY;
    return this;
}

int Sexy::WidgetManager::UnmapMouse(int &theX, int &theY)
{
    theX = mMouseSourceRect.mWidth * (theX - mMouseDestRect.mX) / mMouseDestRect.mWidth + mMouseSourceRect.mX;
    theY = mMouseSourceRect.mHeight * (theY - mMouseDestRect.mY) / mMouseDestRect.mHeight + mMouseSourceRect.mY;
    return theY;
}

void Sexy::WidgetManager::MouseEnter(Sexy::Widget *theWidget)
{
    theWidget->mIsOver = true;
    theWidget->MouseEnter();
    if (theWidget->mDoFinger)
        theWidget->ShowFinger(true);
}

void Sexy::WidgetManager::MouseLeave(Sexy::Widget *theWidget)
{
    theWidget->mIsOver = false;
    theWidget->MouseLeave();
    if (theWidget->mDoFinger)
        theWidget->ShowFinger(false);
}

void Sexy::WidgetManager::SetBaseModal(Sexy::Widget *theWidget, Sexy::FlagsMod const &theFlags)
{
    Sexy::Widget *aOverWidget = mOverWidget;
    mBaseModalWidget = theWidget;
    mBelowModalFlagsMod = theFlags;

    if (aOverWidget != nullptr && (mBelowModalFlagsMod.mRemoveFlags & 0x10) != 0)
    {
        if (IsBelow(aOverWidget, theWidget))
        {
            Sexy::Widget *aLeaving = mOverWidget;
            mOverWidget = nullptr;
            MouseLeave(aLeaving);
        }
    }

    if (mLastDownWidget != nullptr && (mBelowModalFlagsMod.mRemoveFlags & 0x10) != 0)
    {
        if (IsBelow(mLastDownWidget, theWidget))
        {
            Sexy::Widget *aDown = mLastDownWidget;
            unsigned int aButtons = (unsigned int)mDownButtons;
            mDownButtons = 0;
            mLastDownWidget = nullptr;
            DoMouseUps(aDown, aButtons);
        }
    }

    if (mFocusWidget != nullptr && (mBelowModalFlagsMod.mRemoveFlags & 0x20) != 0)
    {
        if (IsBelow(mFocusWidget, theWidget))
        {
            Sexy::Widget *aFocus = mFocusWidget;
            mFocusWidget = nullptr;
            aFocus->GetTopLevel()->SetWindowFocus(false);
        }
    }

    if (theWidget != nullptr)
        theWidget->SetWindowFocus(true);
}

void Sexy::WidgetManager::AddBaseModal(Sexy::Widget *theWidget, Sexy::FlagsMod const &theFlags)
{
    PreModalInfo aInfo;
    aInfo.mWidget = theWidget;
    aInfo.mBaseModalWidget = mBaseModalWidget;
    aInfo.mFocusWidget = mFocusWidget;
    aInfo.mFlagsMod = mBelowModalFlagsMod;
    mPreModalInfoList.push_back(aInfo);
    SetBaseModal(theWidget, theFlags);
}

void Sexy::WidgetManager::AddBaseModal(Sexy::Widget *theWidget)
{
    AddBaseModal(theWidget, mDefaultBelowModalFlagsMod);
}

void Sexy::WidgetManager::RemoveBaseModal(Sexy::Widget *theWidget)
{
    bool aFirst = true;
    while (!mPreModalInfoList.empty())
    {
        PreModalInfo aInfo = mPreModalInfoList.back();
        if (aFirst && aInfo.mWidget != theWidget)
            break;
        aFirst = false;

        bool aStop = aInfo.mBaseModalWidget != nullptr || mPreModalInfoList.size() == 1;
        SetBaseModal(aInfo.mBaseModalWidget, aInfo.mFlagsMod);

        if (mFocusWidget == nullptr)
        {
            mFocusWidget = aInfo.mFocusWidget;
            if (mFocusWidget != nullptr)
            {
                mFocusWidget->SetWindowFocus(true);
                mFocusWidget->GotFocus();
            }
        }

        mPreModalInfoList.pop_back();
        if (aStop)
            break;
    }
}

int Sexy::WidgetManager::Resize(TRect<int> const &theDest, TRect<int> const &theSource)
{
    mWidth = theDest.mWidth + 2 * theDest.mX;
    mHeight = theDest.mHeight + 2 * theDest.mY;
    mMouseDestRect = theDest;
    mMouseSourceRect = theSource;
    mX = -theDest.mX;
    mY = -mMouseDestRect.mY;
    return theSource.mX;
}

void Sexy::WidgetManager::GotFocus()
{
    if (mHasFocus)
        return;
    Sexy::Widget *aFocusWidget = mFocusWidget;
    mHasFocus = true;
    if (aFocusWidget == nullptr)
        return;
    aFocusWidget->SetWindowFocus(true);
    mFocusWidget->GotFocus();
}

int Sexy::WidgetManager::InitModalFlags(Sexy::ModalFlags *theFlags)
{
    theFlags->mIsOver = mBaseModalWidget == nullptr;
    int aFlags = GetWidgetFlags();
    theFlags->mOverFlags = aFlags;
    theFlags->mUnderFlags = (aFlags | mBelowModalFlagsMod.mAddFlags) & ~mBelowModalFlagsMod.mRemoveFlags;
    return theFlags->mUnderFlags;
}

bool Sexy::WidgetManager::UpdateFrame()
{
    Sexy::ModalFlags aFlags;
    InitModalFlags(&aFlags);
    int aCount = mUpdateCnt + 1;
    mUpdateCnt = aCount;
    mLastWMUpdateCount = aCount;
    UpdateAll(&aFlags);
    return mDirty;
}

bool Sexy::WidgetManager::UpdateFrameF(float theFrac)
{
    Sexy::ModalFlags aFlags;
    InitModalFlags(&aFlags);
    UpdateFAll(&aFlags, theFrac);
    return mDirty;
}

void Sexy::WidgetManager::SetPopupCommandWidget(Sexy::Widget *theWidget)
{
    mPopupCommandWidget = theWidget;
    AddWidget(theWidget);
}

void Sexy::WidgetManager::RemovePopupCommandWidget()
{
    if (mPopupCommandWidget == nullptr)
        return;
    Sexy::Widget *aWidget = mPopupCommandWidget;
    mPopupCommandWidget = nullptr;
    RemoveWidget(aWidget);
}

int Sexy::WidgetManager::MousePosition(int x, int y)
{
    int aOldX = mLastMouseX;
    int aOldY = mLastMouseY;
    int aWidgetX = 0;
    int aWidgetY = 0;
    mLastMouseX = x;
    mLastMouseY = y;

    Sexy::Widget *aHit = GetWidgetAt(x, y, &aWidgetX, &aWidgetY);
    Sexy::Widget *aOver = mOverWidget;
    if (aHit == aOver)
    {
        if ((aOldY != y || aOldX != x) && aHit != nullptr)
            aHit->MouseMove(aWidgetX, aWidgetY);
    }
    else
    {
        mOverWidget = nullptr;
        if (aOver != nullptr)
            MouseLeave(aOver);
        mOverWidget = aHit;
        if (aHit != nullptr)
        {
            MouseEnter(aHit);
            aHit->MouseMove(aWidgetX, aWidgetY);
        }
    }
    return 1;
}

int Sexy::WidgetManager::RehupMouse()
{
    if (mLastDownWidget != nullptr)
    {
        if (mOverWidget != nullptr)
        {
            Sexy::Widget *aWidgetAt = GetWidgetAt(mLastMouseX, mLastMouseY, nullptr, nullptr);
            if (mOverWidget != aWidgetAt)
            {
                Sexy::Widget *aLeaving = mOverWidget;
                mOverWidget = nullptr;
                MouseLeave(aLeaving);
                return 1;
            }
        }
    }
    else if (mMouseIn)
    {
        return MousePosition(mLastMouseX, mLastMouseY);
    }
    return 1;
}

int Sexy::WidgetManager::MouseUp(int x, int y, int theClickCount)
{
    mInteractiveMethod = 3;
    mLastInputUpdateCnt = mUpdateCnt;

    int aClearMask;
    int aButtonBit;
    if (theClickCount < 0)
    {
        aClearMask = -3;
        aButtonBit = 2;
    }
    else if (theClickCount == 3)
    {
        aClearMask = -5;
        aButtonBit = 4;
    }
    else
    {
        aClearMask = -2;
        aButtonBit = 1;
    }

    int aButtons = mDownButtons;
    Sexy::Widget *aDown = mLastDownWidget;
    mActualDownButtons &= aClearMask;
    int aRemaining = aClearMask & aButtons;
    mDownButtons = aRemaining;

    if (aDown != nullptr && (aButtonBit & aButtons) != 0)
    {
        if (aRemaining == 0)
            mLastDownWidget = nullptr;
        aDown->mIsDown = false;
        Point aAbsPos = aDown->GetAbsPos();
        aDown->MouseUp(x - aAbsPos.mX, y - aAbsPos.mY, theClickCount);
    }
    MousePosition(x, y);
    return 1;
}

int Sexy::WidgetManager::MouseDown(int x, int y, int theClickCount)
{
    int aWidgetX = 0;
    int aWidgetY = 0;
    mInteractiveMethod = 3;
    mLastInputUpdateCnt = mUpdateCnt;

    if (theClickCount < 0)
        mActualDownButtons |= 2;
    else if (theClickCount == 3)
        mActualDownButtons |= 4;
    else
        mActualDownButtons |= 1;

    MousePosition(x, y);

    Sexy::Widget *aPopup = mPopupCommandWidget;
    if (aPopup != nullptr && !aPopup->Contains(x, y))
        RemovePopupCommandWidget();

    Sexy::Widget *aWidgetAt = GetWidgetAt(x, y, &aWidgetX, &aWidgetY);
    Sexy::Widget *aDown = mLastDownWidget;
    int aButtons = mDownButtons;
    if (aDown == nullptr)
        aDown = aWidgetAt;
    else
    {
        Point aAbsPos = aDown->GetAbsPos();
        aWidgetX = x - aAbsPos.mX;
        aWidgetY = y - aAbsPos.mY;
    }

    if (theClickCount < 0)
    {
        mLastDownButtonId = -1;
        mDownButtons = aButtons | 2;
    }
    else if (theClickCount == 3)
    {
        mLastDownButtonId = 2;
        mDownButtons = aButtons | 4;
    }
    else
    {
        mLastDownButtonId = 1;
        mDownButtons = aButtons | 1;
    }

    mLastDownWidget = aDown;
    if (aDown != nullptr)
    {
        if (!aDown->mMousePreservesFocus)
            SetFocus(aDown);
        aDown->mIsDown = true;
        aDown->MouseDown(aWidgetX, aWidgetY, theClickCount);
    }
    return 1;
}

int Sexy::WidgetManager::MouseDrag(int x, int y)
{
    Sexy::Widget *aOver = mOverWidget;
    mLastInputUpdateCnt = mUpdateCnt;
    mLastMouseX = x;
    mLastMouseY = y;
    mMouseIn = true;
    Sexy::Widget *aDown = mLastDownWidget;

    if (aOver != nullptr && aOver != aDown)
    {
        mOverWidget = nullptr;
        MouseLeave(aOver);
        aDown = mLastDownWidget;
    }

    if (aDown == nullptr)
        return 1;

    Point aAbsPos = aDown->GetAbsPos();
    mLastDownWidget->MouseDrag(x - aAbsPos.mX, y - aAbsPos.mY);

    Sexy::Widget *aWidgetAt = GetWidgetAt(x, y, nullptr, nullptr);
    if (aWidgetAt == mLastDownWidget && aWidgetAt != nullptr)
    {
        if (mOverWidget == nullptr)
        {
            mOverWidget = aWidgetAt;
            MouseEnter(aWidgetAt);
        }
    }
    else if (mOverWidget != nullptr)
    {
        Sexy::Widget *aLeaving = mOverWidget;
        mOverWidget = nullptr;
        MouseLeave(aLeaving);
    }
    return 1;
}

int Sexy::WidgetManager::MouseMove(int x, int y)
{
    mLastInputUpdateCnt = mUpdateCnt;
    if (mDownButtons != 0 && Sexy::GetEnvOption("SEXY_NO_DRAG", false) == 0)
        return MouseDrag(x, y);
    mMouseIn = true;
    MousePosition(x, y);
    return 1;
}

int Sexy::WidgetManager::MouseExit(int x, int y)
{
    (void)x;
    (void)y;
    Sexy::Widget *aOver = mOverWidget;
    mMouseIn = false;
    mLastInputUpdateCnt = mUpdateCnt;
    if (aOver != nullptr)
    {
        MouseLeave(aOver);
        mOverWidget = nullptr;
    }
    return 1;
}

void Sexy::WidgetManager::MouseWheel(int x, int y)
{
    mLastInputUpdateCnt = mUpdateCnt;
    if (mOverWidget != nullptr)
    {
        mOverWidget->MouseWheel(x, y);
        return;
    }
    if (mFocusWidget != nullptr)
        mFocusWidget->MouseWheel(x, y);
}

void Sexy::WidgetManager::MouseWheel(int theDelta)
{
    MouseWheel(0, theDelta);
}

void Sexy::WidgetManager::TouchEnter(Sexy::Widget *theWidget)
{
    theWidget->mIsOver = true;
    theWidget->TouchEnter();
}

void Sexy::WidgetManager::TouchLeave(Sexy::Widget *theWidget)
{
    theWidget->mIsOver = false;
    theWidget->TouchLeave();
}

Sexy::WidgetManager *Sexy::WidgetManager::UpdateTouchInfo(const std::vector<Sexy::Event> &theEvents)
{
    for (unsigned int i = 0; i < theEvents.size(); ++i)
    {
        const Sexy::Event &aEvent = theEvents[i];
        TouchInfo &aRecord = mTouchInfo.Get(EventWord(aEvent.eventCodeMask));

        if (aEvent.matchEvent == 0)
            CopyEventToSample(&aRecord.mCurrent, aEvent);
        CopySample(&aRecord.mPrevious, aRecord.mCurrent);
        CopyEventToSample(&aRecord.mCurrent, aEvent);
        if (aEvent.matchEvent == 0)
            CopyEventToSample(&aRecord.mBegin, aEvent);
    }
    return this;
}

void Sexy::WidgetManager::TouchDown(const std::vector<Sexy::Event> &theEvents)
{
    mInteractiveMethod = 4;
    mLastInputUpdateCnt = mUpdateCnt;
    UpdateTouchInfo(theEvents);

    if (mActiveTouchId < 0 && !theEvents.empty())
        mActiveTouchId = EventWord(theEvents[0].eventCodeMask);

    TouchInfo *aInfo = GetTouchInfo(mActiveTouchId);
    int aX = TouchPointX(*aInfo);
    int aY = TouchPointY(*aInfo);
    mActualDownButtons |= 1;
    mLastMouseX = aX;
    mLastMouseY = aY;

    Sexy::Widget *aWidget = mLastDownWidget;
    if (aWidget == nullptr)
    {
        aWidget = GetWidgetAt(aX, aY, nullptr, nullptr);
        mOverWidget = aWidget;
        mLastDownWidget = aWidget;
        if (aWidget == nullptr)
        {
            mLastDownButtonId = 1;
            mDownButtons |= 1;
            return;
        }
        TouchEnter(aWidget);
    }

    mLastDownButtonId = 1;
    mDownButtons |= 1;
    if ((!aWidget->mHasFocus || !aWidget->mHasWindowFocus) && aWidget->WantsFocus())
        SetFocus(aWidget);
    aWidget->mIsDown = true;
    aWidget->TouchEvents(theEvents);
}

void Sexy::WidgetManager::TouchMove(const std::vector<Sexy::Event> &theEvents)
{
    mInteractiveMethod = 4;
    mLastInputUpdateCnt = mUpdateCnt;
    UpdateTouchInfo(theEvents);

    TouchInfo *aInfo = GetTouchInfo(mActiveTouchId);
    int aX = TouchPointX(*aInfo);
    int aY = TouchPointY(*aInfo);
    mLastMouseX = aX;
    mLastMouseY = aY;

    if (mLastDownWidget == nullptr)
        return;

    Sexy::Widget *aWidgetAt = GetWidgetAt(aX, aY, nullptr, nullptr);
    mLastDownWidget->TouchEvents(theEvents);

    if (aWidgetAt == mLastDownWidget && aWidgetAt != nullptr)
    {
        if (mOverWidget == nullptr)
        {
            mOverWidget = aWidgetAt;
            TouchEnter(aWidgetAt);
        }
        return;
    }
    if (mOverWidget != nullptr)
    {
        Sexy::Widget *aOld = mOverWidget;
        mOverWidget = nullptr;
        TouchLeave(aOld);
    }
}

void Sexy::WidgetManager::TouchUp(const std::vector<Sexy::Event> &theEvents)
{
    mInteractiveMethod = 4;
    mLastInputUpdateCnt = mUpdateCnt;
    UpdateTouchInfo(theEvents);
    AdoptReleasedTouch(this);
    if (mLastDownWidget != nullptr)
        mLastDownWidget->TouchEvents(theEvents);
    FinishTouchRelease(this);
}

void Sexy::WidgetManager::TouchCancel(const std::vector<Sexy::Event> &theEvents)
{
    UpdateTouchInfo(theEvents);
    AdoptReleasedTouch(this);
    if (mLastDownWidget != nullptr)
        mLastDownWidget->TouchEvents(theEvents);
    FinishTouchRelease(this);
}

void Sexy::WidgetManager::ConvertTouchEvents(
    const std::vector<Sexy::Event> &theEvents, std::vector<Sexy::Touch> &theTouches)
{
    theTouches.clear();
    for (unsigned int i = 0; i < theEvents.size(); ++i)
    {
        const Event &anEvent = theEvents[i];
        TouchInfo *aInfo = GetTouchInfo(EventWord(anEvent.eventCodeMask));

        Touch aTouch;
        aTouch.mX = (float)(unsigned int)EventWord(anEvent.eventCode);
        aTouch.mY = (float)(unsigned int)EventWord(anEvent.mScaleDivisorX);
        aTouch.mPhase = 0;
        aTouch.mActive = 1;

        bool aUnseeded = aInfo->mPrevious.mModifiers == 0 || aInfo->mPrevious.mMatchEvent == 2;
        if (aUnseeded)
        {
            aTouch.mPrevX = aTouch.mX;
            aTouch.mPrevY = aTouch.mY;
        }
        else
        {
            aTouch.mPrevX = (float)(unsigned int)aInfo->mPrevious.mScaleDivisorY;
            aTouch.mPrevY = (float)(unsigned int)aInfo->mPrevious.mSavedModifierMask;
        }

        aTouch.mId = anEvent.standard;
        aTouch.mTime = (double)(unsigned int)EventWord(anEvent.eventType) * 0.001;

        switch (anEvent.modifiers)
        {
            case 0:
                aTouch.mPhase = 0;
                break;
            case 1:
                aTouch.mPhase = 1;
                break;
            case 2:
                aTouch.mPhase = 3;
                break;
            case 3:
                aTouch.mPhase = 4;
                break;
            default:
                break;
        }
        theTouches.push_back(aTouch);
    }
}

void Sexy::WidgetManager::DisableWidget(Sexy::Widget *theWidget)
{
    if (mOverWidget == theWidget)
    {
        mOverWidget = nullptr;
        if (!mTouchInfo.mInfo.empty())
            TouchLeave(theWidget);
        else
            MouseLeave(theWidget);
    }

    if (mLastDownWidget == theWidget)
    {
        mLastDownWidget = nullptr;
        DoMouseUps(theWidget, (unsigned long)mDownButtons);
        mDownButtons = 0;
    }

    if (mFocusWidget == theWidget)
    {
        mFocusWidget = nullptr;
        theWidget->SetWindowFocus(false);
    }
    else if (theWidget->mHasFocus)
    {
        theWidget->LostFocus();
    }

    if (mBaseModalWidget == theWidget)
        mBaseModalWidget = nullptr;
}

void Sexy::WidgetManager::AxisMoved(Sexy::Event const &theEvent)
{
    mInteractiveMethod = 1;
    mLastInputUpdateCnt = mUpdateCnt;

    if (mFocusWidget != nullptr)
    {
        mFocusWidget->AxisMoved(theEvent);
        return;
    }
    if (mBaseModalWidget != nullptr)
    {
        mBaseModalWidget->AxisMoved(theEvent);
        return;
    }

    Sexy::Widget *aChild = FirstChild(this);
    if (aChild == nullptr)
        return;
    SetFocus(aChild);
    Sexy::Widget *aTarget = mFocusWidget != nullptr ? mFocusWidget : mBaseModalWidget;
    if (aTarget != nullptr)
        aTarget->AxisMoved(theEvent);
}

int Sexy::WidgetManager::UserEvent(Sexy::Event const &theEvent)
{
    mLastInputUpdateCnt = mUpdateCnt;
    if (mFocusWidget != nullptr)
        mFocusWidget->UserEvent(theEvent);
    return 1;
}

int Sexy::WidgetManager::GetInteractiveMethod()
{
    return mInteractiveMethod;
}

bool Sexy::WidgetManager::IsInKeyboardMode()
{
    return (unsigned int)(mInteractiveMethod - 1) <= 1;
}

void Sexy::WidgetManager::SetAllowFullKeyboardMode(bool allow)
{
    mAllowFullKeyboardMode = allow;
}

bool Sexy::WidgetManager::AllowFullKeyboardMode()
{
    return mAllowFullKeyboardMode;
}

Rect Sexy::WidgetManager::GetScreenRect() const
{
    return Rect(0, 0, mMouseDestRect.mWidth, mMouseDestRect.mHeight);
}

Sexy::WidgetManager *Sexy::WidgetManager::DeferOverlay(Sexy::Widget *theWidget, int thePriority)
{
    mDeferredOverlayWidgets.push_back(std::pair<Sexy::Widget *, int>(theWidget, thePriority));
    if (mMinDeferredOverlayPriority > thePriority)
        mMinDeferredOverlayPriority = thePriority;
    return this;
}

Sexy::TouchInfo *Sexy::WidgetManager::GetTouchInfo(int theId)
{
    return &mTouchInfo.Get(theId);
}

bool Sexy::WidgetManager::IsTouchDown(int theId)
{
    return GetTouchInfo(theId)->State() <= 1;
}

int Sexy::WidgetManager::KeyChar(char theChar)
{
    mLastInputUpdateCnt = mUpdateCnt;
    if (theChar == '\t' && mKeyDown.Get(17)->mDown)
        return 1;

    Sexy::Widget *aTarget = mFocusWidget != nullptr ? mFocusWidget : mBaseModalWidget;
    if (aTarget != nullptr)
        aTarget->KeyChar(theChar);
    return 1;
}

int Sexy::WidgetManager::KeyUnicode(int theChar)
{
    mLastInputUpdateCnt = mUpdateCnt;
    if (theChar == 9 && mKeyDown.Get(17)->mDown)
        return 1;

    Sexy::Widget *aTarget = mFocusWidget != nullptr ? mFocusWidget : mBaseModalWidget;
    if (aTarget == nullptr || aTarget->KeyUnicode(theChar) == 0)
    {
        wchar_t aWide[2];
        aWide[0] = (wchar_t)theChar;
        aWide[1] = 0;
        Sexy::String aNarrow;
        WStringToString(&aNarrow, aWide);
        for (unsigned int i = 0; i < aNarrow.length(); ++i)
            KeyChar(aNarrow[i]);
    }
    return 1;
}

int Sexy::WidgetManager::KeyDown(Sexy::KeyCode theKey)
{
    bool aKeepMethod = theKey == 18 || theKey == 27;
    mLastInputUpdateCnt = mUpdateCnt;
    if (!aKeepMethod)
        mInteractiveMethod = 1;

    if ((unsigned int)theKey <= 0xFE)
        mKeyDown.Get(theKey)->mDown = true;

    Sexy::Widget *aTarget = KeyTarget(this);
    if (aTarget != nullptr && aTarget->KeyDown(theKey) != 0)
        return 1;

    int *aMapped = mGamepadKeyMap.Find(theKey);
    if (aMapped == nullptr)
        return 1;
    return KeyDown((Sexy::KeyCode)*aMapped);
}

int Sexy::WidgetManager::KeyUp(Sexy::KeyCode theKey)
{
    bool aKeepMethod = theKey == 18 || theKey == 27;
    mLastInputUpdateCnt = mUpdateCnt;
    if (!aKeepMethod)
        mInteractiveMethod = 1;

    if ((unsigned int)theKey <= 0xFE)
    {
        mKeyDown.Get(theKey)->mDown = false;
        if (theKey == 9 && mKeyDown.Get(17)->mDown)
            return 1;
    }

    Sexy::Widget *aTarget = KeyTarget(this);
    if (aTarget != nullptr && aTarget->KeyUp(theKey) != 0)
        return 1;

    int *aMapped = mGamepadKeyMap.Find(theKey);
    if (aMapped == nullptr)
        return 1;
    return KeyUp((Sexy::KeyCode)*aMapped);
}

void Sexy::WidgetManager::LostFocus()
{
    if (!mHasFocus)
        return;

    mActualDownButtons = 0;
    bool aReleased = false;
    for (std::map<int, KeyState>::iterator aKey = mKeyDown.mDown.begin(); aKey != mKeyDown.mDown.end() && !aReleased;
        ++aKey)
    {
        while (aKey != mKeyDown.mDown.end() && !aKey->second.mDown)
            ++aKey;
        if (aKey == mKeyDown.mDown.end())
        {
            aReleased = true;
            break;
        }
        KeyUp((Sexy::KeyCode)aKey->first);
    }

    Sexy::Widget *aFocusWidget = mFocusWidget;
    mHasFocus = false;
    if (aFocusWidget != nullptr)
        aFocusWidget->SetWindowFocus(false);
}

int Sexy::WidgetManager::KeyUp(Sexy::Event const &theEvent)
{
    int aCode = EventWord(theEvent.eventCodeMask);
    bool aKeepMethod = aCode == 18 || aCode == 27;
    mLastInputUpdateCnt = mUpdateCnt;
    if (!aKeepMethod)
        mInteractiveMethod = 1;
    if ((unsigned int)aCode <= 0xFE)
        mKeyDown.Get(aCode)->mDown = false;

    Sexy::Widget *aTarget = KeyTarget(this);
    if (aTarget != nullptr && aTarget->KeyUp(theEvent) != 0)
        return 1;

    // WidgetManager::KeyUp(Event) follows KeyDown(Event) @0x3B38FA. A miss
    // returns. A hit writes the mapped code back into the event and calls
    // this function again.
    int *aMapped = mGamepadKeyMap.Find(aCode);
    if (aMapped == nullptr)
        return 1;

    Sexy::Event aTranslated = theEvent;
    aTranslated.eventCodeMask = *aMapped;
    return KeyUp(aTranslated);
}

int Sexy::WidgetManager::KeyDown(Sexy::Event const &theEvent)
{
    int aCode = EventWord(theEvent.eventCodeMask);
    bool aKeepMethod = aCode == 18 || aCode == 27;
    mLastInputUpdateCnt = mUpdateCnt;
    if (!aKeepMethod)
        mInteractiveMethod = 1;
    if ((unsigned int)aCode <= 0xFE)
        mKeyDown.Get(aCode)->mDown = true;

    Sexy::Widget *aTarget = KeyTarget(this);
    if (aTarget != nullptr && aTarget->KeyDown(theEvent) != 0)
        return 1;

    // WidgetManager::KeyDown(Event) @0x3B38FA. Look the code up in the map at
    // this+0x14C. A miss returns. A hit writes that value into event+0x14 and
    // calls KeyDown(Event) again at 0x3B3982.
    int *aMapped = mGamepadKeyMap.Find(aCode);
    if (aMapped == nullptr)
        return 1;

    Sexy::Event aTranslated = theEvent;
    aTranslated.eventCodeMask = *aMapped;
    return KeyDown(aTranslated);
}

void Sexy::WidgetManager::InitGamepadKeyMap()
{
    mGamepadKeyMap.Get(304) = 13;
    mGamepadKeyMap.Get(305) = 27;
    mGamepadKeyMap.Get(560) = 37;
    mGamepadKeyMap.Get(561) = 38;
    mGamepadKeyMap.Get(562) = 39;
    mGamepadKeyMap.Get(563) = 40;
    mGamepadKeyMap.Get(564) = 37;
    mGamepadKeyMap.Get(565) = 38;
    mGamepadKeyMap.Get(566) = 39;
    mGamepadKeyMap.Get(567) = 40;
}

Sexy::WidgetManager::WidgetManager(Sexy::SexyAppBase *theApp)
{
    mCurG = nullptr;
    mTransientImage = nullptr;
    mLastInputUpdateCnt = 0;
    mLostFocusFlagsMod.mAddFlags = 0;
    mLostFocusFlagsMod.mRemoveFlags = 0;
    mBelowModalFlagsMod.mAddFlags = 0;
    mBelowModalFlagsMod.mRemoveFlags = 0;
    mDefaultBelowModalFlagsMod.mAddFlags = 0;

    mMouseIn = false;
    mWidgetId = 0;
    mImage = nullptr;
    mLastHadTransients = false;
    mPopupCommandWidget = nullptr;
    mFocusWidget = nullptr;
    mLastDownWidget = nullptr;
    mOverWidget = nullptr;
    mBaseModalWidget = nullptr;
    mWidth = 0;
    mHeight = 0;
    mUpdateCnt = 0;
    mLastDownButtonId = 0;
    mDownButtons = 0;
    mActualDownButtons = 0;
    mLastMouseX = 0;
    mLastMouseY = 0;
    mInteractiveMethod = 0;
    mAllowFullKeyboardMode = false;
    mMinDeferredOverlayPriority = 0x7FFFFFFF;
    mApp = theApp;
    mWidgetManager = this;
    mDefaultBelowModalFlagsMod.mRemoveFlags = 48;
    mHasFocus = true;
    mWidgetFlags = 61;
    mActiveTouchId = -1;

    for (int aKey = 0; aKey != 255; ++aKey)
        mKeyDown.Get(aKey)->mDown = false;
    InitGamepadKeyMap();
}

int Sexy::WidgetManager::FlushDeferredOverlayWidgets(int theMaxPriority)
{
    int aIndex = 0;
    int aNextPriority = 0x7FFFFFFF;
    int aDrew = 0;
    int aCount = (int)mDeferredOverlayWidgets.size();

    while (true)
    {
        while (aIndex < aCount)
        {
            Sexy::Widget *aWidget = mDeferredOverlayWidgets[aIndex].first;
            if (aWidget != nullptr)
            {
                int aPriority = mDeferredOverlayWidgets[aIndex].second;
                if (mMinDeferredOverlayPriority == aPriority)
                {
                    Sexy::Graphics aGraphics(*mCurG);
                    aGraphics.Translate(-mMouseDestRect.mX, -mMouseDestRect.mY);
                    aGraphics.Translate(aWidget->mX, aWidget->mY);
                    aGraphics.SetFastStretch(!aGraphics.mLinearBlend);
                    aGraphics.SetLinearBlend(aGraphics.mLinearBlend);
                    mDeferredOverlayWidgets[aIndex].first = nullptr;
                    aWidget->DrawOverlay(&aGraphics, aPriority);
                    aDrew = 1;
                    aCount = (int)mDeferredOverlayWidgets.size();
                }
                else if (aNextPriority >= aPriority)
                {
                    aNextPriority = aPriority;
                }
            }
            ++aIndex;
        }

        mMinDeferredOverlayPriority = aNextPriority;
        if (aNextPriority == 0x7FFFFFFF)
            break;
        if (aNextPriority >= theMaxPriority)
            return aDrew;

        aIndex = 0;
        aNextPriority = 0x7FFFFFFF;
    }

    if (aCount != 0)
        mDeferredOverlayWidgets.clear();
    return aDrew;
}

void Sexy::WidgetManager::DrawWidgetsTo(Sexy::Graphics *g)
{
    g->Translate(mMouseDestRect.mX, mMouseDestRect.mY);

    Sexy::Graphics aGraphics(*g);
    mCurG = &aGraphics;

    std::vector<std::pair<Sexy::Widget *, int>> aSaved = mDeferredOverlayWidgets;
    mDeferredOverlayWidgets.clear();

    Sexy::ModalFlags aFlags;
    InitModalFlags(&aFlags);
    g->Translate(-mMouseDestRect.mX, -mMouseDestRect.mY);

    for (WidgetListLink *aNode = mWidgets.mNext; aNode != &mWidgets; aNode = aNode->mNext)
    {
        Widget *aWidget = static_cast<WidgetListNode *>(aNode)->mWidget;
        if (!aWidget->mVisible)
            continue;
        g->PushState();
        g->SetFastStretch(true);
        g->Translate(aWidget->mX, aWidget->mY);
        aWidget->DrawAll(&aFlags, g);
        g->PopState();
    }

    FlushDeferredOverlayWidgets(0x7FFFFFFF);
    mDeferredOverlayWidgets = aSaved;
    mCurG = nullptr;
}

int Sexy::WidgetManager::DrawScreen()
{
    Sexy::ModalFlags aFlags;
    InitModalFlags(&aFlags);

    int aDirty = 0;
    for (WidgetListLink *aNode = mWidgets.mNext; aNode != &mWidgets; aNode = aNode->mNext)
    {
        Widget *aWidget = static_cast<WidgetListNode *>(aNode)->mWidget;
        if (aWidget->mDirty && aWidget->mVisible)
        {
            aDirty = 1;
            break;
        }
    }

    mMinDeferredOverlayPriority = 0x7FFFFFFF;
    if (!mDeferredOverlayWidgets.empty())
        mDeferredOverlayWidgets.clear();

    Sexy::Graphics aFrame(mImage);
    mCurG = &aFrame;

    int aCursorX = mLastMouseX;
    int aCursorY = mLastMouseY;
    UnmapMouse(aCursorX, aCursorY);

    bool aCursorMoved = mApp->mAppDriver->UpdateCursor(aCursorX, aCursorY) != 0 && !mApp->mYieldMainThread;
    if (aCursorMoved || aDirty != 0)
    {
        Rect aClip(-mMouseDestRect.mX, -mMouseDestRect.mY, mWidth, mHeight);
        Sexy::Graphics aGraphics(aFrame);
        if (mClip)
            aGraphics.ClipRect(aClip);
        aGraphics.Translate(-mMouseDestRect.mX, -mMouseDestRect.mY);

        bool aAccelerated = mApp->Is3DAccelerated() != 0;
        Rect aClear(0, 0, 1, 1);
        Sexy::Graphics3D *aGraphics3D = aGraphics.Get3D();
        if (aGraphics3D != nullptr)
        {
            aGraphics3D->ClearColorBuffer(Sexy::Color::Black);
            aGraphics3D->ClearDepthBuffer();
        }
        else
        {
            aClear.mWidth = mImage->GetWidth();
            aClear.mHeight = mImage->GetHeight();
        }
        aFrame.FillRect(aClear);

        bool aDrewWidget = false;
        for (WidgetListLink *aNode = mWidgets.mNext; aNode != &mWidgets; aNode = aNode->mNext)
        {
            Widget *aWidget = static_cast<WidgetListNode *>(aNode)->mWidget;
            if (mWidgetManager->mBaseModalWidget == aWidget)
                aFlags.mIsOver = true;
            if (!aWidget->mDirty || !aWidget->mVisible)
                continue;
            aGraphics.PushState();
            aGraphics.SetFastStretch(!aAccelerated);
            aGraphics.SetLinearBlend(aAccelerated);
            aGraphics.Translate(aWidget->mX, aWidget->mY);
            aWidget->DrawAll(&aFlags, &aGraphics);
            aWidget->mDirty = false;
            aGraphics.PopState();
            aDrewWidget = true;
        }

        aDirty = FlushDeferredOverlayWidgets(0x7FFFFFFF) | (aDrewWidget ? 1 : 0);
        aGraphics.Translate(mMouseDestRect.mX, mMouseDestRect.mY);

        bool aInside = !mClip ||
            (aCursorX >= aClip.mX && aCursorX < aClip.mX + aClip.mWidth && aCursorY >= aClip.mY &&
                aCursorY < aClip.mY + aClip.mHeight);
        if (aDirty != 0 && aInside && !mApp->mYieldMainThread)
            mApp->mPlatformDriverOrQueue->DrawCursor(&aGraphics);
        mCurG = nullptr;
    }

    return aDirty;
}

Sexy::WidgetManager::~WidgetManager()
{
    FreeResources();
    mPreModalInfoList.clear();
}
