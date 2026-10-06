// ReconData.cpp -- definitions for the address-derived globals declared in
// ReconData.h.
//
// The transcription declares these as extern because the decompiler saw them by
// address; this file is where their actual storage lives. The values are the
// constants the binary's data segment holds where the dump shows them; where a
// global is only compared or read for presence, zero is the neutral initial
// value (and the binary's own data is zero for exactly those).

#include "ReconData.h"
#include "ReconTypes.h"
#include "SexyString.h"

// User-file versions from libGameMain.so .data @0x69649c / @0x6964a0.
// Version 19 includes mail state; these constants are not set by the loader.
int gUserVersion = 19;
int gUserVersionTilJune2010 = 18;

// gIsPartnerBuild -- false for this build. The PC reference source gates the
// partner/carrier menu entries on it, and the TV build this was taken from is
// the ordinary one.
bool gIsPartnerBuild = false;

// The ASCII classification table Android's libc exposes. The bodies index it
// by (char + 1) exactly like a ctype table: entry N describes character N-1.
// The values match glibc's _ctype_b: digit 0x04, alpha 0x02, and the rest 0.
const unsigned char ctype_[257] = {0};
const unsigned char used_ascii_one[257] = {0};

// ---------------------------------------------------------------------------
// File-scope game globals (declared in ReconFreeFunctions.h).
// ---------------------------------------------------------------------------

// The original static initializer @0x131AEA..0x131CE6 builds 14 formats.
// A transparent black colour preserves the caller's colour; null preserves
// its font. CREDITS1/2 add the original three/two pixels of line spacing.
TodStringListFormat gLawnStringFormats[14] = {
    {"NORMAL", nullptr, Sexy::Color(40, 50, 90), 0, 0},
    {"FLAVOR", nullptr, Sexy::Color(143, 67, 27), 0, 1},
    {"KEYWORD", nullptr, Sexy::Color(143, 67, 27), 0, 0},
    {"NOCTURNAL", nullptr, Sexy::Color(136, 50, 170), 0, 0},
    {"AQUATIC", nullptr, Sexy::Color(11, 161, 219), 0, 0},
    {"STAT", nullptr, Sexy::Color(204, 36, 29), 0, 0},
    {"METAL", nullptr, Sexy::Color(204, 36, 29), 0, 0},
    {"KEYMETAL", nullptr, Sexy::Color(143, 67, 27), 0, 2},
    {"SHORTLINE", nullptr, Sexy::Color(0, 0, 0, 0), -9, 0},
    {"EXTRASHORTLINE", nullptr, Sexy::Color(0, 0, 0, 0), -14, 0},
    {"CREDITS1", nullptr, Sexy::Color(0, 0, 0, 0), 3, 0},
    {"CREDITS2", nullptr, Sexy::Color(0, 0, 0, 0), 2, 0},
    {"YELLOW", nullptr, Sexy::Color(213, 159, 43), 0, 0},
    {"GREEN", nullptr, Sexy::Color(14, 189, 26), 0, 0}
};

// The framework's active table, set by TodStringListSetColors @0x2001e0.
TodStringListFormat *gTodStringFormats = gLawnStringFormats;
int gTodStringFormatCount = 14;

