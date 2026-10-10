// ZenGarden -- reconstructed from libGameMain.so.
//
// No base class and no vtable: the binary has neither a _ZTV nor a _ZTI symbol
// for ZenGarden, and the transcribed bodies are plain member functions
// (ZenGarden.cpp:113 reads gSexyAppBase off `+0`, i.e. mApp).
//
// Resource groups are owned by a standard set; the host does not need the
// original libstdc++ tree layout.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_ZENGARDEN_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_ZENGARDEN_H

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "EffectSystem.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "GridItem.h"
#include "HelpBarWidget.h"
#include "LawnPlayerInfo.h"
#include "PottedPlant.h"
#include "Reanimation.h"
#include "ReanimatorCache.h"
#include "TodParticleSystem.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// Board and LawnApp are reached only through pointers (`Board *mBoard`,
// `Board ** MovePlant(...)`, `LawnApp *mApp`, `LawnApp ** AdvanceCrazyDaveDialog()`),
// and Challenge, Coin, GameStats and StoreScreen are not named in this file at
// all, so all six includes were pure cycle. Board.h includes this header, so that
// edge has to be cut here. Sexy\ReconTypes.h already declares LawnApp.
class Board;
class Challenge;
class Coin;
class GameStats;
class StoreScreen;
class LawnApp;
class HitResult;

// SpecialGridPlacement -- one entry of the per-garden placement table returned
// by GetSpecialGridPlacements. Six ints, not four: the reference tree names only
// the first four, but GridCellWidth/GridCellHeight read [4] and [5] and every
// walker steps the array by 6. 24 bytes.
class SpecialGridPlacement
{
public:
    int mPixelX;
    int mPixelY;
    int mGridX;
    int mGridY;
    int mCellWidth;
    int mCellHeight;
};

class ZenGarden
{
public:
    LawnApp *mApp; // +0x00
    Board *mBoard; // +0x04
    GardenType mGardenType; // +0x08
    // The help bar this garden installs while it owns the board; looked up from
    // `mApp->mHelpBarWidget` on first use and cached here.
    HelpBarWidget *mHelpBar; // +0x0C
    GameObjectType mSelectedTool; // +0x10, last tool recorded by ToolChanged
    // Set while the garden still wants the shop button on the help bar.
    bool mShowShopButton; // +0x14
    std::set<Sexy::String> mLoadedResources;

