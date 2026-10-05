// BaseGamepadControls -- transcribed from libGameMain.so
// Rewritten by hand in stage 2; see build/stage2-brief.md.

#include "BaseGamepadControls.h"

#include "game/Board.h"
#include "Challenge.h"
#include "CursorObject.h"
#include "GameObject.h"
#include "Graphics.h"
#include "ReconTypes.h"
#include <cmath>
#include <cstring>

// ==========================================================================
// BaseGamepadControls::BeginDraw()
// @001BCE04  4 bytes
// ==========================================================================
bool BaseGamepadControls::BeginDraw()
{
  return false;
}

// ==========================================================================
// BaseGamepadControls::Draw()
// @001BCE10  2 bytes
// ==========================================================================
// The dump's body is a bare return; GamepadControls' own slot-7 Draw does the
// work.
void BaseGamepadControls::Draw()
{
}

// ==========================================================================
// BaseGamepadControls::EndDraw()
// @001BCE14  2 bytes
// ==========================================================================
void BaseGamepadControls::EndDraw()
{
}

// ==========================================================================
// BaseGamepadControls::MakeParentGraphicsFrame()
// @001BCE20  2 bytes
// ==========================================================================
void BaseGamepadControls::MakeParentGraphicsFrame()
{
}

// ==========================================================================
// BaseGamepadControls::OnKeyUp(Sexy::KeyCode,unsigned int)
// @001BCE5C  4 bytes
// ==========================================================================
int BaseGamepadControls::OnKeyUp(Sexy::KeyCode theKey, unsigned int theFlags)
{
  (void)theKey;
  (void)theFlags;
  return 0;
}

// ==========================================================================
// BaseGamepadControls::GotoState(BaseGamepadControls::MovementState)
// @001BCE78  36 bytes
// ==========================================================================
// The dump's `VTSLOT(this, 44)` is slot [11] ExitState, and its `v5 + 40` is slot
// [10] EnterState -- which is handed the state being left, not the new one.
int BaseGamepadControls::GotoState(MovementState theState)
{
  ExitState();

  MovementState aPrevious = mGamepadState;
  mGamepadState = theState;
  int aResult = EnterState((int)aPrevious);
  if (theState != aPrevious)
    mMovementStateTimer = 0.0f;
  return aResult;
}

// ==========================================================================
// BaseGamepadControls::~BaseGamepadControls()
// @001BCEAC  32 bytes
// ==========================================================================
// The dump stores the GamepadListener and GameObject vptrs and then destroys the
// GameObject at +4. Our header declares GameObject as a base, so the compiler
// emits all three from the declarations and an explicit pair here would
// double-destroy mSyncBlocks.
BaseGamepadControls::~BaseGamepadControls()
{
}

// ==========================================================================
// BaseGamepadControls::OnButtonDown(int,int,unsigned int)
// @001BD064  14 bytes
// ==========================================================================
// The dump reaches Board::mChallenge (at Board + 0x254) through an offset on the
// board pointer; the challenge owns the button handling while one is running.
void BaseGamepadControls::OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags)
{
  if (mBoard->mChallenge != nullptr)
    mBoard->mChallenge->GameButtonDown(theButton, thePlayerIndex, theFlags);
}

// A button release and a stick move have no work of their own here. A derived
// controls class replaces both; the base keeps the vtable slots filled.
void BaseGamepadControls::OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags)
{
  (void)theButton;
  (void)thePlayerIndex;
  (void)theFlags;
}

void BaseGamepadControls::OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex)
{
  (void)theAxis;
  (void)theValue;
  (void)thePlayerIndex;
}

// ==========================================================================
// BaseGamepadControls::OnKeyDown(Sexy::KeyCode,unsigned int)
// @001BD074  14 bytes
// ==========================================================================
bool BaseGamepadControls::OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags)
{
  if (mBoard->mChallenge != nullptr)
    return mBoard->mChallenge->OnKeyDown(theKey, theFlags) != 0;
  return false;
}

