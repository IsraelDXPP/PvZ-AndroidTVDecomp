// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
// Native adaptation of console trash piles, 2026-10-09.

#include "TrashBin.h"

#include "Graphics.h"
#include "LawnApp.h"
#include "LawnPlayerInfo.h"
#include "ReconTypes.h"
#include "Widget.h"
#include <algorithm>
#include <cmath>

namespace {

// The level the pile is built for: the player's own piled-up piece count, or a
// flat 50 once the pile has grown past the point where the count matters.
constexpr int kMaxTrashPieces = 10;
constexpr int kFullPilePieceCount = 50;

} // namespace

// ==========================================================================
// TrashBin::Draw(Sexy::Graphics *)
// @00143CD0  2 bytes
// ==========================================================================
void TrashBin::Draw(Sexy::Graphics * g)
{
  // The TV Draw @0x143CD0 is empty. Geometry follows the reference Draw.
  const int pieceHeight = mTrashPileType == PLANT_PILE ? plantPileHeight : zombiePileHeight;
  Sexy::Image* can = gLawnApp->GetConsoleImage(mTrashPileType == PLANT_PILE ? "plant_can" : "zombie_can");
  if (can != nullptr)
    g->DrawImage(can, mX - (mTrashPileType == PLANT_PILE ? 22 : 0),
                 mY + (mTrashPileType == PLANT_PILE ? 15 : 0));
  const int fullPieces = std::min(kMaxTrashPieces, int(mTrashHeight) / pieceHeight);
  const int partialHeight = int(mTrashHeight) % pieceHeight;
  const auto drawPiece = [&](int piece)
  {
    if (piece >= mPileNum || mPileImage[piece] == nullptr)
      return;
    g->DrawImageMirror(mPileImage[piece], mX,
        mY + partialHeight - pieceHeight * (piece + 1), mMirrorPile[piece]);
  };
  // Reference TrashBin::Draw always draws the top piece, even at zero height.
  // It covers the straight cut at the top of the can artwork. Clipping the
  // pile to mTrashHeight removes that cap and exposes a horizontal seam.
  if (fullPieces < kMaxTrashPieces)
    drawPiece(fullPieces);
  for (int piece = 0; piece < fullPieces; ++piece)
    drawPiece(piece);
}

// ==========================================================================
// TrashBin::~TrashBin()
// @00143CD4  24 bytes
// ==========================================================================
TrashBin::~TrashBin()
{
  // The explicit base destructor and vptr restore the transcription inserted are
  // what the compiler emits for a real base class (see TrashBin.h).
}

// ==========================================================================
// TrashBin::GetZombieTrashPiece(int)
// @00143D08  4 bytes
// ==========================================================================
Sexy::Image * TrashBin::GetZombieTrashPiece(int theLevel)
{
  const int group = std::min(mPileNum, 2);
  int roll;
  do
    roll = RandRangeInt(0, 100);
  while (roll >= mRandomPileWeight[group] && roll <= mRandomPileWeight[group + 3]);
  const int variant = roll <= 29 ? 1 : (roll <= 59 ? 0 : 2);
  mRandomPileWeight[group] = variant == 1 ? 0 : (variant == 0 ? 31 : 61);
  mRandomPileWeight[group + 3] = variant == 1 ? 30 : (variant == 0 ? 60 : 100);
  Sexy::String name;
  StrFormat(&name, "zombie_pile%02d_stack%02d", group + 1, variant + 1);
  return gLawnApp->GetConsoleImage(name.c_str());
}

// ==========================================================================
// TrashBin::GetPlantTrashPiece(int)
// @00143D0C  4 bytes
// ==========================================================================
Sexy::Image * TrashBin::GetPlantTrashPiece(int theLevel)
{
  const int group = std::min(mPileNum, 2);
  const bool second = RandRangeInt(mRandomPileWeight[group], mRandomPileWeight[group + 3]) > 49;
  mRandomPileWeight[group] = second ? 0 : 50;
  mRandomPileWeight[group + 3] = second ? 50 : 100;
  Sexy::String name;
  StrFormat(&name, "plant_pile%02d_stack%02d", group + 1, second ? 2 : 1);
  return gLawnApp->GetConsoleImage(name.c_str());
}

// ==========================================================================
// TrashBin::TrashBin(TrashPileType,float)
// @00143D10  246 bytes
// ==========================================================================
TrashBin::TrashBin(TrashPileType thePileType, float theHeight)
{
  mPileNum = 0;
  mTrashPileType = thePileType;
  const int pieceHeight = thePileType == PLANT_PILE ? plantPileHeight : zombiePileHeight;
  mTrashHeight = std::clamp(theHeight, 0.0f, float(pieceHeight * kMaxTrashPieces));

  // The piece count is the player's level, unless the pile is already deep
  // enough that the first level's set is the one to use.
  int aPieceCount = gLawnApp->mPlayerInfo->mLevel;
  if (gLawnApp->HasFinishedAdventure())
    aPieceCount = kFullPilePieceCount;

  int aPileCount = int(mTrashHeight) / pieceHeight + 1;
  if ( aPileCount >= kMaxTrashPieces )
    aPileCount = kMaxTrashPieces;

  // The last six words are the per-piece random weights the drawer scales by;
  // the constructor leaves them zero and only writes the piece table.
  for (int group = 0; group < 3; ++group)
  {
    mRandomPileWeight[group] = thePileType == PLANT_PILE ? 0 : 31;
    mRandomPileWeight[group + 3] = thePileType == PLANT_PILE ? 100 : 60;
  }

  for (int aPiece = 0; aPiece < aPileCount; ++aPiece)
  {
    mPileImage[aPiece] = (mTrashPileType != PLANT_PILE)
                    ? GetZombieTrashPiece(aPieceCount)
                    : GetPlantTrashPiece(aPieceCount);
    mMirrorPile[aPiece] = RandRangeInt(0, 100) <= 49;
    ++mPileNum;
  }
}
