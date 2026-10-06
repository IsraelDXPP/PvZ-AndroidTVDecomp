// Widget is a focusable mouse, touch and key target. It draws nothing by default.

#include "Widget.h"

#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "ReconExterns.h"
#include "SexyAppBase.h"
#include "SharedImageRef.h"
#include "WidgetAnimator.h"
#include "WidgetContainer.h"
#include "WidgetManager.h"
#include <algorithm>
#include <vector>

char Sexy::Widget::DefaultAddToManager = 0;

namespace {

void ReleaseAnimator(Sexy::WidgetAnimator *theAnimator)
{
    if (theAnimator != nullptr && --theAnimator->mRefCount <= 0)
        delete theAnimator;
}

void ToWidgetSpace(Sexy::Touch *theTouch, const Sexy::Point &theAbsPos)
{
    theTouch->mX -= (float)theAbsPos.mX;
    theTouch->mY -= (float)theAbsPos.mY;
    theTouch->mPrevX -= (float)theAbsPos.mX;
    theTouch->mPrevY -= (float)theAbsPos.mY;
}

Sexy::Touch *FindLiveTouch(std::vector<Sexy::Touch> &theTouches, int theId)
{
    for (unsigned int i = 0; i < theTouches.size(); ++i)
    {
        if (theTouches[i].mId == theId)
            return &theTouches[i];
    }
    return nullptr;
}

// The first child that both has focus and can take it.
Sexy::Widget *FocusedChild(const std::vector<Sexy::Widget *> &theChildren)
{
    for (unsigned int i = 0; i < theChildren.size(); ++i)
    {
        Sexy::Widget *aChild = theChildren[i];
        if (aChild->mHasFocus && aChild->IsFocusable())
            return aChild;
    }
    return nullptr;
}

bool HasFocusLink(Sexy::Widget *theWidget)
{
    return theWidget->mFocusLinks[0] != nullptr
        || theWidget->mFocusLinks[1] != nullptr
        || theWidget->mFocusLinks[2] != nullptr
        || theWidget->mFocusLinks[3] != nullptr;
}

// Walk a focus-link chain. The first on-screen focusable widget in this
// container's z-order vector is the answer. A cycle stops the walk.
Sexy::Widget *FollowFocusLink(std::vector<Sexy::Widget *> &theChildren,
                              Sexy::Widget *theRelative, int theLink)
{
    Sexy::Widget *aFirst = theRelative->mFocusLinks[theLink];
    Sexy::Widget *aCurrent = aFirst;
    while (aCurrent != nullptr && aCurrent != theRelative)
    {
        if (aCurrent->IsFocusable() && aCurrent->IsOnScreen())
        {
            std::vector<Sexy::Widget *>::iterator aFound =
                std::find(theChildren.begin(), theChildren.end(), aCurrent);
            if (aFound != theChildren.end())
                return *aFound;
        }
        aCurrent = aCurrent->mFocusLinks[theLink];
        if (aCurrent == aFirst)
            break;
    }
    return nullptr;
}

// Attached shared images live in the user-data map under this key. The list
// object is the sentinel; each node holds one SharedImageRef.
const char *kSharedImageKey = "sharedimageref";

struct WidgetSharedImageNode
{
    WidgetSharedImageNode *mNext;
    WidgetSharedImageNode *mPrev;
    Sexy::SharedImageRef mRef;

    WidgetSharedImageNode()
        : mNext(nullptr)
        , mPrev(nullptr)
        , mRef()
    {
    }

    explicit WidgetSharedImageNode(Sexy::SharedImageRef &theRef)
        : mNext(nullptr)
        , mPrev(nullptr)
        , mRef(theRef)
    {
    }
};

struct WidgetSharedImageList : Sexy::WidgetUserData
{
    WidgetSharedImageNode *mNext;
    WidgetSharedImageNode *mPrev;

    WidgetSharedImageList()
        : mNext(nullptr)
        , mPrev(nullptr)
    {
    }
};

WidgetSharedImageList *SharedImageList(Sexy::Widget *theWidget, bool theCreate)
{
    Sexy::String aKey = kSharedImageKey;
    WidgetSharedImageList *aList = static_cast<WidgetSharedImageList *>(theWidget->GetUserData(aKey));
    if (aList == nullptr && theCreate)
    {
        aList = new WidgetSharedImageList;
        WidgetSharedImageNode *aHeader = new WidgetSharedImageNode;
        aHeader->mNext = aHeader;
        aHeader->mPrev = aHeader;
        aList->mNext = aHeader;
        aList->mPrev = aHeader;
        theWidget->AddUserData(aKey, aList);
    }
    return aList;
}

void ClearSharedImageNodes(WidgetSharedImageList *theList)
{
    WidgetSharedImageNode *aHeader = theList->mNext;
    WidgetSharedImageNode *aNode = aHeader->mNext;
    while (aNode != aHeader)
    {
        WidgetSharedImageNode *aNext = aNode->mNext;
        delete aNode;
        aNode = aNext;
    }
    aHeader->mNext = aHeader;
    aHeader->mPrev = aHeader;
}

} // namespace

void Sexy::Widget::MouseWheel(int x, int y)
{
    MouseWheel(y / 100);
    (void)x;
}

void Sexy::Widget::OrderInManagerChanged() {}

bool Sexy::Widget::IsPointVisible(int x, int y)
{
    (void)x;
    (void)y;
    return true;
}

void Sexy::Widget::Draw(Sexy::Graphics *g) { (void)g; }

void Sexy::Widget::DrawOther(Sexy::Graphics *g) { (void)g; }

void Sexy::Widget::DrawOverlay(Sexy::Graphics *g) { (void)g; }

void Sexy::Widget::DrawOverlay(Sexy::Graphics *g, int thePriority)
{
    // @0x3AE744 forwards deferred overlays to the widget's drawing override.
    (void)thePriority;
    DrawOverlay(g);
}

void Sexy::Widget::Resize(TRect<int> const &theRect)
{
    Resize(theRect.mX, theRect.mY, theRect.mWidth, theRect.mHeight);
}

void Sexy::Widget::Move(int theNewX, int theNewY)
{
    Resize(theNewX, theNewY, mWidth, mHeight);
}

bool Sexy::Widget::WantsFocus()
{
    return mWantsFocus;
}

bool Sexy::Widget::HasWindowFocus()
{
    return mHasWindowFocus;
}

void Sexy::Widget::GotWindowFocus()
{
    mHasWindowFocus = true;
}

void Sexy::Widget::LostWindowFocus()
{
    mHasWindowFocus = false;
}

void Sexy::Widget::UpdateF(float theFrac)
{
    (void)theFrac;
}

void Sexy::Widget::SetFocusLink(WidgetFocusDir theDir, Sexy::Widget *theWidget)
{
    mFocusLinks[theDir] = theWidget;
}

