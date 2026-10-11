// WidgetContainer is the child list, hit testing, dirty marks and user data.

#include "WidgetContainer.h"
#include "Graphics.h"
#include "Widget.h"
#include "WidgetManager.h"
#include "SexyAppBase.h"
#include <algorithm>
#include <cstring>

Sexy::WidgetUserData::~WidgetUserData() = default;

namespace Sexy
{
namespace
{

WidgetListLink *EndChild(WidgetContainer *theContainer)
{
    return &theContainer->mWidgets;
}

WidgetListLink *BeginChild(WidgetContainer *theContainer)
{
    return theContainer->mWidgets.mNext;
}

WidgetListLink *NextChild(WidgetListLink *theNode)
{
    return theNode->mNext;
}

WidgetListLink *PrevChild(WidgetListLink *theNode)
{
    return theNode->mPrev;
}

Widget *ChildWidget(WidgetListLink *theNode)
{
    return static_cast<WidgetListNode *>(theNode)->mWidget;
}

void LinkChildBefore(WidgetListLink *thePos, WidgetListLink *theNode)
{
    theNode->mNext = thePos;
    theNode->mPrev = thePos->mPrev;
    thePos->mPrev->mNext = theNode;
    thePos->mPrev = theNode;
}

WidgetListLink *NewChild(Widget *theWidget)
{
    WidgetListNode *aNode = new WidgetListNode;
    aNode->mWidget = theWidget;
    return aNode;
}

void EraseChild(WidgetListLink *theNode)
{
    theNode->mPrev->mNext = theNode->mNext;
    theNode->mNext->mPrev = theNode->mPrev;
    operator delete(theNode);
}

void NoteIteratorErased(WidgetContainer *theContainer, WidgetListLink *theNode)
{
    if (theNode != theContainer->mUpdateIterator)
        return;
    theContainer->mUpdateIterator = theNode->mNext;
    theContainer->mUpdateIteratorModified = true;
}

} // namespace

void Sexy::WidgetContainer::SortWidgets()
{
    mWidgetVector.clear();
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
        mWidgetVector.push_back(ChildWidget(aNode));
}

void Sexy::WidgetContainer::SortWidgets(Widget *theWidget)
{
    mWidgetVector.erase(std::remove(mWidgetVector.begin(), mWidgetVector.end(), theWidget), mWidgetVector.end());
    mWidgetVector.push_back(theWidget);
}

void Sexy::WidgetContainer::RemoveAllWidgets(bool doDelete, bool recursive, Sexy::WidgetManager *theManager)
{
    while (BeginChild(this) != EndChild(this))
    {
        Widget *aWidget = ChildWidget(BeginChild(this));
        RemoveWidget(aWidget);
        if (recursive)
            aWidget->RemoveAllWidgets(doDelete, true, nullptr);
        if (!doDelete)
            continue;
        // @0x3ACE9A reads WidgetManager::mApp, then calls SafeDeleteWidget.
        // A screen may be removed while one of its children is handling input.
        if (theManager != nullptr && theManager->mApp != nullptr)
            theManager->mApp->SafeDeleteWidget(aWidget);
        else
            delete aWidget;
    }
}

Rect Sexy::WidgetContainer::GetRect()
{
    return Rect(mX, mY, mWidth, mHeight);
}

bool Sexy::WidgetContainer::HasWidget(Sexy::Widget *theWidget)
{
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        if (ChildWidget(aNode) == theWidget)
            return true;
    }
    return false;
}

Widget *Sexy::WidgetContainer::FindWidget(int theId)
{
    if (theId == -1)
        return nullptr;

    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        Widget *aWidget = ChildWidget(aNode);
        if (aWidget->mWidgetId == theId)
            return aWidget;
    }
    return nullptr;
}

void Sexy::WidgetContainer::MarkAllDirty()
{
    MarkDirty();
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        Widget *aWidget = ChildWidget(aNode);
        aWidget->mDirty = true;
        aWidget->MarkDirty();
    }
}

Point Sexy::WidgetContainer::GetAbsPos() const
{
    if (mParent == nullptr || mParent == mWidgetManager)
        return Point(mX, mY);

    Point aParentPos = mParent->GetAbsPos();
    return Point(aParentPos.mX + mX, aParentPos.mY + mY);
}

