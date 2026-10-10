// Slider is a draggable 0..1 value widget.
//
// mHorizontal picks the axis. mDragging is set while the thumb is held.
// The grab offsets are the point inside the thumb the pointer landed on.
// TouchCancel reports the value and then forwards to Widget.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SLIDER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SLIDER_H

#include "ReconTypes.h"
#include "Graphics.h"
#include "SelectableWidget.h"
#include "SliderListener.h"
#include "Widget.h"
#include <vector>

namespace Sexy
{

class Slider : public SelectableWidget
{
public:
    virtual ~Slider();

    virtual void Draw(Sexy::Graphics *g);
    virtual int KeyDown(Sexy::KeyCode theKey);
    virtual void MouseLeave();
    virtual void MouseMove(int x, int y);
    virtual void MouseDown(int x, int y, int theClickCount);
    virtual void MouseUp(int x, int y);
    virtual void MouseDrag(int x, int y);
    virtual void TouchDown(int theId, int x, int y);
    virtual void TouchMove(int theId, int x, int y);
    virtual void TouchUp(int theId, int x, int y);
    virtual void TouchCancel(const std::vector<Sexy::Event> &theEvents);

    virtual void SetValue(double theValue);
    virtual bool HasTransparencies();

    Slider(Sexy::Image *theTrack, Sexy::Image *theThumb, int theId, Sexy::SliderListener *theListener);

    Sexy::Image *mTrackImage;
    Sexy::Image *mThumbImage;
    int mId;
    Sexy::SliderListener *mListener;
    double mVal;
    int mDragging;
    int mGrabOffsetX;
    int mGrabOffsetY;
    bool mHorizontal;
};

} // namespace Sexy

using Sexy::Slider;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SLIDER_H