// ==========================================================================
// ==========================================================================
// BaseGamepadControls::Update(float)
// @001BD084  222 bytes
// ==========================================================================
int BaseGamepadControls::Update(float theDeltaTime)
{
  ++mGamepadFrameCounter;

  if (mGamepadIndex != -1)
  {
    // The dump reaches Board::mApp through an offset on the board pointer, and
    // its word 522 is GamepadApp::mGamepads (+0x828), indexed by the gamepad.
    Sexy::Gamepad *aGamepad = mBoard->mApp->mGamepads[mGamepadIndex];
    mCursorHighlightAnimPhase += theDeltaTime;

    mInputLeftStickX = aGamepad->GetAxisXPositionRamped();
    mInputLeftStickY = aGamepad->GetAxisYPositionRamped();
    mInputRightStickX = aGamepad->GetRightAxisXPosition();
    mInputRightStickY = aGamepad->GetRightAxisYPosition();

    // Anything inside the stick deadzone reads as centred.
    if (fabsf(mInputLeftStickX) < 0.274f)
      mInputLeftStickX = 0.0f;
    if (fabsf(mInputLeftStickY) < 0.274f)
      mInputLeftStickY = 0.0f;

    // On a roofed stage the cursor cannot reach the top rows, so the vertical
    // stick also nudges it sideways.
    if (mBoard->StageHasRoof() != 0
        && mBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY) <= 4)
      mInputLeftStickY = mInputLeftStickY + mInputLeftStickX * 0.25f;
  }

  // The dump leaves the object pointer (or StageHasRoof's result) in r0; no
  // caller reads it.
  return 0;
}

// ==========================================================================
// BaseGamepadControls::GetSnapToGridXY()
// @001BD168  126 bytes
// ==========================================================================
// sret: ARM passes a by-value result's destination in r0, so the dump's `this`
// IS that destination and the object arrives in the next register -- hence the
// `BaseGamepadControls *` return type the transcription carried.
//
// The cursor is projected a fraction of a cell along its velocity and the
// resulting pixel is snapped to a grid cell; the fraction is a quarter of a
// cell on the map with the roofed special grid (game mode 44) and a tenth
// everywhere else.
SnapToGridPosition BaseGamepadControls::GetSnapToGridXY()
{
  float aFraction = (mApp->mGameMode == GAMEMODE_CHALLENGE_ZEN_GARDEN) ? 0.25f : 0.1f;

  int aX = (int)(mCursorPositionX + aFraction * mGamepadVelocityLeftX);
  int aY = (int)(mCursorPositionY + aFraction * mGamepadVelocityLeftY);

  SnapToGridPosition aResult;
  aResult.mX = (float)mBoard->PixelToGridXKeepOnBoard(aX, aY);
  aResult.mY = (float)mBoard->PixelToGridYKeepOnBoard(aX, aY);
  return aResult;
}

// ==========================================================================
// BaseGamepadControls::GetSnapToGridPos()
// @001BD258  184 bytes
// ==========================================================================
// sret again (see GetSnapToGridXY). The dump calls this object's own slot [15]
// GetSnapToGridXY through the vtable, then centres the result inside its cell.
SnapToGridPosition BaseGamepadControls::GetSnapToGridPos()
{
  // The binary stores the raw cursor position here first and lets the virtual
  // call below overwrite both words; the stores are kept because they happen.
  SnapToGridPosition aResult;
  aResult.mX = mCursorPositionX;
  aResult.mY = mCursorPositionY;

  SnapToGridPosition aSnapped = GetSnapToGridXY();
  int aGridX = (int)aSnapped.mX;
  int aGridY = (int)aSnapped.mY;

  int aCellX = mBoard->GridToPixelX(aGridX, aGridY);
  int aCellWidth = mBoard->GridCellWidth(aGridX, aGridY);
  int aCellY = mBoard->GridToPixelY(aGridX, aGridY);
  int aCellHeight = mBoard->GridCellHeight(aGridX, aGridY);

  aResult.mX = (float)(aCellWidth / 2) + (float)aCellX;
  aResult.mY = (float)(aCellHeight / 2) + (float)aCellY;
  return aResult;
}