Point Sexy::WidgetContainer::GetAbsPosInManager() const
{
    return GetAbsPos();
}

void Sexy::WidgetContainer::OnViewportChanged()
{
    for (WidgetListLink *node = BeginChild(this); node != EndChild(this); node = NextChild(node))
        ChildWidget(node)->OnViewportChanged();
}

Point Sexy::WidgetContainer::GetAbsPosInScreen() const
{
    if (mParent == nullptr)
        return Point(mX, mY);

    Point aScreenPos = mParent->GetAbsPosInScreen();
    return Point(aScreenPos.mX + mX, aScreenPos.mY + mY);
}

Point Sexy::WidgetContainer::GetCenter() const
{
    return Point(mX + mWidth / 2, mY + mHeight / 2);
}

Point Sexy::WidgetContainer::GetAbsCenter() const
{
    Point anAbsPos = GetAbsPos();
    return Point(anAbsPos.mX + mWidth / 2, anAbsPos.mY + mHeight / 2);
}

Point Sexy::WidgetContainer::GetAbsCenterInScreen() const
{
    Point aScreenPos = GetAbsPosInScreen();
    return Point(aScreenPos.mX + mWidth / 2, aScreenPos.mY + mHeight / 2);
}

void Sexy::WidgetContainer::AddedToManager(Sexy::WidgetManager *theWidgetManager)
{
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        Widget *aWidget = ChildWidget(aNode);
        aWidget->mWidgetManager = theWidgetManager;
        aWidget->AddedToManager(theWidgetManager);
        MarkDirty();
    }
    MarkAllDirty();
}

void Sexy::WidgetContainer::RemovedFromManager(Sexy::WidgetManager *theWidgetManager)
{
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        Widget *aWidget = ChildWidget(aNode);
        theWidgetManager->RemoveWidget(aWidget);
        aWidget->RemovedFromManager(theWidgetManager);
        aWidget->mWidgetManager = nullptr;
    }
    if (theWidgetManager->mFocusWidget == this)
        theWidgetManager->mFocusWidget = nullptr;
    MarkAllDirty();
}

void Sexy::WidgetContainer::MarkDirty()
{
    if (mParent != nullptr)
    {
        mParent->MarkDirty(this);
        return;
    }
    mDirty = true;
}

void Sexy::WidgetContainer::MarkDirtyFull()
{
    if (mParent != nullptr)
    {
        mParent->MarkDirtyFull(this);
        return;
    }
    mDirty = true;
}

void Sexy::WidgetContainer::Update()
{
    ++mUpdateCnt;
}

void Sexy::WidgetContainer::UpdateAll(ModalFlags *theFlags)
{
    int aSavedOver = theFlags->mOverFlags;
    int aSavedUnder = theFlags->mUnderFlags;

    theFlags->mOverFlags = (mWidgetFlagsMod.mAddFlags | theFlags->mOverFlags) & ~mWidgetFlagsMod.mRemoveFlags;
    theFlags->mUnderFlags = (mWidgetFlagsMod.mAddFlags | theFlags->mUnderFlags) & ~mWidgetFlagsMod.mRemoveFlags;

    // WIDGETFLAGS_MARK_DIRTY is 2. A visible widget marks itself every frame
    // so a later draw sees it.
    int aFlags = theFlags->mIsOver ? theFlags->mOverFlags : theFlags->mUnderFlags;
    if ((aFlags & 2) != 0)
        MarkDirty();

    // WIDGETFLAGS_UPDATE is 1. The manager's own count was just advanced, so
    // this call is the one Update of the frame.
    aFlags = theFlags->mIsOver ? theFlags->mOverFlags : theFlags->mUnderFlags;
    if ((aFlags & 1) != 0)
        Update();

    if (mWidgetManager == nullptr)
    {
        theFlags->mOverFlags = aSavedOver;
        theFlags->mUnderFlags = aSavedUnder;
        return;
    }

    aFlags = theFlags->mIsOver ? theFlags->mOverFlags : theFlags->mUnderFlags;
    if ((aFlags & 1) != 0 && mLastWMUpdateCount != mWidgetManager->mUpdateCnt)
    {
        mLastWMUpdateCount = mWidgetManager->mUpdateCnt;
        UpdateF(0.0f);
    }

    // Help screens raise themselves and their help bar during Update. Each
    // child must still update once even when its list node moves or disappears.
    std::vector<Widget *> aChildren;
    aChildren.reserve(mWidgetVector.size());
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
        aChildren.push_back(ChildWidget(aNode));
    for (Widget *aWidget : aChildren)
    {
        if (HasWidget(aWidget))
            aWidget->UpdateAll(theFlags);
    }
    mUpdateIterator = EndChild(this);
    mUpdateIteratorModified = true;
    theFlags->mOverFlags = aSavedOver;
    theFlags->mUnderFlags = aSavedUnder;
}