void Sexy::Widget::SetFocusLinks(Sexy::Widget *up, Sexy::Widget *down, Sexy::Widget *left, Sexy::Widget *right)
{
    mFocusLinks[0] = up;
    mFocusLinks[1] = down;
    mFocusLinks[2] = left;
    mFocusLinks[3] = right;
}

void Sexy::Widget::FocusDirectionHint(WidgetFocusDir theDir)
{
    (void)theDir;
}

int Sexy::Widget::KeyDown(Sexy::Event const &theEvent)
{
    // Widget::KeyDown(Event) @0x3AE804. Store the event, dispatch
    // eventCodeMask through KeyDown(KeyCode), then clear it. The return is
    // what WidgetManager uses to decide whether the pad map still runs.
    mEvent = const_cast<Sexy::Event *>(&theEvent);
    int aResult = KeyDown((Sexy::KeyCode)(int)theEvent.eventCodeMask);
    mEvent = nullptr;
    return aResult;
}

int Sexy::Widget::KeyUp(Sexy::Event const &theEvent)
{
    // Widget::KeyUp(Event) @0x3AE820. Same shape as KeyDown.
    mEvent = const_cast<Sexy::Event *>(&theEvent);
    int aResult = KeyUp((Sexy::KeyCode)(int)theEvent.eventCodeMask);
    mEvent = nullptr;
    return aResult;
}

void Sexy::Widget::MouseEnter() {}

void Sexy::Widget::MouseLeave() {}

void Sexy::Widget::MouseMove(int x, int y)
{
    (void)x;
    (void)y;
}

void Sexy::Widget::MouseDown(int x, int y, int theBtnNum, int theClickCount)
{
    (void)x;
    (void)y;
    (void)theBtnNum;
    (void)theClickCount;
}

void Sexy::Widget::MouseUp(int x, int y)
{
    (void)x;
    (void)y;
}

void Sexy::Widget::MouseUp(int x, int y, int theBtnNum, int theClickCount)
{
    (void)x;
    (void)y;
    (void)theBtnNum;
    (void)theClickCount;
}

void Sexy::Widget::MouseDrag(int x, int y)
{
    (void)x;
    (void)y;
}

void Sexy::Widget::MouseWheel(int theDelta)
{
    (void)theDelta;
}

void Sexy::Widget::TouchEnter() {}

void Sexy::Widget::TouchLeave() {}

void Sexy::Widget::TouchDown(int theId, int x, int y) { (void)theId; (void)x; (void)y; }

void Sexy::Widget::TouchUp(int theId, int x, int y) { (void)theId; (void)x; (void)y; }

void Sexy::Widget::TouchCancel(int theId, int x, int y) { (void)theId; (void)x; (void)y; }

void Sexy::Widget::TouchBegan(Touch *theTouch) { (void)theTouch; }

void Sexy::Widget::TouchMoved(Touch *theTouch) { (void)theTouch; }

void Sexy::Widget::TouchEnded(Touch *theTouch) { (void)theTouch; }

void Sexy::Widget::TouchesCanceled() {}

void Sexy::Widget::TouchCanceled(Touch *theTouch) { (void)theTouch; }

void Sexy::Widget::TouchBegan(std::vector<Touch> &theTouches)
{
    if (mLiveTouchId >= 0 || theTouches.empty())
        return;

    Touch *aTouch = &theTouches[0];
    mLiveTouchId = aTouch->mId;
    ToWidgetSpace(aTouch, GetAbsPos());
    TouchBegan(aTouch);
}

void Sexy::Widget::TouchMoved(std::vector<Touch> &theTouches)
{
    Touch *aTouch = FindLiveTouch(theTouches, mLiveTouchId);
    if (aTouch == nullptr)
        return;
    ToWidgetSpace(aTouch, GetAbsPos());
    TouchMoved(aTouch);
}

void Sexy::Widget::TouchEnded(std::vector<Touch> &theTouches)
{
    Touch *aTouch = FindLiveTouch(theTouches, mLiveTouchId);
    if (aTouch == nullptr)
        return;
    ToWidgetSpace(aTouch, GetAbsPos());
    TouchEnded(aTouch);
    mLiveTouchId = -1;
}

void Sexy::Widget::TouchCanceled(std::vector<Touch> &theTouches)
{
    Touch *aTouch = FindLiveTouch(theTouches, mLiveTouchId);
    if (aTouch == nullptr)
        return;
    ToWidgetSpace(aTouch, GetAbsPos());
    TouchCanceled(aTouch);
    TouchesCanceled();
    mLiveTouchId = -1;
}

void Sexy::Widget::UserEvent(Sexy::Event const &theEvent)
{
    (void)theEvent;
}

void Sexy::Widget::TouchEvents(const std::vector<Sexy::Event> &theEvents)
{
    if (theEvents.empty())
        return;

    switch (theEvents[0].matchEvent)
    {
    case 0:
        TouchDown(theEvents);
        return;
    case 1:
        TouchMove(theEvents);
        return;
    case 2:
        TouchUp(theEvents);
        return;
    case 3:
        TouchCancel(theEvents);
        return;
    default:
        return;
    }
}

void Sexy::Widget::TouchDown(const std::vector<Sexy::Event> &theEvents)
{
    std::vector<Touch> aTouches;
    mWidgetManager->ConvertTouchEvents(theEvents, aTouches);
    TouchBegan(aTouches);
}

void Sexy::Widget::TouchMove(const std::vector<Sexy::Event> &theEvents)
{
    std::vector<Touch> aTouches;
    mWidgetManager->ConvertTouchEvents(theEvents, aTouches);
    TouchMoved(aTouches);
}

void Sexy::Widget::TouchUp(const std::vector<Sexy::Event> &theEvents)
{
    std::vector<Touch> aTouches;
    mWidgetManager->ConvertTouchEvents(theEvents, aTouches);
    TouchEnded(aTouches);
}

void Sexy::Widget::TouchCancel(const std::vector<Sexy::Event> &theEvents)
{
    std::vector<Touch> aTouches;
    mWidgetManager->ConvertTouchEvents(theEvents, aTouches);
    TouchCanceled(aTouches);
}

int Sexy::Widget::GetNumDigits(int number)
{
    int aCount = 1;
    if (number > 9)
    {
        for (int aScale = 10; aScale <= number; aScale *= 10)
            ++aCount;
    }
    return aCount;
}

bool Sexy::Widget::Contains(int x, int y)
{
    if (mX > x)
        return false;
    if (x >= mX + mWidth)
        return false;
    return mY <= y && y < mHeight + mY;
}

TRect<int> Sexy::Widget::GetInsetRect()
{
    return TRect<int>(mX + mMouseInsets.mLeft,
                      mY + mMouseInsets.mTop,
                      mWidth - mMouseInsets.mLeft - mMouseInsets.mRight,
                      mHeight - mMouseInsets.mTop - mMouseInsets.mBottom);
}

