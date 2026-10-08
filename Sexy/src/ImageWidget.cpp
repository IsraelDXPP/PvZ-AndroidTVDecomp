// ImageWidget draws its image at the origin, tinted by mAlpha. An empty
// widget draws nothing.

#include "ImageWidget.h"

#include "Graphics.h"
#include "Widget.h"

void Sexy::ImageWidget::Draw(Sexy::Graphics * theGraphics)
{
  if (mImage == NULL)
    return;

  theGraphics->SetColorizeImages(true);
  theGraphics->SetColor(Color(255, 255, 255, mAlpha));
  theGraphics->DrawImage(mImage, 0, 0);
}

Sexy::ImageWidget::~ImageWidget()
{
}

Sexy::ImageWidget::ImageWidget()
{
  mImage = NULL;
  mAlpha = 255;
  mClip = false;
  mMouseVisible = false;
}