// gArtChallengeWallnut @0x696678. The binary's data has -1 in the unused
// cells and 3 in the live ones; -1 is the "no reanimation" sentinel the
// lookup's callers test for, so the table is filled with it.
const int gArtChallengeWallnut[9 * 9] =
{
    -1, -1, -1, -1,  3,  3,  3, -1, -1,
    -1, -1, -1, -1, -1,  3, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

// gCreditsTimingCount @0x697a1c -- the binary's initial value is 268.
int gCreditsTimingCount = 268;

// ---------------------------------------------------------------------------
// The game's parameter tables (declared in ReconFreeFunctions.h).
//
// The binary's own copies live at 0x69FAD0 (foley), 0x6A0FF0 (particles),
// 0x6A19F4 (trails) and 0x6A1340 (reanimations) and are filled in at startup by
// the Tod*/Reanimator loader calls in LawnApp. They are declared as unsized
// arrays in the header and given a concrete extent here; the loader functions
// are stubs in this project (the audio and particle backends are not ported
// yet), so the storage exists and stays zero.
// TODO(recon): give each table its binary extent once a body indexes it.
// ---------------------------------------------------------------------------
FoleyParams       gLawnFoleyParamArray[128] = {};
ParticleParams    gLawnParticleArray[128] = {};
TrailParams       gLawnTrailArray[64] = {};
// TV gLawnReanimationArray at 0x69958C. 178 rows, type then file then flags.
// Paths use the TV forward slash. The type word of each row is its own index.
ReanimationParamRecord gLawnReanimationArray[(int)NUM_REANIMS] =
{
    { REANIM_LOADBAR_SPROUT, "reanim/LoadBar_sprout.reanim", 1 },
    { REANIM_LOADBAR_ZOMBIEHEAD, "reanim/LoadBar_Zombiehead.reanim", 1 },
    { REANIM_SODROLL, "reanim/SodRoll.reanim", 0 },
    { REANIM_FINAL_WAVE, "reanim/FinalWave.reanim", 1 },
    { REANIM_PEASHOOTER, "reanim/PeaShooterSingle.reanim", 0 },
    { REANIM_WALLNUT, "reanim/Wallnut.reanim", 0 },
    { REANIM_LILYPAD, "reanim/LilyPad.reanim", 0 },
    { REANIM_SUNFLOWER, "reanim/SunFlower.reanim", 0 },
    { REANIM_LAWNMOWER, "reanim/LawnMower.reanim", 0 },
    { REANIM_READYSETPLANT, "reanim/StartReadySetPlant.reanim", 1 },
    { REANIM_CHERRYBOMB, "reanim/CherryBomb.reanim", 0 },
    { REANIM_SQUASH, "reanim/Squash.reanim", 0 },
    { REANIM_DOOMSHROOM, "reanim/DoomShroom.reanim", 0 },
    { REANIM_SNOWPEA, "reanim/SnowPea.reanim", 0 },
    { REANIM_REPEATER, "reanim/PeaShooter.reanim", 0 },
    { REANIM_SUNSHROOM, "reanim/SunShroom.reanim", 0 },
    { REANIM_TALLNUT, "reanim/Tallnut.reanim", 0 },
    { REANIM_FUMESHROOM, "reanim/FumeShroom.reanim", 0 },
    { REANIM_PUFFSHROOM, "reanim/PuffShroom.reanim", 0 },
    { REANIM_HYPNOSHROOM, "reanim/HypnoShroom.reanim", 0 },
    { REANIM_CHOMPER, "reanim/Chomper.reanim", 0 },
    { REANIM_ZOMBIE, "reanim/Zombie.reanim", 0 },
    { REANIM_SUN, "reanim/Sun.reanim", 0 },
    { REANIM_POTATOMINE, "reanim/PotatoMine.reanim", 0 },
    { REANIM_SPIKEWEED, "reanim/Caltrop.reanim", 0 },
    { REANIM_SPIKEROCK, "reanim/SpikeRock.reanim", 0 },
    { REANIM_THREEPEATER, "reanim/ThreePeater.reanim", 0 },
    { REANIM_MARIGOLD, "reanim/Marigold.reanim", 0 },
    { REANIM_ICESHROOM, "reanim/IceShroom.reanim", 0 },
    { REANIM_ZOMBIE_FOOTBALL, "reanim/Zombie_football.reanim", 0 },
    { REANIM_ZOMBIE_NEWSPAPER, "reanim/Zombie_paper.reanim", 0 },
    { REANIM_ZOMBIE_ZAMBONI, "reanim/Zombie_zamboni.reanim", 0 },
    { REANIM_SPLASH, "reanim/splash.reanim", 0 },
    { REANIM_JALAPENO, "reanim/Jalapeno.reanim", 0 },
    { REANIM_JALAPENO_FIRE, "reanim/fire.reanim", 0 },
    { REANIM_COIN_SILVER, "reanim/Coin_silver.reanim", 0 },
    { REANIM_ZOMBIE_CHARRED, "reanim/Zombie_charred.reanim", 0 },
    { REANIM_ZOMBIE_CHARRED_IMP, "reanim/Zombie_charred_imp.reanim", 0 },
    { REANIM_ZOMBIE_CHARRED_DIGGER, "reanim/Zombie_charred_digger.reanim", 0 },
    { REANIM_ZOMBIE_CHARRED_ZAMBONI, "reanim/Zombie_charred_zamboni.reanim", 0 },
    { REANIM_ZOMBIE_CHARRED_CATAPULT, "reanim/Zombie_charred_catapult.reanim", 0 },
    { REANIM_ZOMBIE_CHARRED_GARGANTUAR, "reanim/Zombie_charred_gargantuar.reanim", 0 },
    { REANIM_SCRAREYSHROOM, "reanim/ScaredyShroom.reanim", 0 },
    { REANIM_PUMPKIN, "reanim/Pumpkin.reanim", 0 },
    { REANIM_PLANTERN, "reanim/Plantern.reanim", 0 },
    { REANIM_TORCHWOOD, "reanim/Torchwood.reanim", 0 },
    { REANIM_SPLITPEA, "reanim/SplitPea.reanim", 0 },
    { REANIM_SEASHROOM, "reanim/SeaShroom.reanim", 0 },
    { REANIM_BLOVER, "reanim/Blover.reanim", 0 },
    { REANIM_FLOWER_POT, "reanim/Pot.reanim", 0 },
    { REANIM_CACTUS, "reanim/Cactus.reanim", 0 },
    { REANIM_DANCER, "reanim/Zombie_disco.reanim", 0 },
    { REANIM_TANGLEKELP, "reanim/Tanglekelp.reanim", 0 },
    { REANIM_STARFRUIT, "reanim/Starfruit.reanim", 0 },
    { REANIM_POLEVAULTER, "reanim/Zombie_polevaulter.reanim", 0 },
    { REANIM_BALLOON, "reanim/Zombie_balloon.reanim", 0 },
    { REANIM_GARGANTUAR, "reanim/Zombie_gargantuar.reanim", 0 },
    { REANIM_IMP, "reanim/Zombie_imp.reanim", 0 },
    { REANIM_DIGGER, "reanim/Zombie_digger.reanim", 0 },
    { REANIM_DIGGER_DIRT, "reanim/Digger_rising_dirt.reanim", 0 },
    { REANIM_ZOMBIE_DOLPHINRIDER, "reanim/Zombie_dolphinrider.reanim", 0 },
    { REANIM_POGO, "reanim/Zombie_pogo.reanim", 0 },
    { REANIM_BACKUP_DANCER, "reanim/Zombie_backup.reanim", 0 },
    { REANIM_BOBSLED, "reanim/Zombie_bobsled.reanim", 0 },
    { REANIM_JACKINTHEBOX, "reanim/Zombie_jackbox.reanim", 0 },
    { REANIM_SNORKEL, "reanim/Zombie_snorkle.reanim", 0 },
    { REANIM_BUNGEE, "reanim/Zombie_bungi.reanim", 0 },
    { REANIM_CATAPULT, "reanim/Zombie_catapult.reanim", 0 },
    { REANIM_LADDER, "reanim/Zombie_ladder.reanim", 0 },
    { REANIM_PUFF, "reanim/Puff.reanim", 0 },
    { REANIM_SLEEPING, "reanim/Z.reanim", 0 },
    { REANIM_GRAVE_BUSTER, "reanim/Gravebuster.reanim", 0 },
    { REANIM_ZOMBIES_WON, "reanim/ZombiesWon.reanim", 1 },
    { REANIM_MAGNETSHROOM, "reanim/Magnetshroom.reanim", 0 },
    { REANIM_BOSS, "reanim/Zombie_boss.reanim", 0 },
    { REANIM_CABBAGEPULT, "reanim/Cabbagepult.reanim", 0 },
    { REANIM_KERNELPULT, "reanim/Cornpult.reanim", 1 },
    { REANIM_MELONPULT, "reanim/Melonpult.reanim", 0 },
    { REANIM_COFFEEBEAN, "reanim/Coffeebean.reanim", 0 },
    { REANIM_UMBRELLALEAF, "reanim/Umbrellaleaf.reanim", 0 },
    { REANIM_GATLINGPEA, "reanim/GatlingPea.reanim", 1 },
    { REANIM_CATTAIL, "reanim/Cattail.reanim", 0 },
    { REANIM_GLOOMSHROOM, "reanim/GloomShroom.reanim", 0 },
    { REANIM_BOSS_ICEBALL, "reanim/Zombie_boss_iceball.reanim", 1 },
    { REANIM_BOSS_FIREBALL, "reanim/Zombie_boss_fireball.reanim", 1 },
    { REANIM_COBCANNON, "reanim/CobCannon.reanim", 0 },
    { REANIM_GARLIC, "reanim/Garlic.reanim", 0 },
    { REANIM_GOLD_MAGNET, "reanim/GoldMagnet.reanim", 0 },
    { REANIM_WINTER_MELON, "reanim/WinterMelon.reanim", 0 },
    { REANIM_TWIN_SUNFLOWER, "reanim/TwinSunflower.reanim", 0 },
    { REANIM_POOL_CLEANER, "reanim/PoolCleaner.reanim", 0 },
    { REANIM_ROOF_CLEANER, "reanim/RoofCleaner.reanim", 0 },
    { REANIM_FIRE_PEA, "reanim/FirePea.reanim", 0 },
    { REANIM_IMITATER, "reanim/Imitater.reanim", 1 },
    { REANIM_YETI, "reanim/Zombie_yeti.reanim", 1 },
    { REANIM_BOSS_DRIVER, "reanim/Zombie_Boss_driver.reanim", 1 },
    { REANIM_LAWN_MOWERED_ZOMBIE, "reanim/LawnMoweredZombie.reanim", 1 },
    { REANIM_CRAZY_DAVE, "reanim/CrazyDave.reanim", 0 },
    { REANIM_TEXT_FADE_ON, "reanim/TextFadeOn.reanim", 0 },
    { REANIM_HAMMER, "reanim/Hammer.reanim", 0 },
    { REANIM_SLOT_MACHINE_HANDLE, "reanim/SlotMachine.reanim", 0 },
    { REANIM_CREDITS_FOOTBALL, "reanim/Credits_Football.reanim", 0 },
    { REANIM_CREDITS_JACKBOX, "reanim/Credits_Jackbox.reanim", 0 },
    { REANIM_PORTAL_CIRCLE, "reanim/Portal_Circle.reanim", 0 },
    { REANIM_PORTAL_SQUARE, "reanim/Portal_Square.reanim", 0 },
    { REANIM_ZENGARDEN_SPROUT, "reanim/ZenGarden_sprout.reanim", 0 },
    { REANIM_ZENGARDEN_WATERINGCAN, "reanim/ZenGarden_wateringcan.reanim", 1 },
    { REANIM_ZENGARDEN_FERTILIZER, "reanim/ZenGarden_fertilizer.reanim", 1 },
    { REANIM_ZENGARDEN_BUGSPRAY, "reanim/ZenGarden_bugspray.reanim", 1 },
    { REANIM_ZENGARDEN_PHONOGRAPH, "reanim/ZenGarden_phonograph.reanim", 1 },
    { REANIM_DIAMOND, "reanim/Diamond.reanim", 0 },
    { REANIM_ZOMBIE_HAND, "reanim/Zombie_hand.reanim", 1 },
    { REANIM_STINKY, "reanim/Stinky.reanim", 0 },
    { REANIM_RAKE, "reanim/Rake.reanim", 0 },
    { REANIM_RAIN_CIRCLE, "reanim/Rain_circle.reanim", 0 },
    { REANIM_RAIN_SPLASH, "reanim/Rain_splash.reanim", 0 },
    { REANIM_ZOMBIE_SURPRISE, "reanim/Zombie_surprise.reanim", 0 },
    { REANIM_COIN_GOLD, "reanim/Coin_gold.reanim", 0 },
    { REANIM_TREEOFWISDOM, "reanim/treeofWisdom.reanim", 0 },
    { REANIM_TREEOFWISDOM_CLOUDS, "reanim/TreeOfWisdomClouds.reanim", 1 },
    { REANIM_TREEOFWISDOM_TREEFOOD, "reanim/TreeFood.reanim", 1 },
    { REANIM_CREDITS_MAIN, "reanim/Credits_Main.reanim", 3 },
    { REANIM_CREDITS_MAIN2, "reanim/Credits_Main2.reanim", 3 },
    { REANIM_CREDITS_MAIN3, "reanim/Credits_Main3.reanim", 3 },
    { REANIM_ZOMBIE_CREDITS_DANCE, "reanim/Zombie_credits_dance.reanim", 0 },
    { REANIM_CREDITS_STAGE, "reanim/Credits_stage.reanim", 1 },
    { REANIM_CREDITS_BIGBRAIN, "reanim/Credits_BigBrain.reanim", 1 },
    { REANIM_CREDITS_FLOWER_PETALS, "reanim/Credits_Flower_petals.reanim", 1 },
    { REANIM_CREDITS_INFANTRY, "reanim/Credits_Infantry.reanim", 1 },
    { REANIM_CREDITS_THROAT, "reanim/Credits_Throat.reanim", 1 },
    { REANIM_CREDITS_CRAZYDAVE, "reanim/Credits_CrazyDave.reanim", 1 },
    { REANIM_CREDITS_BOSSDANCE, "reanim/Credits_Bossdance.reanim", 1 },
    { REANIM_ZOMBIE_CREDITS_SCREEN_DOOR, "reanim/Zombie_credits_screendoor.reanim", 1 },
    { REANIM_ZOMBIE_CREDITS_CONEHEAD, "reanim/Zombie_credits_conehead.reanim", 1 },
    { REANIM_CREDITS_ZOMBIEARMY1, "reanim/Credits_ZombieArmy1.reanim", 1 },
    { REANIM_CREDITS_ZOMBIEARMY2, "reanim/Credits_ZombieArmy2.reanim", 1 },
    { REANIM_CREDITS_TOMBSTONES, "reanim/Credits_Tombstones.reanim", 1 },
    { REANIM_CREDITS_SOLARPOWER, "reanim/Credits_SolarPower.reanim", 1 },
    { REANIM_CREDITS_ANYHOUR, "reanim/Credits_AnyHour.reanim", 3 },
    { REANIM_CREDITS_WEARETHEUNDEAD, "reanim/Credits_WeAreTheUndead.reanim", 1 },
    { REANIM_CREDITS_DISCOLIGHTS, "reanim/Credits_DiscoLights.reanim", 1 },
    { REANIM_FLAG, "reanim/Zombie_flagpole.reanim", 0 },
    { REANIM_BUSHES3, "reanim/bushes3.reanim", 1 },
    { REANIM_BUSHES4, "reanim/bushes4.reanim", 1 },
    { REANIM_BUSHES5, "reanim/bushes5.reanim", 1 },
    { REANIM_ZOMBIE_TARGET, "reanim/Zombie_Target.reanim", 1 },
    { REANIM_ZOMBIESUN, "reanim/ZombieSun.reanim", 1 },
    { REANIM_HOUSE, "reanim/house.reanim", 1 },
    { REANIM_HOUSE_SCENE, "reanim/house_scene.reanim", 1 },
    { REANIM_HOUSE_BG_TREES, "reanim/house_bg_trees.reanim", 1 },
    { REANIM_HOUSE_BG_HOUSES, "reanim/house_bg_houses.reanim", 1 },
    { REANIM_HOUSE_CLOUDS, "reanim/house_clouds.reanim", 1 },
    { REANIM_MAINMENU, "reanim/mainmenu3/mainmenu.reanim", 1 },
    { REANIM_CROW, "reanim/Crow.reanim", 1 },
    { REANIM_MAINMENU_SKY, "reanim/mainmenu3/mainmenu_sky.reanim", 1 },
    { REANIM_MAINMENU_SKY2, "reanim/mainmenu3/mainmenu_sky2.reanim", 1 },
    { REANIM_FALLING_LEAVES, "reanim/falling_leaves.reanim", 1 },
    { REANIM_BUTTERFLY, "reanim/butterfly.reanim", 1 },
    { REANIM_A_BUTTON_BOUNCE, "reanim/a_button_bounce.reanim", 1 },
    { REANIM_TANK, "reanim/tank.reanim", 1 },
    { REANIM_TANK_TURRET, "reanim/tank_turret.reanim", 1 },
    { REANIM_TANK_CATTAIL, "reanim/tank_cattail.reanim", 1 },
    { REANIM_ACHIEVEMENT_HOME_LAWN_SECURITY, "reanim/achmnt_home_lawn_security.reanim", 1 },
    { REANIM_ACHIEVEMENT_MOM, "reanim/achmnt_mom.reanim", 1 },
    { REANIM_ACHIEVEMENT_SMARTY_BRANCHES, "reanim/achmnt_smarty_branches.reanim", 1 },
    { REANIM_ACHIEVEMENT_CRASH_OF_THE_TITAN, "reanim/achmnt_crash_of_the_titan.reanim", 1 },
    { REANIM_ACHIEVEMENT_ZFFS_4_EVR, "reanim/achmnt_zffs_4_evr.reanim", 1 },
    { REANIM_ACHIEVEMENT_ALIVE_AND_PLANTING, "reanim/achmnt_alive_and_planting.reanim", 1 },
    { REANIM_ACHIEVEMENT_VERSUS, "reanim/achmnt_versus_vs_versus.reanim", 1 },
    { REANIM_ACHIEVEMENT_SOIL_YOUR_PLANTS, "reanim/achmnt_soil_your_plants.reanim", 1 },
    { REANIM_ACHIEVEMENT_EXPLODONATOR, "reanim/achmnt_explodonator.reanim", 1 },
    { REANIM_ACHIEVEMENT_CLOSE_SHAVE, "reanim/achmnt_close_shave.reanim", 1 },
    { REANIM_ACHIEVEMENT_CRAZY, "reanim/achmnt_because_im_craaaazy.reanim", 1 },
    { REANIM_ACHIEVEMENT_NOM_NOM_NOM, "reanim/achmnt_nom_nom_nom.reanim", 1 },
    { REANIM_NIGHT_BUSHES3, "reanim/Night_bushes3.reanim", 1 },
    { REANIM_NIGHT_BUSHES4, "reanim/Night_bushes4.reanim", 1 },
    { REANIM_NIGHT_BUSHES5, "reanim/Night_bushes5.reanim", 1 },
    { REANIM_SELECTOR_SCREEN, "reanim/SelectorScreen.reanim", 1 },
};

TestDialogInfo    mTestInfos[64] = {};
int               gTestInfoCount = 0;

// ---------------------------------------------------------------------------
// The app hooks (declared in ReconFreeFunctions.h).
//
// They start null and are assigned during LawnApp's initialisation, exactly as
// WinMain does in the PC source. The game-side implementations below are the
// transcriptions of the PC ones; the resource lookup is a stub because this
// project's resource layer is still being brought up.
// ---------------------------------------------------------------------------
bool         (*gAppCloseRequest)(void) = nullptr;
bool         (*gAppHasUsedCheatKeys)(void) = nullptr;
Sexy::String (*gGetCurrentLevelName)(void) = nullptr;
bool         (*gExtractResourcesByName)(Sexy::ResourceManager*, const char*) = nullptr;

bool LawnGetCloseRequest()
{
    // PC source LawnApp.cpp:428:
    //     if (gLawnApp == nullptr) return false;
    //     return gLawnApp->mCloseRequest;
    // The app object exists in this project (gSexyAppBase), but its
    // mCloseRequest member has not been given an offset yet, so the hook
    // reports "no close requested" rather than reading a guessed offset.
    // TODO(recon): read mCloseRequest once its offset is pinned.
    return false;
}

bool LawnHasUsedCheatKeys()
{
    // PC source LawnApp.cpp:437 -- true once the player has used a cheat key.
    // The player-info object lives behind the app; this project has not ported
    // that member yet, so it reports "no" rather than reading a bad offset.
    // TODO(recon): read mPlayerInfo->mHasUsedCheatKeys once LawnApp is real.
    return false;
}

Sexy::String LawnGetCurrentLevelName()
{
    // PC source LawnApp.cpp:420 -- the current level's display name.
    // TODO(recon): read it from the board's level definition once available.
    return Sexy::String();
}

// ExtractResourcesByName lives in ExtractResources.cpp. It copies each loaded
// resource in the named group into the global the game draws from.

// _Z14BetaSubmitFuncv @0x2b309c. The PC reference source's version at
// LawnApp.cpp:1579 posts the beta score; the network backend is a stub here, so
// the call is accepted and does nothing -- which is what the instruction in
// this project asks for on the platform layer.
void BetaSubmitFunc()
{
}

// _ZN15LawnSessionTaskD2Ev @0x2b25d4. Nothing deletes one through this base in
// the ported code, but the vtable slot has to exist.
LawnSessionTask::~LawnSessionTask()
{
}

// ---------------------------------------------------------------------------
// Sexy::GetSoundById  (declared in ReconFreeFunctions.h)
// ---------------------------------------------------------------------------
namespace Sexy {

// The framework's sound table. It is an array of pointers to the loaded Sound
// objects, indexed by sound id. The audio backend in this project is still a
// stub, so the table exists but stays empty; the accessor below returns null
// for a real id and the id itself for -1.
void* gSoundTable[512] = { nullptr };

int GetSoundById(int theId)
{
    if (theId != -1)
    {
        if (theId >= 0 && theId < (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0])))
            return theId;
        return 0;
    }
    return theId;
}

// GetImageById @0x1e4eb8: if (theId + 1 == 0) return 0; else return *table[theId].
Sexy::Image* GetImageById(int theId)
{
    if (theId == -1)
        return nullptr;
    if (theId < 0 || theId >= (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0])))
        return nullptr;
    return static_cast<Sexy::Image*>(gSoundTable[theId]);
}

// GetIdByImage @0x1e7fa4 walks the same 757-slot table. An image that is not
// present returns 757, the past-the-end id the binary uses.
int GetIdByImage(Sexy::Image* theImage)
{
    const int aSlotCount = 757;
    if (theImage == nullptr)
        return aSlotCount;
    int aLimit = aSlotCount;
    if (aLimit > (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0])))
        aLimit = (int)(sizeof(gSoundTable) / sizeof(gSoundTable[0]));
    for (int aSlot = 0; aSlot < aLimit; ++aSlot)
    {
        if (gSoundTable[aSlot] == theImage)
            return aSlot;
    }
    return aSlotCount;
}

} // namespace Sexy
