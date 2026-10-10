// ScrollbuttonWidget is a ButtonWidget with a direction and a hide flag.
//
// The vtable replaces the destructor and Draw. Update stays ButtonWidget's.
// The object is ButtonWidget plus two words: the hide-when-idle flag and the
// direction the constructor stores, which Draw uses as a cel index.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SCROLLBUTTONWIDGET_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SCROLLBUTTONWIDGET_H

#include "ReconTypes.h"
#include "ButtonWidget.h"
#include "Graphics.h"

namespace Sexy
{

class ScrollbarWidget;

class ScrollbuttonWidget : public ButtonWidget
{
public:
    bool mInvisIfNoScroll; // +0x330
    int mDirection; // +0x334

    virtual ~ScrollbuttonWidget();
    virtual void Draw(Sexy::Graphics *g);

    ScrollbuttonWidget(int theId, Sexy::ButtonListener *theListener, int theDirection);
};

} // namespace Sexy

using Sexy::ScrollbuttonWidget;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SCROLLBUTTONWIDGET_H
