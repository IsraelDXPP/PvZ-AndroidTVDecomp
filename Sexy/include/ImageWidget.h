// ImageWidget draws one image at its origin, tinted by mAlpha. It adds that
// image and the alpha to Widget, and replaces Draw.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_IMAGEWIDGET_H
#define LAWNPROJECTTV_SEXY_INCLUDE_IMAGEWIDGET_H

#include "ReconTypes.h"
#include "Graphics.h"
#include "Image.h"
#include "Widget.h"

namespace Sexy
{

class ImageWidget : public Widget
{
public:
    virtual ~ImageWidget();
    virtual void Draw(Sexy::Graphics *theGraphics);

    ImageWidget();

    Image *mImage;
    int mAlpha;
};

} // namespace Sexy

using Sexy::ImageWidget;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_IMAGEWIDGET_H
