// CursorPreview -- reconstructed from libGameMain.so.
//
// The translucent plant that follows the gamepad cursor while a seed is being
// carried. There is one preview per player (Board::mCursorPreview): it converts
// the pad's pixel position into a grid cell, decides whether the seed could be
// planted there at all, and draws the seed at that cell.
//
// It is a GameObject (GameObject : SyncObject), and the whole derived area
// +0x34..+0x3F -- mGridX, mGridY, mPlayerIndex -- is registered as one
// synchronised block by the constructor. Update and Draw are not virtual in the
// binary; the vtable only holds the two destructors plus GameObject's
// BeginDraw/EndDraw/MakeParentGraphicsFrame.

#include "CursorPreview.h"
#include "BaseGamepadControls.h"
#include "game/Board.h"
#include "DataArrayMethods.h"
#include "LawnPlayerInfo.h"
#include "GameObject.h"
#include "Graphics.h"
#include "LawnApp.h"
#include "Plant.h"
#include "ZenGarden.h"
#include <cstring>

// ==========================================================================
// CursorPreview::~CursorPreview()
// @00146B20  24 bytes
// ==========================================================================
CursorPreview::~CursorPreview()
{
  // The binary's two instructions are the vptr store and the base destructor
  // call; with a real base class the compiler emits both, so this is empty.
}

// ==========================================================================
// CursorPreview::Update()
// @00146B68  198 bytes
// ==========================================================================
unsigned int CursorPreview::Update()
{
  // Board::Update only ever calls this for its side effects: the binary leaves
  // whichever call ran last in r0 and no caller reads it, so 0 stands in for
  // that value at both exits.
  if ( mApp->mGameScene != SCENE_PLAYING )
  {
    mVisible = false;
    return 0;
  }

  SeedType aSeedType = (SeedType)mBoard->GetSeedTypeInCursor(mPlayerIndex);

  int aPixelX;
  int aPixelY;
  // The binary reaches the board through mApp->mBoard rather than through its
  // own mBoard member, and it always uses the first player's pad. Board.h only
  // forward-declares GamepadControls -- including its header here would close an
  // include cycle -- so the call goes through the base, exactly as the binary's
  // direct call to BaseGamepadControls::GetCursorPosition does.
  ((BaseGamepadControls *)mApp->mBoard->mGamepadControls[0])->GetCursorPosition(aPixelX, aPixelY);

  mGridX = mBoard->PlantingPixelToGridX(aPixelX, aPixelY, aSeedType);
  mGridY = mBoard->PlantingPixelToGridY(aPixelX, aPixelY, aSeedType);
  // Unsigned on purpose: PlantingPixelToGrid* answers -1 for a position off the
  // board, and the binary's BHI rejects that along with 9.. and 6.. .
  if ( (unsigned int)mGridX > 8 || (unsigned int)mGridY > 5 )
  {
    mVisible = false;
    return 0;
  }

  // A carried plant is shown only where it could actually be put down: either a
  // seed is already on the cursor and the cell refuses it, or the cursor holds
  // the wheelbarrow with a plant in it and the cell refuses that plant.
  if ( (mBoard->IsPlantInCursor(mPlayerIndex) && mBoard->CanPlantAt(mGridX, mGridY, aSeedType) == PLANTING_OK)
    || (mBoard->mCursorObject[mPlayerIndex]->mCursorType == CURSOR_TYPE_WHEEELBARROW
        && mApp->mZenGarden->GetPottedPlantInWheelbarrow() != 0
        && mBoard->CanPlantAt(mGridX, mGridY, aSeedType) == PLANTING_OK) )
  {
    mX = mBoard->GridToPixelX(mGridX, mGridY);
    mY = mBoard->GridToPixelY(mGridX, mGridY);
    mVisible = true;
  }
  else
  {
    mVisible = false;
  }
  return 0;
}

