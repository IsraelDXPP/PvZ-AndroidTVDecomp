// Widget is a focusable mouse, touch and key target.
//
// It draws nothing by default. TouchBegan claims the first touch id into
// mLiveTouchId and converts screen coordinates to widget space. The later
// touch methods find that id and forward to the single-touch overload.
// TouchEvents dispatches a batch by the first event's phase.
//
// Virtual order matches the binary so overrides in src/Game still bind.
// WidgetContainer owns slots before these. Do not reorder the declarations.

#pragma once

#include "Graphics.h"
#include "SharedImageRef.h"
#include "WidgetContainer.h"
#include <vector>

namespace Sexy {

// One finger. ConvertTouchEvents builds one of these from one Event.
// Phase is 0 down, 1 moved, 3 up, 4 cancelled.
struct Touch
{
    int mId;
    float mX;
    float mY;
    float mPrevX;
    float mPrevY;
    int mActive;
    double mTime;
    int mPhase;
};

class Widget : public WidgetContainer
{
public:
    virtual ~Widget();

    virtual void Update();
    virtual void UpdateF(float theFrac);
    virtual void DrawPre(Sexy::Graphics *g);
    virtual void Draw(Sexy::Graphics *g);
    virtual void DrawOther(Sexy::Graphics *g);

    virtual void OrderInManagerChanged();
    virtual void SetVisible(bool isVisible);
    virtual void SetColors(int (*theColors)[3], int theNumColors);
    virtual void SetColors(int (*theColors)[4], int theNumColors);
    virtual void SetColor(int theIdx, const Color &theColor);
    virtual Color GetColor(int theIdx);
    virtual Color GetColor(int theIdx, const Color &theDefaultColor);
    virtual void SetDisabled(bool isDisabled);
    virtual void ShowFinger(bool on);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual void Resize(const TRect<int> &theRect);
    virtual void Move(int theNewX, int theNewY);
    virtual bool WantsFocus();
    virtual void DrawOverlay(Sexy::Graphics *g);
    virtual void DrawOverlay(Sexy::Graphics *g, int thePriority);
    virtual void SetDefaultFocus(Widget *theWidget);
    virtual void SetFocus(Widget *theWidget, bool force);
    virtual void SetFocusLink(WidgetFocusDir theDir, Widget *theWidget);
    virtual void SetFocusLinks(Widget *up, Widget *down, Widget *left, Widget *right);
    virtual void FocusDirectionHint(WidgetFocusDir theDir);
    virtual void GotFocus();
    virtual void LostFocus();
    virtual bool HasWindowFocus();
    virtual void SetWindowFocus(bool focused);
    virtual void GotWindowFocus();
    virtual void LostWindowFocus();
    virtual int KeyChar(char theChar);
    virtual int KeyUnicode(int theChar);
    virtual int KeyDown(const Sexy::Event &theEvent);
    virtual int KeyUp(const Sexy::Event &theEvent);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual int KeyUp(Sexy::KeyCode theKey);
    virtual void MouseEnter();
    virtual void MouseLeave();
    virtual void MouseMove(int x, int y);
    virtual void MouseDown(int x, int y, int theClickCount);
    virtual void MouseDown(int x, int y, int theBtnNum, int theClickCount);
    virtual void MouseUp(int x, int y);
    virtual void MouseUp(int x, int y, int theClickCount);
    virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount);
    virtual void MouseDrag(int x, int y);
    virtual void MouseWheel(int theDelta);
    virtual void MouseWheel(int x, int y);
    virtual void TouchEnter();
    virtual void TouchLeave();
    virtual void TouchDown(int theId, int x, int y);
    virtual void TouchMove(int theId, int x, int y);
    virtual void TouchUp(int theId, int x, int y);
    virtual void TouchCancel(int theId, int x, int y);
    virtual void TouchEvents(const std::vector<Sexy::Event> &theEvents);
    virtual void TouchDown(const std::vector<Sexy::Event> &theEvents);
    virtual void TouchMove(const std::vector<Sexy::Event> &theEvents);
    virtual void TouchUp(const std::vector<Sexy::Event> &theEvents);
    virtual void TouchCancel(const std::vector<Sexy::Event> &theEvents);
    virtual void TouchBegan(std::vector<Touch> &theTouches);
    virtual void TouchMoved(std::vector<Touch> &theTouches);
    virtual void TouchEnded(std::vector<Touch> &theTouches);
    virtual void TouchCanceled(std::vector<Touch> &theTouches);
    virtual void TouchBegan(Touch *theTouch);
    virtual void TouchMoved(Touch *theTouch);
    virtual void TouchEnded(Touch *theTouch);
    virtual void TouchCanceled(Touch *theTouch);
    virtual void TouchesCanceled();
    virtual void AxisMoved(const Sexy::Event &theEvent);
    virtual void UserEvent(const Sexy::Event &theEvent);
    virtual bool IsPointVisible(int x, int y);
    virtual bool IsOnScreen();
    virtual TRect<int> WriteCenteredLine(Sexy::Graphics *g, int y, const Sexy::String &text);
    virtual TRect<int> WriteCenteredLine(Sexy::Graphics *g, int y, const Sexy::String &text,
                                   Color color1, Color color2, const TPoint<int> &offset);
    virtual void WriteString(Sexy::Graphics *g, const Sexy::String &text, int x, int y,
                             int width, int justification, bool drawString, int offset,
                             int length);
    virtual void WriteWordWrapped(Sexy::Graphics *g, const TRect<int> &rect,
                                  const Sexy::String &text, int lineSpacing,
                                  int justification);
    virtual int GetWordWrappedHeight(Sexy::Graphics *g, int width, const Sexy::String &text,
                                     int lineSpacing);
    virtual int GetNumDigits(int number);
    virtual void WriteNumberFromStrip(Sexy::Graphics *g, int number, int x, int y,
                                      Sexy::Image *image, int digitWidth);
    virtual bool Contains(int x, int y);
    virtual TRect<int> GetInsetRect();
    virtual Widget *FindFocusableWidget(int direction, Widget *relativeWidget);
    virtual Widget *FindClosest(Widget *widget);
    virtual int OnArrowKeys(Sexy::KeyCode theKey);
    virtual int OnKeyReturn();
    virtual int OnKeyEscape();