void Sexy::WidgetContainer::UpdateF(float) {}

void Sexy::WidgetContainer::UpdateFAll(ModalFlags *theFlags, float theFrac)
{
    int aSavedOver = theFlags->mOverFlags;
    int aSavedUnder = theFlags->mUnderFlags;

    theFlags->mOverFlags = (mWidgetFlagsMod.mAddFlags | theFlags->mOverFlags) & ~mWidgetFlagsMod.mRemoveFlags;
    theFlags->mUnderFlags = (mWidgetFlagsMod.mAddFlags | theFlags->mUnderFlags) & ~mWidgetFlagsMod.mRemoveFlags;

    int aFlags = theFlags->mIsOver ? theFlags->mOverFlags : theFlags->mUnderFlags;
    if ((aFlags & 1) != 0)
        UpdateF(theFrac);

    std::vector<Widget *> aChildren;
    aChildren.reserve(mWidgetVector.size());
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
        aChildren.push_back(ChildWidget(aNode));
    for (Widget *aWidget : aChildren)
    {
        if (HasWidget(aWidget))
            aWidget->UpdateFAll(theFlags, theFrac);
    }
    mUpdateIterator = EndChild(this);
    mUpdateIteratorModified = true;
    theFlags->mOverFlags = aSavedOver;
    theFlags->mUnderFlags = aSavedUnder;
}

void Sexy::WidgetContainer::DrawOther(Sexy::Graphics *) {}

void Sexy::WidgetContainer::DrawPre(Sexy::Graphics *) {}

void Sexy::WidgetContainer::Draw(Sexy::Graphics *) {}

void Sexy::WidgetContainer::DrawAll(ModalFlags *theFlags, Sexy::Graphics *g)
{
    if (mPriority > mWidgetManager->mMinDeferredOverlayPriority)
        mWidgetManager->FlushDeferredOverlayWidgets(mPriority);

    int aSavedOver = theFlags->mOverFlags;
    int aSavedUnder = theFlags->mUnderFlags;
    int aOver = (mWidgetFlagsMod.mAddFlags | theFlags->mOverFlags) & ~mWidgetFlagsMod.mRemoveFlags;
    int aUnder = (mWidgetFlagsMod.mAddFlags | theFlags->mUnderFlags) & ~mWidgetFlagsMod.mRemoveFlags;
    theFlags->mOverFlags = aOver;
    theFlags->mUnderFlags = aUnder;

    int aFlags = theFlags->mIsOver ? aOver : aUnder;
    if (mClip && (aFlags & 8) != 0)
        g->ClipRect(0, 0, mWidth, mHeight);

    bool aDrawThis = (aFlags & 4) != 0;
    if (BeginChild(this) == EndChild(this))
    {
        if (aDrawThis)
        {
            Draw(g);
            DrawOther(g);
        }
        theFlags->mOverFlags = aSavedOver;
        theFlags->mUnderFlags = aSavedUnder;
        return;
    }

    if (aDrawThis)
    {
        g->PushState();
        Draw(g);
        DrawOther(g);
        g->PopState();
    }

    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        Widget *aWidget = ChildWidget(aNode);
        if (!aWidget->mVisible)
            continue;

        if (mWidgetManager->mBaseModalWidget == aWidget)
            theFlags->mIsOver = true;

        Graphics aWidgetGraphics(*g);
        aWidgetGraphics.Translate(aWidget->mX, aWidget->mY);
        aWidget->DrawAll(theFlags, &aWidgetGraphics);
        aWidget->mDirty = false;
    }

    theFlags->mOverFlags = aSavedOver;
    theFlags->mUnderFlags = aSavedUnder;
}

