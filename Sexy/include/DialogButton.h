// DialogButton -- the yes/no button a Dialog owns.
//
// `class DialogButton : public ButtonWidget`. It overrides Draw and adds the
// image, the two translation offsets, the two text offsets and the app
// back-pointer. The rest of the table is ButtonWidget's.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_DIALOGBUTTON_H
#define LAWNPROJECTTV_SEXY_INCLUDE_DIALOGBUTTON_H

#include "ReconTypes.h"
#include "ButtonWidget.h"
#include "Font.h"
#include "Graphics.h"
#include "Image.h"
#include "TextLayout.h"

class LawnApp;

namespace Sexy
{

class DialogButton : public ButtonWidget
{
public:
    virtual ~DialogButton();

    virtual void Draw(Sexy::Graphics *theGraphics);

    Image *mImage;
    int mTranslateX;
    int mTranslateY;
    int mTextOffsetX;
    int mTextOffsetY;
    LawnApp *mApp;

    DialogButton(Sexy::Image *theImage, int theId, Sexy::ButtonListener *theListener);
};

} // namespace Sexy

using Sexy::DialogButton;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_DIALOGBUTTON_H