void Sexy::Widget::SetColors(int (*theColors)[3], int theNumColors)
{
    mColors.clear();
    for (int i = 0; i < theNumColors; ++i)
        SetColor(i, Color(theColors[i][0], theColors[i][1], theColors[i][2]));
    MarkDirty();
}

void Sexy::Widget::SetColors(int (*theColors)[4], int theNumColors)
{
    mColors.clear();
    for (int i = 0; i < theNumColors; ++i)
        SetColor(i, Color(theColors[i][0], theColors[i][1], theColors[i][2], theColors[i][3]));
    MarkDirty();
}

void Sexy::Widget::SetVisible(bool isVisible)
{
    if (mVisible == isVisible)
        return;
    mVisible = isVisible;
    if (isVisible)
        MarkDirty();
    else
        MarkDirtyFull();
    if (mWidgetManager != nullptr)
        mWidgetManager->RehupMouse();
}

void Sexy::Widget::Resize(int theX, int theY, int theWidth, int theHeight)
{
    if (mX == theX && mY == theY && mWidth == theWidth && mHeight == theHeight)
        return;
    MarkDirtyFull();
    mX = theX;
    mY = theY;
    mWidth = theWidth;
    mHeight = theHeight;
    MarkDirty();
    if (mWidgetManager != nullptr)
        mWidgetManager->RehupMouse();
    SortWidgets();
    if (mParent != nullptr)
        mParent->SortWidgets();
}

Color Sexy::Widget::GetColor(int theIdx)
{
    static Color aDefaultColor;
    if (theIdx >= (int)mColors.size())
        return aDefaultColor;
    return mColors[theIdx];
}

void Sexy::Widget::SetDisabled(bool isDisabled)
{
    if (mDisabled == isDisabled)
        return;
    mDisabled = isDisabled;
    if (isDisabled)
    {
        if (mWidgetManager != nullptr)
            mWidgetManager->DisableWidget(this);
        MarkDirty();
        return;
    }
    MarkDirty();
    if (mWidgetManager != nullptr
        && Contains(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY))
        mWidgetManager->MousePosition(mWidgetManager->mLastMouseX, mWidgetManager->mLastMouseY);
}

void Sexy::Widget::ShowFinger(bool on)
{
    if (mWidgetManager != nullptr)
        mWidgetManager->mApp->SetCursor(on);
}

TRect<int> Sexy::Widget::WriteCenteredLine(Sexy::Graphics *g, int y, Sexy::String const &text)
{
    Font *aFont = g->GetFont();
    int aWidth = aFont->StringWidth(text);
    int aX = (mWidth - aWidth) / 2;
    g->DrawString(text, aX, y);
    return TRect<int>(aX, y - aFont->GetAscent(), aWidth, aFont->GetHeight());
}

TRect<int> Sexy::Widget::WriteCenteredLine(Sexy::Graphics *g, int y, Sexy::String const &text,
                                           Color color1, Color color2, const TPoint<int> &offset)
{
    Font *aFont = g->GetFont();
    int aWidth = aFont->StringWidth(text);
    int aX = (mWidth - aWidth) / 2;

    g->SetColor(color2);
    g->DrawString(text, aX + offset.mX, y + offset.mY);
    g->SetColor(color1);
    g->DrawString(text, aX, y);

    int aRectX = offset.mX > 0 ? aX : aX + offset.mX;
    int aRectY = y - aFont->GetAscent() + (offset.mY > 0 ? 0 : offset.mY);
    int aRectWidth = aWidth + (offset.mX < 0 ? -offset.mX : offset.mX);
    int aRectHeight = aFont->GetHeight() + (offset.mY < 0 ? -offset.mY : offset.mY);
    return TRect<int>(aRectX, aRectY, aRectWidth, aRectHeight);
}

void Sexy::Widget::WriteString(Sexy::Graphics *g, Sexy::String const &text, int x, int y,
                               int width, int justification, bool drawString, int offset, int length)
{
    bool aSaved = g->mWriteColoredString;
    g->mWriteColoredString = mWriteColoredString;
    g->WriteString(text, x, y, width, justification, drawString, offset, length, -1);
    g->mWriteColoredString = aSaved;
}

void Sexy::Widget::WriteWordWrapped(Sexy::Graphics *g, TRect<int> const &rect, Sexy::String const &text,
                                    int lineSpacing, int justification)
{
    bool aSaved = g->mWriteColoredString;
    g->mWriteColoredString = mWriteColoredString;
    g->WriteWordWrapped(rect, text, lineSpacing, justification, 0, -1, 0);
    g->mWriteColoredString = aSaved;
}

int Sexy::Widget::GetWordWrappedHeight(Sexy::Graphics *g, int width, Sexy::String const &text, int lineSpacing)
{
    return g->GetWordWrappedHeight(width, text, lineSpacing, 0);
}

void Sexy::Widget::WriteNumberFromStrip(Sexy::Graphics *g, int number, int x, int y,
                                        Sexy::Image *image, int digitSpacing)
{
    int aNumDigits = 1;
    int aDivisor = 10;
    if (number > 9)
    {
        do
        {
            ++aNumDigits;
            aDivisor *= 10;
        }
        while (number >= aDivisor);
    }

    int aDigitWidth = image->GetWidth() / 10;
    int aDigit = 0;
    do
    {
        ++aDigit;
        aDivisor /= 10;
        Graphics *aGraphics = g->Create();
        aGraphics->ClipRect(x, y, aDigitWidth, image->GetHeight());
        int aX = x - aDigitWidth * (number / aDivisor % 10);
        x += digitSpacing + aDigitWidth;
        aGraphics->DrawImage(image, aX, y);
        delete aGraphics;
    }
    while (aNumDigits > aDigit);
}

void Sexy::Widget::GotFocus()
{
    if (!mHasFocus)
    {
        mHasFocus = true;
        MarkDirty();
    }
}

void Sexy::Widget::LostFocus()
{
    bool aWasFocusable = true;
    if (!mHasFocus)
        aWasFocusable = mSelected;

    Widget *aFocusedChild = mFocusedChildWidget;
    mHasFocus = false;
    mSelected = false;
    if (aFocusedChild != nullptr)
        aFocusedChild->LostFocus();

    WidgetContainer *aParent = mParent;
    mFocusedChildWidget = nullptr;
    if (aParent != nullptr && aParent != mWidgetManager
        && static_cast<Widget *>(aParent)->mFocusedChildWidget == this)
        static_cast<Widget *>(aParent)->mFocusedChildWidget = nullptr;

    if (aWasFocusable)
        MarkDirty();
}

void Sexy::Widget::SetWindowFocus(bool focused)
{
    if (mHasWindowFocus == focused)
        return;
    if (focused)
        GotWindowFocus();
    else
        LostWindowFocus();
}

void Sexy::Widget::MouseDown(int x, int y, int theClickCount)
{
    if (theClickCount == 3)
        MouseDown(x, y, 2, 1);
    else if (theClickCount < 0)
        MouseDown(x, y, 1, -theClickCount);
    else
        MouseDown(x, y, 0, theClickCount);
}

