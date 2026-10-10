#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_NEWLAWNBUTTON_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_NEWLAWNBUTTON_H

#include "ReconTypes.h"
#include "ButtonWidget.h"
#include "DialogButton.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "Widget.h"

class NewLawnButton : public Sexy::DialogButton
{
public:
    virtual ~NewLawnButton();

    virtual void Draw(Sexy::Graphics *g);
    virtual bool IsPointVisible(int x, int y);
    virtual void SetLabel(const Sexy::String &theLabel);

    NewLawnButton(Sexy::Image *theComponentImage, int theId, Sexy::ButtonListener *theListener);

    Sexy::Font *mHiliteFont;
    int mTextDownOffsetX;
    int mTextDownOffsetY;
    int mButtonOffsetX;
    int mButtonOffsetY;
    bool mUsePolygonShape;
    Sexy::Point mPolygonShape[4];
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_NEWLAWNBUTTON_H