void Sexy::WidgetContainer::SysColorChanged() {}

void Sexy::WidgetContainer::SysColorChangedAll()
{
    SysColorChanged();
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
        ChildWidget(aNode)->SysColorChangedAll();
}

void Sexy::WidgetContainer::DisableWidget(Sexy::Widget *) {}

void Sexy::WidgetContainer::SetFocus(Sexy::Widget *) {}

bool Sexy::WidgetContainer::Intersects(Sexy::WidgetContainer *theOther)
{
    Rect aThisRect = GetRect();
    Rect anOtherRect = theOther->GetRect();
    return aThisRect.Intersects(anOtherRect);
}

void Sexy::WidgetContainer::MarkDirty(Sexy::WidgetContainer *theWidget)
{
    if (theWidget->mDirty)
        return;

    MarkDirty();
    theWidget->mDirty = true;
    if (mParent != nullptr)
        return;

    if (theWidget->mHasAlpha)
    {
        MarkDirtyFull(theWidget);
        return;
    }

    bool aFound = theWidget->mHasAlpha;
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        Widget *aChild = ChildWidget(aNode);
        if (theWidget == aChild)
            aFound = true;
        else if (aFound && aChild->mClip && aChild->Intersects(theWidget))
            MarkDirtyFull(aChild);
    }
}

void Sexy::WidgetContainer::MarkDirtyFull(Sexy::WidgetContainer *theWidget)
{
    MarkDirtyFull();
    theWidget->mDirty = true;
    if (mParent != nullptr)
        return;

    WidgetListLink *aEnd = EndChild(this);
    WidgetListLink *aFirst = BeginChild(this);
    if (aFirst == aEnd)
        return;

    WidgetListLink *aNode = aFirst;
    if (theWidget == ChildWidget(aNode))
    {
        while (true)
        {
            Widget *aChild = ChildWidget(aNode);
            if (aChild->mVisible && aChild->Intersects(theWidget))
                MarkDirty(aChild);

            aFirst = NextChild(aFirst);
            if (aFirst == aEnd)
                return;
            aNode = aFirst;
        }
    }

    while (true)
    {
        aFirst = NextChild(aFirst);
        if (aFirst == aEnd)
            return;
        aNode = aFirst;
        if (theWidget != ChildWidget(aNode))
            continue;

        if (aNode == BeginChild(this))
            break;

        WidgetListLink *aWalk = PrevChild(aNode);
        WidgetListLink *aMarked = aNode;
        while (true)
        {
            Widget *aChild = ChildWidget(aWalk);
            if (aChild->mVisible)
            {
                if (!aChild->mHasTransparencies && !aChild->mDirty)
                {
                    int aLeft = theWidget->mX;
                    int aTop = theWidget->mY;
                    int aClippedLeft = aLeft & ~(aLeft >> 31);
                    int aRight = theWidget->mWidth + aLeft;
                    int aWidth;
                    if (aRight > mWidth)
                        aWidth = mWidth - aClippedLeft;
                    else
                        aWidth = aRight - aClippedLeft;

                    int aClippedTop = aTop & ~(aTop >> 31);
                    int aBottom = theWidget->mHeight + aTop;
                    if (aBottom >= mHeight)
                        aBottom = mHeight;

                    int aHeight;
                    if (aWidth < 0 || (aHeight = aBottom - aClippedTop) < 0)
                    {
                        aHeight = aChild->mHasAlpha;
                        aWidth = aHeight;
                        aClippedTop = aHeight;
                        aClippedLeft = aHeight;
                    }

                    if (aChild->Contains(aClippedLeft, aClippedTop) &&
                        aChild->Contains(aWidth + aClippedLeft - 1, aHeight + aClippedTop - 1))
                    {
                        aChild->MarkDirty();
                        aNode = aMarked;
                        break;
                    }
                }

                if (aChild->Intersects(theWidget))
                    MarkDirty(aChild);
            }

            if (aWalk == BeginChild(this))
                break;
            aWalk = PrevChild(aWalk);
        }

        aFirst = aNode;
        aNode = aFirst;
        while (true)
        {
            Widget *aChild = ChildWidget(aNode);
            if (aChild->mVisible && aChild->Intersects(theWidget))
                MarkDirty(aChild);

            aFirst = NextChild(aFirst);
            if (aFirst == aEnd)
                return;
            aNode = aFirst;
        }
    }

    while (true)
    {
        Widget *aChild = ChildWidget(aNode);
        if (aChild->mVisible && aChild->Intersects(theWidget))
            MarkDirty(aChild);

        aFirst = NextChild(aFirst);
        if (aFirst == aEnd)
            return;
        aNode = aFirst;
    }
}

