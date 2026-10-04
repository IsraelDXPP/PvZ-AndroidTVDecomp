bool GamepadControls::OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags)
{
  // Direct call, not the virtual. A nonzero base result is consumed as true.
  if (BaseGamepadControls::OnKeyDown(theKey, theFlags))
    return true;

  // Adventure levels that are not a multiple of 5 swallow the key unless the
  // shop pad is allowed to show. 0x66666667 is signed divide-by-5.
  if (mBoard->mCutScene != nullptr
      && mApp->mGameMode == GAMEMODE_ADVENTURE
      && mBoard->mLevel % 5 != 0
      && mApp->ShopPadCanShow() == 0)
    return true;

  SeedBank *aSeedBank = GetSeedBank();
  MovementState aState = mGamepadState;

  // LABEL_13: quick-dig, shovel escape, whack/potter, then the return-key plant.
  auto aDispatchAction = [&]() -> bool
  {
    if (QuickDigKeyCodeDown(theKey) != 0)
    {
      int aGridX = mBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
      int aGridY = mBoard->PixelToGridYKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
      if (mBoard->GetTopPlantAt(aGridX, aGridY, TOPPLANT_CATAPULT_ORDER) != nullptr
          && mBoard->mShowShovel)
      {
        if (mBoard->mCursorObject[mPlayerIndex]->mCursorType == CURSOR_TYPE_PLANT_FROM_USABLE_COIN)
        {
          mBoard->RefreshSeedPacketFromCursor(mPlayerIndex);
          mBoard->ClearCursor(mPlayerIndex);
        }
        GotoState(MOVEMENT_STATE_DIG_INDICATOR_WAIT);
        GotoState(MOVEMENT_STATE_DIG_HOLD);
        mDigIndicatorPercentage += 2.3f;
        GotoState(MOVEMENT_STATE_NORMAL);
        mIsShowingDigIndicator = true;
        return true;
      }
    }

    if (theKey == Sexy::KEYCODE_ESCAPE)
    {
      if ((theFlags & 0x80) != 0)
        return true;
      if (!mIsZombie)
      {
        int aGridX = mBoard->PixelToGridXKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
        int aGridY = mBoard->PixelToGridYKeepOnBoard((int)mCursorPositionX, (int)mCursorPositionY);
        if (mBoard->GetTopPlantAt(aGridX, aGridY, TOPPLANT_CATAPULT_ORDER) != nullptr
            && mBoard->mShowShovel)
        {
          if (mBoard->mCursorObject[mPlayerIndex]->mCursorType == CURSOR_TYPE_PLANT_FROM_USABLE_COIN)
          {
            mBoard->RefreshSeedPacketFromCursor(mPlayerIndex);
            mBoard->ClearCursor(mPlayerIndex);
          }
          GotoState(MOVEMENT_STATE_DIG_INDICATOR_WAIT);
          mDigIndicatorStartCounter = mGamepadFrameCounter;
          return true;
        }
        if (mBoard->mCursorObject[mPlayerIndex]->mCursorType == CURSOR_TYPE_PLANT_FROM_USABLE_COIN)
        {
          mApp->PlayFoley(FOLEY_DROP);
          mBoard->RefreshSeedPacketFromCursor(mPlayerIndex);
          mBoard->ClearCursor(mPlayerIndex);
          return true;
        }
        return false;
      }
    }

    if (XButtonKeyCodeDown(theKey) != 0
        && (mApp->IsWhackAZombieLevel() != 0
            || mApp->mGameMode == GAMEMODE_CHALLENGE_ZOMBIQUARIUM
            || mApp->IsScaryPotterLevel() != 0))
    {
      HitResult aHit;
      mBoard->MouseHitTest((int)mCursorPositionX, (int)mCursorPositionY, &aHit, 0);
      if (mBoard->mChallenge->MouseDown(
              (int)mCursorPositionX, (int)mCursorPositionY, 0, &aHit, mPlayerIndex))
        return true;
    }

    if (ButtonDownPlantTest(theKey, 0, theFlags) != 0)
      return true;
    if (theKey != Sexy::KEYCODE_RETURN)
      return false;
    if ((theFlags & 0x80) == 0 && ButtonDownFireCobcannonTest() != 0)
      return true;
    if (mGamepadState == MOVEMENT_STATE_DIG_HOLD)
      return true;

    if (mBoard->mCursorObject[mPlayerIndex]->mCursorType == CURSOR_TYPE_PLANT_FROM_USABLE_COIN)
    {
      mBoard->MouseDownWithPlant((int)mCursorPositionX, (int)mCursorPositionY, 1, mPlayerIndex);
      // Reloaded after MouseDownWithPlant; that call can replace the cursor.
      CursorObject *aCursor = mBoard->mCursorObject[mPlayerIndex];
      aCursor->mCursorType = CURSOR_TYPE_NORMAL;
      aCursor->mType = SEED_NONE;
      mSelectedSeedType = SEED_NONE;
      GotoState(MOVEMENT_STATE_NORMAL);
      return false;
    }

    // SeedBank+0x1C is GameObject::mY. A negative Y means the bank is not usable.
    if (aSeedBank == nullptr || aSeedBank->mY < 0 || mBoard->mBoardFadeOutCounter > 0)
      return false;

    bool aCanPick = false;
    if (aSeedBank->mNumPackets > 0)
    {
      for (int aIndex = 0; aIndex < aSeedBank->mNumPackets; ++aIndex)
      {
        if (aSeedBank->mSeedPackets[aIndex].CanPickUp() != 0)
        {
          aCanPick = true;
          break;
        }
      }
    }

    if (mSelectedSeedIndex < 0)
      mSelectedSeedIndex = 0;

    MovementState aPlantState = mGamepadState;
    if (aPlantState != MOVEMENT_STATE_SELECT_SEED && aPlantState != MOVEMENT_STATE_PLANT_CURSOR)
    {
      GotoState(MOVEMENT_STATE_SELECT_SEED);
      return true;
    }

    if (aPlantState != MOVEMENT_STATE_SELECT_SEED)
    {
      if (mIsInShopSeedBank)
      {
        // Fresh GetSeedBank, not the pointer captured on entry.
        SeedBank *aShopBank = GetSeedBank();
        SeedType aShopSeed = aShopBank->mShopSeedPackets[mSelectedShopSeedIndex].mSeedType;
        int aGridX = mBoard->PlantingPixelToGridX((int)mCursorPositionX, (int)mCursorPositionY, aShopSeed);
        int aGridY = mBoard->PlantingPixelToGridY((int)mCursorPositionX, (int)mCursorPositionY, aShopSeed);
        // Shop packets plant only when CanPlantAt returns PLANTING_OK.
        if (mBoard->CanPlantAt(aGridX, aGridY, aShopSeed) != PLANTING_OK)
          return true;
        mApp->PlaySample(SOUND_SEEDLIFT, true);
        mBoard->MouseDownWithPlant((int)mCursorPositionX, (int)mCursorPositionY, 1, mPlayerIndex);
        mBoard->ClearCursor(mPlayerIndex);
        GotoState(MOVEMENT_STATE_NORMAL);
        return true;
      }

      if (!aCanPick)
      {
        mBoard->mCursorPreview[mPlayerIndex]->mVisible = false;
        mBoard->ClearCursor(mPlayerIndex);
        if (mApp->mGameScene == SCENE_PLAYING)
          mApp->PlaySample(SOUND_BUZZER, true);
        return true;
      }

      SeedPacket *aPacket = &aSeedBank->mSeedPackets[mSelectedSeedIndex];
      if (aPacket->CanPickUp() == 0)
        return true;

      int aGridX = mBoard->PlantingPixelToGridX(
          (int)mCursorPositionX, (int)mCursorPositionY, aPacket->mPacketType);
      int aGridY = mBoard->PlantingPixelToGridY(
          (int)mCursorPositionX, (int)mCursorPositionY, aPacket->mPacketType);
      // PLANTING_OK is 0. The imitater check short-circuits the packet-type check.
      bool aPlanted = true;
      if ((aPacket->mPacketType != SEED_IMITATER
              || mBoard->CanPlantAt(aGridX, aGridY, aPacket->mImitaterType) != PLANTING_OK)
          && mBoard->CanPlantAt(aGridX, aGridY, aPacket->mPacketType) != PLANTING_OK)
        aPlanted = false;
      else
        aPacket->MouseDown(0, 0, 1, mPlayerIndex);

      mBoard->MouseDownWithPlant((int)mCursorPositionX, (int)mCursorPositionY, 1, mPlayerIndex);
      if (mBoard->HasConveyorBeltSeedBank(false) != 0 && mSelectedSeedIndex > 0)
      {
        if (!aPlanted)
          return true;
        --mSelectedSeedIndex;
      }
      else if (!aPlanted)
      {
        return true;
      }
      mBoard->ClearCursor(mPlayerIndex);
      GotoState(MOVEMENT_STATE_NORMAL);
      return true;
    }

    if (mIsInShopSeedBank)
    {
      if (mSelectedShopSeedIndex == 6)
      {
        if (mApp->judgeDoTransRecharge())
        {
          mApp->doTransRecharge();
          return true;
        }
        {
          // Thumb ADD-to-PC clears bit 1, so the pool at 0x1C1E30 is the 'c'
          // at 0x57D7E8, not IDA's empty gEmptyCString.
          Sexy::String aDialogName("c");
          mApp->DoBuyMoneyDialog(aDialogName);
        }
        Sexy::Dialog *aDialog = mApp->GetDialog(82);
        if (aDialog != nullptr)
          aDialog->WaitForResult(true);
        return true;
      }
      // 0x1C1BB6 branches unconditionally over GotoBuyTool; that call is dead.
      if (PlantShopPacket() != 0)
      {
        GotoState(MOVEMENT_STATE_NORMAL);
        return true;
      }
    }
    else
    {
      if (!aCanPick)
        return true;
      SeedPacket *aPacket = &aSeedBank->mSeedPackets[mSelectedSeedIndex];
      if (aPacket->CanPickUp() == 0)
        return true;
      if (mApp->mGameMode == GAMEMODE_CHALLENGE_HEAVY_WEAPON)
      {
        aPacket->MouseDown(0, 0, 1, mPlayerIndex);
        mApp->PlaySample(SOUND_SEEDLIFT, true);
        mBoard->MouseDownWithPlant((int)mCursorPositionX, (int)mCursorPositionY, 1, mPlayerIndex);
        mBoard->ClearCursor(mPlayerIndex);
        GotoState(MOVEMENT_STATE_NORMAL);
        return true;
      }
      if (mApp->mGameMode == GAMEMODE_CHALLENGE_BEGHOULED
          || mBoard->mApp->mGameMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST)
      {
        if (aPacket->mPacketType == SEED_NONE)
          return true;
        mBoard->mChallenge->BeghouledPacketClicked(aPacket);
        return true;
      }
    }

    GotoState(MOVEMENT_STATE_PLANT_CURSOR);
    return true;
  };

  // LABEL_11: escape cancels a selected cob cannon, then the action dispatcher.
  auto aDispatchKey = [&]() -> bool
  {
    if (theKey == Sexy::KEYCODE_ESCAPE && mIsCobCannonSelected)
    {
      mIsCobCannonSelected = false;
      mBoard->ClearCursor(mPlayerIndex);
      mBoard->mCobCannonCursorDelayCounter = 0;
      mCobCannonAnimCounter = 0;
      mCobCannonPlantIndexInList = 0;
      return true;
    }
    return aDispatchAction();
  };

  // LABEL_54: arrows step the grid; escape cancels a plant cursor or a beghouled drag.
  auto aDispatchDirection = [&]() -> bool
  {
    if (theKey >= Sexy::KEYCODE_LEFT && theKey <= Sexy::KEYCODE_DOWN)
    {
      SnapToGridPosition aStep = GetNextGridDirection(
          theKey, (int)mCursorPositionX, (int)mCursorPositionY);
      mGamepadVelocityLeftX = aStep.mX;
      mGamepadVelocityLeftY = aStep.mY;
      GotoState(MOVEMENT_STATE_GRID_STEP);
      GotoState(MOVEMENT_STATE_NORMAL);
      if (aState == MOVEMENT_STATE_PLANT_CURSOR)
        GotoState(MOVEMENT_STATE_PLANT_CURSOR);
      return true;
    }

    if (aState == MOVEMENT_STATE_PLANT_CURSOR)
    {
      if (theKey != Sexy::KEYCODE_ESCAPE)
        return aDispatchAction();
      mBoard->mCursorObject[mPlayerIndex]->mType = SEED_NONE;
      mSelectedSeedType = SEED_NONE;
      GotoState(MOVEMENT_STATE_NORMAL);
      return true;
    }

    if (aState != MOVEMENT_STATE_BUTTER_RELEASED)
      return aDispatchKey();
    if (theKey != Sexy::KEYCODE_ESCAPE)
      return aDispatchAction();
    mBoard->mChallenge->BeghouledDragCancel();
    GotoState(MOVEMENT_STATE_NORMAL);
    return true;
  };

  bool aHasPackets = aSeedBank != nullptr && aSeedBank->mNumPackets > 0;
  if (aHasPackets && aState == MOVEMENT_STATE_SELECT_SEED)
  {
    switch (theKey)
    {
      case Sexy::KEYCODE_LEFT:
        if (mIsInShopSeedBank)
          DecShopSeed();
        else
          DecSelectedSeed();
        return true;
      case Sexy::KEYCODE_RIGHT:
        if (mIsInShopSeedBank)
          IncShopSeed();
        else
          IncSelectedSeed();
        return true;
      case Sexy::KEYCODE_UP:
        if (mApp->CanShopLevel() != 0)
          UpSelectedSeed();
        return true;
      case Sexy::KEYCODE_DOWN:
        if (mApp->CanShopLevel() != 0)
          DownSelectedSeed();
        return true;
      default:
        break;
    }
    if (theKey == Sexy::KEYCODE_ESCAPE)
    {
      mBoard->mCursorObject[mPlayerIndex]->mType = SEED_NONE;
      mSelectedSeedType = SEED_NONE;
      GotoState(MOVEMENT_STATE_NORMAL);
      return true;
    }
  }
  else if (aState == MOVEMENT_STATE_GRID_STEP)
  {
    float aDx = mGridCenterPositionX - mCursorPositionX;
    float aDy = mGridCenterPositionY - mCursorPositionY;
    float aDist = sqrtf(aDx * aDx + aDy * aDy);
    if (mApp->mGameMode == GAMEMODE_CHALLENGE_BEGHOULED_TWIST || aDist < 3.0f)
    {
      mCursorPositionX = mGridCenterPositionX;
      mCursorPositionY = mGridCenterPositionY;
      return aDispatchDirection();
    }
  }
  else if (aState != MOVEMENT_STATE_SELECT_SEED && aState != MOVEMENT_STATE_BUTTER_HELD)
  {
    return aDispatchDirection();
  }

  return aDispatchKey();
}
