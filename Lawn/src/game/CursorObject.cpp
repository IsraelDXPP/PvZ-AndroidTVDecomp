// CursorObject -- reconstructed from libGameMain.so.
//
// The object is one player's cursor: Board owns two of them (mCursorObject[2])
// and GamepadControls/BaseGamepadControls drive them. Update() tracks the
// gamepad cursor, Draw() paints whatever the cursor currently holds, and the
// constructor attaches the Whack-a-Zombie hammer reanimation.
//
// The PC reference source's Lawn\Board\CursorObject.cpp is a hook stub in this
// tree -- it only trampolines the inherited BeginDraw/EndDraw virtuals -- so
// every body below comes from the IDA decompilation.
//
// GameObject's constructor already set mApp/mBoard and registered the base sync
// block +0x18..+0x33, so this file only registers the derived one.

#include "CursorObject.h"
#include "BaseGamepadControls.h"
#include "game/Board.h"
#include "CutScene.h"
#include "DataArrayMethods.h"
#include "GameObject.h"
// Complete type: Update() calls mGamepadControls[i]->GetCursorPosition.
#include "GamepadControls.h"
#include "Graphics.h"
#include "Image.h"
#include "LawnApp.h"
#include "Plant.h"
// Complete type: Draw reads PottedPlant::mPlantAge for the wheelbarrow cursor.
#include "PottedPlant.h"
#include "Reanimation.h"
#include "ZenGarden.h"
#include <cstring>



// ==========================================================================
// CursorObject::~CursorObject()
// @0013BEC4  24 bytes
// ==========================================================================
CursorObject::~CursorObject()
{
  // Nothing to release here: the cursor reanimation belongs to mApp, and
  // GameObject::~GameObject -- which destroys mSyncBlocks -- runs implicitly.
}

// ==========================================================================
// CursorObject::Update()
// @0013BF0C  74 bytes
// ==========================================================================
void CursorObject::Update()
{
  if ( mApp->mGameScene != SCENE_PLAYING )
  {
    // Outside a level the cursor is drawn only for the shovel tutorial.
    if ( !mBoard->mCutScene->IsInShovelTutorial() )
    {
      mVisible = false;
      return;
    }
  }

  // The Whack-a-Zombie hammer is the only cursor that owns a reanimation.
  Reanimation *aReanim = (Reanimation *)mApp->ReanimationTryToGet(mReanimCursorID);
  if ( aReanim != nullptr )
    aReanim->Update();

  mVisible = true;
  mBoard->mGamepadControls[0]->GetCursorPosition(mX, mY);
}