Sexy::WidgetContainer::WidgetContainer()
{
    mWidgets.mNext = &mWidgets;
    mWidgets.mPrev = &mWidgets;
    mUpdateIterator = &mWidgets;
    mWidgetManager = nullptr;
    mParent = nullptr;
    mUpdateIteratorModified = false;
    mLastWMUpdateCount = 0;
    mUpdateCnt = 0;
    mDirty = false;
    mX = 0;
    mY = 0;
    mWidth = 0;
    mHeight = 0;
    mHasAlpha = false;
    mClip = true;
    mWidgetFlagsMod.mAddFlags = 0;
    mWidgetFlagsMod.mRemoveFlags = 0;
    mPriority = 0;
    mZOrder = 0;
    mWidgetId = 0;
}

Widget *Sexy::WidgetContainer::GetWidgetAtHelper(
    int x, int y, int theFlags, bool *found, int *theWidgetX, int *theWidgetY)
{
    if (BeginChild(this) == EndChild(this))
    {
        *found = false;
        return nullptr;
    }

    int aHitFlags = (theFlags | mWidgetFlagsMod.mAddFlags) & ~mWidgetFlagsMod.mRemoveFlags;
    bool aFoundBelow = false;
    // GetWidgetAtHelper @0x3AD5A0 uses reverse iterators: test the front first.
    WidgetListLink *aNode = PrevChild(EndChild(this));

    while (true)
    {
        Widget *aWidget = ChildWidget(aNode);
        int aWidgetFlags = (aWidget->mWidgetFlagsMod.mAddFlags | aHitFlags) & ~aWidget->mWidgetFlagsMod.mRemoveFlags;

        if ((aWidgetFlags & 0x10) != 0 && aWidget->mVisible)
        {
            bool aChildFound = false;
            Widget *aResult = aWidget->GetWidgetAtHelper(
                x - aWidget->mX, y - aWidget->mY, aWidgetFlags, &aChildFound, theWidgetX, theWidgetY);
            if (aResult != nullptr || aChildFound)
            {
                *found = true;
                return aResult;
            }

            if (aWidget->mMouseVisible)
            {
                Rect aRect = aWidget->GetInsetRect();
                if (x >= aRect.mX && x < aRect.mX + aRect.mWidth && y >= aRect.mY && y < aRect.mY + aRect.mHeight)
                {
                    *found = true;
                    Widget *aHit = aWidget->IsPointVisible(x - aWidget->mX, y - aWidget->mY) ? aWidget : nullptr;
                    if (aHit != nullptr)
                    {
                        if (theWidgetX != nullptr)
                            *theWidgetX = x - aWidget->mX;
                        if (theWidgetY != nullptr)
                            *theWidgetY = y - aWidget->mY;
                        return aWidget;
                    }
                }
            }
        }

        if (mWidgetManager->mBaseModalWidget == aWidget)
            aFoundBelow = true;
        aNode = PrevChild(aNode);
        if (aNode == EndChild(this))
        {
            *found = false;
            return nullptr;
        }

        if (aFoundBelow)
            aHitFlags = (aHitFlags | mWidgetManager->mBelowModalFlagsMod.mAddFlags) &
                ~mWidgetManager->mBelowModalFlagsMod.mRemoveFlags;
    }
}

