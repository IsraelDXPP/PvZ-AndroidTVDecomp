// EditWidget is a single-line text field and the second base is the keyboard sink.
//
// Virtual order matches the binary so overrides still bind. Do not reorder
// the declarations. EditSink methods that this class overrides stay in the
// base's slot order.

#pragma once

#include "EditSink.h"
#include "Font.h"
#include "Graphics.h"
#include "SexyAppBase.h"
#include "Widget.h"
#include "WidgetContainer.h"
#include <vector>

namespace Sexy {

class EditOption;
class EditSink;

// The platform keyboard the widget registers with. AndroidAppDriver implements
// this as a second base; RegisterEditSink stores that sub-object.
class EditSource
{
public:
    virtual ~EditSource() {}
    virtual int GetEditText() { return 0; }
    virtual int RegisterEditSink(Sexy::EditSink *) { return 0; }
    virtual int UnregisterEditSink(Sexy::EditSink *) { return 0; }
    virtual void SetEditOption(Sexy::EditSink *, Sexy::EditOption const &) {}
    virtual void UpdateEditArea(Sexy::EditSink *) {}
    virtual void ShowEditBox(Sexy::EditSink *) {}
    virtual void HideEditBox(Sexy::EditSink *) {}
    virtual Sexy::EditSink *GetCurrentEditSink() { return nullptr; }
};

// Four hooks the on-screen editor calls back into. The base does nothing.
class EditController
{
public:
    virtual ~EditController() {}
    virtual int EditFinished(Sexy::EditSink *theSink)
    {
        (void)theSink;
        return 0;
    }
    virtual int EditCancelled(Sexy::EditSink *theSink)
    {
        (void)theSink;
        return 0;
    }
    virtual int KeyboardWasShown(Sexy::EditSink *theSink, TRect<int> const &theRect)
    {
        (void)theSink;
        (void)theRect;
        return 0;
    }
    virtual int KeyboardWillBeHidden(Sexy::EditSink *theSink)
    {
        (void)theSink;
        return 0;
    }
};

// Fallback controller used when none was installed.
class DefaultEditController : public EditController
{
public:
    virtual ~DefaultEditController() {}
};

// Record handed to EditSource::SetEditOption. Three strings, the keyboard
// mode, and the text colour. The trailing word the driver also copies is
// written 0 and never read here, so it is not a field.
struct EditOption
{
    Sexy::String mText;
    Sexy::String mTitle;
    Sexy::String mHint;
    int mMode;
    Sexy::Color mColor;
};

class EditWidget : public Widget, public EditSink
{
public:
    virtual ~EditWidget();

    virtual void MarkDirty();
    virtual void AddedToManager(Sexy::WidgetManager *theWidgetManager);
    virtual void RemovedFromManager(Sexy::WidgetManager *theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual bool WantsFocus();
    virtual void GotFocus();
    virtual void LostFocus();
    virtual void GotWindowFocus();
    virtual void LostWindowFocus();
    virtual int KeyChar(char theChar);
    virtual int KeyUnicode(int theChar);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual void MouseEnter();
    virtual void MouseLeave();
    virtual void MouseDown(int theX, int theY, int theBtnNum, int theClickCount);
    virtual void MouseDown(int theX, int theY, int theClickCount);
    virtual void MouseUp(int theX, int theY);
    virtual void MouseUp(int theX, int theY, int theBtnNum, int theClickCount);
    virtual void MouseUp(int theX, int theY, int theClickCount);
    virtual void MouseDrag(int theX, int theY);

    // One width-check entry: a private font copy and the pixel budget it
    // measures the text against.
    struct WidthCheckFont
    {
        Sexy::Font *mFont;
        int mMaxPixels;
    };

    int mId;
    Sexy::String mString;
    Sexy::String mPasswordDisplayString;
    Sexy::Font *mFont;
    // Horizontal inset of the text clip. Constructor stores 4.
    int mClipInset;
    // DrawString x. Constructor stores 4.
    int mTextOffsetX;
    // Extra x added to the highlight and caret clips. Constructor stores 0.
    int mHighlightOffsetX;
    int mEditMode;
    Sexy::String mTitle;
    Sexy::String mHint;
    std::vector<WidthCheckFont> mWidthCheckFonts;
    Sexy::EditListener *mEditListener;
    bool mCursorVisible;
    // Draw treats this as focused even when the window has not focused the field.
    bool mDrawFocused;
    // A multi-click is in progress. MouseUp finishes the word highlight.
    bool mMultiClick;
    int mCursorPos;
    int mSelectionPos;
    int mHiliteStart;
    int mHiliteEnd;
    int mBlinkTimer;
    int mBlinkPeriod;
    int mScrollPos;
    int mMaxChars;
    int mMaxPixels;
    char mPasswordChar;
    Sexy::String mUndoString;
    int mUndoCursorPos;
    int mUndoSelectionPos;
    int mLastCursorPos;
    Sexy::EditSource *mEditSource;
    Sexy::EditController *mEditController;
    Sexy::EditOption mEditOption;
    bool mKeyboardShown;

    // EditSink overrides. Return types stay the base's so the override binds.
    int GetEditWidget();
    int GetEditMode();
    void SetEditMode(int theMode);
    int RegisterEditSink();
    int UnregisterEditSink();
    virtual int EditFinished();
    virtual void EditCancelled();
    virtual void ShowKeyboard();
    virtual void HideKeyboard();
    virtual void KeyboardWasShown(Sexy::TRect<int> const &theRect);
    virtual void KeyboardWillBeHidden();
    int SetEditState(Sexy::EditState const &theState);
    EditController *GetEditController();

    Sexy::Rect GetEditArea();
    void UpdateEditText();
    void SetEditController(Sexy::EditController *theController);
    Sexy::EditOption *GetEditOption();
    int GetEditSelectionStart();
    void SetEditSelection(int theStart, int theEnd);
    Sexy::String GetTitle();
    Sexy::String GetHint();
    Sexy::String GetEditText();
    void SetTitle(Sexy::String const &theTitle);
    void SetHint(Sexy::String const &theHint);
    bool IsPartOfWord(char theChar);
    int GetEditSelectionEnd();
    void SetText(Sexy::String const &theText, bool scrollToStart, bool clearSelection, bool tellListener);
    EditWidget(int theId, Sexy::EditListener *theListener);
    Sexy::Font *ClearWidthCheckFonts();
    Sexy::Font *AddWidthCheckFont(Sexy::Font *theFont, int theMaxPixels);
    Sexy::Font *SetFont(Sexy::Font *theFont, Sexy::Font *theWidthCheckFont);
    Sexy::String &GetDisplayString();
    int HiliteWord();
    int GetCharAt(int theX, int theY);
    void FocusCursor(bool byPage);
    void UpdateCaretPos();
    void EnforceMaxPixels();
    int ProcessKey(Sexy::KeyCode theKey, int theChar);
};

} // namespace Sexy

using Sexy::EditWidget;