// ==========================================================================
// CursorObject::Draw(Sexy::Graphics *)
// @0013BF58  912 bytes
// ==========================================================================
void CursorObject::Draw(Sexy::Graphics *theGraphics)
{
  // One case per cursor type, each drawing the held item at its own offset.
  switch ( mCursorType )
  {
  case CURSOR_TYPE_SHOVEL:
    theGraphics->DrawImage(IMAGE_SHOVEL, 10, -30);
    return;

  case CURSOR_TYPE_WATERING_CAN:
  {
    // The gold watering can replaces the plain one once it has been bought.
    // The purchase records which watering can is available.
    Sexy::Image *aWateringCanImage = IMAGE_WATERINGCAN;
    if ( mApp->mPlayerInfo->mPurchases[STORE_ITEM_GOLD_WATERINGCAN] != 0 )
    {
      theGraphics->DrawImage(IMAGE_ZEN_GOLDTOOLRETICLE, -62, -37);
      aWateringCanImage = IMAGE_WATERINGCANGOLD;
    }
    theGraphics->DrawImage(aWateringCanImage, -3, 12);
    return;
  }

  case CURSOR_TYPE_FERTILIZER:
    theGraphics->DrawImage(IMAGE_FERTILIZER, -15, 0);
    return;

  case CURSOR_TYPE_BUG_SPRAY:
    theGraphics->DrawImage(IMAGE_BUG_SPRAY, -9, -1);
    return;

  case CURSOR_TYPE_PHONOGRAPH:
    theGraphics->DrawImage(IMAGE_PHONOGRAPH, -17, 10);
    return;

  case CURSOR_TYPE_CHOCOLATE:
    theGraphics->DrawImage(IMAGE_CHOCOLATE, -2, -8);
    return;

  case CURSOR_TYPE_GLOVE:
    theGraphics->DrawImage(IMAGE_ZEN_GARDENGLOVE, -17, 15);
    return;

  case CURSOR_TYPE_MONEY_SIGN:
    theGraphics->DrawImage(IMAGE_ZEN_MONEYSIGN, -17, -10);
    return;

  case CURSOR_TYPE_TREE_FOOD:
    theGraphics->DrawImage(IMAGE_TREEFOOD, -15, 0);
    return;

  case CURSOR_TYPE_WHEEELBARROW:
  {
    PottedPlant *aPottedPlant = mApp->mZenGarden->GetPottedPlantInWheelbarrow();
    if ( aPottedPlant != nullptr )
    {
      theGraphics->DrawImage(IMAGE_ZEN_WHEELBARROW, -20, -23);
      // A plant that has not finished growing sits higher and is drawn smaller.
      if ( aPottedPlant->mPlantAge == PLANTAGE_SMALL )
        mApp->mZenGarden->DrawPottedPlant(theGraphics, 10.0f, -35.0f, aPottedPlant, 0.6f, true);
      else if ( aPottedPlant->mPlantAge == PLANTAGE_MEDIUM )
        mApp->mZenGarden->DrawPottedPlant(theGraphics, 15.0f, -25.0f, aPottedPlant, 0.5f, true);
      else
        mApp->mZenGarden->DrawPottedPlant(theGraphics, 21.0f, -15.0f, aPottedPlant, 0.4f, true);
    }
    else
    {
      theGraphics->DrawImage(IMAGE_ZEN_WHEELBARROW, -20, -30);
    }
    return;
  }

  case CURSOR_TYPE_PLANT_FROM_GLOVE:
  {
    // mGlovePlantID is a DataArray<Plant> key; the potted plant the glove is
    // carrying is the one the recorded Plant points at.
    Plant *aPlant = mBoard->mPlants.DataArrayTryToGet(mGlovePlantID);
    if (aPlant == nullptr || aPlant->mPottedPlantIndex < 0
        || aPlant->mPottedPlantIndex >= mApp->mPlayerInfo->mNumPottedPlants)
      return;
    PottedPlant *aPottedPlant = mApp->GetPottedPlantByIndex(aPlant->mPottedPlantIndex);
    // In the Zen garden itself the plant is drawn under the cursor; elsewhere
    // (the tree of wisdom) it is drawn above it.
    if ( mBoard->mBackground == BACKGROUND_MUSHROOM_GARDEN || mBoard->mBackground == BACKGROUND_ZOMBIQUARIUM )
      mApp->mZenGarden->DrawPottedPlant(theGraphics, -10.0f, -10.0f, aPottedPlant, 1.0f, false);
    else
      mApp->mZenGarden->DrawPottedPlant(theGraphics, -22.0f, -38.0f, aPottedPlant, 1.0f, true);
    return;
  }

  case CURSOR_TYPE_PLANT_FROM_WHEEL_BARROW:
  {
    PottedPlant *aPottedPlant = mApp->mZenGarden->GetPottedPlantInWheelbarrow();
    if ( mBoard->mBackground == BACKGROUND_MUSHROOM_GARDEN || mBoard->mBackground == BACKGROUND_ZOMBIQUARIUM )
      mApp->mZenGarden->DrawPottedPlant(theGraphics, -10.0f, -10.0f, aPottedPlant, 1.0f, false);
    else
      mApp->mZenGarden->DrawPottedPlant(theGraphics, -22.0f, -38.0f, aPottedPlant, 1.0f, true);
    return;
  }

  case CURSOR_TYPE_PLANT_FROM_BANK:
  case CURSOR_TYPE_PLANT_FROM_USABLE_COIN:
  case CURSOR_TYPE_PLANT_FROM_DUPLICATOR:
  {
    // The three "carrying a seed" cursors all draw the seed at the same spot.
    float aOffsetY = (float)PlantDrawHeightOffset(mBoard, nullptr, mType, -1, -1) - 10.0f;
    if ( Plant::IsFlying(mType) || mType == SEED_GRAVEBUSTER )
      aOffsetY += 30.0f;

    float aOffsetX = -10.0f;
    float aPosY = aOffsetY - 15.0f;
    if ( mApp->IsIZombieLevel() )
    {
      aOffsetX = -65.0f;
      aPosY -= 70.0f;
    }

    // A negative type means "nothing selected yet"; there is nothing to draw.
    if ( mType != SEED_NONE )
      Plant::DrawSeedType(theGraphics, mType, mImitaterType, VARIATION_NORMAL, aOffsetX, aPosY);
    return;
  }

  case CURSOR_TYPE_HAMMER:
  {
    Reanimation *aReanim = mApp->ReanimationGet(mReanimCursorID);
    aReanim->Draw(theGraphics);
    return;
  }

  case CURSOR_TYPE_COBCANNON_TARGET:
  {
    // MouseHitTest's third parameter is declared `GridItem **`, but the binary
    // writes a HitResult through it: the hit object and its GameObjectType.
    HitResult aHitResult;
    mBoard->MouseHitTest(mBoard->mPrevMouseX, mBoard->mPrevMouseY, &aHitResult, 0);
    if ( aHitResult.mObjectType == OBJECT_TYPE_NONE && mBoard->mPrevMouseY > 79 )
      theGraphics->DrawImageCel(IMAGE_COBCANNON_TARGET, -11, 7, aHitResult.mObjectType);
    return;
  }

  default:
    return;
  }
}