void Sexy::Widget::MouseUp(int x, int y, int theClickCount)
{
    MouseUp(x, y);
    if (theClickCount == 3)
        MouseUp(x, y, 2, 1);
    else if (theClickCount < 0)
        MouseUp(x, y, 1, -theClickCount);
    else
        MouseUp(x, y, 0, theClickCount);
}

void Sexy::Widget::TouchMove(int theId, int x, int y)
{
    if (mWidgetManager->IsTouchDown(theId))
        MouseDrag(x, y);
    else
        MouseMove(x, y);
}

Color Sexy::Widget::GetColor(int theIdx, Color const &theDefaultColor)
{
    if (theIdx >= (int)mColors.size())
        return theDefaultColor;
    return mColors[theIdx];
}

void Sexy::Widget::SetDefaultFocus(Sexy::Widget *theWidget)
{
    if (theWidget != nullptr && theWidget->mParent == nullptr)
        return;

    for (WidgetListLink *aNode = mWidgets.mNext; aNode != &mWidgets; aNode = aNode->mNext)
        static_cast<WidgetListNode *>(aNode)->mWidget->SetDefaultFocus(nullptr);

    mDefaultFocusWidget = nullptr;
    if (theWidget == nullptr || mWidgets.mNext == &mWidgets)
        return;

    Widget *aWidget = static_cast<Widget *>(theWidget->mParent);
    if (aWidget != nullptr && aWidget != this)
    {
        while (true)
        {
            Widget *aNext = static_cast<Widget *>(aWidget->mParent);
            aWidget->mDefaultFocusWidget = theWidget;
            if (aNext == nullptr || aNext == this)
                break;
            aWidget = aNext;
        }
    }
    else
    {
        aWidget = theWidget;
    }
    mDefaultFocusWidget = aWidget;
}

void Sexy::Widget::DrawPre(Sexy::Graphics *g)
{
    (void)g;
    if (mAnimator == nullptr)
        return;

    mAnimator->Update();
    if (!mAnimator->IsCompleted())
        return;

    if (mAnimator->HideOnCompletion())
    {
        mVisible = false;
        mDisabled = true;
        ReleaseAnimator(mAnimator);
        mAnimator = nullptr;
        return;
    }

    ReleaseAnimator(mAnimator);
    mAnimator = nullptr;

    WidgetAnimator *aStateAnimator = mAnimatorForState[0];
    if (aStateAnimator != nullptr && !aStateAnimator->IsCompleted() && aStateAnimator != mAnimator)
    {
        ReleaseAnimator(mAnimator);
        ++aStateAnimator->mRefCount;
        mAnimator = aStateAnimator;
    }
}

Sexy::Widget::Widget()
{
    mUnusedRect[0] = 0;
    mUnusedRect[1] = 0;
    mUnusedRect[2] = -1;
    mUnusedRect[3] = -1;
    mHighlightColor = Color(255, 255, 0);
    mUsesWidgetColor = true;

    mWidgetManager = nullptr;
    mAnimator = nullptr;
    mAnimatorForState[0] = nullptr;
    mAnimatorForState[1] = nullptr;
    mAnimatorForState[2] = nullptr;

    mDisabled = false;
    mIsDown = false;
    mIsOver = false;
    mDoFinger = false;
    mHasFocus = false;
    mHasTransparencies = false;
    mWantsFocus = false;
    mSelected = false;
    mVisible = true;
    mMouseVisible = true;
    mDeselectOnEscape = true;
    mFocusable = true;

    mTabFocus[0] = nullptr;
    mTabFocus[1] = nullptr;
    mAddToManager = DefaultAddToManager != 0;
    mFocusedChildWidget = nullptr;
    mFocusLinks[0] = nullptr;
    mFocusLinks[1] = nullptr;
    mFocusLinks[2] = nullptr;
    mFocusLinks[3] = nullptr;
    mDefaultFocusWidget = nullptr;
    mHasWindowFocus = false;
    mLiveTouchId = -1;
    mEvent = nullptr;
    mWidgetId = -1;
}

Widget *Sexy::Widget::SetDefaultAddToManager(bool add)
{
    DefaultAddToManager = add;
    return this;
}

int Sexy::Widget::WidgetRemovedHelper()
{
    if (mWidgetManager == nullptr)
        return 0;

    for (WidgetListLink *aNode = mWidgets.mNext; aNode != &mWidgets; aNode = aNode->mNext)
        static_cast<WidgetListNode *>(aNode)->mWidget->WidgetRemovedHelper();

    mWidgetManager->DisableWidget(this);

    for (std::list<WidgetManager::PreModalInfo>::iterator aInfo = mWidgetManager->mPreModalInfoList.begin();
         aInfo != mWidgetManager->mPreModalInfoList.end();
         ++aInfo)
    {
        if (aInfo->mBaseModalWidget == this)
            aInfo->mBaseModalWidget = nullptr;
        if (aInfo->mFocusWidget == this)
            aInfo->mFocusWidget = nullptr;
    }

    RemovedFromManager(mWidgetManager);
    MarkDirtyFull(this);
    mWidgetManager = nullptr;
    return 0;
}

bool Sexy::Widget::IsFocusable()
{
    if (mVisible && mFocusable)
        return !mDisabled;
    return false;
}

void Sexy::Widget::AxisMoved(Sexy::Event const &theEvent)
{
    Widget *aChild = FocusedChild(mWidgetVector);
    if (aChild != nullptr)
        aChild->AxisMoved(theEvent);
}

Widget *Sexy::Widget::FindClosest(Sexy::Widget *widget)
{
    if (widget == nullptr || mWidgetVector.empty())
        return nullptr;

    Point aPos = widget->GetAbsCenter();
    Widget *aClosest = nullptr;
    int aBest = 0x7FFFFFFF;
    bool aSawOne = false;
    for (unsigned int i = 0; i < mWidgetVector.size(); ++i)
    {
        Widget *aChild = mWidgetVector[i];
        if (!aChild->IsFocusable() || !aChild->IsOnScreen())
        {
            if (!aSawOne && i + 1 == mWidgetVector.size())
                return nullptr;
            continue;
        }
        aSawOne = true;
        Point aCandidatePos = aChild->GetAbsCenter();
        int aDist = (aPos.mY - aCandidatePos.mY) * (aPos.mY - aCandidatePos.mY)
                  + (aPos.mX - aCandidatePos.mX) * (aPos.mX - aCandidatePos.mX);
        if (aBest > aDist)
        {
            aBest = aDist;
            aClosest = aChild;
        }
    }
    return aClosest;
}

