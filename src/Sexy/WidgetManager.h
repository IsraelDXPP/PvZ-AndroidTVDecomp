// WidgetManager owns the widget tree, mouse capture, focus and the touch map.
//
// It is a WidgetContainer. Mouse and key methods dispatch into the widget
// under the pointer or the focused widget. The four touch methods update the
// per-finger records and then hand the batch to the held widget.

#pragma once

#include "Graphics.h"
#include "Graphics3D.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include <list>
#include <map>
#include <vector>

namespace Sexy {

// One held-key record. Callers read and write it as Get(theKey)->mDown.
struct KeyState
{
    bool mDown;

    KeyState() : mDown(false) {}
};

// Which key codes are currently held. Get inserts a not-held record for a missing key.
class KeyDownMap
{
public:
    std::map<int, KeyState> mDown;

    KeyState *Find(int theKey);
    const KeyState *Find(int theKey) const;
    KeyState *Get(int theKey);
};

// One finger's history. Current, previous and begin are the three slots
// UpdateTouchInfo copies an Event into. State 2 means the finger is up.
struct TouchSample
{
    int mModifiers;
    int mModifierMask;
    int mDevice;
    int mEventType;
    int mEventCode;
    int mEventCodeMask;
    int mMatchEvent;
    int mStandard;
    int mScaleDivisorX;
    int mScaleDivisorY;
    int mSavedModifierMask;
    int mSavedEventCodeMask;
    int mSavedMatchEvent;

    TouchSample();
};

struct TouchInfo
{
    TouchSample mCurrent;
    TouchSample mPrevious;
    TouchSample mBegin;

    TouchInfo();

    int &State() { return mBegin.mMatchEvent; }
    int State() const { return mBegin.mMatchEvent; }
};

class TouchInfoMap
{
public:
    std::map<int, TouchInfo> mInfo;

    TouchInfo *Find(int theKey);
    const TouchInfo *Find(int theKey) const;
    TouchInfo &Get(int theKey);
};

// Gamepad button to the key code widgets expect.
class GamepadKeyMap
{
public:
    std::map<int, int> mKeys;

    int *Find(int theKey);
    const int *Find(int theKey) const;
    int &Get(int theKey);
};

class WidgetManager : public WidgetContainer
{
public:
    virtual ~WidgetManager();

    virtual void DisableWidget(Sexy::Widget *theWidget);
    virtual void SetFocus(Sexy::Widget *theWidget);

    // Saved when a base modal widget is added, restored when it is removed.
    struct PreModalInfo
    {
        Widget *mWidget;
        Widget *mBaseModalWidget;
        Widget *mFocusWidget;
        FlagsMod mFlagsMod;
    };

    Graphics *mCurG;
    SexyAppBase *mApp;
    MemoryImage *mImage;
    MemoryImage *mTransientImage;
    bool mLastHadTransients;
    Widget *mPopupCommandWidget;
    std::vector<std::pair<Sexy::Widget *, int> > mDeferredOverlayWidgets;
    int mMinDeferredOverlayPriority;
    bool mHasFocus;
    Widget *mFocusWidget;
    Widget *mLastDownWidget;
    Widget *mOverWidget;
    Widget *mBaseModalWidget;
    FlagsMod mLostFocusFlagsMod;
    FlagsMod mBelowModalFlagsMod;
    FlagsMod mDefaultBelowModalFlagsMod;
    std::list<PreModalInfo> mPreModalInfoList;
    Rect mMouseDestRect;
    Rect mMouseSourceRect;
    bool mMouseIn;
    int mLastMouseX;
    int mLastMouseY;
    int mDownButtons;
    int mActualDownButtons;
    int mLastInputUpdateCnt;
    KeyDownMap mKeyDown;
    int mLastDownButtonId;
    int mWidgetFlags;
    // Finger id the manager is currently tracking. -1 when none is live.
    int mActiveTouchId;
    TouchInfoMap mTouchInfo;
    // 1 and 2 are keyboard, 3 is mouse, 4 is touch.
    int mInteractiveMethod;
    bool mAllowFullKeyboardMode;
    GamepadKeyMap mGamepadKeyMap;

