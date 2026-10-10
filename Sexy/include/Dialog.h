// Dialog -- a modal panel with a header, lines, a footer and up to two buttons.
//
// `class Dialog : public Widget, public ButtonListener`. The ButtonListener
// sub-object sits immediately after Widget. Only Dialog's own table entries
// are declared here; the inherited Widget slots it does not override come
// from Widget.h.
//
// Overrides: AddedToManager, RemovedFromManager, Update, Draw,
// OrderInManagerChanged, SetColor, Resize, the key and mouse handlers.
// New slots: SetButtonFont, SetHeaderFont, SetLinesFont, GetPreferredHeight,
// IsModal, WaitForResult, and the three ButtonListener overrides.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_DIALOG_H
#define LAWNPROJECTTV_SEXY_INCLUDE_DIALOG_H

#include "ReconTypes.h"
#include "ButtonListener.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "SexyAppBase.h"
#include "TextLayout.h"
#include "Widget.h"
#include "WidgetContainer.h"

namespace Sexy
{

class DialogButton;

class Dialog : public Widget, public ButtonListener
{
public:
    DialogListener *mDialogListener;
    Image *mComponentImage;
    DialogButton *mYesButton;
    DialogButton *mNoButton;
    int mNumButtons;
    Sexy::String mDialogHeader;
    Sexy::String mDialogFooter;
    Sexy::String mDialogLines;
    // The lines the layout was last built from, so a redraw can skip a rebuild.
    Sexy::String mCachedLines;
    TextLayout mTextLayout;
    int mButtonMode;
    Font *mHeaderFont;
    Font *mLinesFont;
    int mTextAlign;
    int mLineSpacingOffset;
    int mButtonHeight;
    Insets mBackgroundInsets;
    Insets mContentInsets;
    int mSpaceAfterHeader;
    bool mDragging;
    int mDragMouseX;
    int mDragMouseY;
    int mId;
    bool mIsModal;
    int mResult;
    int mButtonHorzSpacing;
    int mButtonSidePadding;

    virtual ~Dialog();
    virtual void AddedToManager(WidgetManager *theWidgetManager);
    virtual void RemovedFromManager(WidgetManager *theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics *g);
    virtual void OrderInManagerChanged();
    virtual void SetColor(int theIdx, const Color &theColor);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual int KeyUp(Sexy::KeyCode theKey);
    virtual void MouseDown(int x, int y, int theClickCount);
    virtual void MouseDown(int x, int y, int theBtnNum, int theClickCount);
    virtual void MouseUp(int x, int y);
    virtual void MouseUp(int x, int y, int theClickCount);
    virtual void MouseUp(int x, int y, int theBtnNum, int theClickCount);
    virtual void MouseDrag(int x, int y);

    virtual void SetButtonFont(Font *theFont);
    virtual void SetHeaderFont(Font *theFont);
    virtual void SetLinesFont(Font *theFont);
    virtual int GetPreferredHeight(int theWidth);
    virtual bool IsModal();
    virtual int WaitForResult(bool autoKill);
    virtual void ButtonPress(int theId);
    virtual void ButtonDepress(int theId);
    virtual void ButtonDownTick(int theId);

    void EnsureFonts();
    int GetPreferredHeight();
    Dialog(Image *theComponentImage, Image *theDialogImage, int theId, bool isModal,
        const Sexy::String &theDialogHeader, const Sexy::String &theDialogLines, const Sexy::String &theDialogFooter,
        int theButtonMode);
};

} // namespace Sexy

using Sexy::Dialog;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_DIALOG_H