    ZenGarden();
    void RemovePottedPlant(Plant *thePlant);
    void ZenGardenStart();
    TodParticleSystem *AddHappyEffect(Plant *thePlant);
    void RemoveHappyEffect(Plant *thePlant);
    bool WasPlantNeedFulfilledToday(PottedPlant *thePottedPlant);
    bool WasPlantFertilizedInLastHour(PottedPlant *thePottedPlant);
    bool PlantShouldRefreshNeed(PottedPlant *thePottedPlant);
    void RefreshPlantNeeds(PottedPlant *thePottedPlant);
    PottedPlantNeed GetPlantsNeed(PottedPlant *thePottedPlant);
    bool PlantHighOnChocolate(PottedPlant *thePottedPlant);
    float PlantPottedDrawHeightOffset(SeedType theSeedType, float theScale);
    void StinkyWakeUp(GridItem *theStinky);
    void StinkyStartFallingAsleep(GridItem *theStinky);
    void StinkyFinishFallingAsleep(GridItem *theStinky, int theBlendTime);
    void UpdateStinkyMotionTrail(GridItem *theStinky, bool theStinkyHighOnChocolate);
    GridItem *GetStinky();
    SpecialGridPlacement *GetSpecialGridPlacements(int &theCount);
    void StinkyPickGoal(GridItem *theStinky);
    int PixelToGridX(int theX, int theY);
    int PixelToGridY(int theX, int theY);
    void GetGridSize(int &theGridHeight, int &theGridWidth);
    int GridToPixelX(int theGridX, int theGridY);
    int GridToPixelY(int theGridX, int theGridY);
    int GridCellWidth(int theGridX, int theGridY);
    int GridCellHeight(int theGridX, int theGridY);
    void DrawBackdrop(Sexy::Graphics *g);
    void ShowTutorialArrowOnWateringCan();
    PottedPlant *PottedPlantFromIndex(int thePottedPlantIndex);
    bool PlantCanHaveChocolate(Plant *thePlant);
    bool IsZenGardenFull(bool theIncludeDroppedPresents);
    void FindOpenZenGardenSpot(unsigned char &theSpotX, unsigned char &theSpotY);
    int GetPlantSellPrice(Plant *thePlant);
    void PlantFulfillNeed(Plant *thePlant);
    int PlantsNeedWater();
    bool PlantCanBeWatered(Plant *thePlant);
    int CountPlantsNeedingFertilizer();
    int AllPlantsHaveBeenFertilized();
    void UpdatePlantEffectState(Plant *thePlant);
    void UpdatePlantNeeds();
    void FeedChocolateToPlant(Plant *thePlant);
    void MouseDownWithEmptyWheelBarrow(Plant *thePlant);
    PottedPlant *GetPottedPlantInWheelbarrow();
    void MovePlant(Plant *thePlant, int theGridX, int theGridY);
    void PlantWatered(Plant *thePlant);
    void SetPlantAnimSpeed(Plant *thePlant);
    int PlantGetMinutesSinceHappy(Plant *thePlant);
    int PlantSetLaunchCounter(Plant *thePlant);
    Plant *PlacePottedPlant(int thePottedPlantIndex);
    int AddPottedPlant(PottedPlant *thePottedPlant);
    void AdvanceCrazyDaveDialog();
    void PlantFertilized(Plant *thePlant);
    void MouseDownWithFullWheelBarrow(int x, int y);
    void PlantUpdateProduction(Plant *thePlant);
    void DrawPlantOverlay(Sexy::Graphics *g, Plant *thePlant);
    int CanDropPottedPlantLoot();
    int HasPurchasedStinky();
    int CanDropChocolate();
    int WakeStinky();
    void DoFeedingTool(int x, int y, GridItemState theToolType);
    void ZenToolUpdate(GridItem *theZenTool);
    int MouseDownZenGarden(int x, int y, int theClickCount, HitResult *theHitResult);
    bool IsStinkySleeping();
    bool IsStinkyHighOnChocolate();
    void StinkyAnimRateUpdate(GridItem *theStinky);
    bool ShouldStinkyBeAwake();
    SeedType PickRandomSeedType();
    int LeaveGarden();
    void ResetPlantTimers(PottedPlant *thePottedPlant);
    void PottedPlantUpdate(Plant *thePlant);
    void ResetStinkyTimers();
    void StinkyUpdate(GridItem *theStinky);
    int MakeStinkySleeping();
    float ZenPlantOffsetX(PottedPlant *thePottedPlant);
    void GameButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags);
    int RestoreGlove();
    bool CanFeedStinkyChocolate();
    bool CanFeedPlantChocolate();
    void RebuildHelpbar(bool theFromDialog);
    void ZenGardenUpdate();
    void SetupForZenTutorial();
    void ToolChanged(GameObjectType theObjectType);
    void MouseDownWithMoneySign(Plant *thePlant);
    void MouseDownWithFeedingTool(int x, int y, CursorType theCursorType, int thePlayerIndex);
    void MouseDownWithTool(int x, int y, CursorType theCursorType, int thePlayerIndex);
    void AddStinky();
    void ZenGardenInitLevel(bool theJustSwitchingGardens);
    void OpenStore();
    ~ZenGarden();
    int LoadResources(Sexy::String const &theGroup);
    void DrawPottedPlant(
        Sexy::Graphics *g, float x, float y, PottedPlant *thePottedPlant, float theScale, bool theDrawPot);
    void DrawPottedPlantIcon(Sexy::Graphics *g, float x, float y, PottedPlant *thePottedPlant);
    void GotoGarden(int theGardenIndex);
    void GotoNextGarden();
};

// Layout check: three named members plus the unnamed 0x24-byte tail.

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_ZENGARDEN_H
