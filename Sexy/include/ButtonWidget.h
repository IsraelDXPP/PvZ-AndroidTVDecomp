// ButtonWidget -- a labelled button with four images and a fade.
//
// `class ButtonWidget : public SelectableWidget`. The class's own members
// start after SelectableWidget. Entries that still point at SelectableWidget,
// Widget or WidgetContainer code are not redeclared here.
//
// Overrides: Update, Draw, SetDisabled, KeyDown, KeyUp, the mouse and touch
// handlers. Own virtuals, in binary order: DrawButtonImage, SetFont, SetLabel,
// IsButtonDown, LockState, UnlockState, IsLockedToState, IsLockedToDownState,
// UpdateLabel. MSVC gives the destructor one slot where the binary gives two;
// only the declaration order matters.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_BUTTONWIDGET_H
#define LAWNPROJECTTV_SEXY_INCLUDE_BUTTONWIDGET_H

#include "ReconTypes.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "SelectableWidget.h"
#include "TextLayout.h"
#include "Widget.h"

namespace Sexy
{

// Six floats Font::StringExtents writes for a label: the glyph box, then the
// advance. Draw uses the minimum Y as a vertical nudge and the height as the
// reserved line height.
using LabelExtents = Font::TextExtents;

class ButtonListener;

class ButtonWidget : public SelectableWidget
{
public:
    virtual ~ButtonWidget();

    virtual void Update() override;
    virtual void Draw(Sexy::Graphics *g) override;
    virtual void SetDisabled(bool isDisabled) override;
    virtual int KeyDown(Sexy::KeyCode theKey) override;
    virtual int KeyUp(Sexy::KeyCode theKey) override;
    virtual void MouseEnter() override;
    virtual void MouseLeave() override;
    virtual void MouseMove(int x, int y) override;
    virtual void MouseDown(int x, int y, int theClickCount) override;
    virtual void MouseDown(int x, int y, int theBtnNum, int theClickCount) override;
    virtual void MouseUp(int x, int y) override;
    virtual void MouseUp(int x, int y, int theClickCount) override;
    virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount) override;
    virtual void TouchEnter() override;
    virtual void TouchLeave() override;
    virtual void TouchDown(int theId, int x, int y) override;
    virtual void TouchUp(int theId, int x, int y) override;

    virtual void DrawButtonImage(Sexy::Graphics *g, Sexy::Image *image, const TRect<int> &rect, int x, int y);
    virtual void SetFont(Sexy::Font *theFont);
    virtual void SetLabel(const Sexy::String &theLabel);
    virtual bool IsButtonDown();
    virtual void LockState(int theState);
    virtual void UnlockState();
    virtual bool IsLockedToState(int theState) const;
    virtual bool IsLockedToDownState() const;
    virtual void UpdateLabel();

    ButtonWidget(int theId, Sexy::ButtonListener *theButtonListener);

    bool HaveButtonImage(Sexy::Image *theImage, const TRect<int> &theRect);

    Sexy::String mLabel;
    // Caches the label the layout was built from, so UpdateLabel can skip a rebuild.
    Sexy::String mCachedLabel;
    TextLayout mTextLayout;
    LabelExtents mLabelExtents;
    Insets mInsets;
    int mLabelJustify;
    Sexy::Font *mFont;
    Sexy::Image *mButtonImage;
    Sexy::Image *mOverImage;
    Sexy::Image *mDownImage;
    Sexy::Image *mDisabledImage;
    TRect<int> mNormalRect;
    TRect<int> mOverRect;
    TRect<int> mDownRect;
    TRect<int> mDisabledRect;
    bool mInverted;
    bool mBtnNoDraw;
    bool mFrameNoDraw;
    Sexy::ButtonListener *mListener;
    int mUnusedWord;
    double mOverAlpha;
    double mOverAlphaSpeed;
    double mOverAlphaFadeInSpeed;
    // The return-key press KeyDown and KeyUp maintain.
    bool mReturnPressed;
    bool mMousePressed = false;
    bool IsPointVisible(int x, int y) override;
    // Set while LockState has pinned mLockedState.
    bool mStateLocked;
    int mLockedState;
};

} // namespace Sexy

using Sexy::ButtonWidget;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_BUTTONWIDGET_H