// ==========================================================================
// BaseGamepadControls::EnterState(BaseGamepadControls::MovementState)
// @001BD32C  510 bytes
// ==========================================================================
// GotoState hands this the state being LEFT, and the state being entered is the
// one already stored in mGamepadState. Only the normal and butter-held states
// run the shared tail that clears every velocity.
int BaseGamepadControls::EnterState(int theState)
{
  bool aEnteringGridStep = false;

  switch (mGamepadState)
  {
    case MOVEMENT_STATE_NORMAL:
      aEnteringGridStep = true;
      break;

    case MOVEMENT_STATE_BUTTER_HELD:
    {
      // The dump's `VTSLOT(self, 56)` is slot [14] GetSnapToGridPos.
      SnapToGridPosition aCentre = GetSnapToGridPos();
      mGridCenterPositionX = aCentre.mX;
      mGridCenterPositionY = aCentre.mY;
      mButterCursorMoveCounter = 0;
      aEnteringGridStep = true;
      break;
    }

    case MOVEMENT_STATE_RESERVED_2:
      mGamepadVelocityLeftX = 0.0f;
      mGamepadVelocityLeftY = 0.0f;
      break;

    case MOVEMENT_STATE_GRID_STEP:
    {
      SnapToGridPosition aCentre = GetSnapToGridPos();
      mGridCenterPositionX = aCentre.mX;
      mGridCenterPositionY = aCentre.mY;
      break;
    }

    case MOVEMENT_STATE_SELECT_SEED:
    case MOVEMENT_STATE_DIG_HOLD:
    {
      // Both states centre the cursor inside the cell it is over; dig-hold also
      // picks the shovel up.
      int aGridX = mBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
      int aGridY = mBoard->PixelToGridYKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
      int aCellX = mBoard->GridToPixelX(aGridX, aGridY);
      int aCellWidth = mBoard->GridCellWidth(aGridX, aGridY);
      int aCellY = mBoard->GridToPixelY(aGridX, aGridY);
      int aCellHeight = mBoard->GridCellHeight(aGridX, aGridY);
      mGridCenterPositionX = (float)(aCellWidth / 2) + (float)aCellX;
      mGridCenterPositionY = (float)(aCellHeight / 2) + (float)aCellY;

      if (mGamepadState == MOVEMENT_STATE_DIG_HOLD)
        mBoard->PickUpTool(OBJECT_TYPE_SHOVEL, mPlayerIndex);
      break;
    }

    default:
      return 0;
  }

  if (aEnteringGridStep)
  {
    mGamepadVelocityLeftX = 0.0f;
    mGamepadVelocityLeftY = 0.0f;
    mGamepadVelocityRightX = 0.0f;
    mGamepadVelocityRightY = 0.0f;

    // Leaving a grid step, or entering one while the butter is held, puts the
    // cursor straight on the cell centre.
    if (theState == MOVEMENT_STATE_GRID_STEP || mGamepadState == MOVEMENT_STATE_BUTTER_HELD)
    {
      mCursorPositionX = mGridCenterPositionX;
      mCursorPositionY = mGridCenterPositionY;
    }
  }
  return 0;
}

// ==========================================================================
// BaseGamepadControls::ExitState()
// @001BD584  92 bytes
// ==========================================================================
int BaseGamepadControls::ExitState()
{
  if (mGamepadState == MOVEMENT_STATE_RESERVED_2)
  {
    mGamepadVelocityRightX = 0.0f;
    mGamepadVelocityRightY = 0.0f;
  }
  else if (mGamepadState == MOVEMENT_STATE_DIG_HOLD && mDigIndicatorPercentage > 1.3f)
  {
    // The dump spells this `return Board::MouseDownWithTool(...)`, but that
    // function is void: the word it returns is the receiver, not a result.
    mBoard->MouseDownWithTool((int)mCursorPositionX, (int)mCursorPositionY, 1, (CursorType)6, 0);
  }
  return 0;
}

