// HelpTextWidget -- restored from libGameMain.so.

#include "HelpTextWidget.h"

#include "Graphics.h"
#include "Widget.h"
// ==========================================================================
// HelpTextWidget::Draw(Sexy::Graphics *)
// @0018D684  84 bytes
// ==========================================================================
void HelpTextWidget::Draw(Sexy::Graphics *theGraphics)
{
  const TRect<int> aRect(mX, mY, mWidth, mHeight);
  TodDrawStringWrappedHelper(theGraphics, mText, aRect, mFont, GetColor(0),
                             mJustification, true, false);
}

// ==========================================================================
// HelpTextWidget::HelpTextWidget()
// @0018D74C  62 bytes
// ==========================================================================
HelpTextWidget::HelpTextWidget()
  : mText(gEmptyCString)
{
  mJustification = 0;
  mClip = false;
  mFont = FONT_DWARVENTODCRAFT24;
}

// ==========================================================================
// HelpTextWidget::Resize(int,int,int,int)
// @0018D7A4  4 bytes
// ==========================================================================
void HelpTextWidget::Resize(int theX, int theY, int theWidth, int theHeight)
{
  Sexy::Widget::Resize(theX, theY, theWidth, theHeight);
}
