// WidgetContainer is the child list of the Sexy widget tree.
//
// It has no base of its own. Widget and WidgetManager both derive from it.
// The list is the draw and update order. mWidgetVector is the same children
// rebuilt by SortWidgets for the key and focus walks. Hit testing, dirty
// marks and the user-data map live here too.

#pragma once

#include "ReconTypes.h"
#include <map>
#include <vector>

namespace Sexy {

class Graphics;
class Widget;
class WidgetManager;

// std::list's sentinel is only the two link words. A real child node is that
// base plus the Widget pointer. The sentinel is not a WidgetListNode and has
// no mWidget. External code walks mWidgets through these links, so the list
// stays a circular pair rather than a std::list object.
struct WidgetListLink
{
    WidgetListLink *mNext;
    WidgetListLink *mPrev;
};

struct WidgetListNode : WidgetListLink
{
    Widget *mWidget;
};

struct WidgetListHead : WidgetListLink
{
};

// The value type of mUserData. Callers derive from it and the map deletes the
// stored pointer through this destructor.
class WidgetUserData
{
public:
    virtual ~WidgetUserData();
};

class WidgetContainer
{
public:
    virtual ~WidgetContainer();

    virtual Rect GetRect();
    virtual bool Intersects(WidgetContainer *theOther);
    virtual void SortWidgets();
    virtual void SortWidgets(Widget *theWidget);
    virtual void AddWidget(Widget *theWidget);
    virtual void RemoveWidget(Widget *theWidget);
    virtual bool HasWidget(Widget *theWidget);
    virtual Widget *FindWidget(int theId);
    virtual void DisableWidget(Widget *theWidget);
    virtual void RemoveAllWidgets(bool doDelete, bool recursive, WidgetManager *theManager);
    virtual void SetFocus(Widget *theWidget);
    virtual bool IsBelow(Widget *theWidget1, Widget *theWidget2);
    virtual void MarkAllDirty();
    virtual void BringToFront(Widget *theWidget);
    virtual void BringToBack(Widget *theWidget);
    virtual void PutBehind(Widget *theWidget, Widget *theRefWidget);
    virtual void PutInfront(Widget *theWidget, Widget *theRefWidget);
    virtual Point GetAbsPos() const;
    virtual Point GetAbsPosInManager() const;
    virtual Point GetAbsPosInScreen() const;
    virtual Point GetCenter() const;
    virtual Point GetAbsCenter() const;
    virtual Point GetAbsCenterInScreen() const;
    virtual void MarkDirty();
    virtual void MarkDirtyFull();
    virtual void MarkDirtyFull(WidgetContainer *theWidget);
    virtual void MarkDirty(WidgetContainer *theWidget);
    virtual void AddedToManager(WidgetManager *theWidgetManager);
    virtual void RemovedFromManager(WidgetManager *theWidgetManager);
    virtual void Update();
    virtual void UpdateAll(ModalFlags *theFlags);
    virtual void UpdateF(float theFrac);
    virtual void UpdateFAll(ModalFlags *theFlags, float theFrac);
    virtual void DrawPre(Graphics *g);
    virtual void Draw(Graphics *g);
    virtual void DrawOther(Graphics *g);
    virtual void DrawAll(ModalFlags *theFlags, Graphics *g);
    virtual void SysColorChangedAll();
    virtual void SysColorChanged();

    WidgetListHead mWidgets;
    std::vector<Widget *> mWidgetVector;
    WidgetManager *mWidgetManager;
    WidgetContainer *mParent;
    bool mUpdateIteratorModified;
    WidgetListLink *mUpdateIterator;
    int mLastWMUpdateCount;
    int mUpdateCnt;
    bool mDirty;
    int mX;
    int mY;
    int mWidth;
    int mHeight;
    bool mHasAlpha;
    bool mClip;
    FlagsMod mWidgetFlagsMod;
    int mPriority;
    int mZOrder;
    std::map<Sexy::String, WidgetUserData *> mUserData;
    int mWidgetId;

    WidgetContainer();
    Widget *GetWidgetAtHelper(int x, int y, int theFlags, bool *found, int *theWidgetX, int *theWidgetY);
    bool IsBelowHelper(Widget *theWidget1, Widget *theWidget2, bool *found);
    void InsertWidgetHelper(WidgetListLink *theWhere, Widget *theWidget);
    WidgetUserData *GetUserData(Sexy::String const &theKey);
    void RemoveUserData(Sexy::String const &theKey);
    void ClearUserData();
    void AddUserData(Sexy::String const &theKey, WidgetUserData *theData);
};

} // namespace Sexy

using Sexy::WidgetContainer;
