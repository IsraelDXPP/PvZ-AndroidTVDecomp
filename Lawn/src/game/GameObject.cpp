// GameObject -- reconstructed from libGameMain.so.
//
// The PC reference source's Lawn\GameObject.cpp corresponds word for word:
//     mApp = gLawnApp;  mBoard = gLawnApp->mBoard;
//     mX = mY = mWidth = mHeight = 0;  mVisible = true;
//     mRow = -1;  mRenderOrder = RenderLayer::RENDER_LAYER_TOP;
//     BeginDraw:  g->Translate( mX,  mY);  return mVisible;
//     EndDraw / MakeParentGraphicsFrame:  g->Translate(-mX, -mY);

#include "GameObject.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "SexyAppBase.h"
#include <cstring>

// ==========================================================================
// GameObject::EndDraw(Sexy::Graphics *)
// @001B7AF0  42 bytes
// ==========================================================================
void GameObject::EndDraw(Sexy::Graphics *theGraphics)
{
  theGraphics->mTransX -= (float)mX;
  theGraphics->mTransY -= (float)mY;
}

// ==========================================================================
// GameObject::MakeParentGraphicsFrame(Sexy::Graphics *)
// @001B7B1C  42 bytes
// ==========================================================================
void GameObject::MakeParentGraphicsFrame(Sexy::Graphics *theGraphics)
{
  theGraphics->mTransX -= (float)mX;
  theGraphics->mTransY -= (float)mY;
}

// ==========================================================================
// GameObject::BeginDraw(Sexy::Graphics *)
// @001B7B80  50 bytes
// ==========================================================================
bool GameObject::BeginDraw(Sexy::Graphics *theGraphics)
{
  if (!mVisible)
    return false;

  theGraphics->mTransX += (float)mX;
  theGraphics->mTransY += (float)mY;
  return true;
}

// ==========================================================================
// GameObject::GameObject()
// @001B7BB4  122 bytes
// ==========================================================================
GameObject::GameObject()
{
  // The object registers its own state area mX..mRenderOrder (+0x18..+0x33,
  // 28 bytes) as one sync block, then zero-fills exactly that range. The
  // vector's three-pointer initialisation is libstdcxx::vector's own default
  // constructor, so it is not repeated here.
  SyncBlockInfo aBlock = MakeSyncBlock(mX, mRenderOrder);
  mSyncBlocks.push_back(aBlock);
  memset(aBlock.mData, 0, aBlock.mSize);

  // mRenderOrder's value is 400000 == RenderLayer::RENDER_LAYER_TOP. The dump
  // renders it as the address of the string "...HAND_DAMAGE2E", because that
  // literal happens to sit at 0x61A66 and the immediate is 0x00061A80.
  mRenderOrder = 400000;

  mRow = -1;
  mVisible = true;

  mApp = (LawnApp *)gSexyAppBase;
  mBoard = mApp->mBoard;
}