// ==========================================================================
// CursorObject::Die()
// @0013C324  10 bytes
// ==========================================================================
void CursorObject::Die()
{
  mApp->RemoveReanimation(mReanimCursorID);
}

// ==========================================================================
// CursorObject::CursorObject()
// @0013C460  216 bytes
// ==========================================================================
CursorObject::CursorObject()
{
  // The derived member area +0x34..+0x5B (40 bytes) is this object's sync block,
  // and registering it also clears it.
  SyncBlockInfo aBlock = MakeSyncBlock(mSelectedIndex, mReanimCursorID);
  mSyncBlocks.push_back(aBlock);
  memset(aBlock.mData, 0, aBlock.mSize);

  mSelectedIndex = 0;
  // -1 is "no seed chosen"; SeedType has no NONE enumerator.
  mType = (SeedType)-1;
  mImitaterType = (SeedType)-1;
  mCursorType = CURSOR_TYPE_NORMAL;
  mCoinID = 0;
  mGlovePlantID = PLANTID_NULL;
  mDuplicatorPlantID = PLANTID_NULL;
  mCobCannonPlantID = PLANTID_NULL;
  mReanimCursorID = REANIMATIONID_NULL;

  if ( mApp->IsWhackAZombieLevel() )
  {
    // 99 is the hammer reanimation; the cursor is the only user of it.
    ReanimatorEnsureDefinitionLoaded(REANIM_HAMMER, true);
    Reanimation *aReanim = mApp->AddReanimation(-25.0f, 16.0f, 0, REANIM_HAMMER);
    aReanim->mIsAttachment = true;
    aReanim->PlayReanim("anim_whack_zombie", REANIM_PLAY_ONCE_AND_HOLD, 0, 24.0f);
    aReanim->mAnimTime = 1.0f;
    mReanimCursorID = mApp->ReanimationGetID(aReanim);
  }

  mWidth = 80;
  mHeight = 80;
}
