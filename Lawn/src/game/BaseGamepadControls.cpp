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
// BaseGamepadControls::BeginDraw(Sexy::Graphics *)
// @001BCE04  4 bytes
// ==========================================================================
bool BaseGamepadControls::BeginDraw(Sexy::Graphics *theGraphics)
{
  return false;
}

// ==========================================================================
// BaseGamepadControls::Draw(Sexy::Graphics *)
// @001BCE10  2 bytes
// ==========================================================================
// Derived controls supply the cursor drawing.
void BaseGamepadControls::Draw(Sexy::Graphics *theGraphics)
{
}

// ==========================================================================
// BaseGamepadControls::EndDraw(Sexy::Graphics *)
// @001BCE14  2 bytes
// ==========================================================================
void BaseGamepadControls::EndDraw(Sexy::Graphics *theGraphics)
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
    // GotoState @0x1BCE78 resets +0x68, the elapsed time for this state.
    // UpdateStates uses it to raise the cursor speed only after 0.5 seconds.
    mDigIndicatorPercentage = 0.0f;
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

  if (mBoard->mMouseInputActive[mPlayerIndex])
  {
    Sexy::Gamepad* gamepad = mGamepadIndex >= 0 ? mBoard->mApp->mGamepads[mGamepadIndex] : nullptr;
    if (gamepad != nullptr && gamepad->IsConnected()
        && (fabsf(gamepad->GetAxisXPositionRamped()) >= 0.274f
            || fabsf(gamepad->GetAxisYPositionRamped()) >= 0.274f))
      mBoard->ReleaseMouseControl(mPlayerIndex);
  }
  if (mBoard->mMouseInputActive[mPlayerIndex])
  {
    mCursorHighlightAnimPhase += theDeltaTime;
    mInputLeftStickX = 0.0f;
    mInputLeftStickY = 0.0f;
    mInputRightStickX = 0.0f;
    mInputRightStickY = 0.0f;
    return 0;
  }

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
  if (mBoard->mMouseInputActive[mPlayerIndex])
    return 0;
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
void BaseGamepadControls::DrawFrame(Sexy::Graphics *theGraphics, int thePhase)
{
  GameMode aMode = mApp->mGameMode;
  if (mGamepadIndex == -1 || !mDrawCursorFrame
      || aMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST
      || aMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
    return;

  CursorObject *aCursor = mBoard->mCursorObject[mPlayerIndex];
  // DrawFrame @0x1BE9EC also runs independently of GamepadControls::Draw.
  // An idle mouse has no selected tool or plant, so neither half is drawn.
  if (mBoard->mMouseInputActive[mPlayerIndex] && aCursor->mCursorType == CURSOR_TYPE_NORMAL)
    return;
  if (aCursor->mCursorType == CURSOR_TYPE_HAMMER && aCursor->BeginDraw(theGraphics))
  {
    aCursor->Draw(theGraphics);
    aCursor->EndDraw(theGraphics);
  }

  int aGridX = mBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
  int aGridY = mBoard->PixelToGridYKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
  float aXOffset = aMode == GAMEMODE_CHALLENGE_ZEN_GARDEN ? -3.0f : -8.0f;
  float aYOffset = mCursorPositionYJitter - 5.0f;
  if (aMode == GAMEMODE_CHALLENGE_ZEN_GARDEN)
    aYOffset += 33.0f;
  float aShadowOffset = 2.0f - mCursorPositionYJitter;

  SexyVector2 aTopLeft, aTopRight, aBottomLeft, aBottomRight;
  SexyVector2 aLeftTopCenter, aLeftBottomCenter, aRightTopCenter, aRightBottomCenter;
  mBoard->GridCellBounds(aGridX, aGridY, aTopLeft, aTopRight, aBottomLeft, aBottomRight,
                        aLeftTopCenter, aLeftBottomCenter, aRightTopCenter, aRightBottomCenter);

  auto aSelectHalf = [thePhase](SexyVector2 &theTop, SexyVector2 &theBottom)
  {
    float aMiddleY = theTop.mY + (theBottom.mY - theTop.mY) * 0.5f;
    if (thePhase == 0)
      theBottom.mY = aMiddleY;
    else
      theTop.mY = aMiddleY;
  };
  aSelectHalf(aTopLeft, aBottomLeft);
  aSelectHalf(aTopRight, aBottomRight);
  aSelectHalf(aLeftTopCenter, aLeftBottomCenter);
  aSelectHalf(aRightTopCenter, aRightBottomCenter);
  float aTopV = thePhase == 0 ? 0.0f : 0.5f;
  float aBottomV = thePhase == 0 ? 0.5f : 1.0f;
  auto aVertex = [aXOffset, aYOffset](const SexyVector2 &thePoint, float theU, float theV)
  {
    return SexyVertex2D{thePoint.mX + aXOffset, thePoint.mY + aYOffset, theU, theV, 0};
  };

  // Original DrawFrame at 0x1BE9EC uses two triangles for each side of the cell.
  // Store position and UV in the current vertex type rather than its original binary layout.
  SexyVertex2D aTriangles[4][3] = {
    {aVertex(aTopLeft, 0.0f, aTopV), aVertex(aBottomLeft, 0.0f, aBottomV),
     aVertex(aLeftBottomCenter, 0.5f, aBottomV)},
    {aVertex(aLeftBottomCenter, 0.5f, aBottomV), aVertex(aLeftTopCenter, 0.5f, aTopV),
     aVertex(aTopLeft, 0.0f, aTopV)},
    {aVertex(aRightTopCenter, 0.5f, aTopV), aVertex(aRightBottomCenter, 0.5f, aBottomV),
     aVertex(aBottomRight, 1.0f, aBottomV)},
    {aVertex(aBottomRight, 1.0f, aBottomV), aVertex(aTopRight, 1.0f, aTopV),
     aVertex(aRightTopCenter, 0.5f, aTopV)}
  };

  bool aDrawArrows = aMode == GAMEMODE_CHALLENGE_BEGHOULED
      ? mGamepadState == MOVEMENT_STATE_BUTTER_HELD || mGamepadState == MOVEMENT_STATE_BUTTER_RELEASED
      : mGamepadState == MOVEMENT_STATE_BUTTER_RELEASED;
  if (!aDrawArrows && aMode == GAMEMODE_CHALLENGE_ZOMBIQUARIUM)
    return;

  Sexy::Image *aShadow = aDrawArrows ? IMAGE_GAMEPAD_CURSOR_ARROWS_SHADOW : IMAGE_GAMEPAD_CURSOR_FRAME_SHADOW;
  Sexy::Image *aFrame = aDrawArrows ? IMAGE_GAMEPAD_CURSOR_ARROWS
      : mPlayerIndex == 1 ? IMAGE_GAMEPAD_CURSOR_FRAME_BLUE : IMAGE_GAMEPAD_CURSOR_FRAME;
  theGraphics->PushState();
  theGraphics->SetDrawMode(0);
  theGraphics->SetColorizeImages(true);
  theGraphics->SetColor(Sexy::Color::White);
  // DrawFrame @0x1BE9EC applies the same shadow offset on both axes.
  theGraphics->mTransX += aShadowOffset;
  theGraphics->mTransY += aShadowOffset;
  theGraphics->DrawTrianglesTex(aShadow, aTriangles, 4);
  theGraphics->mTransX -= aShadowOffset;
  theGraphics->mTransY -= aShadowOffset;
  bool aHas3D = theGraphics->Get3D() != nullptr;
  if (!aHas3D && !aDrawArrows)
    aShadow->SetWrapMode(0, 0);
  theGraphics->DrawTrianglesTex(aFrame, aTriangles, 4);
  if (!aHas3D && aDrawArrows)
    aFrame->SetWrapMode(0, 0);
  theGraphics->PopState();
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
  SyncBlockInfo aFrameBlock = MakeSyncBlock(mDrawCursorFrame, mCursorHighlightAnimPhase);
  mSyncBlocks.push_back(aFrameBlock);
  memset(aFrameBlock.mData, 0, aFrameBlock.mSize);

  SyncBlockInfo aMovementBlock = MakeSyncBlock(mGamepadAccLeftX, mButterCursorMoveCounter);
  mSyncBlocks.push_back(aMovementBlock);
  memset(aMovementBlock.mData, 0, aMovementBlock.mSize);

  mPlayerIndex = thePlayerIndex;
  mGamepadIndex = theGamepadIndex;

  mDrawCursorFrame = true;
  mButterCursorMoveCounter = 0;
  mGamepadFrameCounter = 0;
}