    Widget();

    Widget *SetDefaultAddToManager(bool add);
    int WidgetRemovedHelper();
    bool IsFocusable();
    int DoKeyChar(char theChar);
    int DoKeyUnicode(int theChar);
    int KeyDownUp(Sexy::KeyCode theKey, bool isDown);
    Widget *GetTopLevel();
    void DeferOverlay(int thePriority);
    int AttachSharedImage(Sexy::SharedImageRef &theRef);
    void RemovedSharedImage(Sexy::Image *theImage);
    Sexy::WidgetAnimator *SetAnimator(Sexy::WidgetAnimator *theAnimator);
    Sexy::WidgetAnimator *GetAnimator();
    Sexy::WidgetAnimator *SetAnimatorForState(Sexy::WidgetAnimationState theState, Sexy::WidgetAnimator *theAnimator);
    Sexy::WidgetAnimator *GetAnimatorForState(Sexy::WidgetAnimationState theState);
    int Layout(int theLayoutFlags, Sexy::Widget *theRelativeWidget, int theLeftPad, int theTopPad, int theWidthPad, int theHeightPad);
    void RemovedSharedImages();

    // Copied into mAddToManager by the constructor.
    static char DefaultAddToManager;

    bool mVisible;
    bool mMouseVisible;
    bool mDisabled;
    bool mHasFocus;
    bool mIsDown;
    bool mIsOver;
    bool mHasTransparencies;
    bool mAddToManager;
    std::vector<Color> mColors;
    Insets mMouseInsets;
    bool mDoFinger;
    bool mWantsFocus;
    bool mMousePreservesFocus = false;
    bool mSelected;
    bool mDeselectOnEscape;
    bool mFocusable;
    // Written 0, 0, -1, -1 by the constructor. No reader in this tree.
    int mUnusedRect[4];
    Color mHighlightColor;
    bool mUsesWidgetColor;
    Widget *mFocusLinks[4];
    // [0] is the shift-tab target, [1] is the tab target.
    Widget *mTabFocus[2];
    Widget *mFocusedChildWidget;
    Widget *mDefaultFocusWidget;
    bool mHasWindowFocus;
    // Touch id claimed by TouchBegan. -1 when no touch is live.
    int mLiveTouchId;
    Sexy::Event *mEvent;
    Sexy::WidgetAnimator *mAnimator;
    Sexy::WidgetAnimator *mAnimatorForState[3];

    // WriteString stores this into Graphics as the colour-code flag.
    static const unsigned char mWriteColoredString = 1;
};

} // namespace Sexy

using Sexy::Widget;