int Sexy::Widget::DoKeyChar(char theChar)
{
    Widget *aFocused = FocusedChild(mWidgetVector);
    if (aFocused == nullptr)
        return 0;
    bool aWasSelected = aFocused->mSelected;
    int aResult = aFocused->KeyChar(theChar);
    if (aResult == 0)
        return 0;
    if (aFocused->mSelected == aWasSelected)
        aFocused->mSelected = true;
    return aResult;
}

int Sexy::Widget::KeyChar(char theChar)
{
    bool aWasSelected = mSelected;
    int aResult = DoKeyChar(theChar);
    if (aResult != 0 && mSelected == aWasSelected)
        mSelected = true;
    return aResult;
}

int Sexy::Widget::DoKeyUnicode(int theChar)
{
    Widget *aFocused = FocusedChild(mWidgetVector);
    if (aFocused == nullptr)
        return 0;
    bool aWasSelected = aFocused->mSelected;
    int aResult = aFocused->KeyUnicode(theChar);
    if (aResult == 0)
        return 0;
    if (aFocused->mSelected == aWasSelected)
        aFocused->mSelected = true;
    return aResult;
}

int Sexy::Widget::KeyUnicode(int theChar)
{
    bool aWasSelected = mSelected;
    int aResult = DoKeyUnicode(theChar);
    if (aResult != 0 && mSelected == aWasSelected)
        mSelected = true;
    return aResult;
}

int Sexy::Widget::KeyDownUp(Sexy::KeyCode theKey, bool isDown)
{
    bool aWasSelected = mSelected;
    int aResult = isDown ? KeyDown(theKey) : KeyUp(theKey);
    if (aResult == 0)
        return aResult;
    if (mSelected == aWasSelected)
        mSelected = true;
    return 1;
}

int Sexy::Widget::OnKeyReturn()
{
    Widget *aFocused = nullptr;
    int aIsFocusable = 0;
    for (unsigned int i = 0; i < mWidgetVector.size(); ++i)
    {
        Widget *aChild = mWidgetVector[i];
        if (!aChild->mHasFocus)
            continue;
        aIsFocusable = aChild->IsFocusable();
        if (aIsFocusable != 0)
        {
            aFocused = aChild;
            break;
        }
    }
    if (aFocused == nullptr)
        return 0;

    int aResult = aFocused->KeyDownUp(KEYCODE_RETURN, true);
    if (aResult != 0)
        return aResult;
    if (aFocused->mSelected)
        return 0;
    aFocused->mSelected = true;
    return aIsFocusable;
}

int Sexy::Widget::OnKeyEscape()
{
    Widget *aFocused = FocusedChild(mWidgetVector);
    if (aFocused == nullptr)
        return 0;
    int aResult = aFocused->KeyDownUp(KEYCODE_ESCAPE, true);
    if (aResult != 0)
        return aResult;

    if (aFocused->mSelected && aFocused->mDeselectOnEscape)
    {
        bool aWasSet = aFocused->mDeselectOnEscape;
        aFocused->mSelected = false;
        aFocused->MarkDirty();
        return aWasSet;
    }

    if (mWidgetManager == nullptr)
        return aResult;
    int aFullKeyboardMode = mWidgetManager->AllowFullKeyboardMode();
    if (aFullKeyboardMode == 0)
        return aResult;
    SetFocus(nullptr, false);
    return aFullKeyboardMode;
}

int Sexy::Widget::KeyUp(Sexy::KeyCode theKey)
{
    Widget *aFocused = FocusedChild(mWidgetVector);
    if (aFocused == nullptr)
        return 0;
    return aFocused->KeyDownUp(theKey, false);
}

Widget *Sexy::Widget::GetTopLevel()
{
    WidgetContainer *aTop = mParent;
    if (aTop == nullptr || aTop == mWidgetManager)
        return this;

    while (true)
    {
        WidgetContainer *aNext = aTop->mParent;
        if (aNext == nullptr || aNext == aTop->mWidgetManager)
            break;
        aTop = aTop->mParent;
    }
    return static_cast<Widget *>(aTop);
}

bool Sexy::Widget::IsOnScreen()
{
    if (mParent == nullptr)
        return true;

    TRect<int> aScreenRect;
    if (mWidgetManager != nullptr)
    {
        aScreenRect = mWidgetManager->GetScreenRect();
    }
    else
    {
        Widget *aTopLevel = GetTopLevel();
        Point aTopLevelPos = aTopLevel->GetAbsPosInScreen();
        aScreenRect = TRect<int>(aTopLevelPos.mX, aTopLevelPos.mY, aTopLevel->mWidth, aTopLevel->mHeight);
    }

    Point aPos = GetAbsPosInScreen();
    if (aPos.mX >= aScreenRect.mWidth + aScreenRect.mX
        || aPos.mY >= aScreenRect.mHeight + aScreenRect.mY
        || aScreenRect.mX >= aPos.mX + mWidth)
        return false;
    return aScreenRect.mY < mHeight + aPos.mY;
}

int Sexy::Widget::OnArrowKeys(Sexy::KeyCode theKey)
{
    int aDirection = 0;
    int aHint = 0;
    switch (theKey)
    {
    case KEYCODE_LEFT:  aDirection = 2048; aHint = 2; break;
    case KEYCODE_UP:    aDirection = 256;  aHint = 0; break;
    case KEYCODE_RIGHT: aDirection = 1024; aHint = 3; break;
    case KEYCODE_DOWN:  aDirection = 512;  aHint = 1; break;
    default: break;
    }

    Widget *aFocused = nullptr;
    bool aFound = false;
    for (unsigned int i = 0; i < mWidgetVector.size(); ++i)
    {
        if (mWidgetVector[i]->mHasFocus)
        {
            aFocused = mWidgetVector[i];
            aFound = true;
            break;
        }
    }

    if (aFound)
    {
        int aResult = aFocused->KeyDown(theKey);
        if (aResult != 0)
            return aResult;
    }

    if (mWidgetManager != nullptr && mWidgetManager->AllowFullKeyboardMode() == 0)
        return 0;

    while (true)
    {
        Widget *aNext = FindFocusableWidget(aDirection, aFocused);
        if (aNext == nullptr)
        {
            Widget *aDeepest = mFocusedChildWidget;
            if (aDeepest != nullptr)
            {
                while (aDeepest->mFocusedChildWidget != nullptr)
                    aDeepest = aDeepest->mFocusedChildWidget;
                if (!aDeepest->mSelected)
                    SetFocus(nullptr, false);
            }
            return 0;
        }

        aFocused = aNext;
        if (aFocused->mWidgets.mNext == &aFocused->mWidgets)
            break;

        int aResult = aFocused->KeyDown(theKey);
        if (aResult != 0)
            return aResult;
    }

    if (aFocused->mHasFocus)
        return 0;
    aFocused->FocusDirectionHint((WidgetFocusDir)aHint);
    GetTopLevel()->SetFocus(aFocused, true);
    return 1;
}

void Sexy::Widget::DeferOverlay(int thePriority)
{
    mWidgetManager->DeferOverlay(this, thePriority);
}