    void FreeResources();
    int GetWidgetFlags();
    Widget *GetAnyWidgetAt(int x, int y, int *theWidgetX, int *theWidgetY);
    Widget *GetWidgetAt(int x, int y, int *theWidgetX, int *theWidgetY);
    int IsLeftButtonDown();
    bool IsMiddleButtonDown();
    bool IsRightButtonDown();
    int DoMouseUps(Sexy::Widget *theWidget, unsigned long theDownCode);
    void DoMouseUps();
    int RemapMouse(int &theX, int &theY);
    WidgetManager *RemapMouse(float &theX, float &theY);
    int UnmapMouse(int &theX, int &theY);
    void MouseEnter(Sexy::Widget *theWidget);
    void MouseLeave(Sexy::Widget *theWidget);
    void SetBaseModal(Sexy::Widget *theWidget, Sexy::FlagsMod const &theFlags);
    void AddBaseModal(Sexy::Widget *theWidget, Sexy::FlagsMod const &theFlags);
    void AddBaseModal(Sexy::Widget *theWidget);
    void RemoveBaseModal(Sexy::Widget *theWidget);
    int Resize(TRect<int> const &theDest, TRect<int> const &theSource);
    void GotFocus();
    int InitModalFlags(Sexy::ModalFlags *theFlags);
    bool UpdateFrame();
    bool UpdateFrameF(float theFrac);
    void SetPopupCommandWidget(Sexy::Widget *theWidget);
    void RemovePopupCommandWidget();
    int MousePosition(int x, int y);
    int RehupMouse();
    int MouseUp(int x, int y, int theClickCount);
    int MouseDown(int x, int y, int theClickCount);
    int MouseDrag(int x, int y);
    int MouseMove(int x, int y);
    int MouseExit(int x, int y);
    void MouseWheel(int x, int y);
    void MouseWheel(int theDelta);
    void TouchEnter(Sexy::Widget *theWidget);
    void TouchLeave(Sexy::Widget *theWidget);
    void TouchDown(const std::vector<Sexy::Event> &theEvents);
    void TouchMove(const std::vector<Sexy::Event> &theEvents);
    void TouchUp(const std::vector<Sexy::Event> &theEvents);
    void TouchCancel(const std::vector<Sexy::Event> &theEvents);
    Sexy::WidgetManager *UpdateTouchInfo(const std::vector<Sexy::Event> &theEvents);
    void ConvertTouchEvents(const std::vector<Sexy::Event> &theEvents,
                            std::vector<Sexy::Touch> &theTouches);
    void AxisMoved(Sexy::Event const &theEvent);
    int UserEvent(Sexy::Event const &theEvent);
    int GetInteractiveMethod();
    bool IsInKeyboardMode();
    void SetAllowFullKeyboardMode(bool allow);
    bool AllowFullKeyboardMode();
    Rect GetScreenRect() const;
    WidgetManager *DeferOverlay(Sexy::Widget *theWidget, int thePriority);
    TouchInfo *GetTouchInfo(int theId);
    bool IsTouchDown(int theId);
    int KeyChar(char theChar);
    int KeyUnicode(int theChar);
    int KeyDown(Sexy::KeyCode theKey);
    int KeyUp(Sexy::KeyCode theKey);
    void LostFocus();
    int KeyUp(Sexy::Event const &theEvent);
    int KeyDown(Sexy::Event const &theEvent);
    void InitGamepadKeyMap();
    WidgetManager(Sexy::SexyAppBase *theApp);
    int FlushDeferredOverlayWidgets(int theMaxPriority);
    void DrawWidgetsTo(Sexy::Graphics *g);
    int DrawScreen();
};

} // namespace Sexy

using Sexy::WidgetManager;
