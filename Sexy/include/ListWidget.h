// ListWidget is a scrollable line list. ScrollListener is the second base.
//
// Virtual order matches the binary so overrides still bind. Do not reorder
// the declarations.

#pragma once

#include "Font.h"
#include "Graphics.h"
#include "ScrollListener.h"
#include "Widget.h"
#include <vector>

namespace Sexy {

class ListListener;
class ScrollbarWidget;
class Image;

class ListWidget : public Widget, public ScrollListener
{
public:
    virtual ~ListWidget();

    virtual void RemovedFromManager(WidgetManager *theWidgetManager);
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void OrderInManagerChanged();
    virtual void SetColor(int theIdx, Color const &theColor);
    virtual void Resize(int theX, int theY, int theWidth, int theHeight);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual void MouseLeave();
    virtual void MouseMove(int theX, int theY);
    virtual void MouseDown(int theX, int theY, int theClickCount);
    virtual void MouseDown(int theX, int theY, int theBtnNum, int theClickCount);
    virtual void MouseWheel(int theDelta);
    virtual Sexy::String GetSortKey(int theIdx);
    virtual void Sort(bool descending);
    virtual Sexy::String GetStringAt(int theIdx);
    virtual int AddLine(Sexy::String const &theLine, bool sorted);
    virtual void SetLine(int theIdx, Sexy::String const &theLine);
    virtual int GetLineCount();
    virtual int GetLineIdx(Sexy::String const &theLine);
    virtual void SetColor(Sexy::String const &theLine, Color const &theColor);
    virtual void SetLineColor(int theIdx, Color const &theColor);
    virtual void RemoveLine(int theIdx);
    virtual void RemoveAll();
    virtual int GetOptimalWidth();
    virtual int GetOptimalHeight();
    virtual void ScrollPosition(int theId, double thePosition);
    virtual void SetSelect(int theIdx);
    virtual int MoveSelect(int theDelta);

    int mId;
    Sexy::Font *mFont;
    ScrollbarWidget *mScrollbar;
    int mJustify;
    // Lines one wheel notch scrolls. Constructor stores 5.
    int mWheelLines;
    std::vector<Sexy::String> mLines;
    std::vector<Sexy::Color> mLineColors;
    double mPosition;
    double mPageSize;
    int mHiliteIdx;
    int mSelectIdx;
    ListListener *mListListener;
    ListWidget *mParent;
    ListWidget *mChild;
    bool mSortFromChild;
    bool mDrawOutline;
    int mMaxNumericPlaces;
    int mItemHeight;
    bool mDrawSelectWhenHilited;
    bool mDoFingerWhenHilited;
    int mTextOffset;
    Sexy::Image *mItemImage;

    ListWidget(int theId, Sexy::Font *theFont, Sexy::ListListener *theListener);
    int SetHilite(int theIdx, bool notify);
};

} // namespace Sexy

using Sexy::ListWidget;