int Sexy::Widget::AttachSharedImage(Sexy::SharedImageRef &theRef)
{
    WidgetSharedImageList *aList = SharedImageList(this, true);
    WidgetSharedImageNode *aNode = new WidgetSharedImageNode(theRef);
    ListHook(aNode, &aList->mNext);
    return 1;
}

void Sexy::Widget::RemovedSharedImage(Sexy::Image *theImage)
{
    WidgetSharedImageList *aList = SharedImageList(this, false);
    if (aList == nullptr)
        return;

    WidgetSharedImageNode *aHeader = aList->mNext;
    for (WidgetSharedImageNode *aNode = aHeader->mNext; aNode != aHeader; aNode = aNode->mNext)
    {
        if ((Sexy::Image *)aNode->mRef == theImage)
        {
            ListUnhook(aNode);
            delete aNode;
            return;
        }
    }
}

Sexy::WidgetAnimator *Sexy::Widget::SetAnimator(Sexy::WidgetAnimator *theAnimator)
{
    Sexy::WidgetAnimator *aOld = mAnimator;
    if (theAnimator != aOld)
    {
        ReleaseAnimator(aOld);
        if (theAnimator != nullptr)
            ++theAnimator->mRefCount;
        mAnimator = theAnimator;
    }
    return aOld;
}

Sexy::WidgetAnimator *Sexy::Widget::GetAnimator()
{
    return mAnimator;
}

Sexy::WidgetAnimator *Sexy::Widget::SetAnimatorForState(Sexy::WidgetAnimationState theState,
                                                        Sexy::WidgetAnimator *theAnimator)
{
    Sexy::WidgetAnimator *aOld = mAnimatorForState[theState];
    if (theAnimator != aOld)
    {
        ReleaseAnimator(aOld);
        if (theAnimator != nullptr)
            ++theAnimator->mRefCount;
        mAnimatorForState[theState] = theAnimator;
    }
    return aOld;
}

Sexy::WidgetAnimator *Sexy::Widget::GetAnimatorForState(Sexy::WidgetAnimationState theState)
{
    return mAnimatorForState[theState];
}

int Sexy::Widget::Layout(int theLayoutFlags, Sexy::Widget *theRelativeWidget,
                         int theLeftPad, int theTopPad, int theWidthPad, int theHeightPad)
{
    int aWidgetY = theRelativeWidget->mY;
    int aWidgetX = theRelativeWidget->mX;
    int aWidgetWidth = theRelativeWidget->mWidth;
    if (mParent == theRelativeWidget)
    {
        aWidgetY = 0;
        aWidgetX = 0;
    }
    int aWidgetHeight = theRelativeWidget->mHeight;
    int aX = mX;
    int aY = mY;
    int aRight = aWidgetX + aWidgetWidth;
    int aRightPlus = aWidgetX + aWidgetWidth + theLeftPad;
    int aBottom = aWidgetY + aWidgetHeight;
    int aWidth = mWidth;
    int aHeight = mHeight;

    for (int aFlag = 1; aFlag <= 0x3FFFFE; aFlag *= 2)
    {
        if ((aFlag & theLayoutFlags) == 0)
            continue;
        if (aFlag == 1)
            aWidth = theWidthPad + aWidgetWidth;
        else if (aFlag == 2)
            aHeight = theHeightPad + aWidgetHeight;
        else if (aFlag == 16)
            aX = theLeftPad;
        else if (aFlag == 32)
            aY = theTopPad;
        else if (aFlag == 64)
            aWidth = theWidthPad;
        else if (aFlag == 128)
            aHeight = theHeightPad;
        else if (aFlag == 256)
            aY = aWidgetY - aHeight + theTopPad;
        else if (aFlag == 512)
            aY = theTopPad + aWidgetY + aWidgetHeight;
        else if (aFlag == 1024)
            aX = aRightPlus;
        else if (aFlag == 2048)
            aX = aWidgetX - aWidth + theLeftPad;
        else if (aFlag == 4096)
            aX = aWidgetX + theLeftPad;
        else if (aFlag == 0x2000)
            aX = aRight - aWidth + theLeftPad;
        else if (aFlag == 0x4000)
            aY = theTopPad + aWidgetY;
        else if (aFlag == 0x8000)
            aY = aBottom - aHeight + theTopPad;
        else if (aFlag == 0x10000)
            aWidth = aRight - aX + theWidthPad;
        else if (aFlag == 0x20000)
            aWidth = aWidgetX - aX + theWidthPad;
        else if (aFlag == 0x40000)
            aHeight = aWidgetY - aY + theHeightPad;
        else if (aFlag == 0x80000)
            aHeight = aBottom - aY + theHeightPad;
        else if (aFlag == 0x100000)
            aX = aWidgetX + (aWidgetWidth - aWidth) / 2 + theLeftPad;
        else if (aFlag == 0x200000)
            aY = aWidgetY + (aWidgetHeight - aHeight) / 2 + theTopPad;
    }

    Resize(aX, aY, aWidth, aHeight);
    return 0;
}

void Sexy::Widget::SetFocus(Sexy::Widget *theWidget, bool force)
{
    if (theWidget == nullptr)
    {
        LostFocus();
        return;
    }

    std::vector<Widget *> aOldChain;
    std::vector<Widget *> aNewChain;

    // Widget::SetFocus @0x3B00D0. The old chain is the children already under
    // mFocusedChildWidget, starting at that child's own child. The new chain
    // is the parents of the widget being focused, walking +0x1C until this.
    // The target itself is not a chain entry. GotFocus on the chain sets
    // mHasFocus, and the call after the loop sets it on the target.
    for (Widget *aWidget = mFocusedChildWidget; aWidget != nullptr; aWidget = aWidget->mFocusedChildWidget)
    {
        Widget *aDeeper = aWidget->mFocusedChildWidget;
        if (aDeeper == nullptr)
            break;
        aOldChain.push_back(aDeeper);
    }

    Widget *aNewChild = theWidget;
    Widget *aParent = static_cast<Widget *>(theWidget->mParent);
    while (aParent != this)
    {
        aNewChild = aParent;
        aNewChain.push_back(aParent);
        aParent = static_cast<Widget *>(aParent->mParent);
    }

    int aOldCount = (int)aOldChain.size();
    int aNewCount = (int)aNewChain.size();
    int aCommon = aOldCount < aNewCount ? aOldCount : aNewCount;

    Widget *aShared = nullptr;
    unsigned int aDiverged = 0;
    bool aChainsMatch = aCommon != 0 && aOldChain[0] == aNewChain[0];
    if (aChainsMatch)
    {
        aShared = aOldChain[0];
        while (++aDiverged != (unsigned int)aCommon)
        {
            if (aOldChain[aDiverged] != aNewChain[aDiverged])
                break;
            aShared = aOldChain[aDiverged];
        }
        if ((int)aDiverged < aOldCount)
            aOldChain[aDiverged]->LostFocus();
    }
    else if (mFocusedChildWidget != nullptr)
    {
        mFocusedChildWidget->LostFocus();
    }

    bool aNewWindowFocus = theWidget->mHasWindowFocus;
    bool aThisWindowFocus = mHasWindowFocus;
    mSelected = force;
    mFocusedChildWidget = aNewChild;
    bool aWindowFocusChanged = aNewWindowFocus != aThisWindowFocus;

    if (aNewCount != 0)
    {
        // @0x3B01DE walks the parent chain from its first entry. The last
        // entry points at the target. Every earlier entry points at the
        // entry that was pushed after it.
        for (int i = 0; i < aNewCount; ++i)
        {
            if (aNewChain[i]->mHasWindowFocus != aThisWindowFocus)
                aWindowFocusChanged = true;
            aNewChain[i]->mFocusedChildWidget = i + 1 == aNewCount ? theWidget : aNewChain[i + 1];
            aNewChain[i]->mSelected = force;
        }
        for (int i = 0; i < aNewCount; ++i)
        {
            if (aNewChain[i] != aShared)
                aNewChain[i]->GotFocus();
        }
    }

    if (aWindowFocusChanged)
    {
        if (aNewCount != 0)
            aNewChain[0]->SetWindowFocus(aThisWindowFocus);
        else
            theWidget->SetWindowFocus(aThisWindowFocus);
    }

    theWidget->mSelected = force;
    theWidget->GotFocus();
}