bool Sexy::WidgetContainer::IsBelowHelper(Sexy::Widget *theWidget1, Sexy::Widget *theWidget2, bool *found)
{
    WidgetListLink *aEnd = EndChild(this);
    WidgetListLink *aNode = BeginChild(this);
    if (aNode == aEnd)
        return false;

    while (true)
    {
        Widget *aWidget = ChildWidget(aNode);
        if (aWidget == theWidget1)
        {
            *found = true;
            return true;
        }
        if (aWidget == theWidget2)
        {
            *found = true;
            return false;
        }

        bool aResult = aWidget->IsBelowHelper(theWidget1, theWidget2, found);
        if (*found)
            return aResult;

        aNode = NextChild(aNode);
        if (aNode == aEnd)
            return false;
    }
}

bool Sexy::WidgetContainer::IsBelow(Sexy::Widget *theWidget1, Sexy::Widget *theWidget2)
{
    bool aFound = false;
    return IsBelowHelper(theWidget1, theWidget2, &aFound);
}

void Sexy::WidgetContainer::InsertWidgetHelper(Sexy::WidgetListLink *theWhere, Sexy::Widget *theWidget)
{
    WidgetListLink *aEnd = EndChild(this);
    WidgetListLink *aNode = theWhere;
    int aNewZ = theWidget->mZOrder;
    int aNodeZ = 0;

    if (aNode != aEnd)
    {
        aNodeZ = ChildWidget(aNode)->mZOrder;
        if (aNodeZ < aNewZ)
        {
            while (true)
            {
                aNode = NextChild(aNode);
                if (aNode == aEnd)
                    break;
                aNodeZ = ChildWidget(aNode)->mZOrder;
                if (aNodeZ >= aNewZ)
                    break;
            }
            if (aNode == aEnd)
            {
                while (aNode != BeginChild(this))
                {
                    aNode = PrevChild(aNode);
                    if (ChildWidget(aNode)->mZOrder <= aNewZ)
                    {
                        LinkChildBefore(NextChild(aNode), NewChild(theWidget));
                        return;
                    }
                }
                LinkChildBefore(aNode, NewChild(theWidget));
                return;
            }
        }

        if (BeginChild(this) == aNode || aNewZ >= aNodeZ)
        {
            LinkChildBefore(aNode, NewChild(theWidget));
            return;
        }
    }

    while (aNode != BeginChild(this))
    {
        aNode = PrevChild(aNode);
        if (ChildWidget(aNode)->mZOrder <= aNewZ)
        {
            LinkChildBefore(NextChild(aNode), NewChild(theWidget));
            return;
        }
    }
    LinkChildBefore(aNode, NewChild(theWidget));
}

void Sexy::WidgetContainer::AddWidget(Sexy::Widget *theWidget)
{
    for (WidgetListLink *aNode = BeginChild(this); aNode != EndChild(this); aNode = NextChild(aNode))
    {
        if (ChildWidget(aNode) == theWidget)
        {
            SortWidgets(theWidget);
            return;
        }
    }

    InsertWidgetHelper(&mWidgets, theWidget);

    WidgetManager *aManager = mWidgetManager;
    theWidget->mParent = this;
    theWidget->mWidgetManager = aManager;
    if (mWidgetManager != nullptr)
    {
        theWidget->AddedToManager(mWidgetManager);
        theWidget->OnViewportChanged();
        theWidget->MarkDirtyFull();
        mWidgetManager->RehupMouse();
    }
    MarkDirty();
    SortWidgets(theWidget);
}

void Sexy::WidgetContainer::BringToFront(Sexy::Widget *theWidget)
{
    WidgetListLink *aEnd = EndChild(this);
    for (WidgetListLink *aNode = BeginChild(this); aNode != aEnd; aNode = NextChild(aNode))
    {
        if (ChildWidget(aNode) != theWidget)
            continue;

        NoteIteratorErased(this, aNode);
        EraseChild(aNode);
        InsertWidgetHelper(&mWidgets, theWidget);
        theWidget->OrderInManagerChanged();
        return;
    }
}

