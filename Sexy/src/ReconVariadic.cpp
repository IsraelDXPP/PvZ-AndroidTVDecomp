// ReconVariadic.cpp -- definitions for the free functions the transcriber left
// as untyped variadic declarations.
//
// WHAT THESE ARE
//
// tools/ida_transcribe.py could not recover a parameter list for a large group
// of the binary's functions, so it emitted `int Name(...);` into
// Sexy/ReconFreeFunctions.h (and a handful of files declare the same shape
// file-locally). Every call site compiled -- an ellipsis accepts any argument
// list -- and every one of them was undefined, so nothing could link.
//
// WHY THE ELLIPSIS HAS TO STAY
//
// The ellipsis is baked into the symbol name. The linker asks for
// `?TodStringTranslate@@YAHZZ`, and the trailing `Z` is the parameter list:
// replacing the declaration with named parameters changes the mangled name to
// something else entirely, so the demand would simply move rather than be
// satisfied. The declaration and the definition therefore both keep `(...)`.
//
// WHY THE BODIES ARE PLACEHOLDERS
//
// A function whose parameter-declaration-clause is only an ellipsis cannot read
// its own arguments: C++ gives it no name to hand to va_start, and MSVC's
// <cstdarg> refuses the one-argument form (`va_start(ap)` is C2059/C3553 --
// measured, not assumed). So no real semantics can be written here, only a
// value the caller survives.
//
// These are real functions in libGameMain.so, not linker artefacts. Each one
// has a body there, and the honest way to get it back is to decompile that body
// from its own dump and replace the placeholder below with a named declaration
// (and the matching definition) at the same time -- declaration and definition
// must move together, or the symbol changes and the error comes back.
//
// Every definition below is therefore marked `// TODO(recon)`. The set is
// exactly the `?Name@@YAHZZ` group in build/unresolved.txt; the three
// SLODWORD/SHIDWORD/SLOBYTE entries of that group are NOT here, because those
// are IDA's address-taking accessors and are macros instead -- see the note at
// their old declaration site in Sexy/ReconFreeFunctions.h.

#include "SexyTypes.h"
#include "EditWidget.h"
#include "Graphics.h"
#include "ReconExterns.h"
#include "ReconTypes.h"
#include "ReconFreeFunctions.h"
#include "Game/Board.h"
#include "Game/Plant.h"
#include "Game/LawnApp.h"
#include "Game/LawnStoneButton.h"
#include "Dialog.h"
#include "SexyAppBase.h"
#include "ReconData.h"
#include "ReconHelpers.h"
#include "PerfTimer.h"
#include "MemoryImage.h"
#include "TodParticleEmitter.h"
#include "Image.h"
#include "ButtonGlyphs.h"
#include "GamepadApp.h"
#include "Font.h"
#include "SexyTransform2D.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cmath>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <map>
#include <vector>
#include <cctype>

PlantDefinition gPlantDefs[NUM_SEED_TYPES] = {
    { SEED_PEASHOOTER, nullptr, REANIM_PEASHOOTER, 0, 100, 750, 1, 150, "PEASHOOTER" },
    { SEED_SUNFLOWER, nullptr, REANIM_SUNFLOWER, 1, 50, 750, 0, 2500, "SUNFLOWER" },
    { SEED_CHERRYBOMB, nullptr, REANIM_CHERRYBOMB, 3, 150, 5000, 0, 0, "CHERRY_BOMB" },
    { SEED_WALLNUT, nullptr, REANIM_WALLNUT, 2, 50, 3000, 0, 0, "WALL_NUT" },
    { SEED_POTATOMINE, nullptr, REANIM_POTATOMINE, 37, 25, 3000, 0, 0, "POTATO_MINE" },
    { SEED_SNOWPEA, nullptr, REANIM_SNOWPEA, 4, 175, 750, 1, 150, "SNOW_PEA" },
    { SEED_CHOMPER, nullptr, REANIM_CHOMPER, 31, 150, 750, 0, 0, "CHOMPER" },
    { SEED_REPEATER, nullptr, REANIM_REPEATER, 5, 200, 750, 1, 150, "REPEATER" },
    { SEED_PUFFSHROOM, nullptr, REANIM_PUFFSHROOM, 6, 0, 750, 1, 150, "PUFF_SHROOM" },
    { SEED_SUNSHROOM, nullptr, REANIM_SUNSHROOM, 7, 25, 750, 0, 2500, "SUN_SHROOM" },
    { SEED_FUMESHROOM, nullptr, REANIM_FUMESHROOM, 9, 75, 750, 1, 150, "FUME_SHROOM" },
    { SEED_GRAVEBUSTER, nullptr, REANIM_GRAVE_BUSTER, 40, 75, 750, 0, 0, "GRAVE_BUSTER" },
    { SEED_HYPNOSHROOM, nullptr, REANIM_HYPNOSHROOM, 10, 75, 3000, 0, 0, "HYPNO_SHROOM" },
    { SEED_SCAREDYSHROOM, nullptr, REANIM_SCRAREYSHROOM, 33, 25, 750, 1, 150, "SCAREDY_SHROOM" },
    { SEED_ICESHROOM, nullptr, REANIM_ICESHROOM, 36, 75, 5000, 0, 0, "ICE_SHROOM" },
    { SEED_DOOMSHROOM, nullptr, REANIM_DOOMSHROOM, 20, 125, 5000, 0, 0, "DOOM_SHROOM" },
    { SEED_LILYPAD, nullptr, REANIM_LILYPAD, 19, 25, 750, 0, 0, "LILY_PAD" },
    { SEED_SQUASH, nullptr, REANIM_SQUASH, 21, 50, 3000, 0, 0, "SQUASH" },
    { SEED_THREEPEATER, nullptr, REANIM_THREEPEATER, 12, 325, 750, 1, 150, "THREEPEATER" },
    { SEED_TANGLEKELP, nullptr, REANIM_TANGLEKELP, 17, 25, 3000, 0, 0, "TANGLE_KELP" },
    { SEED_JALAPENO, nullptr, REANIM_JALAPENO, 11, 125, 5000, 0, 0, "JALAPENO" },
    { SEED_SPIKEWEED, nullptr, REANIM_SPIKEWEED, 22, 100, 750, 0, 0, "SPIKEWEED" },
    { SEED_TORCHWOOD, nullptr, REANIM_TORCHWOOD, 29, 175, 750, 0, 0, "TORCHWOOD" },
    { SEED_TALLNUT, nullptr, REANIM_TALLNUT, 28, 125, 3000, 0, 0, "TALL_NUT" },
    { SEED_SEASHROOM, nullptr, REANIM_SEASHROOM, 39, 0, 3000, 1, 150, "SEA_SHROOM" },
    { SEED_PLANTERN, nullptr, REANIM_PLANTERN, 38, 25, 3000, 0, 2500, "PLANTERN" },
    { SEED_CACTUS, nullptr, REANIM_CACTUS, 15, 125, 750, 1, 150, "CACTUS" },
    { SEED_BLOVER, nullptr, REANIM_BLOVER, 18, 100, 750, 0, 0, "BLOVER" },
    { SEED_SPLITPEA, nullptr, REANIM_SPLITPEA, 32, 125, 750, 1, 150, "SPLIT_PEA" },
    { SEED_STARFRUIT, nullptr, REANIM_STARFRUIT, 30, 125, 750, 1, 150, "STARFRUIT" },
    { SEED_PUMPKINSHELL, nullptr, REANIM_PUMPKIN, 25, 125, 3000, 0, 0, "PUMPKIN" },
    { SEED_MAGNETSHROOM, nullptr, REANIM_MAGNETSHROOM, 35, 100, 750, 0, 0, "MAGNET_SHROOM" },
    { SEED_CABBAGEPULT, nullptr, REANIM_CABBAGEPULT, 13, 100, 750, 1, 300, "CABBAGE_PULT" },
    { SEED_FLOWERPOT, nullptr, REANIM_FLOWER_POT, 33, 25, 750, 0, 0, "FLOWER_POT" },
    { SEED_KERNELPULT, nullptr, REANIM_KERNELPULT, 13, 100, 750, 1, 300, "KERNEL_PULT" },
    { SEED_INSTANT_COFFEE, nullptr, REANIM_COFFEEBEAN, 33, 75, 750, 0, 0, "COFFEE_BEAN" },
    { SEED_GARLIC, nullptr, REANIM_GARLIC, 8, 50, 750, 0, 0, "GARLIC" },
    { SEED_UMBRELLA, nullptr, REANIM_UMBRELLALEAF, 23, 100, 750, 0, 0, "UMBRELLA_LEAF" },
    { SEED_MARIGOLD, nullptr, REANIM_MARIGOLD, 24, 50, 3000, 0, 2500, "MARIGOLD" },
    { SEED_MELONPULT, nullptr, REANIM_MELONPULT, 14, 300, 750, 1, 300, "MELON_PULT" },
    { SEED_GATLINGPEA, nullptr, REANIM_GATLINGPEA, 5, 250, 5000, 1, 150, "GATLING_PEA" },
    { SEED_TWINSUNFLOWER, nullptr, REANIM_TWIN_SUNFLOWER, 1, 150, 5000, 0, 2500, "TWIN_SUNFLOWER" },
    { SEED_GLOOMSHROOM, nullptr, REANIM_GLOOMSHROOM, 27, 150, 5000, 1, 200, "GLOOM_SHROOM" },
    { SEED_CATTAIL, nullptr, REANIM_CATTAIL, 27, 225, 5000, 1, 150, "CATTAIL" },
    { SEED_WINTERMELON, nullptr, REANIM_WINTER_MELON, 27, 200, 5000, 1, 300, "WINTER_MELON" },
    { SEED_GOLD_MAGNET, nullptr, REANIM_GOLD_MAGNET, 27, 50, 5000, 0, 0, "GOLD_MAGNET" },
    { SEED_SPIKEROCK, nullptr, REANIM_SPIKEROCK, 27, 125, 5000, 0, 0, "SPIKEROCK" },
    { SEED_COBCANNON, nullptr, REANIM_COBCANNON, 16, 500, 5000, 0, 600, "COB_CANNON" },
    { SEED_IMITATER, nullptr, REANIM_IMITATER, 33, 0, 750, 0, 0, "IMITATER" },
    { NUM_SEEDS_IN_CHOOSER, nullptr, REANIM_NONE, 0, 0, 0, 0, 0, "" },
    { SEED_EXPLODE_O_NUT, nullptr, REANIM_WALLNUT, 2, 0, 3000, 0, 0, "EXPLODE_O_NUT" },
    { SEED_GIANT_WALLNUT, nullptr, REANIM_WALLNUT, 2, 0, 3000, 0, 0, "GIANT_WALLNUT" },
    { SEED_SPROUT, nullptr, REANIM_ZENGARDEN_SPROUT, 33, 0, 3000, 0, 0, "SPROUT" },
    { SEED_LEFTPEATER, nullptr, REANIM_REPEATER, 5, 200, 750, 1, 150, "REPEATER" },
};