Widget *Sexy::Widget::FindFocusableWidget(int direction, Sexy::Widget *relativeWidget)
{
    if (mWidgetVector.empty())
        return nullptr;

    Widget *aRelative = relativeWidget;
    std::vector<Widget *>::iterator aFound = mWidgetVector.begin();

    if (relativeWidget == nullptr)
    {
        for (std::vector<Widget *>::iterator aWidget = mWidgetVector.begin();
             aWidget != mWidgetVector.end();
             ++aWidget)
        {
            if ((*aWidget)->mHasFocus && (*aWidget)->IsFocusable())
            {
                aFound = aWidget;
                aRelative = *aWidget;
                break;
            }
        }

        if (aRelative == nullptr && mDefaultFocusWidget != nullptr)
        {
            aFound = std::find(mWidgetVector.begin(), mWidgetVector.end(), mDefaultFocusWidget);
            if (aFound != mWidgetVector.end())
            {
                if ((*aFound)->IsFocusable() && (*aFound)->IsOnScreen())
                    return *aFound;
                aRelative = *aFound;
            }
        }

        if (aRelative == nullptr)
        {
            Widget *aTopLevel = GetTopLevel();
            if (aTopLevel->mFocusedChildWidget != nullptr)
            {
                Widget *aDeepest = aTopLevel->mFocusedChildWidget;
                while (aDeepest->mFocusedChildWidget != nullptr)
                    aDeepest = aDeepest->mFocusedChildWidget;
                return FindClosest(aDeepest);
            }
            for (unsigned int i = 0; i < mWidgetVector.size(); ++i)
            {
                if (mWidgetVector[i]->IsFocusable() && mWidgetVector[i]->IsOnScreen())
                    return mWidgetVector[i];
            }
            return nullptr;
        }
    }
    else
    {
        aFound = std::find(mWidgetVector.begin(), mWidgetVector.end(), aRelative);
    }

    Point aRelativeCenter = aRelative->GetAbsCenter();
    Point aRelativePos = aRelative->GetAbsPos();
    Widget *aCandidate = nullptr;
    Point aSlotPos;
    std::vector<Widget *>::iterator anEnd = mWidgetVector.end();

    switch (direction)
    {
    case 512:
    {
        Widget *aLinked = FollowFocusLink(mWidgetVector, aRelative, 1);
        if (aLinked != nullptr)
            return aLinked;

        if (aFound != anEnd && HasFocusLink(aRelative))
            return nullptr;
        if (aFound == anEnd)
            return nullptr;

        std::vector<Widget *>::iterator aSlot = aFound + 1;
        if (aSlot == mWidgetVector.end())
            return nullptr;
        while (true)
        {
            aFound = aSlot;
            aSlotPos = (*aSlot)->GetAbsPos();
            Widget *aWidget = *aSlot++;
            if (aWidget->IsFocusable() && aWidget->IsOnScreen() && aSlotPos.mY > aRelativePos.mY)
                break;
            if (aSlot == mWidgetVector.end())
                return nullptr;
        }

        aCandidate = *aFound;
        if (aFound != mWidgetVector.end())
        {
            Point aCandidateCenter = aCandidate->GetAbsCenter();
            int aBest = (aRelativeCenter.mY - aCandidateCenter.mY) * (aRelativeCenter.mY - aCandidateCenter.mY)
                      + (aRelativeCenter.mX - aCandidateCenter.mX) * (aRelativeCenter.mX - aCandidateCenter.mX);
            if (aSlot != mWidgetVector.end())
            {
                Widget *aNext = aFound[1];
                if (aNext->mX > aCandidate->mX)
                {
                    std::vector<Widget *>::iterator aScan = aFound + 2;
                    do
                    {
                        if (aNext->IsFocusable() && (*(aScan - 1))->IsOnScreen())
                        {
                            Point aScanCenter = (*(aScan - 1))->GetAbsCenter();
                            int aDistY = (aRelativeCenter.mY - aScanCenter.mY) * (aRelativeCenter.mY - aScanCenter.mY);
                            int aDist = aDistY + (aRelativeCenter.mX - aScanCenter.mX) * (aRelativeCenter.mX - aScanCenter.mX);
                            if (aDist < aBest)
                            {
                                aCandidate = *(aScan - 1);
                                aFound = aScan - 1;
                                aBest = aDist;
                            }
                        }
                        if (aScan == mWidgetVector.end())
                            break;
                        aNext = *aScan++;
                    }
                    while (aNext->mX > (*(aScan - 2))->mX);
                }
            }
        }
        break;
    }

    case 256:
    {
        Widget *aLinked = FollowFocusLink(mWidgetVector, aRelative, 0);
        if (aLinked != nullptr)
            return aLinked;

        if ((aFound != anEnd && HasFocusLink(aRelative)) || aFound == mWidgetVector.begin())
            return nullptr;

        std::vector<Widget *>::iterator aSlot = aFound - 1;
        while (true)
        {
            aFound = aSlot;
            aSlotPos = (*aSlot)->GetAbsPos();
            Widget *aWidget = *aSlot;
            if (aWidget->IsFocusable() && aWidget->IsOnScreen() && aSlotPos.mY < aRelativePos.mY)
                break;
            if (aFound == mWidgetVector.begin())
                return nullptr;
            --aSlot;
        }

        if (aSlot != mWidgetVector.begin())
            --aSlot;
        aCandidate = *aFound;
        if (aFound != mWidgetVector.begin())
        {
            Point aCandidateCenter = aCandidate->GetAbsCenter();
            int aBest = (aRelativeCenter.mY - aCandidateCenter.mY) * (aRelativeCenter.mY - aCandidateCenter.mY)
                      + (aRelativeCenter.mX - aCandidateCenter.mX) * (aRelativeCenter.mX - aCandidateCenter.mX);
            if (aSlot != mWidgetVector.begin())
            {
                Widget *aPrev = aFound[-1];
                if (aPrev->mX < aCandidate->mX)
                {
                    std::vector<Widget *>::iterator aScan = aFound - 2;
                    do
                    {
                        if (aPrev->IsFocusable() && aScan[1]->IsOnScreen())
                        {
                            Point aScanCenter = aScan[1]->GetAbsCenter();
                            int aDistY = (aRelativeCenter.mY - aScanCenter.mY) * (aRelativeCenter.mY - aScanCenter.mY);
                            int aDist = aDistY + (aRelativeCenter.mX - aScanCenter.mX) * (aRelativeCenter.mX - aScanCenter.mX);
                            if (aDist < aBest)
                            {
                                aCandidate = aScan[1];
                                aFound = aScan + 1;
                                aBest = aDist;
                            }
                        }
                        if (aScan == mWidgetVector.begin())
                            break;
                        aPrev = *aScan--;
                    }
                    while (aPrev->mX < aScan[2]->mX);
                }
            }
        }
        break;
    }

    case 1024:
    {
        Widget *aLinked = FollowFocusLink(mWidgetVector, aRelative, 3);
        if (aLinked != nullptr)
            return aLinked;

        bool aScanLeft = aFound == anEnd;
        if (!aScanLeft && HasFocusLink(aRelative))
            return nullptr;

        if (!aScanLeft)
        {
            std::vector<Widget *>::iterator aSlot = aFound;
            while (true)
            {
                std::vector<Widget *>::iterator aSlotStart = aSlot;
                aSlotPos = (*aSlot)->GetAbsPos();
                Widget *aWidget = *aSlot++;
                if (aWidget->IsFocusable() && aSlot[-1]->IsOnScreen() && aSlotPos.mX > aRelativePos.mX)
                {
                    aFound = aSlotStart;
                    aCandidate = *aSlotStart;
                    break;
                }
                if (aSlot == mWidgetVector.end())
                {
                    aFound = aSlotStart;
                    aScanLeft = true;
                    break;
                }
            }
        }

        if (aScanLeft)
        {
            if (aFound == mWidgetVector.begin())
                return nullptr;
            std::vector<Widget *>::iterator aSlot = aFound - 1;
            while (true)
            {
                std::vector<Widget *>::iterator aSlotStart = aSlot;
                aSlotPos = (*aSlot)->GetAbsPos();
                Widget *aWidget = *aSlot;
                if (aWidget->IsFocusable() && aWidget->IsOnScreen() && aSlotPos.mX > aRelativePos.mX)
                {
                    aFound = aSlotStart;
                    aCandidate = *aSlotStart;
                    break;
                }
                if (aSlotStart == mWidgetVector.begin())
                    return nullptr;
                --aSlot;
            }
        }
        break;
    }

    case 2048:
    {
        Widget *aLinked = FollowFocusLink(mWidgetVector, aRelative, 2);
        if (aLinked != nullptr)
            return aLinked;

        if (aFound == anEnd || !HasFocusLink(aRelative))
        {
            if (aFound != mWidgetVector.begin())
            {
                std::vector<Widget *>::iterator aSlot = aFound - 1;
                while (true)
                {
                    std::vector<Widget *>::iterator aSlotStart = aSlot;
                    aSlotPos = (*aSlot)->GetAbsPos();
                    Widget *aWidget = *aSlot;
                    if (aWidget->IsFocusable() && aWidget->IsOnScreen() && aSlotPos.mX <= aRelativePos.mX)
                    {
                        aFound = aSlotStart;
                        aCandidate = *aSlotStart;
                        break;
                    }
                    if (aSlotStart == mWidgetVector.begin())
                        break;
                    --aSlot;
                }
            }

            if (aCandidate == nullptr && aFound != anEnd)
            {
                std::vector<Widget *>::iterator aSlot = aFound + 1;
                while (aSlot != anEnd)
                {
                    std::vector<Widget *>::iterator aSlotStart = aSlot;
                    aSlotPos = (*aSlot)->GetAbsPos();
                    Widget *aWidget = *aSlot++;
                    if (aWidget->IsFocusable() && aSlot[-1]->IsOnScreen() && aSlotPos.mX <= aRelativePos.mX)
                    {
                        aFound = aSlotStart;
                        aCandidate = *aSlotStart;
                        break;
                    }
                    if (aSlot == mWidgetVector.end())
                        break;
                }
            }
        }
        break;
    }

    default:
        return nullptr;
    }

    if (aCandidate == nullptr)
        return nullptr;
    return *aFound;
}