// ==========================================================================
// BaseGamepadControls::UpdateStates(float)
// @001BE4D4  1018 bytes
// ==========================================================================
// mGamepadState (+0x60) selects the case. +0x68 is advanced by the frame time
// on every entry. Stick Y is negated into the right-hand velocity: screen Y
// grows downward.
int BaseGamepadControls::UpdateStates(float theDeltaTime)
{
  mDigIndicatorPercentage += theDeltaTime;

  switch (mGamepadState)
  {
    case MOVEMENT_STATE_NORMAL:
    {
      float aStickX = mInputLeftStickX;
      float aStickY = mInputLeftStickY;
      if (aStickX * aStickX + aStickY * aStickY > 0.0f)
        return GotoState(MOVEMENT_STATE_RESERVED_2);
      if (mDigIndicatorPercentage > 1.0f)
        mMovementStateTimer = 0.0f;
      break;
    }

    case MOVEMENT_STATE_RESERVED_2:
    {
      mMovementStateTimer += theDeltaTime;

      float aStickX = mInputLeftStickX;
      float aStickY = mInputLeftStickY;
      GameMode aMode = mApp->mGameMode;
      if (aStickX * aStickX + aStickY * aStickY == 0.0f)
      {
        if (aMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
        {
          // Zero stick still integrates this frame. GotoState is virtual, so
          // the stick and the mode are read again after it returns.
          GotoState(MOVEMENT_STATE_RESERVED_9);
          aStickX = mInputLeftStickX;
          aStickY = mInputLeftStickY;
          aMode = mApp->mGameMode;
        }
        else if (aMode != GAMEMODE_CHALLENGE_ZOMBIQUARIUM)
        {
          return GotoState(MOVEMENT_STATE_GRID_STEP);
        }
      }

      float aScale = 1800.0f;
      if (aMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST)
        aScale = 1500.0f;

      float aTargetX = aStickX * aScale;
      float aTargetY = -aStickY * aScale;
      mGamepadVelocityRightX = aTargetX;
      mGamepadVelocityRightY = aTargetY;

      float aVelX = mGamepadVelocityLeftX;
      float aVelY = mGamepadVelocityLeftY;
      if (aTargetX * aVelX + aTargetY * aVelY <= 0.0f)
      {
        aVelX *= 0.5f;
        aVelY *= 0.5f;
      }
      aVelX += aTargetX * theDeltaTime;
      aVelY += aTargetY * theDeltaTime;

      float aCap = 250.0f;
      if (mDigIndicatorPercentage > 0.5f)
        aCap = 375.0f;

      mGamepadVelocityLeftX = aVelX;
      mGamepadVelocityLeftY = aVelY;
      float aSpeed = sqrtf(aVelX * aVelX + aVelY * aVelY);
      if (aSpeed > aCap)
      {
        if (aSpeed != 0.0f)
        {
          aVelX /= aSpeed;
          aVelY /= aSpeed;
        }
        mGamepadVelocityLeftX = aVelX * aCap;
        mGamepadVelocityLeftY = aVelY * aCap;
      }
      break;
    }

    case MOVEMENT_STATE_BUTTER_HELD:
    {
      // Sampled before the button calls so a virtual override cannot change it.
      float aDirX = mInputLeftStickX;
      float aDirY = mInputLeftStickY;
      float aAlongXSq = aDirX * aDirX;
      float aAlongYSq = aDirY * aDirY;
      Sexy::Gamepad *aGamepad = mApp->mGamepads[mGamepadIndex];

      if (aGamepad->IsButtonDown(GAMEPAD_BUTTON_DPAD_UP) != 0)
      {
        aDirY = 1.0f;
        aAlongYSq = 1.0f;
      }
      else if (aGamepad->IsButtonDown(GAMEPAD_BUTTON_DPAD_DOWN) != 0)
      {
        aDirY = -1.0f;
        aAlongYSq = 1.0f;
      }
      else if (aGamepad->IsButtonDown(GAMEPAD_BUTTON_DPAD_LEFT) != 0)
      {
        aDirX = -1.0f;
        aAlongXSq = 1.0f;
      }
      else if (aGamepad->IsButtonDown(GAMEPAD_BUTTON_DPAD_RIGHT) != 0)
      {
        aDirX = 1.0f;
        aAlongXSq = 1.0f;
      }

      // Decremented only while a direction is held. The step fires when the
      // stored counter is <= 0, i.e. it was already <= 1.
      if (aAlongXSq + aAlongYSq > 0.0f && --mButterCursorMoveCounter <= 0)
      {
        float aNegY = -aDirY;
        float aStep = sqrtf(aAlongXSq + aNegY * aNegY) * 32.0f;
        mCursorPositionX = mGridCenterPositionX + aStep * aDirX;
        mCursorPositionY = mGridCenterPositionY + aStep * aNegY;
      }
      else
      {
        float aDx = mGridCenterPositionX - mCursorPositionX;
        float aDy = mGridCenterPositionY - mCursorPositionY;
        float aDist = sqrtf(aDx * aDx + aDy * aDy);
        if (aDist >= 4.0f)
        {
          if (aDist != 0.0f)
          {
            aDx /= aDist;
            aDy /= aDist;
          }
          mGamepadVelocityLeftX = aDx * 250.0f;
          mGamepadVelocityLeftY = aDy * 250.0f;
        }
        else
        {
          mGamepadVelocityLeftX = 0.0f;
          mGamepadVelocityLeftY = 0.0f;
          mGamepadVelocityRightX = 0.0f;
          mGamepadVelocityRightY = 0.0f;
          mCursorPositionX = mGridCenterPositionX;
          mCursorPositionY = mGridCenterPositionY;
        }
      }
      break;
    }

    case MOVEMENT_STATE_GRID_STEP:
    {
      float aStickX = mInputLeftStickX;
      float aStickY = mInputLeftStickY;
      if (aStickX * aStickX + aStickY * aStickY > 0.0f)
        return GotoState(MOVEMENT_STATE_RESERVED_2);

      float aDx = mGridCenterPositionX - mCursorPositionX;
      float aDy = mGridCenterPositionY - mCursorPositionY;
      float aDist = sqrtf(aDx * aDx + aDy * aDy);
      if (aDist < 3.0f)
        return GotoState(MOVEMENT_STATE_NORMAL);
      if (aDist != 0.0f)
      {
        aDx /= aDist;
        aDy /= aDist;
      }
      mGamepadVelocityLeftX = aDx * 250.0f;
      mGamepadVelocityLeftY = aDy * 250.0f;
      if (mDigIndicatorPercentage > 1.0f)
        mMovementStateTimer = 0.0f;
      break;
    }

    case MOVEMENT_STATE_RESERVED_9:
    {
      float aStickX = mInputLeftStickX;
      float aStickY = mInputLeftStickY;
      if (aStickX * aStickX + aStickY * aStickY > 0.0f)
        return GotoState(MOVEMENT_STATE_RESERVED_2);

      float aVelX = mGamepadVelocityLeftX;
      float aVelY = mGamepadVelocityLeftY;
      if (aVelX * aVelX + aVelY * aVelY < 50.0f)
      {
        // EnterState(NORMAL) clears the velocity; the damp uses the reloaded values.
        GotoState(MOVEMENT_STATE_NORMAL);
        aVelX = mGamepadVelocityLeftX;
        aVelY = mGamepadVelocityLeftY;
      }
      mGamepadVelocityLeftX = aVelX * 0.9f;
      mGamepadVelocityLeftY = aVelY * 0.9f;
      break;
    }

    default:
      break;
  }
  return 0;
}

// ==========================================================================
// ==========================================================================
// BaseGamepadControls::GetCursorPosition(float &,float &)
// @001BE8EC  10 bytes
// ==========================================================================
int BaseGamepadControls::GetCursorPosition(float & theX, float & theY)
{
  theX = mCursorPositionX;
  theY = mCursorPositionY;
  return 0;
}

// ==========================================================================
// BaseGamepadControls::GetCursorPosition(int &,int &)
// @001BE8F8  26 bytes
// ==========================================================================
int BaseGamepadControls::GetCursorPosition(int & theX, int & theY)
{
  theX = (int)mCursorPositionX;
  theY = (int)mCursorPositionY;
  return 0;
}

// ==========================================================================
// BaseGamepadControls::GetGamepadVelocity(float &,float &)
// @001BE914  10 bytes
// ==========================================================================
int BaseGamepadControls::GetGamepadVelocity(float & theX, float & theY)
{
  theX = mGamepadVelocityLeftX;
  theY = mGamepadVelocityLeftY;
  return 0;
}

// ==========================================================================
// BaseGamepadControls::GetCursorGridPosition()
// @001BE920  96 bytes
// ==========================================================================
// sret again (see GetSnapToGridXY): the dump's `this` is the destination and the
// object is the second register, which is why its body both reads the cursor
// through `this` and writes the two grid coordinates back to it.
SnapToGridPosition BaseGamepadControls::GetCursorGridPosition()
{
  SnapToGridPosition aResult;
  aResult.mX = (float)mBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
  aResult.mY = (float)mBoard->PixelToGridYKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
  return aResult;
}

// ==========================================================================
// BaseGamepadControls::DrawFrame(Sexy::Graphics *,BaseGamepadControls::FrameRenderPhase)
// @001BE9EC  1432 bytes
// ==========================================================================
Board * BaseGamepadControls::DrawFrame(Sexy::Graphics *theGraphics, int thePhase)
{
  // Early exits hand mBoard back in r0. The success path hands back whatever
  // Graphics::SetDrawMode returns; that stub is still declared int.
  if ( mGamepadIndex == -1 )
    return mBoard;

  Board *aBoard = mBoard;
  GameMode aMode = aBoard->mApp->mGameMode;
  if ( aMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST
    || aMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON
    || !mDrawCursorFrame )
    return aBoard;

  CursorObject *aCursor = aBoard->mCursorObject[mPlayerIndex];
  if ( aCursor->mCursorType == CURSOR_TYPE_HAMMER )
  {
    if ( aCursor->BeginDraw(theGraphics) )
    {
      aCursor->Draw(theGraphics);
      aCursor->EndDraw(theGraphics);
    }
    aBoard = mBoard;
  }

  int aGridX = aBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
  int aGridY = mBoard->PixelToGridYKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);

  float aJitter = mCursorPositionYJitter;
  aMode = mApp->mGameMode;
  float aYOffset = aJitter - 5.0f;
  float aShadowShift = 2.0f - aJitter;
  if ( aMode == GAMEMODE_CHALLENGE_ZEN_GARDEN )
    aJitter = 33.0f;

  float aXOffset;
  if ( aMode == GAMEMODE_CHALLENGE_ZEN_GARDEN )
  {
    aXOffset = -3.0f;
    aYOffset = aYOffset + aJitter;
  }
  else
  {
    aXOffset = -8.0f;
  }

  // Eight cell corners. GridCellBounds writes them as SexyVector2 pairs.
  SexyVector2 aCorner0;
  SexyVector2 aCorner1;
  SexyVector2 aCorner2;
  SexyVector2 aCorner3;
  SexyVector2 aCorner4;
  SexyVector2 aCorner5;
  SexyVector2 aCorner6;
  SexyVector2 aCorner7;
  mBoard->GridCellBounds(
      aGridX, aGridY,
      aCorner0, aCorner1, aCorner2, aCorner3,
      aCorner4, aCorner5, aCorner6, aCorner7);

  // Phase 0 keeps the near Y and writes the midpoint into the far Y.
  // Phase 1 keeps the far Y and writes the midpoint into the near Y, and
  // swaps the two texture V values (0 and 0.5).
  float aV0 = 0.0f;
  float aV1 = 0.5f;
  if ( thePhase != 0 )
  {
    aV0 = 0.5f;
    aV1 = 1.0f;
    aCorner0.mY = aCorner0.mY + (aCorner2.mY - aCorner0.mY) * 0.5f;
    aCorner1.mY = aCorner1.mY + (aCorner3.mY - aCorner1.mY) * 0.5f;
    aCorner4.mY = aCorner4.mY + (aCorner5.mY - aCorner4.mY) * 0.5f;
    aCorner6.mY = aCorner6.mY + (aCorner7.mY - aCorner6.mY) * 0.5f;
  }
  else
  {
    aCorner2.mY = aCorner0.mY + (aCorner2.mY - aCorner0.mY) * 0.5f;
    aCorner3.mY = aCorner1.mY + (aCorner3.mY - aCorner1.mY) * 0.5f;
    aCorner5.mY = aCorner4.mY + (aCorner5.mY - aCorner4.mY) * 0.5f;
    aCorner7.mY = aCorner6.mY + (aCorner7.mY - aCorner6.mY) * 0.5f;
  }

  // Twelve 32-byte vertices. DrawTrianglesTex consumes the first four as
  // triangles, so the array is passed as SexyVertex2D[3]. The clear loop
  // writes 0, 1.0, 0, 0 into words 0..3 of each vertex; the stores below
  // then overwrite the words the binary actually writes, at the same offsets
  // from SP+var_484 (vertex 0).
  //
  // Offset map (float index = byte/4), taken from the VSTR/STR at 0x1BEC2A:
  //   0  x = aXOffset + corner0.x          1  y = nearY0 + aYOffset
  //   2  u = 0                             3  v = 1.0
  //   4  0                                 5  0
  //   6  0                                 7  phase V0
  //   8  x = aXOffset + corner2.x          9  y = farY2 + aYOffset
  //   14 half                              15 phase V1
  //   16 x = aXOffset + corner5.x          17 y = farY5 + aYOffset
  //   22 half                              23 phase V1
  //   24 x = aXOffset + corner5.x          25 y = farY5 + aYOffset
  //   30 half                              31 phase V0
  //   32 x = aXOffset + corner0.x          33 y = nearY0 + aYOffset
  //   39 phase V1
  //   40 x = aXOffset + corner4.x          41 y = nearY4 + aYOffset
  //   46 half                              47 phase V0
  //   48 x = aXOffset + corner6.x          49 y = farY7 + aYOffset
  //   54 half                              55 phase V0
  //   56 x = aXOffset + corner3.x          57 y = farY3 + aYOffset
  //   62 half                              63 phase V1
  //   64 x = aXOffset + corner3.x          65 y = farY3 + aYOffset
  //   70 1.0                               71 phase V1
  //   72 x = aXOffset + corner1.x          73 y = nearY1 + aYOffset
  //   78 1.0                               79 phase V1
  //   80 x = aXOffset + corner6.x          81 y = nearY6 + aYOffset
  //   86 half                              87 phase V0
  // Unlisted words stay at the clear-loop value.
  float aVertWords[12 * 8];
  for ( int i = 0; i < 12; ++i )
  {
    aVertWords[i * 8 + 0] = 0.0f;
    aVertWords[i * 8 + 1] = 1.0f;
    aVertWords[i * 8 + 2] = 0.0f;
    aVertWords[i * 8 + 3] = 0.0f;
  }

  const float kOne = 1.0f;
  const float kHalf = 0.5f;
  const float aY0 = aCorner0.mY;
  const float aY1 = aCorner1.mY;
  const float aY2 = aCorner2.mY;
  const float aY3 = aCorner3.mY;
  const float aY4 = aCorner4.mY;
  const float aY5 = aCorner5.mY;
  const float aY6 = aCorner6.mY;
  const float aY7 = aCorner7.mY;

  aVertWords[0] = aXOffset + aCorner0.mX;
  aVertWords[1] = aY0 + aYOffset;
  aVertWords[2] = 0.0f;
  aVertWords[3] = kOne;
  aVertWords[7] = aV0;

  aVertWords[8] = aXOffset + aCorner2.mX;
  aVertWords[9] = aY2 + aYOffset;
  aVertWords[10] = 0.0f;
  aVertWords[11] = kOne;
  aVertWords[14] = kHalf;
  aVertWords[15] = aV1;

  aVertWords[16] = aXOffset + aCorner5.mX;
  aVertWords[17] = aY5 + aYOffset;
  aVertWords[18] = 0.0f;
  aVertWords[19] = kOne;
  aVertWords[22] = kHalf;
  aVertWords[23] = aV1;

  aVertWords[24] = aXOffset + aCorner5.mX;
  aVertWords[25] = aY5 + aYOffset;
  aVertWords[26] = 0.0f;
  aVertWords[27] = kOne;
  aVertWords[30] = kHalf;
  aVertWords[31] = aV0;

  aVertWords[32] = aXOffset + aCorner0.mX;
  aVertWords[33] = aY0 + aYOffset;
  aVertWords[34] = 0.0f;
  aVertWords[35] = kOne;
  aVertWords[39] = aV1;

  aVertWords[40] = aXOffset + aCorner4.mX;
  aVertWords[41] = aY4 + aYOffset;
  aVertWords[42] = 0.0f;
  aVertWords[43] = kOne;
  aVertWords[46] = kHalf;
  aVertWords[47] = aV0;

  aVertWords[48] = aXOffset + aCorner6.mX;
  aVertWords[49] = aY7 + aYOffset;
  aVertWords[50] = 0.0f;
  aVertWords[51] = kOne;
  aVertWords[54] = kHalf;
  aVertWords[55] = aV0;

  aVertWords[56] = aXOffset + aCorner3.mX;
  aVertWords[57] = aY3 + aYOffset;
  aVertWords[58] = 0.0f;
  aVertWords[59] = kOne;
  aVertWords[62] = kHalf;
  aVertWords[63] = aV1;

  aVertWords[64] = aXOffset + aCorner3.mX;
  aVertWords[65] = aY3 + aYOffset;
  aVertWords[66] = 0.0f;
  aVertWords[67] = kOne;
  aVertWords[70] = kOne;
  aVertWords[71] = aV1;

  aVertWords[72] = aXOffset + aCorner1.mX;
  aVertWords[73] = aY1 + aYOffset;
  aVertWords[74] = 0.0f;
  aVertWords[75] = kOne;
  aVertWords[78] = kOne;
  aVertWords[79] = aV1;

  aVertWords[80] = aXOffset + aCorner6.mX;
  aVertWords[81] = aY6 + aYOffset;
  aVertWords[82] = 0.0f;
  aVertWords[83] = kOne;
  aVertWords[86] = kHalf;
  aVertWords[87] = aV0;

  SexyVertex2D const (*aTriangles)[3] = (SexyVertex2D const (*)[3])aVertWords;

  // A null 3D context is the software path, which resets the image wrap mode.
  bool aHas3D = theGraphics->Get3D() != nullptr;
  aMode = mApp->mGameMode;

  auto aDrawArrows = [&]() -> Board *
  {
    theGraphics->mTransX = theGraphics->mTransX + aShadowShift;
    theGraphics->mTransY = theGraphics->mTransY + aShadowShift;
    theGraphics->DrawTrianglesTex(IMAGE_GAMEPAD_CURSOR_ARROWS_SHADOW, aTriangles, 4);
    theGraphics->mTransX = theGraphics->mTransX - aShadowShift;
    theGraphics->mTransY = theGraphics->mTransY - aShadowShift;
    theGraphics->DrawTrianglesTex(IMAGE_GAMEPAD_CURSOR_ARROWS, aTriangles, 4);
    if ( !aHas3D )
      IMAGE_GAMEPAD_CURSOR_ARROWS->SetWrapMode(0, 0);
    theGraphics->SetDrawMode(0);
    return aBoard;
  };

  if ( aMode == GAMEMODE_CHALLENGE_BEGHOULED )
  {
    unsigned int aState = (unsigned int)(mGamepadState - 3);
    if ( aState <= 1 )
      return aDrawArrows();
  }
  else if ( mGamepadState == MOVEMENT_STATE_BUTTER_RELEASED )
  {
    return aDrawArrows();
  }
  else if ( aMode == GAMEMODE_CHALLENGE_ZOMBIQUARIUM )
  {
    return aBoard;
  }

  theGraphics->mTransX = theGraphics->mTransX + aShadowShift;
  theGraphics->mTransY = theGraphics->mTransY + aShadowShift;
  theGraphics->DrawTrianglesTex(IMAGE_GAMEPAD_CURSOR_FRAME_SHADOW, aTriangles, 4);
  theGraphics->mTransX = theGraphics->mTransX - aShadowShift;
  theGraphics->mTransY = theGraphics->mTransY - aShadowShift;
  if ( !aHas3D )
    IMAGE_GAMEPAD_CURSOR_FRAME_SHADOW->SetWrapMode(0, 0);

  Sexy::Image *aFrame = IMAGE_GAMEPAD_CURSOR_FRAME;
  if ( mPlayerIndex == 1 )
    aFrame = IMAGE_GAMEPAD_CURSOR_FRAME_BLUE;
  theGraphics->DrawTrianglesTex(aFrame, aTriangles, 4);
  theGraphics->SetDrawMode(0);
  return aBoard;
}

