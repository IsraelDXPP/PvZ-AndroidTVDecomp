// SoundControllerWidget.cpp -- the volume overlay, from libGameMain.so.
//
// The keys are the Windows virtual-key codes the binary compares against:
// 0x74 raises the level, 0x71 lowers it, 0x73 toggles mute. The overlay draws
// for two seconds after the last change and then hides itself.

#include "SoundControllerWidget.h"

#include "Image.h"
#include "ReconData.h"
#include "ReconHelpers.h"
#include "SexyAppBase.h"

#include <stdio.h>

namespace
{
// The app the widget adjusts. gSexyAppBase is the one the constructor and
// every volume call read.
Sexy::SexyAppBase *App()
{
    return gSexyAppBase;
}

void ApplyMusicVolume(int theSize)
{
    Sexy::SexyAppBase *anApp = App();
    if (anApp != nullptr)
        anApp->SetMusicVolume((double)theSize * 0.1);
}
}

Sexy::SoundControllerWidget::SoundControllerWidget() :
    mOpenImage(nullptr),
    mClosedImage(nullptr),
    mRaisedImage(nullptr),
    mLoweredImage(nullptr),
    mSoundSize(0),
    mState(STATE_OPEN),
    mShowing(false),
    mShownTick(0)
{
    Sexy::SexyAppBase *anApp = App();
    if (anApp != nullptr)
        mSoundSize = (int)(anApp->GetMusicVolume() * 10.0);
}

Sexy::SoundControllerWidget::~SoundControllerWidget() {}

// @0x531594. While the overlay is up, the image for the current state is
// centred in the widget; the base draw runs either way.
void Sexy::SoundControllerWidget::Draw(Sexy::Graphics *g)
{
    if (!mShowing)
    {
        Widget::Draw(g);
        return;
    }

    Sexy::Image *anImage = nullptr;
    if (mState == STATE_OPEN)
        anImage = mLoweredImage;
    else if (mState == STATE_CLOSED)
        anImage = mRaisedImage;
    else if (mState == STATE_RAISED)
        anImage = mOpenImage;
    else if (mState == STATE_LOWERED)
        anImage = mClosedImage;

    if (anImage != nullptr)
    {
        g->DrawImage(anImage, (mWidth - anImage->mWidth) / 2, (mHeight - anImage->mHeight) / 2);
    }
    Widget::Draw(g);
}

// @0x531674. Two seconds after the last change the overlay drops, unless the
// level is zero, in which case it settles on the closed state instead.
void Sexy::SoundControllerWidget::Update()
{
    if (mShowing)
    {
        unsigned int aNow = (unsigned int)Sexy::GetTickCount();
        if ((int)(aNow - mShownTick) > 2000)
        {
            mShowing = false;
            if (mSoundSize == 0 && mState != STATE_CLOSED)
            {
                mShownTick = aNow;
                mState = STATE_CLOSED;
                mShowing = true;
            }
        }
    }
    Widget::Update();
}

// @0x5316c8. 0x74 raises, 0x71 lowers, 0x73 toggles between open and closed.
int Sexy::SoundControllerWidget::KeyUp(Sexy::KeyCode theKey)
{
    if (theKey == 0x74)
    {
        if (mSoundSize <= 9)
            addSoundSize();
    }
    else if (theKey == 0x71)
    {
        if (mSoundSize > 0)
            reduceSoundSize();
    }
    else if (theKey == 0x73)
    {
        if (mState == STATE_CLOSED)
            openSound();
        else
            closeSound();
    }
    logfi("*****theKey is%d\n\n", (int)theKey);
    return Widget::KeyUp(theKey);
}

// @0x531430. Shows the open image and restores the stored level.
void Sexy::SoundControllerWidget::openSound()
{
    mShowing = true;
    mShownTick = (unsigned int)Sexy::GetTickCount();
    mState = STATE_OPEN;
    ApplyMusicVolume(mSoundSize);
}

// @0x531480. Shows the closed image and mutes.
void Sexy::SoundControllerWidget::closeSound()
{
    mShowing = true;
    mShownTick = (unsigned int)Sexy::GetTickCount();
    mState = STATE_CLOSED;
    ApplyMusicVolume(0);
}

// @0x5314b8. The level is stored and applied, and the overlay is shown.
int Sexy::SoundControllerWidget::setSoundSize(int theSize)
{
    logfi("mSoundSize is111111111111111111111%d\n", mSoundSize);
    mShowing = true;
    mShownTick = (unsigned int)Sexy::GetTickCount();
    ApplyMusicVolume(theSize);
    return theSize;
}

// @0x531518.
void Sexy::SoundControllerWidget::addSoundSize()
{
    mSoundSize += 1;
    setSoundSize(mSoundSize);
    mState = STATE_RAISED;
    logfi("mSoundSize is%d\n", mSoundSize);
}

// @0x531544.
void Sexy::SoundControllerWidget::reduceSoundSize()
{
    mSoundSize -= 1;
    setSoundSize(mSoundSize);
    mState = STATE_LOWERED;
    logfi("mSoundSize is%d\n", mSoundSize);
}

// @0x531570. Six bytes: the state is the argument.
void Sexy::SoundControllerWidget::setSoundStatus(int theState)
{
    mState = theState;
}

// @0x531578. The four images, in the order the binary stores them.
Sexy::SoundControllerWidget *Sexy::SoundControllerWidget::setSoundControllerImage(
    Sexy::Image *theOpen, Sexy::Image *theClosed, Sexy::Image *theRaised, Sexy::Image *theLowered)
{
    mOpenImage = theOpen;
    mLoweredImage = theLowered;
    mClosedImage = theClosed;
    mRaisedImage = theRaised;
    return this;
}
