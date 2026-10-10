// ScrollbarWidget is a track with two arrow buttons and a draggable thumb.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SCROLLBARWIDGET_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SCROLLBARWIDGET_H

#include "ReconTypes.h"
#include "ButtonListener.h"
#include "Graphics.h"
#include "ScrollbuttonWidget.h"
#include "Widget.h"
#include "WidgetContainer.h"

namespace Sexy
{

class ScrollListener;

class ScrollbarWidget : public Widget, public ButtonListener
{
public:
    virtual ~ScrollbarWidget();

    virtual void Update();
    virtual void Draw(Sexy::Graphics *theGraphics);
    virtual void MouseDown(int theX, int theY, int theClickCount);
    virtual void MouseDown(int theX, int theY, int theBtnNum, int theClickCount);
    virtual void MouseUp(int theX, int theY);
    virtual void MouseUp(int theX, int theY, int theBtnNum, int theClickCount);
    virtual void MouseDrag(int theX, int theY);

    virtual void SetInvisIfNoScroll(bool invisIfNoScroll);
    virtual void SetMaxValue(double theMaxValue);
    virtual void SetPageSize(double thePageSize);
    virtual void SetValue(double theValue);
    virtual void SetHorizontal(bool isHorizontal);
    virtual void SetButtonImages(Sexy::Image *theButtonImage, Sexy::Image *theDownImage, Sexy::Image *theDisabledImage);
    virtual void SetThumbImage(Sexy::Image *theThumbImage);
    virtual void SetBarImages(Sexy::Image *theBarImage, Sexy::Image *thePagingImage);
    virtual void SetButtonColors(int (*theColors)[3], int theCount);
    virtual void SetButtonColors(int (*theColors)[4], int theCount);
    virtual void SetButtonColor(int theIdx, Color const &theColor);
    virtual int ResizeScrollbar(int theX, int theY, int theWidth, int theHeight);
    virtual bool AtBottom();
    virtual void GoToBottom();
    virtual int DrawThumb(Sexy::Graphics *theGraphics, int theX, int theY, int theWidth, int theHeight);
    virtual int DrawThumb(Sexy::Graphics *theGraphics, TRect<int> theRect);
    virtual int GetTrackSize();
    virtual int GetThumbSize();
    virtual int GetThumbPosition();
    virtual int ClampValue();
    virtual void SetThumbPosition(int thePosition);
    virtual void ButtonPress(int theId);
    virtual void ButtonDepress(int theId);
    virtual void ButtonDownTick(int theId);
    virtual int ThumbCompare(int theX, int theY);

    ScrollbuttonWidget *mUpButton;
    ScrollbuttonWidget *mDownButton;
    bool mInvisIfNoScroll;
    int mId;
    double mValue;
    double mMaxValue;
    double mPageSize;
    bool mHorizontal;
    bool mPressedOnThumb;
    int mMouseDownThumbPos;
    int mMouseDownX;
    int mMouseDownY;
    int mUpdateMode;
    int mUpdateAcc;
    int mButtonAcc;
    int mLastMouseX;
    int mLastMouseY;
    ScrollListener *mScrollListener;
    Image *mThumbImage;
    Image *mBarImage;
    Image *mPagingImage;

    ScrollbarWidget(int theId, Sexy::ScrollListener *theScrollListener);
};

} // namespace Sexy

using Sexy::ScrollbarWidget;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SCROLLBARWIDGET_H