// ==========================================================================
// ==========================================================================
// BaseGamepadControls::GetHitResult()
// @001C0B6C  48 bytes
// ==========================================================================
// sret: the dump's `this` is the 8-byte HitResult the caller receives and the
// object is the second register -- which is why the transcriber's body passes
// `this` straight to MouseHitTest and returns it. The recon declaration still
// types that out parameter `GridItem **`, so the address travels through a cast.
HitResult BaseGamepadControls::GetHitResult()
{
  HitResult aResult;
  mBoard->MouseHitTest((int)mCursorPositionX, (int)mCursorPositionY, &aResult, 0);
  return aResult;
}

// ==========================================================================
// BaseGamepadControls::BaseGamepadControls(Board *,int,int)
// @001C2D78  270 bytes
// ==========================================================================
BaseGamepadControls::BaseGamepadControls(Board * theBoard, int thePlayerIndex, int theGamepadIndex)
{
  // The compiler emits the GamepadListener and GameObject base construction --
  // the embedded GameObject's vptr store and its own base work -- so writing
  // that pair by hand would construct GameObject, and its mSyncBlocks, twice.
  mBoard = theBoard;

  // Two sync blocks: the cursor frame byte and the sixteen-word movement area.
  // The dump's `memset`s are what zero every one of the movement members.
  SyncBlockInfo aFrameBlock;
  aFrameBlock.mData = &mDrawCursorFrame;
  aFrameBlock.mSize = 8;
  mSyncBlocks.push_back(aFrameBlock);
  memset(&mDrawCursorFrame, 0, 8);

  SyncBlockInfo aMovementBlock;
  aMovementBlock.mData = &mGamepadAccLeftX;
  aMovementBlock.mSize = 96;
  mSyncBlocks.push_back(aMovementBlock);
  memset(&mGamepadAccLeftX, 0, 96);

  mPlayerIndex = thePlayerIndex;
  mGamepadIndex = theGamepadIndex;

  mDrawCursorFrame = true;
  mButterCursorMoveCounter = 0;
  mGamepadFrameCounter = 0;
}
