#include "ReanimationWidget.h"

#include "EffectSystem.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "Reanimation.h"
#include "ReanimationHolder.h"
#include "Widget.h"

void ReanimationWidget::Draw(Sexy::Graphics *theGraphics)
{
    if (mReanim != nullptr)
        mReanim->Draw(theGraphics);
}

void ReanimationWidget::Update()
{
    if (mReanim != nullptr)
    {
        mReanim->Update();
        mParent->MarkDirty();
    }
}

ReanimationWidget::ReanimationWidget()
{
    mApp = gLawnApp;
    mReanim = nullptr;
    mLawnDialog = nullptr;
    mPosX = 0.0f;
    mPosY = 0.0f;
    mHasAlpha = true;
    mClip = false;
    mMouseVisible = false;
}

void ReanimationWidget::AddReanimation(float x, float y, ReanimationType theReanimationType)
{
    ReanimationHolder *holder = mApp->mEffectSystem->mReanimationHolder;

    mPosX = x;
    mPosY = y;
    mReanim = holder->AllocReanimation(x, y, 0, theReanimationType);

    mReanim->mLoopType = REANIM_LOOP;
    mReanim->mIsAttachment = true;
    if (mReanim->TrackExists("anim_idle") != 0)
        mReanim->SetFramesForLayer("anim_idle");

    Resize((int)x, (int)y, 10, 10);
}

void ReanimationWidget::Dispose()
{
    if (mReanim == nullptr)
        return;

    ReanimationHolder *holder = mApp->mEffectSystem->mReanimationHolder;
    // mBlock is the slot array; the owned reanimation is one slot's mItem.
    ReanimationSlot *slots = holder->mBlock;
    ReanimationSlot *slot = nullptr;
    for (unsigned int index = 0; index < holder->mMaxSize; ++index)
    {
        if (&slots[index].mItem == mReanim)
        {
            slot = &slots[index];
            break;
        }
    }
    if (slot == nullptr)
        return;

    mReanim->~Reanimation();

    // DataArray free list: the next free index is threaded through the slot id.
    unsigned int nextFree = holder->mFreeListHead;
    holder->mFreeListHead = static_cast<unsigned short>(slot->mID);
    slot->mID = nextFree;
    --holder->mSize;

    mReanim = nullptr;
}

ReanimationWidget::~ReanimationWidget()
{
    Dispose();
}