// ==========================================================================
// CursorPreview::Draw(Sexy::Graphics *)
// @00146C30  462 bytes
// ==========================================================================
int CursorPreview::Draw(Sexy::Graphics * g)
{
  SeedType aSeedType = (SeedType)mBoard->GetSeedTypeInCursor(mPlayerIndex);
  // -1 means no seed sits on this player's cursor, so there is nothing to show.
  if ( aSeedType == SEED_NONE )
    return aSeedType;

  g->SetColorizeImages(true);
  g->SetColor(Sexy::Color(255, 255, 255, 100));

  CursorObject * aCursorObject = mBoard->mCursorObject[mPlayerIndex];
  CursorType aCursorType = aCursorObject->mCursorType;

  // A carried garden plant uses its potted-plant record.
  PottedPlant * aPottedPlant = nullptr;
  if ( aCursorType == CURSOR_TYPE_WHEEELBARROW || aCursorType == CURSOR_TYPE_PLANT_FROM_WHEEL_BARROW )
  {
    aPottedPlant = mApp->mZenGarden->GetPottedPlantInWheelbarrow();
  }
  else if ( aCursorType == CURSOR_TYPE_PLANT_FROM_GLOVE )
  {
    Plant *aGlovePlant = mBoard->mPlants.DataArrayTryToGet(aCursorObject->mGlovePlantID);
    if (aGlovePlant == nullptr || aGlovePlant->mPottedPlantIndex < 0
        || aGlovePlant->mPottedPlantIndex >= mApp->mPlayerInfo->mNumPottedPlants)
    {
      g->SetColorizeImages(false);
      return 0;
    }
    aPottedPlant = mApp->GetPottedPlantByIndex(aGlovePlant->mPottedPlantIndex);
  }

  if ( aPottedPlant != nullptr )
  {
    // The mushroom garden and the aquarium draw the pot themselves.
    bool aGardenBackground = mBoard->mBackground == BACKGROUND_MUSHROOM_GARDEN
                          || mBoard->mBackground == BACKGROUND_ZOMBIQUARIUM;
    mApp->mZenGarden->DrawPottedPlant(g, 0.0f, 0.0f, aPottedPlant, 1.0f, !aGardenBackground);
  }
  else if ( mApp->IsIZombieLevel() )
  {
    // In I, Zombie the "seed" is the zombie about to be placed, and the preview
    // sits higher on the lawn.
    float aHeightOffset = PlantDrawHeightOffset(mBoard, nullptr, aSeedType, mGridX, mGridY);
    if ( aSeedType == SEED_ZOMBIE_GARGANTUAR )
      aHeightOffset -= 30.0f;
    Plant::DrawSeedType(g, aCursorObject->mType, aCursorObject->mImitaterType, (DrawVariation)0,
                        -49.0f, aHeightOffset - 78.0f);
  }
  else
  {
    float aHeightOffset = PlantDrawHeightOffset(mBoard, nullptr, aSeedType, mGridX, mGridY);
    Plant::DrawSeedType(g, aCursorObject->mType, aCursorObject->mImitaterType, (DrawVariation)0,
                        0.0f, aHeightOffset);
  }

  // Column mode: show, for every other row, that the seed cannot go there.
  if ( mApp->mGameMode == GAMEMODE_CHALLENGE_COLUMN )
  {
    for ( int i = 0; i != 6; ++i )
    {
      if ( mGridY != i && mBoard->CanPlantAt(mGridX, i, aSeedType) == PLANTING_OK )
      {
        float aRowOffset = PlantDrawHeightOffset(mBoard, nullptr, aSeedType, mGridX, i);
        Plant::DrawSeedType(g, aCursorObject->mType, aCursorObject->mImitaterType, (DrawVariation)0,
                            0.0f, aRowOffset + (float)(i - mGridY) * 85.0f);
      }
    }
  }
  g->SetColorizeImages(false);
  return 0;
}

// ==========================================================================
// CursorPreview::CursorPreview(int)
// @00146E08  118 bytes
// ==========================================================================
CursorPreview::CursorPreview(int thePlayerIndex)
{
  // The derived state block mGridX..mPlayerIndex (+0x34..+0x3F, 12 bytes) is
  // registered with the sync vector and then cleared -- the same idiom
  // GameObject, Plant, Music and Trail use for their own state.
  //
  // The clear covers mPlayerIndex, and the binary really does store the argument
  // first (0x146E2A) and clear afterwards (0x146E50), with no later store to
  // +0x3C, so a constructed CursorPreview has mPlayerIndex == 0. That is
  // reproduced here exactly as the shipped binary behaves.
  mPlayerIndex = thePlayerIndex;

  const int aSyncBlockSize = 12;
  SyncBlockInfo aBlock;
  aBlock.mData = &mGridX;
  aBlock.mSize = aSyncBlockSize;

  mSyncBlocks.push_back(aBlock);
  memset(&mGridX, 0, aSyncBlockSize);

  mGridX = 0;
  mGridY = 0;
  mX = 0;
  mY = 0;
  mWidth = 80;
  mHeight = 80;
  mVisible = false;
}