void Sexy::WidgetContainer::BringToBack(Sexy::Widget *theWidget)
{
    WidgetListLink *aEnd = EndChild(this);
    for (WidgetListLink *aNode = BeginChild(this); aNode != aEnd; aNode = NextChild(aNode))
    {
        if (ChildWidget(aNode) != theWidget)
            continue;

        NoteIteratorErased(this, aNode);
        EraseChild(aNode);
        InsertWidgetHelper(BeginChild(this), theWidget);
        theWidget->OrderInManagerChanged();
        return;
    }
}

void Sexy::WidgetContainer::PutBehind(Sexy::Widget *theWidget, Sexy::Widget *theRefWidget)
{
    WidgetListLink *aEnd = EndChild(this);
    for (WidgetListLink *aNode = BeginChild(this); aNode != aEnd; aNode = NextChild(aNode))
    {
        if (ChildWidget(aNode) != theWidget)
            continue;

        NoteIteratorErased(this, aNode);
        EraseChild(aNode);

        WidgetListLink *aWhere = BeginChild(this);
        while (aWhere != aEnd && ChildWidget(aWhere) != theRefWidget)
            aWhere = NextChild(aWhere);

        InsertWidgetHelper(aWhere, theWidget);
        theWidget->OrderInManagerChanged();
        return;
    }
}

void Sexy::WidgetContainer::PutInfront(Sexy::Widget *theWidget, Sexy::Widget *theRefWidget)
{
    WidgetListLink *aEnd = EndChild(this);
    for (WidgetListLink *aNode = BeginChild(this); aNode != aEnd; aNode = NextChild(aNode))
    {
        if (ChildWidget(aNode) != theWidget)
            continue;

        NoteIteratorErased(this, aNode);
        EraseChild(aNode);

        WidgetListLink *aWhere = BeginChild(this);
        while (aWhere != aEnd && ChildWidget(aWhere) != theRefWidget)
            aWhere = NextChild(aWhere);
        if (aWhere != aEnd)
            aWhere = NextChild(aWhere);

        InsertWidgetHelper(aWhere, theWidget);
        theWidget->OrderInManagerChanged();
        return;
    }
}

void Sexy::WidgetContainer::RemoveWidget(Sexy::Widget *theWidget)
{
    WidgetListLink *aEnd = EndChild(this);
    for (WidgetListLink *aNode = BeginChild(this); aNode != aEnd; aNode = NextChild(aNode))
    {
        if (ChildWidget(aNode) != theWidget)
            continue;

        theWidget->WidgetRemovedHelper();
        NoteIteratorErased(this, aNode);
        theWidget->mParent = nullptr;
        EraseChild(aNode);
        break;
    }

    mWidgetVector.erase(std::remove(mWidgetVector.begin(), mWidgetVector.end(), theWidget), mWidgetVector.end());
}

Sexy::WidgetUserData *Sexy::WidgetContainer::GetUserData(Sexy::String const &theKey)
{
    std::map<Sexy::String, WidgetUserData *>::iterator aFound = mUserData.find(theKey);
    if (aFound == mUserData.end())
        return nullptr;
    return aFound->second;
}

void Sexy::WidgetContainer::RemoveUserData(Sexy::String const &theKey)
{
    std::map<Sexy::String, WidgetUserData *>::iterator aFound = mUserData.find(theKey);
    if (aFound == mUserData.end())
        return;
    delete aFound->second;
    mUserData.erase(aFound);
}

void Sexy::WidgetContainer::ClearUserData()
{
    for (std::map<Sexy::String, WidgetUserData *>::iterator aIt = mUserData.begin(); aIt != mUserData.end(); ++aIt)
        delete aIt->second;
    mUserData.clear();
}

void Sexy::WidgetContainer::AddUserData(Sexy::String const &theKey, Sexy::WidgetUserData *theData)
{
    if (mUserData.find(theKey) != mUserData.end())
        return;
    mUserData.insert(std::make_pair(theKey, theData));
}

Sexy::WidgetContainer::~WidgetContainer()
{
    ClearUserData();
    while (mWidgets.mNext != &mWidgets)
        EraseChild(mWidgets.mNext);
}

} // namespace Sexy