ChallengeDefinition gChallengeDefs[94] = {
    { GAMEMODE_SURVIVAL_NORMAL_STAGE_1, 0, 0, 0, 0, "[SURVIVAL_DAY_NORMAL]" },
    { GAMEMODE_SURVIVAL_NORMAL_STAGE_2, 1, 0, 0, 1, "[SURVIVAL_NIGHT_NORMAL]" },
    { GAMEMODE_SURVIVAL_NORMAL_STAGE_3, 2, 0, 0, 2, "[SURVIVAL_POOL_NORMAL]" },
    { GAMEMODE_SURVIVAL_NORMAL_STAGE_4, 3, 0, 0, 3, "[SURVIVAL_FOG_NORMAL]" },
    { GAMEMODE_SURVIVAL_NORMAL_STAGE_5, 4, 0, 0, 4, "[SURVIVAL_ROOF_NORMAL]" },
    { GAMEMODE_SURVIVAL_HARD_STAGE_1, 5, 0, 1, 0, "[SURVIVAL_DAY_HARD]" },
    { GAMEMODE_SURVIVAL_HARD_STAGE_2, 6, 0, 1, 1, "[SURVIVAL_NIGHT_HARD]" },
    { GAMEMODE_SURVIVAL_HARD_STAGE_3, 7, 0, 1, 2, "[SURVIVAL_POOL_HARD]" },
    { GAMEMODE_SURVIVAL_HARD_STAGE_4, 8, 0, 1, 3, "[SURVIVAL_FOG_HARD]" },
    { GAMEMODE_SURVIVAL_HARD_STAGE_5, 9, 0, 1, 4, "[SURVIVAL_ROOF_HARD]" },
    { GAMEMODE_SURVIVAL_ENDLESS_STAGE_1, 10, 0, 2, 0, "[SURVIVAL_DAY_ENDLESS]" },
    { GAMEMODE_SURVIVAL_ENDLESS_STAGE_2, 11, 0, 2, 1, "[SURVIVAL_NIGHT_ENDLESS]" },
    { GAMEMODE_SURVIVAL_ENDLESS_STAGE_3, 12, 0, 2, 2, "[SURVIVAL_POOL_ENDLESS]" },
    { GAMEMODE_SURVIVAL_ENDLESS_STAGE_4, 13, 0, 2, 3, "[SURVIVAL_FOG_ENDLESS]" },
    { GAMEMODE_SURVIVAL_ENDLESS_STAGE_5, 14, 0, 2, 4, "[SURVIVAL_ROOF_ENDLESS]" },
    { GAMEMODE_CHALLENGE_WAR_AND_PEAS, 0, 1, 0, 0, "[WAR_AND_PEAS]" },
    { GAMEMODE_CHALLENGE_WALLNUT_BOWLING, 1, 1, 0, 1, "[WALL_NUT_BOWLING]" },
    { GAMEMODE_CHALLENGE_SLOT_MACHINE, 2, 1, 0, 2, "[SLOT_MACHINE]" },
    { GAMEMODE_CHALLENGE_HEAVY_WEAPON, 36, 1, 0, 3, "[HEAVY_WEAPON]" },
    { GAMEMODE_CHALLENGE_BEGHOULED, 4, 1, 0, 4, "[BEGHOULED]" },
    { GAMEMODE_CHALLENGE_INVISIGHOUL, 5, 1, 1, 0, "[INVISIGHOUL]" },
    { GAMEMODE_CHALLENGE_SEEING_STARS, 6, 1, 1, 1, "[SEEING_STARS]" },
    { GAMEMODE_CHALLENGE_ZOMBIQUARIUM, 7, 1, 1, 2, "[ZOMBIQUARIUM]" },
    { GAMEMODE_CHALLENGE_BEGHOULED_TWIST, 8, 1, 1, 3, "[BEGHOULED_TWIST]" },
    { GAMEMODE_CHALLENGE_LITTLE_TROUBLE, 9, 1, 1, 4, "[LITTLE_TROUBLE]" },
    { GAMEMODE_CHALLENGE_PORTAL_COMBAT, 10, 1, 2, 0, "[PORTAL_COMBAT]" },
    { GAMEMODE_CHALLENGE_COLUMN, 11, 1, 2, 1, "[COLUMN_AS_YOU_SEE_EM]" },
    { GAMEMODE_CHALLENGE_BOBSLED_BONANZA, 12, 1, 2, 2, "[BOBSLED_BONANZA]" },
    { GAMEMODE_CHALLENGE_SPEED, 13, 1, 2, 3, "[ZOMBIES_ON_SPEED]" },
    { GAMEMODE_CHALLENGE_WHACK_A_ZOMBIE, 14, 1, 2, 4, "[WHACK_A_ZOMBIE]" },
    { GAMEMODE_CHALLENGE_LAST_STAND, 15, 1, 3, 0, "[LAST_STAND]" },
    { GAMEMODE_CHALLENGE_WAR_AND_PEAS_2, 16, 1, 3, 1, "[WAR_AND_PEAS_2]" },
    { GAMEMODE_CHALLENGE_WALLNUT_BOWLING_2, 17, 1, 3, 2, "[WALL_NUT_BOWLING_EXTREME]" },
    { GAMEMODE_CHALLENGE_POGO_PARTY, 18, 1, 3, 3, "[POGO_PARTY]" },
    { GAMEMODE_CHALLENGE_FINAL_BOSS, 19, 1, 3, 4, "[FINAL_BOSS]" },
    { GAMEMODE_CHALLENGE_ART_CHALLENGE_WALLNUT, 20, 1, 4, 0, "[ART_CHALLENGE_WALL_NUT]" },
    { GAMEMODE_CHALLENGE_SUNNY_DAY, 21, 1, 4, 1, "[SUNNY_DAY]" },
    { GAMEMODE_CHALLENGE_RESODDED, 22, 1, 4, 2, "[UNSODDED]" },
    { GAMEMODE_CHALLENGE_BIG_TIME, 23, 1, 4, 3, "[BIG_TIME]" },
    { GAMEMODE_CHALLENGE_ART_CHALLENGE_SUNFLOWER, 24, 1, 4, 4, "[ART_CHALLENGE_SUNFLOWER]" },
    { GAMEMODE_CHALLENGE_AIR_RAID, 25, 1, 5, 0, "[AIR_RAID]" },
    { GAMEMODE_CHALLENGE_ICE, 6, 2, 1, 1, "[ICE_LEVEL]" },
    { GAMEMODE_CHALLENGE_ZEN_GARDEN, 7, 2, 1, 2, "[ZEN_GARDEN]" },
    { GAMEMODE_CHALLENGE_HIGH_GRAVITY, 26, 1, 5, 1, "[HIGH_GRAVITY]" },
    { GAMEMODE_CHALLENGE_GRAVE_DANGER, 27, 1, 5, 2, "[GRAVE_DANGER]" },
    { GAMEMODE_CHALLENGE_SHOVEL, 28, 1, 5, 3, "[CAN_YOU_DIG_IT]" },
    { GAMEMODE_CHALLENGE_STORMY_NIGHT, 29, 1, 6, 1, "[DARK_STORMY_NIGHT]" },
    { GAMEMODE_CHALLENGE_BUNGEE_BLITZ, 30, 1, 2, 2, "[BUNGEE_BLITZ]" },
    { GAMEMODE_CHALLENGE_SQUIRREL, 31, 1, 5, 4, "[SQUIRREL]" },
    { GAMEMODE_TREE_OF_WISDOM, 10, 2, 2, 1, "[TREE_OF_WISDOM]" },
    { GAMEMODE_SCARY_POTTER_1, 32, 4, 0, 0, "[SCARY_POTTER_1]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_1, 34, 4, 2, 0, "[I_ZOMBIE_1]" },
    { GAMEMODE_SCARY_POTTER_2, 32, 4, 0, 1, "[SCARY_POTTER_2]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_2, 34, 4, 2, 1, "[I_ZOMBIE_2]" },
    { GAMEMODE_SCARY_POTTER_3, 32, 4, 0, 2, "[SCARY_POTTER_3]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_3, 34, 4, 2, 2, "[I_ZOMBIE_3]" },
    { GAMEMODE_SCARY_POTTER_4, 32, 4, 0, 3, "[SCARY_POTTER_4]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_4, 34, 4, 2, 3, "[I_ZOMBIE_4]" },
    { GAMEMODE_SCARY_POTTER_5, 32, 4, 0, 4, "[SCARY_POTTER_5]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_5, 34, 4, 2, 4, "[I_ZOMBIE_5]" },
    { GAMEMODE_SCARY_POTTER_6, 32, 4, 1, 0, "[SCARY_POTTER_6]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_6, 34, 4, 3, 0, "[I_ZOMBIE_6]" },
    { GAMEMODE_SCARY_POTTER_7, 32, 4, 1, 1, "[SCARY_POTTER_7]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_7, 34, 4, 3, 1, "[I_ZOMBIE_7]" },
    { GAMEMODE_SCARY_POTTER_8, 32, 4, 1, 2, "[SCARY_POTTER_8]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_8, 34, 4, 3, 2, "[I_ZOMBIE_8]" },
    { GAMEMODE_SCARY_POTTER_9, 32, 4, 1, 3, "[SCARY_POTTER_9]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_9, 34, 4, 3, 3, "[I_ZOMBIE_9]" },
    { GAMEMODE_SCARY_POTTER_ENDLESS, 33, 4, 1, 4, "[SCARY_POTTER_ENDLESS]" },
    { GAMEMODE_PUZZLE_I_ZOMBIE_ENDLESS, 35, 4, 3, 4, "[I_ZOMBIE_ENDLESS]" },
    { GAMEMODE_UPSELL, 10, 2, 3, 4, "[UPSELL]" },
    { GAMEMODE_INTRO, 10, 2, 2, 3, "[INTRO]" },
    { GAMEMODE_MULTI_PLAYER, 10, 2, 2, 2, "" },
    { GAMEMODE_MP_VS_DEBUG, 10, 2, 2, 3, "" },
    { GAMEMODE_MP_VS, 10, 2, 2, 1, "" },
    { GAMEMODE_MP_VS_COOP, 18, 2, 0, 0, "[COOP]" },
    { GAMEMODE_MP_VS_UNKONWN, 18, 2, 0, 1, "" },
    { GAMEMODE_TWO_PLAYER_COOP_DAY, 0, 3, 0, 0, "[COOP_1]" },
    { GAMEMODE_TWO_PLAYER_COOP_NIGHT, 1, 3, 0, 1, "[COOP_2]" },
    { GAMEMODE_TWO_PLAYER_COOP_POOL, 2, 3, 0, 2, "[COOP_3]" },
    { GAMEMODE_TWO_PLAYER_COOP_ROOF, 4, 3, 0, 3, "[COOP_4]" },
    { GAMEMODE_TWO_PLAYER_COOP_BOWLING, 1, 3, 0, 4, "[COOP_BOWLING]" },
    { GAMEMODE_TWO_PLAYER_COOP_DAY_HARD, 5, 3, 1, 0, "[COOP_HARD_1]" },
    { GAMEMODE_TWO_PLAYER_COOP_NIGHT_HARD, 6, 3, 1, 1, "[COOP_HARD_2]" },
    { GAMEMODE_TWO_PLAYER_COOP_POOL_HARD, 7, 3, 1, 2, "[COOP_HARD_3]" },
    { GAMEMODE_TWO_PLAYER_COOP_ROOF_HARD, 9, 3, 1, 3, "[COOP_HARD_4]" },
    { GAMEMODE_TWO_PLAYER_COOP_BOSS, 19, 3, 1, 4, "[COOP_FINAL_BOSS]" },
    { GAMEMODE_TWO_PLAYER_COOP_ENDLESS, 12, 3, 2, 2, "[COOP_ENDLESS]" },
    { GAMEMODE_CHALLENGE_RAINING_SEEDS, 3, 1, 6, 0, "[ITS_RAINING_SEEDS]" },
    { GAMEMODE_MP_VS, 0, 5, 0, 0, "[MP_VS_DAY]" },
    { GAMEMODE_MP_VS, 1, 5, 0, 1, "[MP_VS_NIGHT]" },
    { GAMEMODE_MP_VS, 2, 5, 0, 2, "[MP_VS_POOL_DAY]" },
    { GAMEMODE_MP_VS, 3, 5, 0, 3, "[MP_VS_POOL_NIGHT]" },
    { GAMEMODE_MP_VS, 4, 5, 0, 4, "[MP_VS_ROOF]" },
};


// ---------------------------------------------------------------------------
// Game-side helpers.
// ---------------------------------------------------------------------------
void AddGameObjectRenderItem(BoardRenderItem* theItems, int& theCount, int theType, GameObject* theObject)
{
    BoardRenderItem& aItem = theItems[theCount];
    aItem.mType = theType;
    aItem.mOrder = theObject->mRenderOrder;
    aItem.mObject = theObject;
    ++theCount;
}

void AddUIRenderItem(BoardRenderItem* theItems, int& theCount, int theType, int theOrder)
{
    BoardRenderItem& aItem = theItems[theCount];
    aItem.mType = theType;
    aItem.mOrder = theOrder;
    aItem.mValue = 0;
    ++theCount;
}
void AlmanacInitForPlayer()
{
    for (int aIndex = 0; aIndex < 34; ++aIndex)
        gZombieDefeated[aIndex] = 0;
}

void AlmanacPlayerDefeatedZombie(ZombieType theZombieType)
{
    gZombieDefeated[theZombieType] = 1;
}

// If the other angle is more than 180 degrees away, the so snaps it onto the
// first angle so the blend does not spin the long way around.
static float ClampBlendAngle(float theOther, float theBase)
{
    if (theOther > theBase + 180.0f)
        theOther = theBase;
    if (theOther < theBase - 180.0f)
        theOther = theBase;
    return theOther;
}

ReanimatorTransform* BlendTransform(ReanimatorTransform* theResult, const ReanimatorTransform* theFirst, const ReanimatorTransform* theSecond, float theFactor)
{
    theResult->mTransX = theFirst->mTransX + theFactor * (theSecond->mTransX - theFirst->mTransX);
    theResult->mTransY = theFirst->mTransY + theFactor * (theSecond->mTransY - theFirst->mTransY);
    theResult->mScaleX = theFirst->mScaleX + theFactor * (theSecond->mScaleX - theFirst->mScaleX);
    theResult->mScaleY = theFirst->mScaleY + theFactor * (theSecond->mScaleY - theFirst->mScaleY);
    theResult->mAlpha = theFirst->mAlpha + theFactor * (theSecond->mAlpha - theFirst->mAlpha);

    float aSkewX = ClampBlendAngle(theSecond->mSkewX, theFirst->mSkewX);
    float aSkewY = ClampBlendAngle(theSecond->mSkewY, theFirst->mSkewY);
    theResult->mSkewX = theFirst->mSkewX + theFactor * (aSkewX - theFirst->mSkewX);
    theResult->mSkewY = theFirst->mSkewY + theFactor * (aSkewY - theFirst->mSkewY);
    theResult->mFrame = theFirst->mFrame;
    theResult->mImage = theFirst->mImage;
    theResult->mImage2 = theFirst->mImage2;
    theResult->mFont = theFirst->mFont;
    theResult->mText = theFirst->mText;
    return theResult;
}
extern int gShownMoreSunTutorial;

void BoardInitForPlayer()
{
    gShownMoreSunTutorial = 0;
}
// TODO(recon)
int CalcParticlePerfInfo(...) { return 0; }

// TODO(recon)
void CenterDialog(Sexy::Dialog* theDialog, int theWidth, int theHeight)
{
    theDialog->Resize((800 - theWidth) / 2, (600 - theHeight) / 2, theWidth, theHeight);
}
bool ChallengeModeRecordsTime(GameMode theMode)
{
    if (theMode >= GAMEMODE_SURVIVAL_NORMAL_STAGE_1 && theMode <= GAMEMODE_SURVIVAL_ENDLESS_STAGE_5)
        return true;
    if (theMode >= GAMEMODE_TWO_PLAYER_COOP_DAY && theMode <= GAMEMODE_TWO_PLAYER_COOP_ROOF)
        return true;
    if (theMode >= GAMEMODE_TWO_PLAYER_COOP_DAY_HARD && theMode <= GAMEMODE_TWO_PLAYER_COOP_POOL_HARD)
        return true;
    return theMode == GAMEMODE_TWO_PLAYER_COOP_ROOF_HARD
        || theMode == GAMEMODE_TWO_PLAYER_COOP_ENDLESS;
}
// TODO(recon)
Sexy::Color* ColorAdd(Sexy::Color* theResult, const Sexy::Color* theFirst, const Sexy::Color* theSecond)
{
    int aRed = theFirst->mRed + theSecond->mRed;
    int aGreen = theFirst->mGreen + theSecond->mGreen;
    int aBlue = theFirst->mBlue + theSecond->mBlue;
    int aAlpha = theFirst->mAlpha + theSecond->mAlpha;
    if (aRed < 0) aRed = 0;
    if (aRed > 255) aRed = 255;
    if (aGreen < 0) aGreen = 0;
    if (aGreen > 255) aGreen = 255;
    if (aBlue < 0) aBlue = 0;
    if (aBlue > 255) aBlue = 255;
    if (aAlpha < 0) aAlpha = 0;
    if (aAlpha > 255) aAlpha = 255;
    theResult->mRed = aRed;
    theResult->mGreen = aGreen;
    theResult->mBlue = aBlue;
    theResult->mAlpha = aAlpha;
    return theResult;
}
// TODO(recon)
int ColorComponentMultiply(int theFirst, int theSecond)
{
    int aProduct = theSecond * theFirst / 255;
    if (aProduct <= 0)
        return 0;
    if (aProduct >= 255)
        return 255;
    return aProduct;
}
// TODO(recon)
Sexy::Color* ColorsMultiply(Sexy::Color* theResult, const Sexy::Color* theFirst, const Sexy::Color* theSecond)
{
    theResult->mRed = ColorComponentMultiply(theFirst->mRed, theSecond->mRed);
    theResult->mGreen = ColorComponentMultiply(theFirst->mGreen, theSecond->mGreen);
    theResult->mBlue = ColorComponentMultiply(theFirst->mBlue, theSecond->mBlue);
    theResult->mAlpha = ColorComponentMultiply(theFirst->mAlpha, theSecond->mAlpha);
    return theResult;
}
// TODO(recon)
Sexy::EditWidget* CreateEditWidget(int theId, Sexy::EditListener* theListener, Sexy::Dialog* theDialog)
{
    void* aMemory = operator new(sizeof(Sexy::EditWidget));
    Sexy::EditWidget* aWidget = new (aMemory) Sexy::EditWidget(theId, theListener);
    aWidget->SetFont(FONT_BRIANNETOD16, nullptr);
    (void)theDialog;
    return aWidget;
}
void DrawEditBox(Sexy::Graphics* theGraphics, Sexy::EditWidget* theWidget)
{
    Sexy::TRect<int> aRect(
        theWidget->mX - 8,
        theWidget->mY - 4,
        theWidget->mWidth + 16,
        theWidget->mHeight + 8);
    theGraphics->DrawImageBox(aRect, IMAGE_EDITBOX);
}
// TODO(recon)
void DrawSeedPacket(Sexy::Graphics* theGraphics, float theX, float theY, SeedType theSeedType,
                   SeedType theImitaterType, float thePercentDark, int theGrayness,
                   bool theDrawCost, bool theUseCurrentCost, bool theZombiePacket, bool theDrawBackground)
{
    SeedType aDrawn = theSeedType;
    if (theImitaterType != SEED_NONE && theSeedType == SEED_IMITATER)
        aDrawn = theImitaterType;

    if (theGrayness != 255)
    {
        theGraphics->SetColor(Sexy::Color(theGrayness, theGrayness, theGrayness));
        theGraphics->SetColorizeImages(true);
    }
    else if (thePercentDark > 0.0f)
    {
        theGraphics->SetColor(Sexy::Color(128, 128, 128, 255));
        theGraphics->SetColorizeImages(true);
    }

    int aCel = 2;
    if (theSeedType == SEED_IMITATER)
        aCel = 0;
    else if (Plant::IsUpgrade(aDrawn) != 0)
        aCel = 1;
    else if (theSeedType == SEED_BEGHOULED_BUTTON_CRATER)
        aCel = 3;
    else if (theSeedType == SEED_BEGHOULED_BUTTON_SHUFFLE)
        aCel = 4;
    else if (theSeedType == SEED_SLOT_MACHINE_SUN)
        aCel = 5;
    else if (theSeedType == SEED_SLOT_MACHINE_DIAMOND)
        aCel = 6;
    else if (theSeedType == SEED_ZOMBIQUARIUM_SNORKLE)
        aCel = 7;
    else if (theSeedType == SEED_ZOMBIQUARIUM_TROPHY)
        aCel = 8;

    const bool aLargePacket = theDrawBackground && theGraphics->mScaleX > 1.0f
        && theSeedType <= SEED_LEFTPEATER;
    if (theDrawBackground)
    {
        if (aLargePacket)
        {
            TodDrawImageCelScaledF(theGraphics, IMAGE_SEEDPACKET_LARGER, theX, theY, 0, 0,
                                   theGraphics->mScaleX * 0.5f, theGraphics->mScaleY * 0.5f);
        }
        else if (theZombiePacket)
        {
            float aShift = theGraphics->mScaleX > 1.2f ? -1.5f : 0.0f;
            TodDrawImageScaledF(theGraphics, IMAGE_ZOMBIE_SEEDPACKET, theX, theY + aShift,
                                theGraphics->mScaleX, theGraphics->mScaleY);
        }
        else
        {
            TodDrawImageCelScaledF(theGraphics, IMAGE_SEEDS, theX, theY, aCel, 0,
                                   theGraphics->mScaleX, theGraphics->mScaleY);
        }
    }

    // Icon offsets and scales from DrawSeedPacket at 0x17171C.
    float aIconX = 5.0f;
    float aIconY = 8.0f;
    float aIconScale = 0.5f;
    bool aHasIcon = true;
    auto aSetIcon = [&](float theIconX, float theIconY, float theIconScale)
    {
        aIconX = theIconX;
        aIconY = theIconY;
        aIconScale = theIconScale;
    };
    switch (aDrawn)
    {
    case SEED_TALLNUT: aSetIcon(12.0f, 22.0f, 0.3f); break;
    case SEED_INSTANT_COFFEE: aSetIcon(0.0f, 9.0f, 0.55f); break;
    case SEED_COBCANNON: aSetIcon(6.0f, 22.0f, 0.26f); break;
    case SEED_CACTUS: aSetIcon(9.0f, 13.0f, 0.5f); break;
    case SEED_MAGNETSHROOM: aSetIcon(5.0f, 12.0f, 0.5f); break;
    case SEED_POTATOMINE:
    case SEED_PUMPKINSHELL:
    case SEED_FUMESHROOM:
    case SEED_DOOMSHROOM:
    case SEED_CHOMPER:
    case SEED_HYPNOSHROOM:
    case SEED_SQUASH:
    case SEED_TANGLEKELP:
    case SEED_SPIKEROCK:
    case SEED_SPIKEWEED:
    case SEED_TORCHWOOD:
    case SEED_PLANTERN: aSetIcon(8.0f, 12.0f, 0.4f); break;
    case SEED_TWINSUNFLOWER:
    case SEED_GLOOMSHROOM: aSetIcon(7.0f, 14.0f, 0.45f); break;
    case SEED_CATTAIL: aSetIcon(8.0f, 13.0f, 0.45f); break;
    case SEED_KERNELPULT: aSetIcon(13.0f, 14.0f, 0.4f); break;
    case SEED_CABBAGEPULT: aSetIcon(15.0f, 14.0f, 0.4f); break;
    case SEED_WINTERMELON:
    case SEED_MELONPULT: aSetIcon(18.0f, 19.0f, 0.35f); break;
    case SEED_GRAVEBUSTER: aSetIcon(10.0f, 15.0f, 0.4f); break;
    case SEED_SPLITPEA: aSetIcon(12.0f, 12.0f, 0.45f); break;
    case SEED_BLOVER: aSetIcon(8.0f, 17.0f, 0.4f); break;
    case SEED_STARFRUIT: aSetIcon(6.0f, 8.0f, 0.5f); break;
    case SEED_THREEPEATER:
    case SEED_UMBRELLA: aSetIcon(5.0f, 10.0f, 0.5f); break;
    case SEED_GATLINGPEA: aSetIcon(2.0f, 8.0f, 0.5f); break;
    case SEED_ZOMBIE_NORMAL:
    case SEED_ZOMBIE_TRAFFIC_CONE:
    case SEED_ZOMBIE_PAIL:
    case SEED_ZOMBIE_FLAG:
    case SEED_ZOMBIE_NEWSPAPER:
    case SEED_ZOMBIE_DANCER: aSetIcon(-3.0f, -7.0f, 0.35f); break;
    case SEED_ZOMBIE_POLEVAULTER: aSetIcon(-8.0f, -12.0f, 0.35f); break;
    case SEED_ZOMBIE_SCREEN_DOOR:
    case SEED_ZOMBIE_LADDER:
    case SEED_ZOMBIE_TRASHCAN:
    case SEED_ZOMBIE_JACK_IN_THE_BOX:
    case SEED_ZOMBIE_DIGGER:
    case SEED_ZOMBIE_POGO: aSetIcon(-3.0f, -10.0f, 0.35f); break;
    case SEED_ZOMBIE_BUNGEE: aSetIcon(1.0f, -1.0f, 0.3f); break;
    case SEED_ZOMBIE_FOOTBALL: aSetIcon(-7.0f, -9.0f, 0.33f); break;
    case SEED_ZOMBIE_BALLOON: aSetIcon(-3.0f, -5.0f, 0.35f); break;
    case SEED_ZOMBIE_IMP: aSetIcon(-12.0f, -17.0f, 0.4f); break;
    case SEED_ZOMBIE_CATAPULT:
    case SEED_ZOMBONI: aSetIcon(1.0f, 3.0f, 0.23f); break;
    case SEED_ZOMBIE_GARGANTUAR: aSetIcon(4.0f, 3.0f, 0.23f); break;
    case SEED_BEGHOULED_BUTTON_SHUFFLE:
    case SEED_BEGHOULED_BUTTON_CRATER:
    case SEED_SLOT_MACHINE_SUN:
    case SEED_SLOT_MACHINE_DIAMOND:
    case SEED_ZOMBIQUARIUM_SNORKLE:
    case SEED_ZOMBIQUARIUM_TROPHY: aHasIcon = false; break;
    default: break;
    }
    if (gLawnApp->mGameMode == GAMEMODE_CHALLENGE_BIG_TIME)
    {
        if (aDrawn == SEED_SUNFLOWER || aDrawn == SEED_WALLNUT || aDrawn == SEED_MARIGOLD)
        {
            aIconX = 16.0f;
            aIconY = 34.0f;
        }
    }
    aIconX *= theGraphics->mScaleX;
    aIconY = (aIconY + 1.0f) * theGraphics->mScaleY;
    if (aDrawn == SEED_GIANT_WALLNUT)
    {
        aIconX = 52.0f;
        aIconY = 59.0f;
        aIconScale *= 0.75f;
    }
    auto aDrawIcon = [&](SeedType theIconType)
    {
        // DrawSeedType at 0x171644 scales only the icon, preserving the packet scale.
        theGraphics->PushState();
        theGraphics->mScaleX *= aIconScale;
        theGraphics->mScaleY *= aIconScale;
        if (theIconType == SEED_ZOMBIE_GRAVESTONE)
            TodDrawImageCelScaledF(theGraphics, IMAGE_MP_TOMBSTONE, theX + aIconX, theY + aIconY,
                                   0, 0, theGraphics->mScaleX, theGraphics->mScaleY);
        else
            Plant::DrawSeedType(theGraphics, theIconType, theImitaterType, VARIATION_NORMAL,
                                theX + aIconX, theY + aIconY);
        theGraphics->PopState();
    };
    if (theDrawBackground && aHasIcon)
        aDrawIcon(aDrawn);

    if (thePercentDark > 0.0f)
    {
        int aDarkHeight = (int)(thePercentDark * 68.0f + (thePercentDark * 68.0f <= 0.0f ? -0.5f : 0.5f));
        theGraphics->PushState();
        theGraphics->SetColor(Sexy::Color(64, 64, 64, 255));
        theGraphics->SetColorizeImages(true);
        theGraphics->ClipRect((int)theX, (int)theY,
                              (int)(theGraphics->mScaleX * 50.0f),
                              (int)((float)(aDarkHeight + 2) * theGraphics->mScaleY));
        if (theDrawBackground)
        {
            TodDrawImageCelScaledF(theGraphics, IMAGE_SEEDS, theX, theY, aCel, 0,
                                   theGraphics->mScaleX, theGraphics->mScaleY);
        }
        if (theDrawBackground && aHasIcon)
            aDrawIcon(theSeedType);
        theGraphics->PopState();
    }

    if (theDrawCost)
    {
        int aCost = Plant::GetCost(theSeedType, theImitaterType);
        Board* aBoard = gLawnApp != nullptr ? gLawnApp->mBoard : nullptr;
        Sexy::String aPrice;
        if (aBoard != nullptr && aBoard->PlantUsesAcceleratedPricing(aDrawn) != 0)
        {
            if (theUseCurrentCost)
                aCost = aBoard->GetCurrentPlantCost(theSeedType, theImitaterType);
            StrFormat(&aPrice, theUseCurrentCost ? "%d" : "%d+", aCost);
        }
        else
        {
            StrFormat(&aPrice, "%d", aCost);
        }
        Sexy::Font* aFont = FONT_BRIANNETOD12;
        int aTextWidth = aFont->StringWidth(aPrice);
        float aTextX = (float)(33 - aTextWidth);
        float aTextY = (float)(aFont->GetAscent() + 50);
        if (aLargePacket)
        {
            // The TV formula uses the normal packet's dimensions. Anchor the
            // price to the larger artwork's bottom-right cost panel instead.
            aTextX += (float)IMAGE_SEEDPACKET_LARGER->GetCelWidth() * 0.5f
                - (float)IMAGE_SEEDS->GetCelWidth();
            aTextY += (float)IMAGE_SEEDPACKET_LARGER->GetCelHeight() * 0.5f
                - (float)IMAGE_SEEDS->GetCelHeight();
        }
        Sexy::Color aBlack(0, 0, 0, 255);
        if (theGraphics->mScaleX == 1.0f && theGraphics->mScaleY == 1.0f)
        {
            TodDrawString(theGraphics, aPrice, (int)(theX + aTextX), (int)(theY + aTextY),
                          aFont, aBlack, 0);
        }
        else
        {
            Sexy::SexyMatrix3 aMatrix;
            TodScaleTransformMatrix(&aMatrix,
                                    theX + theGraphics->mTransX + aTextX * theGraphics->mScaleX,
                                    theY + theGraphics->mTransY - 1.0f + aTextY * theGraphics->mScaleY,
                                    theGraphics->mScaleX, theGraphics->mScaleY);
            if (theGraphics->mScaleX > 1.8f)
                theGraphics->SetLinearBlend(false);
            TodDrawStringMatrix(theGraphics, aFont, aMatrix, aPrice, aBlack);
            theGraphics->SetLinearBlend(true);
        }
    }
    theGraphics->SetColorizeImages(false);
}
// TODO(recon)
void DrawStoneButton(Sexy::Graphics* theGraphics, int theX, int theY, int theWidth, int theHeight,
                     bool theInverted, bool theDown, const Sexy::String& theLabel, bool theDisabled)
{
    Sexy::Image* aLeft = theDown ? IMAGE_BUTTON_GLOW_LEFT : IMAGE_BUTTON_LEFT;
    Sexy::Image* aMiddle = theDown ? IMAGE_BUTTON_GLOW_MIDDLE : IMAGE_BUTTON_MIDDLE;
    Sexy::Image* aRight = theDown ? IMAGE_BUTTON_GLOW_RIGHT : IMAGE_BUTTON_RIGHT;
    int aDrawX = theX;
    int aDrawY = theY;
    if (theInverted)
    {
        aLeft = IMAGE_BUTTON_DOWN_LEFT;
        aMiddle = IMAGE_BUTTON_DOWN_MIDDLE;
        aRight = IMAGE_BUTTON_DOWN_RIGHT;
        aDrawX = theX + 1;
    }
    float aSpan = (float)(theWidth - aLeft->mWidth - aRight->mWidth) / (float)aMiddle->mWidth;
    int aCount = (int)(aSpan <= 0.0f ? aSpan - 0.5f : aSpan + 0.5f);
    theGraphics->DrawImage(aLeft, aDrawX, theY);
    int aCursor = aDrawX + aLeft->mWidth;
    for (int aPiece = 0; aPiece < aCount; ++aPiece)
    {
        theGraphics->DrawImage(aMiddle, aCursor, theY);
        aCursor += aMiddle->mWidth;
    }
    theGraphics->DrawImage(aRight, aCursor, theY);
    int aButtonWidth = aCursor + aRight->mWidth - theX;

    Sexy::Color aColor(0, 196, 0);
    if (theDisabled)
        aColor = Sexy::Color(128, 128, 128);
    else if (theDown)
        aColor = Sexy::Color(1, 233, 1);

    Sexy::Font* aFont = FONT_DWARVENTODCRAFT18;
    theGraphics->SetFont(aFont);
    int aTextWidth = aFont->StringWidth(theLabel);
    float aScale = aTextWidth > 0 ? ((float)aButtonWidth - 36.0f) / (float)aTextWidth : 1.0f;
    if (aScale > 1.0f)
        aScale = 1.0f;
    int aTextX = (int)((float)aDrawX + 1.0f + ((float)aButtonWidth - (float)aTextWidth * aScale) * 0.5f);
    int aAscent = aFont->GetAscent();
    int aTextY = (int)((float)aDrawY - 2.0f + ((float)theHeight + (float)(aAscent + aAscent / -6 - 1) * aScale) * 0.5f);

    Sexy::SexyTransform2D aMatrix(true);
    aMatrix.Scale(aScale * theGraphics->mScaleX, aScale * theGraphics->mScaleY);
    aMatrix.Translate((float)aTextX + theGraphics->mTransX, (float)aTextY + theGraphics->mTransY);
    TodDrawStringMatrix(theGraphics, aFont, aMatrix, theLabel, aColor);
}
// TODO(recon)
Sexy::Image* FilterEffectGetImage(Sexy::Image* theImage, FilterEffect)
{
    // _Z20FilterEffectGetImagePN4Sexy5ImageE12FilterEffect is a single BX LR.
    // The image arrives in r0 and leaves in r0.
    return theImage;
}
// Four maps, 0x6D59E4 through 0x6D5A44. Each value image is owned by the map.
static const int FILTER_MAP_COUNT = 4;
static std::map<Sexy::Image*, Sexy::Image*> gFilterMap[FILTER_MAP_COUNT];

void FilterEffectDisposeForApp()
{
    for (int aIndex = 0; aIndex < FILTER_MAP_COUNT; ++aIndex)
    {
        std::map<Sexy::Image*, Sexy::Image*>& aMap = gFilterMap[aIndex];
        for (std::map<Sexy::Image*, Sexy::Image*>::iterator anItr = aMap.begin();
             anItr != aMap.end();
             ++anItr)
        {
            if (anItr->second != nullptr)
                delete anItr->second;
        }
        aMap.clear();
    }
}

void FilterEffectInitForApp()
{
}
// Opaque pixels bound the icon. A wider box grows in height, a taller box grows
// in width, and the extra is split above or to the left, clamped to the image.
int FindBuddyImageSize(Sexy::MemoryImage *theImage, Sexy::TRect<int> *theRect)
{
    int aWidth = theImage->mWidth;
    int aHeight = theImage->mHeight;
    int aMinX = aWidth;
    int aMaxX = 0;
    int aMinY = aHeight;
    int aMaxY = 0;
    unsigned long *aBits = theImage->mBits;
    for (int x = 0; x < aWidth; ++x)
    {
        for (int y = 0; y < aHeight; ++y)
        {
            if (aBits[x + y * aWidth] != 0)
            {
                if (x < aMinX)
                    aMinX = x;
                if (x > aMaxX)
                    aMaxX = x;
                if (y < aMinY)
                    aMinY = y;
                if (y > aMaxY)
                    aMaxY = y;
            }
        }
    }

    if (aMinX == aWidth || aWidth <= 0)
    {
        theRect->mX = 0;
        theRect->mY = 0;
        theRect->mWidth = 0;
        theRect->mHeight = 0;
        return 0;
    }

    int aBoxWidth = aMaxX - aMinX + 1;
    int aBoxHeight = aMaxY - aMinY + 1;
    if (aBoxWidth > aBoxHeight)
    {
        int aGrown = aBoxWidth;
        if (aGrown > aHeight)
            aGrown = aHeight;
        int aShifted = aMinY - (aGrown - aBoxHeight) / 2;
        if (aShifted < 0)
            aShifted = 0;
        aMinY = aShifted;
        aBoxHeight = aGrown;
    }
    else if (aBoxHeight > aBoxWidth)
    {
        int aGrown = aBoxHeight;
        if (aGrown > aWidth)
            aGrown = aWidth;
        int aShifted = aMinX - (aGrown - aBoxWidth) / 2;
        if (aShifted < 0)
            aShifted = 0;
        aMinX = aShifted;
        aBoxWidth = aGrown;
    }

    theRect->mX = aMinX;
    theRect->mY = aMinY;
    theRect->mWidth = aBoxWidth;
    theRect->mHeight = aBoxHeight;
    return aMinY;
}
// TODO(recon)
float FloatTrackEvaluate(FloatParameterTrack& theTrack, float theTime, float theSeed)
{
    if (theTrack.mCountNodes == 0 || theTrack.mNodes == nullptr)
        return 0.0f;

    FloatParameterTrackNode* aNodes = theTrack.mNodes;
    if (aNodes[0].mTime > theTime)
        return TodCurveEvaluate(theSeed, aNodes[0].mLowValue, aNodes[0].mHighValue, aNodes[0].mDistribution);

    int aLast = theTrack.mCountNodes - 1;
    if (theTrack.mCountNodes <= 1 || theTime > aNodes[aLast].mTime)
    {
        FloatParameterTrackNode& aNode = aNodes[aLast];
        return TodCurveEvaluate(theSeed, aNode.mLowValue, aNode.mHighValue, aNode.mDistribution);
    }

    int aRight = 1;
    while (aRight < theTrack.mCountNodes && theTime > aNodes[aRight].mTime)
        ++aRight;
    if (aRight >= theTrack.mCountNodes)
    {
        FloatParameterTrackNode& aNode = aNodes[aLast];
        return TodCurveEvaluate(theSeed, aNode.mLowValue, aNode.mHighValue, aNode.mDistribution);
    }

    FloatParameterTrackNode& aLeft = aNodes[aRight - 1];
    FloatParameterTrackNode& aRightNode = aNodes[aRight];
    float aSpan = aRightNode.mTime - aLeft.mTime;
    float aFraction = (theTime - aLeft.mTime) / aSpan;
    float aLeftValue = TodCurveEvaluate(theSeed, aLeft.mLowValue, aLeft.mHighValue, aLeft.mDistribution);
    float aRightValue = TodCurveEvaluate(theSeed, aRightNode.mLowValue, aRightNode.mHighValue, aRightNode.mDistribution);
    return TodCurveEvaluate(aFraction, aLeftValue, aRightValue, aLeft.mCurveType);
}
// TODO(recon)
bool FloatTrackIsSet(const FloatParameterTrack* theTrack)
{
    if (theTrack->mCountNodes == 0 || theTrack->mNodes == nullptr)
        return false;
    return theTrack->mNodes[0].mCurveType != CURVE_CONSTANT;
}


const ChallengeDefinition* GetChallengeDefinition(int theIndex)
{
    return &gChallengeDefs[theIndex];
}

int GetChallengeByRowColumn(int thePage, int theRow, int theCol)
{
    for (int aIndex = 0; aIndex < 94; ++aIndex)
    {
        const ChallengeDefinition* aDef = GetChallengeDefinition(aIndex);
        if (aDef->mPage == thePage && aDef->mCol == theCol && aDef->mRow == theRow)
            return aIndex;
    }
    return 2;
}
// TODO(recon)
bool GetCircleRectOverlap(int theCenterX, int theCenterY, int theRadius, const Sexy::TRect<int>& theRect)
{
    int aGapX = 0;
    bool aOutsideX = false;
    if (theRect.mX > theCenterX)
    {
        aGapX = theRect.mX - theCenterX;
        aOutsideX = true;
    }
    else if (theCenterX > theRect.mX + theRect.mWidth)
    {
        aGapX = theCenterX - theRect.mX - theRect.mWidth;
        aOutsideX = true;
    }

    if (theRect.mY > theCenterY)
    {
        int aGapY = theRect.mY - theCenterY;
        if (aOutsideX)
            return theRadius * theRadius >= aGapY * aGapY + aGapX * aGapX;
        return theRadius >= aGapY;
    }
    if (theCenterY > theRect.mY + theRect.mHeight)
    {
        int aGapY = theCenterY - theRect.mY - theRect.mHeight;
        if (aOutsideX)
            return theRadius * theRadius >= aGapY * aGapY + aGapX * aGapX;
        return theRadius >= aGapY;
    }
    return !aOutsideX || aGapX <= theRadius;
}
// TODO(recon)
int GetCurrentDaysSince2000()
{
    std::time_t aNow = std::time(nullptr);
    std::tm aLocal;
    localtime_s(&aLocal, &aNow);
    int aSince1901 = aLocal.tm_year - 101;
    if (aSince1901 < 0)
        aSince1901 = aLocal.tm_year - 98;
    int aLeap = aSince1901 / 4 - aSince1901 / 100 + aSince1901 / 400;
    return aLeap + 1 + 365 * (aLocal.tm_year - 100) + aLocal.tm_yday;
}
// TODO(recon)
Sexy::Color* GetFlashingColor(Sexy::Color* theResult, int theCounter, int thePeriod)
{
    int aHalf = thePeriod / 2;
    int aGray = 200 * abs(aHalf - theCounter % thePeriod) / aHalf + 55;
    if (aGray < 0)
        aGray = 0;
    if (aGray > 255)
        aGray = 255;
    theResult->mRed = aGray;
    theResult->mGreen = aGray;
    theResult->mBlue = aGray;
    theResult->mAlpha = 255;
    return theResult;
}
// TODO(recon)
float GetMatrixScale(const Sexy::SexyMatrix3& theMatrix)
{
    float aRow0 = theMatrix.m00 * theMatrix.m00 + theMatrix.m01 * theMatrix.m01;
    float aRow1 = theMatrix.m10 * theMatrix.m10 + theMatrix.m11 * theMatrix.m11;
    return sqrtf(aRow0 * aRow1);
}
int GetModeByRowColumn(int thePage, int theRow, int theCol)
{
    for (int aIndex = 0; aIndex < 94; ++aIndex)
    {
        const ChallengeDefinition* aDef = GetChallengeDefinition(aIndex);
        if (aDef->mPage == thePage && aDef->mCol == theCol && aDef->mRow == theRow)
            return aDef->mChallengeMode;
    }
    return -1;
}

PlantDefinition* GetPlantDefinition(SeedType theSeedType)
{
    return &gPlantDefs[theSeedType];
}
// TODO(recon)
int GetRectOverlap(const Sexy::TRect<int>& theFirst, const Sexy::TRect<int>& theSecond)
{
    int aLeft;
    int aRight;
    int aFar;
    if (theFirst.mX < theSecond.mX)
    {
        aLeft = theSecond.mX;
        aRight = theFirst.mX + theFirst.mWidth;
        aFar = theSecond.mX + theSecond.mWidth;
    }
    else
    {
        aLeft = theFirst.mX;
        aRight = theSecond.mX + theSecond.mWidth;
        aFar = theFirst.mX + theFirst.mWidth;
    }
    if (aRight <= aLeft || aRight <= aFar)
        return aRight - aLeft;
    return aFar - aLeft;
}
Sexy::String* GetSavedGameName(Sexy::String* theDest, GameMode theMode, int theProfileId, int theId)
{
    Sexy::String aFolder;
    GetAppDataFolder(&aFolder);
    Sexy::String aFile;
    StrFormat(&aFile, "userdata/game%d_%d_%d.dat", theProfileId, theId, (int)theMode);
    *theDest = aFolder;
    *theDest += aFile;
    return theDest;
}
// gZombieDefs @0x697660. GetZombieDefinition @0x1A7F60 is
// &gZombieDefs[theType], one 28-byte record. The reanimation numbers are
// the TV table's, not the PC enum.
static ZombieDefinition gZombieDefs[NUM_ZOMBIE_TYPES] = {
    { 0,  REANIM_ZOMBIE,             1,  1,  1,  4000, "ZOMBIE" },
    { 1,  REANIM_ZOMBIE,             1,  1,  1,     0, "FLAG_ZOMBIE" },
    { 2,  REANIM_ZOMBIE,             2,  3,  1,  4000, "CONEHEAD_ZOMBIE" },
    { 3,  REANIM_POLEVAULTER,        2,  6,  5,  2000, "POLE_VAULTING_ZOMBIE" },
    { 4,  REANIM_ZOMBIE,             4,  8,  1,  3000, "BUCKETHEAD_ZOMBIE" },
    { 5,  REANIM_ZOMBIE_NEWSPAPER,   2, 11,  1,  1000, "NEWSPAPER_ZOMBIE" },
    { 6,  REANIM_ZOMBIE,             4, 13,  5,  3500, "SCREEN_DOOR_ZOMBIE" },
    { 7,  REANIM_ZOMBIE_FOOTBALL,    7, 16,  5,  2000, "FOOTBALL_ZOMBIE" },
    { 8,  REANIM_DANCER,             5, 18,  5,  1000, "DANCING_ZOMBIE" },
    { 9,  REANIM_BACKUP_DANCER,      1, 18,  1,     0, "BACKUP_DANCER" },
    { 10, REANIM_ZOMBIE,             1, 21,  5,     0, "DUCKY_TUBE_ZOMBIE" },
    { 11, REANIM_SNORKEL,            3, 23, 10,  2000, "SNORKEL_ZOMBIE" },
    { 12, REANIM_ZOMBIE_ZAMBONI,     7, 26, 10,  2000, "ZOMBONI" },
    { 13, REANIM_BOBSLED,            3, 26, 10,  2000, "ZOMBIE_BOBSLED_TEAM" },
    { 14, REANIM_ZOMBIE_DOLPHINRIDER,3, 28, 10,  1500, "DOLPHIN_RIDER_ZOMBIE" },
    { 15, REANIM_JACKINTHEBOX,       3, 31, 10,  1000, "JACK_IN_THE_BOX_ZOMBIE" },
    { 16, REANIM_BALLOON,            2, 33, 10,  2000, "BALLOON_ZOMBIE" },
    { 17, REANIM_DIGGER,             4, 36, 10,  1000, "DIGGER_ZOMBIE" },
    { 18, REANIM_POGO,               4, 38, 10,  1000, "POGO_ZOMBIE" },
    { 19, REANIM_YETI,               4, 40,  1,     1, "ZOMBIE_YETI" },
    { 20, REANIM_BUNGEE,             3, 41, 10,  1000, "BUNGEE_ZOMBIE" },
    { 21, REANIM_LADDER,             4, 43, 10,  1000, "LADDER_ZOMBIE" },
    { 22, REANIM_CATAPULT,           5, 46, 10,  1500, "CATAPULT_ZOMBIE" },
    { 23, REANIM_GARGANTUAR,        10, 48, 15,  1500, "GARGANTUAR" },
    { 24, REANIM_IMP,               10, 48,  1,     0, "IMP" },
    { 25, REANIM_BOSS,              10, 50,  1,     0, "BOSS" },
    { 26, REANIM_ZOMBIE,             1, 99,  1,  4000, "TRASHCAN_ZOMBIE" },
    { 27, REANIM_ZOMBIE,             1, 99,  1,  4000, "ZOMBIE" },
    { 28, REANIM_ZOMBIE,             4, 99,  1,  3000, "ZOMBIE" },
    { 29, REANIM_ZOMBIE,             3, 99, 10,  1000, "ZOMBIE" },
    { 30, REANIM_ZOMBIE,             3, 99, 10,  2000, "ZOMBIE" },
    { 31, REANIM_ZOMBIE,             3, 99, 10,  2000, "ZOMBIE" },
    { 32, REANIM_ZOMBIE,             4, 99, 10,  2000, "ZOMBIE" },
    { 33, REANIM_GARGANTUAR,        10, 48, 15,  6000, "REDEYED_GARGANTUAR" },
};

ZombieDefinition* GetZombieDefinition(int theZombieType)
{
    if (theZombieType < 0 || theZombieType >= NUM_ZOMBIE_TYPES)
        return &gZombieDefs[0];
    return &gZombieDefs[theZombieType];
}
// TODO(recon)
unsigned int InterpColor(unsigned int theFirst, unsigned int theSecond, float theFraction)
{
    unsigned int aWeight = (unsigned int)(theFraction * 256.0f);
    unsigned int aRest = 256u - aWeight;
    unsigned int aGreen = ((theSecond & 0xFF00u) * aWeight + (theFirst & 0xFF00u) * aRest) >> 8;
    unsigned int aRedBlue = ((theSecond & 0xFF00FFu) * aWeight + (theFirst & 0xFF00FFu) * aRest) >> 8;
    return (theFirst & 0xFF000000u) | (aGreen & 0xFF00u) | (aRedBlue & 0xFF00FFu);
}
// TODO(recon)

// TODO(recon)
bool IsIdentityMatrix(const Sexy::SexyMatrix3& theMatrix)
{
    return theMatrix.m01 == 0.0f && theMatrix.m02 == 0.0f
        && theMatrix.m10 == 0.0f && theMatrix.m12 == 0.0f
        && theMatrix.m20 == 0.0f && theMatrix.m21 == 0.0f
        && theMatrix.m00 == 1.0f && theMatrix.m11 == 1.0f
        && theMatrix.m22 == 1.0f;
}

// TODO(recon)
bool LineSegmentIntersects(const Sexy::TPoint<float>& theStart, const Sexy::TPoint<float>& theEnd, const Sexy::TPoint<float>& theOtherStart, const Sexy::TPoint<float>& theOtherEnd, float* theFraction, Sexy::TPoint<float>* theHit)
{
    float aDeltaX = theEnd.mX - theStart.mX;
    float aDeltaY = theEnd.mY - theStart.mY;
    float aOtherX = theOtherEnd.mX - theOtherStart.mX;
    float aOtherY = theOtherEnd.mY - theOtherStart.mY;
    float aDenom = aDeltaX * aOtherY + (theStart.mY - theEnd.mY) * aOtherX;
    if (aDenom == 0.0f)
        return false;
    float aAlong = ((theStart.mY - theOtherStart.mY) * aOtherX + (theOtherStart.mX - theStart.mX) * aOtherY) / aDenom;
    if (aAlong > 1.0f || aAlong < 0.0f)
        return false;
    float aAcross = ((theStart.mY - theOtherStart.mY) * aDeltaX + (theEnd.mY - theStart.mY) * (theOtherStart.mX - theStart.mX)) / aDenom;
    if (aAcross > 1.0f || aAcross < 0.0f)
        return false;
    if (theFraction != nullptr)
        *theFraction = aAlong;
    if (theHit != nullptr)
    {
        theHit->mX = aAlong * (theEnd.mX - theStart.mX) + theStart.mX;
        theHit->mY = aAlong * (theEnd.mY - theStart.mY) + theStart.mY;
    }
    return true;
}
// NOT a variadic stub any more: Game/TodFoley.cpp:103 defines the real
// `FoleyParams *LookupFoley(FoleyType)` (@0x1FF778) and every call site passes a
// FoleyType, so the fixed overload wins overload resolution and the `return 0`
// body here was unreachable dead code. Deleted so the name cannot silently fall
// back to it.
// TODO(recon)
LawnStoneButton* MakeButton(int theId, Sexy::ButtonListener* theListener, Sexy::Widget*, const Sexy::String& theLabel)
{
    LawnStoneButton* aButton = new LawnStoneButton(theId, theListener);
    aButton->mDrawStoneButton = true;
    aButton->SetLabel(theLabel);
    aButton->mTranslateX = 1;
    aButton->mTranslateY = 1;
    aButton->mHasAlpha = true;
    aButton->mHasTransparencies = true;
    // The stone face and its glow extend past the button's hit rectangle.
    // Keep the parent dialog's clip, without clipping to the button itself.
    aButton->mClip = false;
    aButton->mHeight = 33;
    return aButton;
}
// TODO(recon)
int MakeMultiple(int theValue, int theStep)
{
    if (theValue <= 0)
        return 0;
    if (theStep > 0)
    {
        int aRemainder = theValue % theStep;
        if (aRemainder != 0)
            return theValue + theStep - aRemainder;
    }
    return theValue;
}
// TODO(recon)
static float gPlantWave[200];
static bool gPlantWaveReady = false;

static float PlantWaveAt(int thePhase, int theGridX, int theGridY)
{
    if (!gPlantWaveReady)
    {
        for (int aIndex = 0; aIndex != 200; ++aIndex)
        {
            float aAngle = (float)aIndex * 0.031416f;
            gPlantWave[aIndex] = sinf(aAngle) * 2.0f;
        }
        gPlantWaveReady = true;
    }
    int aSlot = (thePhase + 100 * theGridY + 25 * theGridX) % 200;
    if (aSlot < 0)
        aSlot += 200;
    return gPlantWave[aSlot];
}

float PlantDrawHeightOffset(Board* theBoard, Plant* thePlant, SeedType theSeed, int theGridX, int theGridY)
{
    float aHeight = 0.0f;
    bool aWaterPlant = theSeed == SEED_TANGLEKELP || theSeed == SEED_LILYPAD
        || theSeed == SEED_SEASHROOM || theSeed == SEED_CATTAIL;
    if (!Plant::IsFlying(theSeed))
    {
        if (theBoard != nullptr)
        {
            if (theBoard->IsPoolSquare(theGridX, theGridY))
                aHeight = PlantWaveAt(theBoard->mMainCounter, theGridX, theGridY);
        }
        else if (aWaterPlant)
        {
            aHeight = PlantWaveAt(gLawnApp->mAppCounter, theGridX, theGridY);
        }
    }

    Plant* aPot = nullptr;
    if (theBoard != nullptr && (thePlant == nullptr || !thePlant->mSquished))
        aPot = theBoard->GetFlowerPotAt(theGridX, theGridY);
    bool aOnPot = aPot != nullptr && !aPot->mSquished;
    if (theSeed == SEED_FLOWERPOT)
        return aHeight + 26.0f;
    if (aOnPot)
        aHeight += PlantFlowerPotHeightOffset(theSeed, 1.0f);

    switch (theSeed)
    {
    case SEED_LILYPAD: return aHeight + 25.0f;
    case SEED_STARFRUIT: return aHeight + 10.0f;
    case SEED_TANGLEKELP: return aHeight + 24.0f;
    case SEED_SEASHROOM: return aHeight + 28.0f;
    case SEED_INSTANT_COFFEE: return aHeight - 20.0f;
    default: break;
    }
    if (Plant::IsFlying(theSeed))
        return aHeight - 30.0f;
    switch (theSeed)
    {
    case SEED_CACTUS: return aHeight;
    case SEED_PUMPKINSHELL: return aHeight + 15.0f;
    case SEED_PUFFSHROOM: return aHeight + 5.0f;
    case SEED_SCAREDYSHROOM: return aHeight - 14.0f;
    case SEED_GRAVEBUSTER: return aHeight - 40.0f;
    default: break;
    }
    if (theSeed != SEED_SPIKEROCK && theSeed != SEED_SPIKEWEED)
        return aHeight;

    int aBottomRow = (theBoard != nullptr && theBoard->StageHas6Rows()) ? 5 : 4;
    if (theSeed == SEED_SPIKEROCK)
        aHeight += 6.0f;
    if (theBoard == nullptr)
        return aHeight + 15.0f;
    if (theBoard->GetFlowerPotAt(theGridX, theGridY) != nullptr && gLawnApp->mGameMode != 44)
        return aHeight + 5.0f;
    if (theBoard->StageHasRoof())
        return aHeight + 15.0f;
    if (theBoard->IsPoolSquare(theGridX, theGridY))
        return aHeight;
    if (theGridY == aBottomRow && theGridX > 6)
        return aHeight + 1.0f;
    if (theGridY == aBottomRow && theGridX <= 6)
        return aHeight + 12.0f;
    return aHeight + 15.0f;
}
// TODO(recon)
float PlantFlowerPotHeightOffset(SeedType theSeed, float theScale)
{
    unsigned int aSeed = (unsigned int)theSeed;
    float aBase = theScale * -5.0f;
    if (aSeed == 6u || aSeed == 25u)
        return aBase - 5.0f;
    if (aSeed == 13u)
        return (theScale * -8.0f + 8.0f) + (aBase + 5.0f);
    bool aShort = aSeed - 8u <= 1u || aSeed == 24u || aSeed == 4u;
    if (aShort)
        return (theScale * -4.0f + 4.0f) + aBase;
    bool aMedium = aSeed == 12u || aSeed - 31u <= 1u || aSeed <= 1u
        || (aSeed & 0xFFFFFFFDu) == 5u || aSeed - 18u <= 1u
        || aSeed - 38u <= 1u || aSeed == 27u || (aSeed & 0xFFFFFFDFu) == 0x15u;
    if (aMedium)
        return (theScale * -8.0f + 8.0f) + aBase;
    if (aSeed == 16u)
        return (theScale * -16.0f + 16.0f) + aBase;
    if (aSeed == 35u)
        return (theScale * -20.0f + 20.0f) + aBase;
    return aBase;
}
// TODO(recon)

// TODO(recon)
int PreLoadReanimForCredits(ReanimationType theType)
{
    ReanimatorEnsureDefinitionLoaded(theType, true);
    ReanimationPreload(theType);
    return 1;
}

bool QuickDigKeyCodeDown(int theKeyCode)
{
    return theKeyCode == 49;
}
// TODO(recon)
float RandRangeFloat(float theMin, float theMax)
{
    return Sexy::Rand(theMax - theMin) + theMin;
}
// TODO(recon)
int RandRangeInt(int theMin, int theMax)
{
    return Sexy::Rand(theMax - theMin + 1) + theMin;
}

// TODO(recon)

// TODO(recon)
float SexyMatrix3Inverse(const Sexy::SexyMatrix3& theMatrix, Sexy::SexyMatrix3& theInverse)
{
    float e00 = theMatrix.m00;
    float e01 = theMatrix.m01;
    float e02 = theMatrix.m02;
    float e10 = theMatrix.m10;
    float e11 = theMatrix.m11;
    float e12 = theMatrix.m12;
    float e20 = theMatrix.m20;
    float e21 = theMatrix.m21;
    float e22 = theMatrix.m22;
    float det = (e22 * e11 - e21 * e12) * e00
              + (e20 * e12 - e10 * e22) * e01
              + (e10 * e21 - e20 * e11) * e02;
    float invDet = 1.0f / det;

    Sexy::SexyMatrix3 aResult;
    aResult.m00 = invDet * (e22 * e11 - e21 * e12);
    aResult.m01 = invDet * (e02 * e21 - e01 * e22);
    aResult.m02 = invDet * (e01 * e12 - e02 * e11);
    aResult.m10 = invDet * (e20 * e12 - e10 * e22);
    aResult.m11 = invDet * (e00 * e22 - e20 * e02);
    aResult.m12 = invDet * (e10 * e02 - e00 * e12);
    aResult.m20 = invDet * (e10 * e21 - e20 * e11);
    aResult.m21 = invDet * (e20 * e01 - e00 * e21);
    aResult.m22 = invDet * (e00 * e11 - e10 * e01);
    theInverse = aResult;
    return aResult.m11;
}
// TODO(recon)
void SexyMatrix3Multiply(Sexy::SexyMatrix3* theResult, const Sexy::SexyMatrix3& theFirst, const Sexy::SexyMatrix3& theSecond)
{
    // TV @0x1fd004 stores the product in row order. DrawTrack calls it as
    // Multiply(&out, pose, out), and pose keeps translation in m02/m12
    // (MatrixFromTransform @0x1f9930). The stored element 0 is
    // left[0]*right[0] + left[4]*right[0xC] + left[8]*right[0x18], which is
    // left's row dotted with right's row. A column product of that storage
    // negates the sod roll and swaps a track's x with its y, so the dirt
    // ends up above the zombie head.
    Sexy::SexyMatrix3 aProduct;
    aProduct.m00 = theFirst.m00 * theSecond.m00 + theFirst.m01 * theSecond.m10 + theFirst.m02 * theSecond.m20;
    aProduct.m01 = theFirst.m00 * theSecond.m01 + theFirst.m01 * theSecond.m11 + theFirst.m02 * theSecond.m21;
    aProduct.m02 = theFirst.m00 * theSecond.m02 + theFirst.m01 * theSecond.m12 + theFirst.m02 * theSecond.m22;
    aProduct.m10 = theFirst.m10 * theSecond.m00 + theFirst.m11 * theSecond.m10 + theFirst.m12 * theSecond.m20;
    aProduct.m11 = theFirst.m10 * theSecond.m01 + theFirst.m11 * theSecond.m11 + theFirst.m12 * theSecond.m21;
    aProduct.m12 = theFirst.m10 * theSecond.m02 + theFirst.m11 * theSecond.m12 + theFirst.m12 * theSecond.m22;
    aProduct.m20 = theFirst.m20 * theSecond.m00 + theFirst.m21 * theSecond.m10 + theFirst.m22 * theSecond.m20;
    aProduct.m21 = theFirst.m20 * theSecond.m01 + theFirst.m21 * theSecond.m11 + theFirst.m22 * theSecond.m21;
    aProduct.m22 = theFirst.m20 * theSecond.m02 + theFirst.m21 * theSecond.m12 + theFirst.m22 * theSecond.m22;
    *theResult = aProduct;
}
// TODO(recon)
void SexyMatrix3Translation(Sexy::SexyMatrix3* theMatrix, float theX, float theY)
{
    theMatrix->m02 += theX;
    theMatrix->m12 += theY;
}
// NOT a variadic stub any more: Game/TodFoley.cpp defines all four
// `SoundSystem*(TodFoley *, ...)` helpers from the binary's own symbols
// (_Z23SoundSystemFindInstanceP8TodFoley9FoleyType @0x1FF810,
// _Z31SoundSystemGetFreeInstanceIndexP8TodFoley9FoleyType @0x1FF86C,
// _Z36SoundSystemHasFoleyPlayedTooRecentlyP8TodFoley9FoleyType @0x1FF82C,
// _Z35SoundSystemReleaseFinishedInstancesP8TodFoley @0x1FF794), and TodFoley.cpp
// is their only caller. The `return 0` bodies here were dead code.
// TODO(recon)

// TODO(recon)
Sexy::TPoint<float>* TransformFPoint(Sexy::TPoint<float>* theResult, const Sexy::SexyMatrix3& theMatrix, const Sexy::TPoint<float>& thePoint)
{
    // operator*(SexyVector2) @0x341b04. x' = x*m00 + y*m01 + m02.
    theResult->mX = thePoint.mX * theMatrix.m00 + thePoint.mY * theMatrix.m01 + theMatrix.m02;
    theResult->mY = thePoint.mX * theMatrix.m10 + thePoint.mY * theMatrix.m11 + theMatrix.m12;
    return theResult;
}
bool XButtonKeyCodeDown(int theKeyCode)
{
    return theKeyCode == 18 || theKeyCode == 50;
}
ZombiePicker* ZombiePickerInitForWave(ZombiePicker* thePicker)
{
    thePicker->mZombieCount = 0;
    thePicker->mZombiePoints = 0;
    for (int aIndex = 0; aIndex < NUM_ZOMBIE_TYPES; ++aIndex)
        thePicker->mZombieTypeCount[aIndex] = 0;
    return thePicker;
}

ZombiePicker* ZombiePickerInit(ZombiePicker* thePicker)
{
    ZombiePickerInitForWave(thePicker);
    for (int aIndex = 0; aIndex < NUM_ZOMBIE_TYPES; ++aIndex)
        thePicker->mAllWavesZombieTypeCount[aIndex] = 0;
    return thePicker;
}
// TODO(recon)
bool ZombieTypeCanGoInPool(ZombieType theType)
{
    unsigned int aType = (unsigned int)theType;
    if (aType <= 2u || aType == 4u || aType == 11u || aType == 16u)
        return true;
    unsigned int aOffset = aType - 27u;
    bool aInRange = aType == 14u || aOffset <= 2u;
    if (aInRange)
        return true;
    return aType == 30u || aType == 32u;
}
// TODO(recon)
bool ZombieTypeCanGoOnHighGround(ZombieType theType)
{
    return (unsigned int)theType - 12u > 1u;
}

// ---------------------------------------------------------------------------
// TodLib -- the engine layer. The `Tod` prefix is the framework's; the binary
// carries all of them and the call sites are spread over the whole game tree.
// PC TodCommon.cpp. Warp the time, then lerp. The bounce family holds the
// start once the time has passed 1; every other curve holds the end.
static float TodCurveQuad(float theTime)
{
    return theTime * theTime;
}

static float TodCurveInvQuad(float theTime)
{
    return 2.0f * theTime - theTime * theTime;
}

static float TodCurveS(float theTime)
{
    return 3.0f * theTime * theTime - 2.0f * theTime * theTime * theTime;
}

static float TodCurveInvQuadS(float theTime)
{
    if (theTime <= 0.5f)
        return TodCurveInvQuad(theTime * 2.0f) * 0.5f;
    return TodCurveQuad((theTime - 0.5f) * 2.0f) * 0.5f + 0.5f;
}

static float TodCurveBounce(float theTime)
{
    return 1.0f - fabsf(2.0f * theTime - 1.0f);
}

float TodCurveEvaluate(float theTime, float thePositionStart, float thePositionEnd, int theCurve)
{
    float aWarpedTime = 0.0f;
    switch ((TodCurves)theCurve)
    {
    case CURVE_CONSTANT:
        aWarpedTime = 0.0f;
        break;
    case CURVE_LINEAR:
        aWarpedTime = theTime;
        break;
    case CURVE_EASE_IN:
        aWarpedTime = TodCurveQuad(theTime);
        break;
    case CURVE_EASE_OUT:
        aWarpedTime = TodCurveInvQuad(theTime);
        break;
    case CURVE_EASE_IN_OUT:
        aWarpedTime = TodCurveS(TodCurveS(theTime));
        break;
    case CURVE_EASE_IN_OUT_WEAK:
        aWarpedTime = TodCurveS(theTime);
        break;
    case CURVE_FAST_IN_OUT:
        aWarpedTime = TodCurveInvQuadS(TodCurveInvQuadS(theTime));
        break;
    case CURVE_FAST_IN_OUT_WEAK:
        aWarpedTime = TodCurveInvQuadS(theTime);
        break;
    case CURVE_BOUNCE:
        aWarpedTime = TodCurveBounce(theTime);
        break;
    case CURVE_BOUNCE_FAST_MIDDLE:
        aWarpedTime = TodCurveQuad(TodCurveBounce(theTime));
        break;
    case CURVE_BOUNCE_SLOW_MIDDLE:
        aWarpedTime = TodCurveInvQuad(TodCurveBounce(theTime));
        break;
    case CURVE_SIN_WAVE:
        aWarpedTime = sinf(6.2831853f * theTime);
        break;
    case CURVE_EASE_SIN_WAVE:
        aWarpedTime = sinf(6.2831853f * TodCurveS(theTime));
        break;
    default:
        aWarpedTime = 0.0f;
        break;
    }
    return (thePositionEnd - thePositionStart) * aWarpedTime + thePositionStart;
}

float TodCurveEvaluateClamped(float theTime, float thePositionStart, float thePositionEnd, int theCurve)
{
    if (theTime <= 0.0f)
        return thePositionStart;
    if (theTime >= 1.0f)
    {
        TodCurves aKind = (TodCurves)theCurve;
        if (aKind == CURVE_BOUNCE
            || aKind == CURVE_BOUNCE_FAST_MIDDLE
            || aKind == CURVE_BOUNCE_SLOW_MIDDLE
            || aKind == CURVE_SIN_WAVE
            || aKind == CURVE_EASE_SIN_WAVE)
            return thePositionStart;
        return thePositionEnd;
    }
    return TodCurveEvaluate(theTime, thePositionStart, thePositionEnd, theCurve);
}

float TodAnimateCurveFloat(int theTimeStart, int theTimeEnd, int theTimeAge, float thePositionStart, float thePositionEnd, int theCurve)
{
    float aTime = (float)(theTimeAge - theTimeStart) / (float)(theTimeEnd - theTimeStart);
    return TodCurveEvaluateClamped(aTime, thePositionStart, thePositionEnd, theCurve);
}

int TodAnimateCurve(int theTimeStart, int theTimeEnd, int theTimeAge, int thePositionStart, float thePositionEnd, int theCurve)
{
    float aValue = TodAnimateCurveFloat(theTimeStart, theTimeEnd, theTimeAge, (float)thePositionStart, thePositionEnd, theCurve);
    if (aValue <= 0.0f)
        return (int)(aValue - 0.5f);
    return (int)(aValue + 0.5f);
}
float TodAnimateCurveFloatTime(float theTimeStart, float theTimeEnd, float theTimeAge, float thePositionStart, float thePositionEnd, int theCurve)
{
    float aTime = (theTimeAge - theTimeStart) / (theTimeEnd - theTimeStart);
    return TodCurveEvaluateClamped(aTime, thePositionStart, thePositionEnd, theCurve);
}

void TodAssertInitForApp(void (*theCallback)())
{
    (void)theCallback;
}
void TodBltMatrix(Sexy::Graphics* theGraphics, Sexy::Image* theImage, const Sexy::SexyMatrix3& theMatrix,
                  const Sexy::TRect<int>& theClipRect, const Sexy::Color& theColor, int theDrawMode,
                  const Sexy::TRect<int>& theSrcRect)
{
    float aPixelOffset = 0.0f;
    if (gSexyAppBase->Is3DAccelerated() != 0)
        aPixelOffset = -0.5f;
    else if (theDrawMode == Sexy::Graphics::DRAWMODE_ADDITIVE)
        gTodTriangleDrawAdditive = 1;

    TodSandImageIfNeeded(theImage);

    Sexy::TRect<int> aExpandedClip(0, 0, 801, 601);
    const Sexy::TRect<int>& aClip =
        (theClipRect.mX == 0 && theClipRect.mY == 0 && theClipRect.mWidth == 800 && theClipRect.mHeight == 600)
            ? aExpandedClip
            : theClipRect;
    // TV BltMatrix takes the source rect then the clip. PC takes the clip then
    // the source. Passing them in PC order makes the clip (often 800x600) the
    // source, so the quad is huge and the real source rect clips it away.
    if (theGraphics->mDestImage != nullptr)
        theGraphics->mDestImage->BltMatrix(theImage, aPixelOffset, aPixelOffset, theMatrix, theSrcRect, theColor,
                                           theDrawMode, aClip, theGraphics->mLinearBlend, true);
    gTodTriangleDrawAdditive = 0;
}

void TodCrash()
{
}
// TODO(recon)

static const Sexy::Color& DrawTint(Sexy::Graphics* theGraphics)
{
    if (theGraphics->mColorizeImages)
        return theGraphics->mColor;
    return Sexy::Color::White;
}

static int RoundHalfAway(float theValue)
{
    if (theValue <= 0.0f)
        return (int)(theValue - 0.5f);
    return (int)(theValue + 0.5f);
}

static void DrawWithScaleMatrix(Sexy::Graphics* theGraphics, Sexy::Image* theImage, float theCenterX, float theCenterY,
                                float theScaleX, float theScaleY, const Sexy::TRect<int>& theSrc)
{
    Sexy::SexyMatrix3 aMatrix;
    TodScaleTransformMatrix(&aMatrix, theCenterX, theCenterY, theScaleX, theScaleY);
    TodBltMatrix(theGraphics, theImage, aMatrix, theGraphics->mClipRect, DrawTint(theGraphics),
                 theGraphics->mDrawMode, theSrc);
}

void TodDrawImageCelCenterScaledF(Sexy::Graphics* theGraphics, Sexy::Image* theImage, float theX, float theY,
                                  int theCelCol, float theScaleX, float theScaleY)
{
    int aCelWidth = theImage->GetCelWidth();
    int aCelHeight = theImage->GetCelHeight();
    Sexy::TRect<int> aSrc(theCelCol * aCelWidth, 0, aCelWidth, aCelHeight);
    float aCenterX = (theX + theGraphics->mTransX) + (float)aCelWidth * 0.5f;
    float aCenterY = (theY + theGraphics->mTransY) + (float)aCelHeight * 0.5f;
    DrawWithScaleMatrix(theGraphics, theImage, aCenterX, aCenterY, theScaleX, theScaleY, aSrc);
}

void TodDrawImageCelF(Sexy::Graphics* theGraphics, Sexy::Image* theImage, float theX, float theY,
                      int theCelCol, int theCelRow)
{
    int aCelWidth = theImage->GetCelWidth();
    int aCelHeight = theImage->GetCelHeight();
    Sexy::TRect<int> aSrc(theCelCol * aCelWidth, theCelRow * aCelHeight, aCelWidth, aCelHeight);
    theGraphics->DrawImageF(theImage, theX, theY, aSrc);
}

void TodDrawImageCelScaled(Sexy::Graphics* theGraphics, Sexy::Image* theImage, int theX, int theY,
                           int theCelCol, int theCelRow, float theScaleX, float theScaleY)
{
    int aCelWidth = theImage->GetCelWidth();
    int aCelHeight = theImage->GetCelHeight();
    Sexy::TRect<int> aSrc(theCelCol * aCelWidth, theCelRow * aCelHeight, aCelWidth, aCelHeight);
    Sexy::TRect<int> aDest(theX, theY, RoundHalfAway((float)aCelWidth * theScaleX),
                           RoundHalfAway((float)aCelHeight * theScaleY));
    theGraphics->DrawImage(theImage, aDest, aSrc);
}

void TodDrawImageCelScaledF(Sexy::Graphics* theGraphics, Sexy::Image* theImage, float theX, float theY,
                            int theCelCol, int theCelRow, float theScaleX, float theScaleY)
{
    int aCelWidth = theImage->GetCelWidth();
    int aCelHeight = theImage->GetCelHeight();
    Sexy::TRect<int> aSrc(theCelCol * aCelWidth, theCelRow * aCelHeight, aCelWidth, aCelHeight);
    float aCenterX = (theX + theGraphics->mTransX) + (theScaleX * 0.5f) * (float)aCelWidth;
    float aCenterY = (theY + theGraphics->mTransY) + (theScaleY * 0.5f) * (float)aCelHeight;
    DrawWithScaleMatrix(theGraphics, theImage, aCenterX, aCenterY, theScaleX, theScaleY, aSrc);
}

void TodDrawImageCenterScaledF(Sexy::Graphics* theGraphics, Sexy::Image* theImage, float theX, float theY,
                               float theScaleX, float theScaleY)
{
    Sexy::TRect<int> aSrc(0, 0, theImage->mWidth, theImage->mHeight);
    float aCenterX = (theGraphics->mTransX + (float)theImage->mWidth * 0.5f) + theX;
    float aCenterY = (theGraphics->mTransY + (float)theImage->mHeight * 0.5f) + theY;
    DrawWithScaleMatrix(theGraphics, theImage, aCenterX, aCenterY, theScaleX, theScaleY, aSrc);
}

void TodDrawImageScaledF(Sexy::Graphics* theGraphics, Sexy::Image* theImage, float theX, float theY,
                         float theScaleX, float theScaleY)
{
    Sexy::TRect<int> aSrc(0, 0, theImage->mWidth, theImage->mHeight);
    float aCenterX = (theX + theGraphics->mTransX) + ((float)theImage->mWidth * 0.5f) * theScaleX;
    float aCenterY = (theY + theGraphics->mTransY) + ((float)theImage->mHeight * 0.5f) * theScaleY;
    DrawWithScaleMatrix(theGraphics, theImage, aCenterX, aCenterY, theScaleX, theScaleY, aSrc);
}

void TodErrorMessageBox(const char* theMessage, const char* theTitle)
{
    (void)theMessage;
    (void)theTitle;
}

// TODO(recon)
FoleyParams* TodFoleyInitialize(FoleyParams* theParams, int theCount)
{
    gFoleyParamArray = theParams;
    gFoleyParamArraySize = theCount;
    return theParams;
}

static int TodSnprintf(char* theBuffer, int theSize, const char* theFormat, ...)
{
    va_list aArgs;
    va_start(aArgs, theFormat);
    int aWritten = TodVsnprintf(theBuffer, theSize, theFormat, aArgs);
    va_end(aArgs);
    return aWritten;
}

static int TodVsnprintfEnsureNewLine(char* theBuffer, int theSize, const char* theFormat, va_list theArgs)
{
    int aWritten = TodVsnprintf(theBuffer, theSize, theFormat, theArgs);
    if (aWritten > 0 && theBuffer[aWritten - 1] != '\n')
    {
        if (theSize > aWritten + 1)
        {
            theBuffer[aWritten] = '\n';
            theBuffer[aWritten + 1] = 0;
        }
        else
        {
            theBuffer[aWritten - 1] = '\n';
        }
    }
    return aWritten;
}

static int HesitationMilliseconds()
{
    __int64 aBits = gHesitationTrace.mTimer.GetDuration();
    double aMilliseconds;
    std::memcpy(&aMilliseconds, &aBits, sizeof(aMilliseconds));
    return (int)aMilliseconds;
}

void TodHesitationStartBuffer()
{
    gHesitationTrace.mTimer.Start();
    gHesitationTrace.mLastMs = 0;
    gHesitationTrace.mOffset = 0;
    gHesitationTrace.mCaptured = false;
    gHesitationTrace.mText[0] = 0;
    gHesitationTrace.mDepth = 1;
    gHesitationTrace.mEnabled = true;
    TodHesitationTrace("****** start *****");
}

void TodHesitationEndBuffer()
{
    if (!gHesitationTrace.mEnabled)
        return;
    gHesitationTrace.mEnabled = false;
    int aMilliseconds = HesitationMilliseconds();
    if (aMilliseconds > 40)
        TodHesitationTrace("****** captured %d ms hesitation***** \n\n", aMilliseconds);
}

int TodHesitationTrace(const char* theFormat, ...)
{
    if (!gHesitationTrace.mEnabled)
        return 0;
    int aRemaining = 0x40000 - gHesitationTrace.mOffset;
    if (aRemaining == 0)
        return 0;

    char aLine[256];
    va_list aArgs;
    va_start(aArgs, theFormat);
    TodVsnprintfEnsureNewLine(aLine, 256, theFormat, aArgs);
    va_end(aArgs);

    int aMilliseconds = HesitationMilliseconds();
    int aDelta = aMilliseconds - gHesitationTrace.mLastMs;
    gHesitationTrace.mLastMs = aMilliseconds;
    char* aDest = gHesitationTrace.mText + gHesitationTrace.mOffset;
    int aWritten;
    if (aDelta <= 1)
    {
        aWritten = TodSnprintf(aDest, aRemaining, "%04d ----%*s%s",
                               aMilliseconds, gHesitationTrace.mDepth, " ", aLine);
    }
    else
    {
        aWritten = TodSnprintf(aDest, aRemaining, "%04d %04d%*s%s",
                               aMilliseconds, aDelta, gHesitationTrace.mDepth, " ", aLine);
    }
    gHesitationTrace.mOffset += aWritten;
    return aWritten;
}
// TODO(recon)
int TodLoadNextResource()
{
    if (gSexyAppBase == nullptr || gSexyAppBase->mResourceManager == nullptr)
        return 0;
    return gSexyAppBase->mResourceManager->LoadNextResource();
}
// NOT a variadic stub any more: the real signature is known.
//
// The binary's symbol is _Z16TodLoadResourcesRKSs -- `int TodLoadResources(
// Sexy::String const&)`, one argument holding the resource group name. The
// definition lives in ReconHelpers.cpp with the rest of the hand-written
// framework helpers, because it is a real implementation rather than a
// placeholder.
//
// Left here as a marker so the name is not re-added to the stub list by a
// future regeneration: see ReconFreeFunctions.h for the declaration.
//
// The body it replaces was `{ return 0; }`, which is why no resource group ever
// loaded and the title screen had nothing to draw with.
// TODO(recon)

// TODO(recon)
TodWeightedArray* TodPickArrayItemFromWeightedArray(TodWeightedArray* theItems, int theCount)
{
    // _Z33TodPickArrayItemFromWeightedArrayP16TodWeightedArrayi. A non-positive
    // count tail-calls Sexy::Rand(0) and returns that integer; there is no
    // record. Callers that dereference the result only do so after a positive
    // count, so the empty case returns null after the same draw.
    if (theCount <= 0)
    {
        Sexy::Rand(0);
        return nullptr;
    }
    int aTotal = 0;
    for (int aIndex = 0; aIndex != theCount; ++aIndex)
        aTotal += theItems[aIndex].mWeight;
    int aRoll = Sexy::Rand(aTotal);
    int aRunning = theItems[0].mWeight;
    if (aRoll < aRunning)
        return &theItems[0];
    for (int aIndex = 1; aIndex != theCount; ++aIndex)
    {
        aRunning += theItems[aIndex].mWeight;
        if (aRoll < aRunning)
            return &theItems[aIndex];
    }
    return &theItems[theCount - 1];
}
// TODO(recon)
int TodPickFromArray(const int* theItems, int theCount)
{
    return theItems[Sexy::Rand(theCount)];
}
float TodCalcSmoothWeight(float theWeight, float theLastPicked, float theSecondLastPicked)
{
    if (theWeight < 0.000001f)
        return 0.0f;

    float aInverse = 1.0f / theWeight;
    float aSecondSpan = aInverse + aInverse;
    float aSecondFactor = ((theSecondLastPicked + 1.0f) - aSecondSpan) / aSecondSpan;
    float aLastFactor = ((theLastPicked + 1.0f) - aInverse) / aInverse;
    float aSmooth = (aSecondFactor + aSecondFactor) * 0.25f + 0.25f
                  + (aLastFactor + aLastFactor) * 0.75f + 0.75f;
    if (aSmooth > 100.0f)
        aSmooth = 100.0f;
    if (aSmooth < 0.01f)
        aSmooth = 0.01f;
    return aSmooth * theWeight;
}

int TodPickFromSmoothArray(TodSmoothArray* theItems, int theCount)
{
    float aScale = 0.0f;
    float aTotal = 0.0f;
    if (theCount > 0)
    {
        float aWeightSum = 0.0f;
        for (int aIndex = 0; aIndex != theCount; ++aIndex)
            aWeightSum += theItems[aIndex].mWeight;
        aScale = 1.0f / aWeightSum;
        for (int aIndex = 0; aIndex != theCount; ++aIndex)
        {
            aTotal += TodCalcSmoothWeight(aScale * theItems[aIndex].mWeight,
                                          theItems[aIndex].mLastPicked,
                                          theItems[aIndex].mSecondLastPicked);
        }
    }

    float aRoll = Sexy::Rand(aTotal);
    int aPick = 0;
    if (theCount > 1)
    {
        float aRunning = 0.0f;
        while (aPick != theCount - 1)
        {
            aRunning += TodCalcSmoothWeight(aScale * theItems[aPick].mWeight,
                                            theItems[aPick].mLastPicked,
                                            theItems[aPick].mSecondLastPicked);
            if (aRoll <= aRunning)
                break;
            ++aPick;
        }
    }
    TodUpdateSmoothArrayPick(theItems, theCount, aPick);
    return theItems[aPick].mItem;
}
// TODO(recon)
int TodPickFromWeightedArray(TodWeightedArray* theItems, int theCount)
{
    // _Z24TodPickFromWeightedArrayP16TodWeightedArrayi loads the item word of
    // the record the pointer picker returns. A non-positive count is the same
    // Sexy::Rand(0) the pointer picker tail-calls, returned as the item.
    if (theCount <= 0)
        return Sexy::Rand(0);
    return TodPickArrayItemFromWeightedArray(theItems, theCount)->mItem;
}
TodWeightedGridArray* TodPickFromWeightedGridArray(TodWeightedGridArray* theItems, int theCount)
{
    if (theCount <= 0)
    {
        Sexy::Rand(theCount);
        return nullptr;
    }

    int aTotal = 0;
    for (int aIndex = 0; aIndex != theCount; ++aIndex)
        aTotal += theItems[aIndex].mWeight;
    int aRoll = Sexy::Rand(aTotal);
    int aRunning = theItems[0].mWeight;
    if (aRoll < aRunning)
        return &theItems[0];
    for (int aIndex = 1; aIndex != theCount; ++aIndex)
    {
        aRunning += theItems[aIndex].mWeight;
        if (aRoll < aRunning)
            return &theItems[aIndex];
    }
    return &theItems[theCount - 1];
}
// TodReplaceButtons @0x1FD454. A connected pad, or both pad-on bytes, leaves
// the tags for the icon drawer. Otherwise <A> becomes [OK_BUTTON], <B>
// becomes [BACK_BUTTON], and <C> and <X> become [MENU_BUTTON].
Sexy::String* TodReplaceButtons(Sexy::String* theResult, const Sexy::String* theSource)
{
    Sexy::GamepadApp* aApp = static_cast<Sexy::GamepadApp*>(gSexyAppBase);
    if (aApp != nullptr && (aApp->HasGamepad() != 0 || (aApp->mGamePad1IsOn && aApp->mGamePad2IsOn)))
    {
        *theResult = *theSource;
        return theResult;
    }
    if (std::strchr(theSource->c_str(), '<') == nullptr)
    {
        *theResult = *theSource;
        return theResult;
    }

    Sexy::String aOk;
    TodStringTranslate(&aOk, "[OK_BUTTON]");
    TodReplaceString(theResult, *theSource, "<A>", aOk);

    Sexy::String aBack;
    TodStringTranslate(&aBack, "[BACK_BUTTON]");
    Sexy::String aAfterBack;
    TodReplaceString(&aAfterBack, *theResult, "<B>", aBack);
    *theResult = aAfterBack;

    Sexy::String aMenu;
    TodStringTranslate(&aMenu, "[MENU_BUTTON]");
    Sexy::String aAfterMenu;
    TodReplaceString(&aAfterMenu, *theResult, "<C>", aMenu);
    *theResult = aAfterMenu;

    Sexy::String aAfterX;
    TodReplaceString(&aAfterX, *theResult, "<X>", aMenu);
    *theResult = aAfterX;
    return theResult;
}
// TODO(recon)
Sexy::String* TodReplaceNumberString(Sexy::String* theResult, const Sexy::String* theFormat, const char* thePlaceholder, int theNumber)
{
    *theResult = *theFormat;
    if (thePlaceholder == nullptr)
        return theResult;
    const char* aText = theResult->c_str();
    const char* aFound = std::strstr(aText, thePlaceholder);
    if (aFound == nullptr)
        return theResult;
    unsigned int aAt = (unsigned int)(aFound - aText);
    char aNumber[16];
    snprintf(aNumber, sizeof(aNumber), "%d", theNumber);
    Sexy::String aHead = theResult->substr(0, aAt);
    Sexy::String aTail = theResult->substr(aAt + (unsigned int)std::strlen(thePlaceholder), theResult->size());
    aHead.Append(aNumber, (unsigned int)std::strlen(aNumber));
    aHead.Append(aTail.c_str(), aTail.size());
    *theResult = aHead;
    return theResult;
}
// TODO(recon)
Sexy::String* TodReplaceString(Sexy::String* theResult, const Sexy::String& theFormat, const char* thePlaceholder, const Sexy::String& theReplacement)
{
    TodStringTranslate(theResult, theFormat);
    if (thePlaceholder == nullptr || thePlaceholder[0] == '\0')
        return theResult;
    Sexy::String aTranslated;
    TodStringTranslate(&aTranslated, theReplacement);
    unsigned int aPlaceLen = (unsigned int)std::strlen(thePlaceholder);
    for (;;)
    {
        const char* aText = theResult->c_str();
        const char* aFound = std::strstr(aText, thePlaceholder);
        if (aFound == nullptr)
            break;
        unsigned int aAt = (unsigned int)(aFound - aText);
        Sexy::String aHead = theResult->substr(0, aAt);
        Sexy::String aTail = theResult->substr(aAt + aPlaceLen);
        aHead.Append(aTranslated.c_str(), aTranslated.size());
        aHead.Append(aTail.c_str(), aTail.size());
        *theResult = aHead;
    }
    return theResult;
}

Sexy::String* TodReplaceString(Sexy::String* theResult, const Sexy::String* theFormat, const char* thePlaceholder, const Sexy::String* theReplacement)
{
    Sexy::String aFormat = (theFormat != nullptr) ? *theFormat : Sexy::String();
    Sexy::String aReplacement = (theReplacement != nullptr) ? *theReplacement : Sexy::String();
    return TodReplaceString(theResult, aFormat, thePlaceholder, aReplacement);
}

Sexy::String* TodReplaceString(Sexy::String* theResult, const char* theFormat, const char* thePlaceholder, const Sexy::String* theReplacement)
{
    Sexy::String aFormat(theFormat != nullptr ? theFormat : "");
    Sexy::String aReplacement = (theReplacement != nullptr) ? *theReplacement : Sexy::String();
    return TodReplaceString(theResult, aFormat, thePlaceholder, aReplacement);
}
void TodSandImageIfNeeded(Sexy::Image*)
{
}
// TODO(recon)
void TodScaleRotateTransformMatrix(Sexy::SexyMatrix3& theMatrix, float theX, float theY, float theAngle, float theScaleX, float theScaleY)
{
    float aSin = sinf(theAngle);
    float aCos = cosf(theAngle);
    theMatrix.m00 = aCos * theScaleX;
    theMatrix.m10 = -aSin * theScaleX;
    theMatrix.m20 = 0.0f;
    theMatrix.m01 = aSin * theScaleY;
    theMatrix.m11 = aCos * theScaleY;
    theMatrix.m21 = 0.0f;
    theMatrix.m02 = theX;
    theMatrix.m12 = theY;
    theMatrix.m22 = 1.0f;
}

Sexy::SexyMatrix3* TodScaleTransformMatrix(Sexy::SexyMatrix3* theMatrix, float theX, float theY, float theScaleX, float theScaleY)
{
    theMatrix->m00 = theScaleX;
    theMatrix->m01 = 0.0f;
    theMatrix->m02 = theX;
    theMatrix->m10 = 0.0f;
    theMatrix->m11 = theScaleY;
    theMatrix->m12 = theY;
    theMatrix->m20 = 0.0f;
    theMatrix->m21 = 0.0f;
    theMatrix->m22 = 1.0f;
    return theMatrix;
}

static bool StringPropertyFound(const Sexy::String& theKey)
{
    if (gSexyAppBase == nullptr)
        return false;
    return gSexyAppBase->mStringProperties.find(theKey) != gSexyAppBase->mStringProperties.end();
}

bool TodStringListExists(const Sexy::String& theKey)
{
    if (theKey.size() <= 2 || theKey.c_str()[0] != '[')
        return false;
    return StringPropertyFound(theKey.substr(1, theKey.size() - 2));
}
static Sexy::String TrimStringListText(const Sexy::String& theText)
{
    unsigned int aStart = 0;
    unsigned int anEnd = theText.size();
    while (aStart < anEnd && isspace((unsigned char)theText[aStart]) != 0)
        ++aStart;
    while (anEnd > aStart && isspace((unsigned char)theText[anEnd - 1]) != 0)
        --anEnd;
    return theText.substr(aStart, anEnd - aStart);
}

static void TodStringRemoveReturnChars(Sexy::String& theText)
{
    const char* aData = theText.c_str();
    unsigned int aLength = theText.size();
    bool aHasReturn = false;
    for (unsigned int aIndex = 0; aIndex < aLength; ++aIndex)
    {
        if (aData[aIndex] == '\r')
        {
            aHasReturn = true;
            break;
        }
    }
    if (!aHasReturn)
        return;

    Sexy::String aClean;
    for (unsigned int aIndex = 0; aIndex < aLength; ++aIndex)
    {
        if (aData[aIndex] != '\r')
            aClean.push_back(aData[aIndex]);
    }
    theText = aClean;
}

// TodStringListReadName @0x1FFD18. A bracketed name advances the cursor by
// the raw name length plus 2, which steps past "[name]" when the cursor is
// already on the '['. Whitespace with no bracket is an empty name and success.
static bool TodStringListReadName(const char** theCursor, Sexy::String* theName)
{
    const char* aText = *theCursor;
    const char* aOpen = strchr(aText, '[');
    if (aOpen != nullptr)
    {
        const char* aNameStart = aOpen + 1;
        const char* aClose = strchr(aNameStart, ']');
        if (aClose != nullptr)
        {
            unsigned int aLength = (unsigned int)(aClose - aNameStart);
            *theName = TrimStringListText(Sexy::String(aNameStart, aLength));
            if (!theName->empty())
            {
                *theCursor = aText + aLength + 2;
                return true;
            }
        }
        return false;
    }

    size_t aSpaces = strspn(aText, " \n\r\t");
    if (aSpaces == strlen(aText))
    {
        theName->clear();
        return true;
    }
    return false;
}

// TodStringListReadValue @0x1FFE50. The value runs up to the next '[' or the
// end of the buffer, then the cursor sits on that boundary.
static bool TodStringListReadValue(const char** theCursor, Sexy::String* theValue)
{
    const char* aText = *theCursor;
    const char* aNext = strchr(aText, '[');
    unsigned int aLength = (aNext != nullptr)
        ? (unsigned int)(aNext - aText)
        : (unsigned int)strlen(aText);
    *theValue = TrimStringListText(Sexy::String(aText, aLength));
    TodStringRemoveReturnChars(*theValue);
    *theCursor = aText + aLength;
    return true;
}

// TodStringListReadItems @0x1FFEF4. Each [name]value pair is registered on
// the app. An empty name, or a name that cannot be read, ends the walk and
// is the function's result.
static int TodStringListReadItems(const char* theText)
{
    const char* aCursor = theText;
    for (;;)
    {
        Sexy::String aName;
        bool aReadName = TodStringListReadName(&aCursor, &aName);
        if (!aReadName || aName.empty())
            return aReadName ? 1 : 0;
        Sexy::String aValue;
        TodStringListReadValue(&aCursor, &aValue);
        gSexyAppBase->SetString(aName, aValue);
    }
}

// TodStringListReadFile @0x2000EC. A missing file is 0. An empty file still
// counts as one successful read, matching the .so's size<=0 path.
static int TodStringListReadFile(const char* thePath)
{
    FILE* aFile = p_fopen(thePath, "r");
    if (aFile == nullptr)
        return 0;

    p_fseek(aFile, 0, SEEK_END);
    long aSize = p_ftell(aFile);
    p_fseek(aFile, 0, SEEK_SET);
    char* aBuffer = new char[(aSize > 0) ? (aSize + 1) : 1];
    int aItems;
    if (aSize > 0)
    {
        int aRead = (int)p_fread(aBuffer, 1, (size_t)aSize, aFile);
        if (aRead <= 0)
        {
            aItems = 0;
            aBuffer[0] = 0;
        }
        else
        {
            aBuffer[aRead] = 0;
            aItems = TodStringListReadItems(aBuffer);
        }
    }
    else
    {
        aItems = 1;
    }
    p_fclose(aFile);
    delete[] aBuffer;
    return aItems;
}

int TodStringListLoad(const char* theFileName)
{
    int aItems = TodStringListReadFile(theFileName);
    if (aItems == 0)
    {
        Sexy::String aMessage;
        StrFormat(&aMessage, "Failed to load string list file '%s'", theFileName);
        TodErrorMessageBox(aMessage.c_str(), "Error");
    }
    return aItems;
}
// TODO(recon)
TodStringListFormat* TodStringListSetColors(TodStringListFormat* theFormats, int theCount)
{
    gTodStringFormats = theFormats;
    gTodStringFormatCount = theCount;
    return theFormats;
}
// A bracketed key is looked up in the app's string table. A miss is the
// so's "<Missing %s>" text. GetString already walks that table and narrows
// the stored wide value; an empty result is a miss.
static Sexy::String TodStringListFind(const Sexy::String& theKey)
{
    if (!StringPropertyFound(theKey))
    {
        Sexy::String aMissing;
        StrFormat(&aMissing, "<Missing %s>", theKey.c_str());
        return aMissing;
    }
    return gSexyAppBase->GetString(theKey);
}

Sexy::String TodStringTranslate(const char* theKey)
{
    if (theKey == nullptr)
        return Sexy::String();
    unsigned int aLength = (unsigned int)std::strlen(theKey);
    if (aLength > 2 && theKey[0] == '[')
        return TodStringListFind(Sexy::String(theKey + 1, aLength - 2));
    return Sexy::String(theKey, aLength);
}

Sexy::String TodStringTranslate(const Sexy::String& theKey)
{
    if (theKey.size() > 2 && theKey.c_str()[0] == '[')
        return TodStringListFind(theKey.substr(1, theKey.size() - 2));
    return theKey;
}

Sexy::String* TodStringTranslate(Sexy::String* theResult, const char* theKey)
{
    *theResult = TodStringTranslate(theKey);
    return theResult;
}

Sexy::String* TodStringTranslate(Sexy::String* theResult, const Sexy::String& theKey)
{
    *theResult = TodStringTranslate(theKey);
    return theResult;
}

Sexy::String* TodStringTranslate(Sexy::String* theResult, const Sexy::String* theKey)
{
    if (theKey == nullptr)
        return TodStringTranslate(theResult, (const char*)nullptr);
    return TodStringTranslate(theResult, *theKey);
}
void TodUpdateSmoothArrayPick(TodSmoothArray* theItems, int theCount, int thePickIndex)
{
    if (theCount > 0)
    {
        for (int aIndex = 0; aIndex != theCount; ++aIndex)
        {
            if (theItems[aIndex].mWeight > 0.0f)
            {
                theItems[aIndex].mLastPicked += 1.0f;
                theItems[aIndex].mSecondLastPicked += 1.0f;
            }
        }
    }
    float aLastPicked = theItems[thePickIndex].mLastPicked;
    theItems[thePickIndex].mLastPicked = 0.0f;
    theItems[thePickIndex].mSecondLastPicked = aLastPicked;
}
// TODO(recon)
int TodVsnprintf(char* theBuffer, int theSize, const char* theFormat, va_list theArgs)
{
    if (theSize <= 0)
        return 0;
    int aWritten = vsnprintf(theBuffer, (size_t)theSize, theFormat, theArgs);
    if (aWritten < 0 || aWritten >= theSize)
    {
        theBuffer[theSize - 1] = 0;
        return theSize - 1;
    }
    return aWritten;
}
// TODO(recon)
void Tod_SWTri_AddAllDrawTriFuncs()
{
}

void SexyDumpUnfreed()
{
}

// ---------------------------------------------------------------------------
// Trail / MD5 / byte order / misc.
// ---------------------------------------------------------------------------
// TODO(recon)


// EndianDouble / EndianShort / EndianUINT16 / EndianUINT32 are real BX LR
// functions, defined next to EndianInt in ReconStubs.cpp. EndianFloat is still
// a stub: its call sites have not been rewritten against the binary yet.
// TODO(recon)
float EndianFloat(float theValue)
{
    // _Z11EndianFloatf is a single BX LR. This build is little-endian, so the
    // float already has the order the callers store.
    return theValue;
}

// MD5Init/MD5Update/MD5Final are the classic RSA reference routines, called
// from Sexy/ImageFont.cpp with a state pointer, a buffer and a length.
// TODO(recon)
static unsigned int Md5Rotate(unsigned int theValue, int theBits)
{
    return (theValue << theBits) | (theValue >> (32 - theBits));
}

static void MD5Transform(unsigned int theState[4], const unsigned char* theBlock)
{
    // RFC 1321. MD5Init plants the same four words, and MD5Update/MD5Final
    // feed this 64-byte blocks with the bit count appended at bytes 56..63.
    static const unsigned int kConstants[64] = {
        0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
        0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
        0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
        0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
        0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
        0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
        0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
        0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
    };
    static const int kShifts[64] = {
        7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
        5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
    };
    unsigned int aWords[16];
    std::memcpy(aWords, theBlock, 64);
    unsigned int a = theState[0];
    unsigned int b = theState[1];
    unsigned int c = theState[2];
    unsigned int d = theState[3];
    for (int aStep = 0; aStep != 64; ++aStep)
    {
        unsigned int aMixed;
        int aIndex;
        if (aStep < 16)
        {
            aMixed = (b & c) | (~b & d);
            aIndex = aStep;
        }
        else if (aStep < 32)
        {
            aMixed = (d & b) | (~d & c);
            aIndex = (5 * aStep + 1) & 15;
        }
        else if (aStep < 48)
        {
            aMixed = b ^ c ^ d;
            aIndex = (3 * aStep + 5) & 15;
        }
        else
        {
            aMixed = c ^ (b | ~d);
            aIndex = (7 * aStep) & 15;
        }
        unsigned int aNext = b + Md5Rotate(a + aMixed + aWords[aIndex] + kConstants[aStep], kShifts[aStep]);
        a = d;
        d = c;
        c = b;
        b = aNext;
    }
    theState[0] += a;
    theState[1] += b;
    theState[2] += c;
    theState[3] += d;
}

void MD5Init(MD5Context* theContext)
{
    theContext->mState[0] = 0x67452301u;
    theContext->mState[1] = 0xefcdab89u;
    theContext->mState[2] = 0x98badcfeu;
    theContext->mState[3] = 0x10325476u;
    theContext->mCount[0] = 0;
    theContext->mCount[1] = 0;
}

void MD5Update(MD5Context* theContext, const unsigned char* theData, unsigned int theLength)
{
    unsigned int aCountLow = theContext->mCount[0];
    unsigned int aAddedBits = theLength << 3;
    theContext->mCount[0] = aCountLow + aAddedBits;
    if (aCountLow > theContext->mCount[0])
        ++theContext->mCount[1];
    theContext->mCount[1] += theLength >> 29;

    unsigned int aOffset = (aCountLow >> 3) & 0x3Fu;
    unsigned int aRemaining = theLength;
    const unsigned char* aCursor = theData;
    if (aOffset != 0)
    {
        unsigned int aSpace = 64 - aOffset;
        if (aRemaining < aSpace)
        {
            std::memcpy(theContext->mBuffer + aOffset, aCursor, aRemaining);
            return;
        }
        std::memcpy(theContext->mBuffer + aOffset, aCursor, aSpace);
        MD5Transform(theContext->mState, theContext->mBuffer);
        aCursor += aSpace;
        aRemaining -= aSpace;
    }
    while (aRemaining > 63)
    {
        std::memcpy(theContext->mBuffer, aCursor, 64);
        MD5Transform(theContext->mState, theContext->mBuffer);
        aCursor += 64;
        aRemaining -= 64;
    }
    std::memcpy(theContext->mBuffer, aCursor, aRemaining);
}

void MD5Final(unsigned char* theDigest, MD5Context* theContext)
{
    unsigned int aOffset = (theContext->mCount[0] >> 3) & 0x3Fu;
    theContext->mBuffer[aOffset] = 0x80;
    if (aOffset > 55)
    {
        std::memset(theContext->mBuffer + aOffset + 1, 0, 63 - aOffset);
        MD5Transform(theContext->mState, theContext->mBuffer);
        std::memset(theContext->mBuffer, 0, 56);
    }
    else
    {
        std::memset(theContext->mBuffer + aOffset + 1, 0, 55 - aOffset);
    }
    std::memcpy(theContext->mBuffer + 56, theContext->mCount, 8);
    MD5Transform(theContext->mState, theContext->mBuffer);
    std::memcpy(theDigest, theContext->mState, 16);
    theContext->mState[0] = 0;
}

// ---------------------------------------------------------------------------
// Sleep / logging.
//
// MSleep is NOT a variadic stub any more: the real function is
// `int MSleep(int theMillis)` in Sexy/ReconHelpers.cpp:580, declared for the
// whole tree by Runtime/AndroidShims.h:380 (which ReconTypes.h includes), so
// every call site already bound to it. The two file-local `int MSleep(...)`
// declarations in Game/LawnSession.cpp:19 and Sexy/SexyCommonApp.cpp:206 are
// the leftovers to delete, not this definition.
//
// The body that used to be here was `{ ::Sleep(1); return 0; }` -- a one
// millisecond yield, because a plain `return 0` inside LawnSession's
// `while (1) { if (HandleTask() == 0) { MSleep(...); ... } }` would have turned
// that thread into a busy loop. ReconHelpers.cpp's version does the same job
// with the caller's real duration, so it is strictly better.
// ---------------------------------------------------------------------------

// TODO(recon): the log helpers format and dispatch through the framework log
// listeners; the argument lists are real (printf-style) but unreadable here.
// There is no global composeLog: the real one is TM::LogComposer::composeLog in
// Game/LogComposer.cpp.
void OutputDebug(const char* theFormat, ...)
{
    va_list aArgs;
    va_start(aArgs, theFormat);
    va_list aMeasure;
    va_copy(aMeasure, aArgs);
    int aLength = vsnprintf(nullptr, 0, theFormat, aMeasure);
    va_end(aMeasure);
    if (aLength < 0)
        aLength = 0;
    std::vector<char> aText((size_t)aLength + 1);
    vsnprintf(aText.data(), aText.size(), theFormat, aArgs);
    va_end(aArgs);
    fprintf(stderr, "%s\n", aText.data());
}

// ---------------------------------------------------------------------------
// The AG* / j_AG* platform layer (Amazon GameCircle / Android).
//
// Each of these is a PLT stub in libGameMain.so that jumps into the platform
// library; there is nothing to transcribe on Windows and no implementation to
// recover, so `return 0` is the final answer rather than a placeholder. The
// TODO stays because a Windows backend could give them meaning later.
// ---------------------------------------------------------------------------
// TODO(recon)
int j_AGGoToMarket(...) { return 0; }
// TODO(recon)
int j_AGPostData(...) { return 0; }
// TODO(recon)
int j_AGPostXMLFileData(...) { return 0; }
// TODO(recon)
int j_AGShowMessageBox(...) { return 0; }

// ---------------------------------------------------------------------------
// The Native:: bridge (GameCenter / ShareManager).
//
// Same story as the AG* family: `__imp__<mangled>` is the name the transcriber
// gave the import stubs, and the Android platform library behind them does not
// exist on Windows. The bodies in Game/GameCenter.cpp and Game/ShareManager.cpp
// fall back to a default when these return 0.
// ---------------------------------------------------------------------------
// TODO(recon)
int __imp__ZN6Native10GameCenter10addFriendsEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter10getFriendsEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter10getUserTagEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter10isLoggedInEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter11getCurrencyEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter11getFeaturesEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter11getPropertyERKSs(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter11getUserNameEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter11hasPropertyERKSs(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter11setListenerEPNS0_8ListenerE(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter11setPropertyERKSsS2_(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter12getSessionIdEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter12inviteFriendERKNS0_6FriendERKSs(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter12queryFriendsEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter13getLoginStateEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter13queryProductsEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter13switchAccountEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter15canMakePurchaseEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter15generateOrderIdEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter18isQueryingProductsEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter18queryAntiAddictionEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter19getInvitableFriendsEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter19getPurchaseFeaturesEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter21getAntiAddictionStateEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter21queryInvitableFriendsEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter4hideERKSs(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter4showERKSs(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter5loginEi(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter6createEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter6logoutEv(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter8Listener7onEventEPS0_iiRKSs(...) { return 0; }
// TODO(recon)
int __imp__ZN6Native10GameCenter9getUserIdEv(...) { return 0; }
// TODO(recon)
int __imp__ZNK6Native10GameCenter7getNameEv(...) { return 0; }

// ---------------------------------------------------------------------------
// The curl / JSON-RPC layer.
// ---------------------------------------------------------------------------
// TODO(recon)
int DrCurlEngine::init(...) { return 0; }
// TODO(recon)
int DrCurlRequest::getField(...) { return 0; }
// TODO(recon)
int DrCurlRequest::getPostFields(...) { return 0; }
// TODO(recon)
int DrCurlRequest::getSortedFields(...) { return 0; }
// TODO(recon)
int DrCurlRequest::getUrl(...) { return 0; }
// TODO(recon)
int DrCurlRequest::setField(...) { return 0; }
// TODO(recon)
int DrDataEncoder::decode(...) { return 0; }
// TODO(recon)
int DrDataEncoder::encode(...) { return 0; }
// TODO(recon)
int LawnSerialNumber::GetNewOne(...) { return 0; }

// ---------------------------------------------------------------------------
// TM:: -- the thread and XML helpers.
// ---------------------------------------------------------------------------
// TODO(recon)
int TM::XML::getChildNode(...) { return 0; }
// TODO(recon)
int TM::XML::getText(...) { return 0; }
// TODO(recon)
int TM::XML::isEmpty(...) { return 0; }
// TODO(recon)
int TM::XML::nChildNode(...) { return 0; }
// TODO(recon)
int TM::XML::nText(...) { return 0; }
// TODO(recon)
int TM::XML::parseString(...) { return 0; }
