#include "BottomRotatorMenu.h"

#include "ButtonWidget.h"
#include "Graphics.h"
#include "Widget.h"

BottomRotatorMenu::BottomRotatorMenu()
{
    mScrollOffset = 0.0f;
}

BottomRotatorMenu::~BottomRotatorMenu() {}

void BottomRotatorMenu::AddRotatorWidget(Sexy::ButtonWidget *theWidget)
{
    mRotatorWidgets.push_back(theWidget);
}

void BottomRotatorMenu::RemoveRotatorWidget(Sexy::ButtonWidget *theWidget)
{
    mRotatorWidgets.remove(theWidget);
}

void BottomRotatorMenu::GotFocus()
{
    Sexy::Widget::GotFocus();
}

void BottomRotatorMenu::Update()
{
    Sexy::Widget::Update();
}

void BottomRotatorMenu::Draw(Sexy::Graphics *theGraphics)
{
    Sexy::Widget::Draw(theGraphics);

    float aSavedTransX = theGraphics->mTransX;
    float aSavedTransY = theGraphics->mTransY;
    // The row sits 300 pixels right of the widget origin and 500 pixels down.
    // mScrollOffset slides that row to the left.
    float aRowX = (aSavedTransX + 300.0f) - mScrollOffset;
    theGraphics->mTransY = aSavedTransY + 500.0f;
    theGraphics->mTransX = aRowX;

    Sexy::ButtonWidget *aWidget = mRotatorWidgets.empty() ? nullptr : mRotatorWidgets.back();
    if (aWidget != nullptr)
    {
        // The scale origin is the centre of an 800 by 1000 stage.
        float aOriginX = 400.0f - (float)aWidget->mWidth * 0.5f;
        float aOriginY = 500.0f - (float)aWidget->mHeight * 0.5f;
        theGraphics->mTransX = aRowX + (float)aWidget->mX;
        theGraphics->SetScale(1.0f, 1.0f, aOriginX, aOriginY);

        aWidget->mBtnNoDraw = false;
        aWidget->Draw(theGraphics);
        aWidget->mBtnNoDraw = true;
    }

    theGraphics->mTransX = aSavedTransX;
    theGraphics->mTransY = aSavedTransY;
}