void Sexy::Widget::RemovedSharedImages()
{
    WidgetSharedImageList *aList = SharedImageList(this, false);
    if (aList != nullptr)
        ClearSharedImageNodes(aList);
}

void Sexy::Widget::SetColor(int theIdx, Color const &theColor)
{
    if (theIdx >= (int)mColors.size())
        mColors.resize((unsigned int)theIdx + 1);
    mColors[theIdx] = theColor;
    MarkDirty();
}

int Sexy::Widget::KeyDown(Sexy::KeyCode theKey)
{
    if (theKey == KEYCODE_TAB)
    {
        KeyState *aShift = mWidgetManager->mKeyDown.Get(KEYCODE_SHIFT);
        if (aShift->mDown)
        {
            if (mTabFocus[0] != nullptr)
                mWidgetManager->SetFocus(mTabFocus[0]);
        }
        else if (mTabFocus[1] != nullptr)
        {
            mWidgetManager->SetFocus(mTabFocus[1]);
        }
        return 1;
    }

    // Widget::KeyDown(KeyCode) @0x3B0D54. Tab, arrows, Return and Escape are
    // handled here. Any other code goes to the focused child through KeyDownUp.
    if ((unsigned int)(theKey - KEYCODE_LEFT) <= 3)
        return OnArrowKeys(theKey);
    if (theKey == KEYCODE_RETURN)
        return OnKeyReturn();
    if (theKey == KEYCODE_ESCAPE)
        return OnKeyEscape();

    Widget *aFocused = FocusedChild(mWidgetVector);
    if (aFocused == nullptr)
        return 0;
    return aFocused->KeyDownUp(theKey, true);
}

void Sexy::Widget::Update()
{
    WidgetContainer::Update();
}

Sexy::Widget::~Widget()
{
    for (int i = 2; i >= 0; --i)
    {
        ReleaseAnimator(mAnimatorForState[i]);
        mAnimatorForState[i] = nullptr;
    }
    ReleaseAnimator(mAnimator);
    mAnimator = nullptr;
}
