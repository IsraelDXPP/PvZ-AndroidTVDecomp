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

// ---------------------------------------------------------------------------
// Particle and trail tables still await restoration of their definitions.
// ---------------------------------------------------------------------------
// gLawnFoleyParamArray @0x69A144: the TV binary's 104 sound definitions.
// Entries keep pointers to the sound globals that resource extraction publishes.
FoleyParams gLawnFoleyParamArray[NUM_FOLEY] =
{
    { FOLEY_SUN, 10.0f, { &SOUND_POINTS }, 0 },
    { FOLEY_SPLAT, 10.0f, { &SOUND_SPLAT, &SOUND_SPLAT2, &SOUND_SPLAT3 }, 0 },
    { FOLEY_LAWNMOWER, 10.0f, { &SOUND_LAWNMOWER }, 0 },
    { FOLEY_THROW, 10.0f, { &SOUND_THROW, &SOUND_THROW, &SOUND_THROW, &SOUND_THROW2 }, 0 },
    { FOLEY_SPAWN_SUN, 10.0f, { &SOUND_THROW }, 0 },
    { FOLEY_CHOMP, 0.0f, { &SOUND_CHOMP, &SOUND_CHOMP2 }, 0 },
    { FOLEY_CHOMP_SOFT, 4.0f, { &SOUND_CHOMPSOFT }, 0 },
    { FOLEY_PLANT, 0.0f, { &SOUND_PLANT, &SOUND_PLANT2 }, 0 },
    { FOLEY_USE_SHOVEL, 0.0f, { &SOUND_PLANT2 }, 0 },
    { FOLEY_DROP, 0.0f, { &SOUND_TAP2 }, 0 },
    { FOLEY_BLEEP, 0.0f, { &SOUND_BLEEP }, 0 },
    { FOLEY_GROAN, 0.0f, { &SOUND_GROAN, &SOUND_GROAN2, &SOUND_GROAN3, &SOUND_GROAN4, &SOUND_GROAN5, &SOUND_GROAN6 }, 0 },
    { FOLEY_BRAINS, 0.0f, { &SOUND_GROAN, &SOUND_GROAN2, &SOUND_GROAN3, &SOUND_GROAN4, &SOUND_GROAN5, &SOUND_GROAN6, &SOUND_SUKHBIR4, &SOUND_SUKHBIR5, &SOUND_SUKHBIR6 }, 0 },
    { FOLEY_SUKHBIR, 0.0f, { &SOUND_GROAN, &SOUND_GROAN2, &SOUND_GROAN3, &SOUND_GROAN4, &SOUND_GROAN5, &SOUND_GROAN6, &SOUND_SUKHBIR, &SOUND_SUKHBIR2, &SOUND_SUKHBIR3 }, 0 },
    { FOLEY_JACKINTHEBOX, 0.0f, { &SOUND_JACKINTHEBOX }, 7 },
    { FOLEY_ART_CHALLENGE, 0.0f, { &SOUND_DIAMOND }, 0 },
    { FOLEY_ZAMBONI, 5.0f, { &SOUND_ZAMBONI }, 0 },
    { FOLEY_THUNDER, 10.0f, { &SOUND_THUNDER }, 0 },
    { FOLEY_FROZEN, 0.0f, { &SOUND_FROZEN }, 0 },
    { FOLEY_ZOMBIESPLASH, 10.0f, { &SOUND_PLANT_WATER, &SOUND_ZOMBIE_ENTERING_WATER }, 0 },
    { FOLEY_BOWLINGIMPACT, -3.0f, { &SOUND_BOWLINGIMPACT }, 0 },
    { FOLEY_SQUISH, 0.0f, { &SOUND_CHOMP, &SOUND_CHOMP2 }, 0 },
    { FOLEY_TIRE_POP, 0.0f, { &SOUND_BALLOON_POP }, 0 },
    { FOLEY_EXPLOSION, 0.0f, { &SOUND_EXPLOSION }, 0 },
    { FOLEY_SLURP, 2.0f, { &SOUND_SLURP }, 0 },
    { FOLEY_LIMBS_POP, 10.0f, { &SOUND_LIMBS_POP }, 0 },
    { FOLEY_POGO_ZOMBIE, 4.0f, { &SOUND_POGO_ZOMBIE }, 0 },
    { FOLEY_SNOW_PEA_SPARKLES, 10.0f, { &SOUND_SNOW_PEA_SPARKLES }, 0 },
    { FOLEY_ZOMBIE_FALLING, 10.0f, { &SOUND_ZOMBIE_FALLING_1, &SOUND_ZOMBIE_FALLING_2 }, 0 },
    { FOLEY_PUFF, 10.0f, { &SOUND_PUFF }, 0 },
    { FOLEY_FUME, 10.0f, { &SOUND_FUME }, 0 },
    { FOLEY_COIN, 10.0f, { &SOUND_COIN }, 0 },
    { FOLEY_KERNEL_SPLAT, 10.0f, { &SOUND_KERNELPULT, &SOUND_KERNELPULT2 }, 0 },
    { FOLEY_DIGGER, 0.0f, { &SOUND_DIGGER_ZOMBIE }, 7 },
    { FOLEY_JACK_SURPRISE, 1.0f, { &SOUND_JACK_SURPRISE, &SOUND_JACK_SURPRISE, &SOUND_JACK_SURPRISE2 }, 0 },
    { FOLEY_VASE_BREAKING, -5.0f, { &SOUND_VASE_BREAKING }, 0 },
    { FOLEY_POOL_CLEANER, 2.0f, { &SOUND_POOL_CLEANER }, 0 },
    { FOLEY_BASKETBALL, 10.0f, { &SOUND_BASKETBALL }, 0 },
    { FOLEY_IGNITE, 5.0f, { &SOUND_IGNITE, &SOUND_IGNITE, &SOUND_IGNITE, &SOUND_IGNITE2 }, 0 },
    { FOLEY_FIREPEA, 10.0f, { &SOUND_FIREPEA }, 0 },
    { FOLEY_THUMP, 2.0f, { &SOUND_GARGANTUAR_THUMP }, 0 },
    { FOLEY_SQUASH_HMM, 2.0f, { &SOUND_SQUASH_HMM, &SOUND_SQUASH_HMM, &SOUND_SQUASH_HMM2 }, 0 },
    { FOLEY_MAGNETSHROOM, 2.0f, { &SOUND_MAGNETSHROOM }, 0 },
    { FOLEY_BUTTER, 2.0f, { &SOUND_BUTTER }, 0 },
    { FOLEY_BUNGEE_SCREAM, 2.0f, { &SOUND_BUNGEE_SCREAM, &SOUND_BUNGEE_SCREAM2, &SOUND_BUNGEE_SCREAM3 }, 0 },
    { FOLEY_BOSS_EXPLOSION_SMALL, 2.0f, { &SOUND_EXPLOSION }, 0 },
    { FOLEY_SHIELD_HIT, 10.0f, { &SOUND_SHIELDHIT, &SOUND_SHIELDHIT2 }, 0 },
    { FOLEY_SWING, 2.0f, { &SOUND_SWING }, 0 },
    { FOLEY_BONK, 2.0f, { &SOUND_BONK }, 0 },
    { FOLEY_RAIN, 0.0f, { &SOUND_RAIN }, 5 },
    { FOLEY_DOLPHIN_BEFORE_JUMPING, 0.0f, { &SOUND_DOLPHIN_BEFORE_JUMPING }, 0 },
    { FOLEY_DOLPHIN_APPEARS, 0.0f, { &SOUND_DOLPHIN_APPEARS }, 0 },
    { FOLEY_PLANT_WATER, 0.0f, { &SOUND_PLANT_WATER }, 0 },
    { FOLEY_ZOMBIE_ENTERING_WATER, 0.0f, { &SOUND_ZOMBIE_ENTERING_WATER }, 0 },
    { FOLEY_GRAVEBUSTERCHOMP, 0.0f, { &SOUND_GRAVEBUSTERCHOMP }, 4 },
    { FOLEY_CHERRYBOMB, 0.0f, { &SOUND_CHERRYBOMB }, 0 },
    { FOLEY_JALAPENO_IGNITE, 0.0f, { &SOUND_JALAPENO }, 0 },
    { FOLEY_REVERSE_EXPLOSION, 0.0f, { &SOUND_REVERSE_EXPLOSION }, 0 },
    { FOLEY_PLASTIC_HIT, 5.0f, { &SOUND_PLASTICHIT, &SOUND_PLASTICHIT2 }, 0 },
    { FOLEY_WINMUSIC, 0.0f, { &SOUND_WINMUSIC }, 0 },
    { FOLEY_BALLOONINFLATE, 10.0f, { &SOUND_BALLOONINFLATE }, 0 },
    { FOLEY_BIGCHOMP, -2.0f, { &SOUND_BIGCHOMP }, 0 },
    { FOLEY_MELONIMPACT, -5.0f, { &SOUND_MELONIMPACT, &SOUND_MELONIMPACT2 }, 0 },
    { FOLEY_PLANTGROW, -2.0f, { &SOUND_PLANTGROW }, 0 },
    { FOLEY_SHOOP, -5.0f, { &SOUND_SHOOP }, 0 },
    { FOLEY_JUICY, 2.0f, { &SOUND_JUICY }, 0 },
    { FOLEY_NEWSPAPER_RARRGH, -2.0f, { &SOUND_NEWSPAPER_RARRGH, &SOUND_NEWSPAPER_RARRGH2, &SOUND_NEWSPAPER_RARRGH2 }, 0 },
    { FOLEY_NEWSPAPER_RIP, -2.0f, { &SOUND_NEWSPAPER_RIP }, 0 },
    { FOLEY_FLOOP, 0.0f, { &SOUND_FLOOP }, 0 },
    { FOLEY_COFFEE, 0.0f, { &SOUND_COFFEE }, 0 },
    { FOLEY_LOW_GROAN, 2.0f, { &SOUND_LOWGROAN, &SOUND_LOWGROAN2 }, 0 },
    { FOLEY_PRIZE, 0.0f, { &SOUND_PRIZE }, 0 },
    { FOLEY_YUCK, 1.0f, { &SOUND_YUCK, &SOUND_YUCK, &SOUND_YUCK2 }, 0 },
    { FOLEY_UMBRELLA, 2.0f, { &SOUND_THROW2 }, 0 },
    { FOLEY_GRASSSTEP, 2.0f, { &SOUND_GRASSSTEP }, 0 },
    { FOLEY_SHOVEL, 5.0f, { &SOUND_SHOVEL }, 0 },
    { FOLEY_COB_LAUNCH, 10.0f, { &SOUND_COBLAUNCH }, 0 },
    { FOLEY_WATERING, 10.0f, { &SOUND_WATERING }, 0 },
    { FOLEY_POLEVAULT, 5.0f, { &SOUND_POLEVAULT }, 0 },
    { FOLEY_GRAVESTONE_RUMBLE, 10.0f, { &SOUND_GRAVESTONE_RUMBLE }, 0 },
    { FOLEY_DIRT_RISE, 5.0f, { &SOUND_DIRT_RISE }, 0 },
    { FOLEY_FERTILIZER, 0.0f, { &SOUND_FERTILIZER }, 0 },
    { FOLEY_PORTAL, 0.0f, { &SOUND_PORTAL }, 0 },
    { FOLEY_WAKEUP, 0.0f, { &SOUND_WAKEUP }, 0 },
    { FOLEY_BUGSPRAY, 3.0f, { &SOUND_BUGSPRAY }, 0 },
    { FOLEY_SCREAM, 0.0f, { &SOUND_SCREAM }, 0 },
    { FOLEY_PAPER, 0.0f, { &SOUND_PAPER }, 0 },
    { FOLEY_MONEYFALLS, 0.0f, { &SOUND_MONEYFALLS }, 0 },
    { FOLEY_IMP, 5.0f, { &SOUND_IMP, &SOUND_IMP2 }, 0 },
    { FOLEY_HYDRAULIC_SHORT, 3.0f, { &SOUND_HYDRAULIC_SHORT }, 0 },
    { FOLEY_HYDRAULIC, 0.0f, { &SOUND_HYDRAULIC }, 0 },
    { FOLEY_GARGANTUDEATH, 3.0f, { &SOUND_GARGANTUDEATH }, 0 },
    { FOLEY_CERAMIC, 0.0f, { &SOUND_CERAMIC }, 0 },
    { FOLEY_BOSS_BOULDER_ATTACK, 0.0f, { &SOUND_BOSSBOULDERATTACK }, 0 },
    { FOLEY_CHIME, 0.0f, { &SOUND_CHIME }, 0 },
    { FOLEY_CRAZY_DAVE_SHORT, 0.0f, { &SOUND_CRAZYDAVESHORT1, &SOUND_CRAZYDAVESHORT2, &SOUND_CRAZYDAVESHORT3 }, 16 },
    { FOLEY_CRAZY_DAVE_LONG, 0.0f, { &SOUND_CRAZYDAVELONG1, &SOUND_CRAZYDAVELONG2, &SOUND_CRAZYDAVELONG3 }, 16 },
    { FOLEY_CRAZY_DAVE_EXTRA_LONG, 0.0f, { &SOUND_CRAZYDAVEEXTRALONG1, &SOUND_CRAZYDAVEEXTRALONG2, &SOUND_CRAZYDAVEEXTRALONG3 }, 16 },
    { FOLEY_CRAZY_DAVE_CRAZY, 0.0f, { &SOUND_CRAZYDAVECRAZY }, 0 },
    { FOLEY_PHONOGRAPH, 0.0f, { &SOUND_PHONOGRAPH }, 0 },
    { FOLEY_DANCER, 0.0f, { &SOUND_DANCER }, 6 },
    { FOLEY_FINAL_FANFARE, 0.0f, { &SOUND_FINALFANFARE }, 0 },
    { FOLEY_CRAZY_DAVE_SCREAM, 0.0f, { &SOUND_CRAZYDAVESCREAM }, 0 },
    { FOLEY_CRAZY_DAVE_SCREAM_2, 0.0f, { &SOUND_CRAZYDAVESCREAM2 }, 0 },
};
// Original TV effect table at 0x699DE0, in ParticleEffect order.
ParticleParamRecord gLawnParticleArray[NUM_PARTICLE_EFFECTS] =
{
    { PARTICLE_MELONSPLASH, "particles/melonimpact.xml" },
    { PARTICLE_WINTERMELON, "particles/WinterMelonImpact.xml" },
    { PARTICLE_FUME_CLOUD, "particles/FumeCloud.xml" },
    { PARTICLE_POPCORNSPLASH, "particles/PopcornSplash.xml" },
    { PARTICLE_POWIE, "particles/Powie.xml" },
    { PARTICLE_JACK_EXPLODE, "particles/JackExplode.xml" },
    { PARTICLE_ZOMBIE_HEAD, "particles/ZombieHead.xml" },
    { PARTICLE_ZOMBIE_ARM, "particles/ZombieArm.xml" },
    { PARTICLE_ZOMBIE_TRAFFIC_CONE, "particles/ZombieTrafficCone.xml" },
    { PARTICLE_ZOMBIE_PAIL, "particles/ZombiePail.xml" },
    { PARTICLE_ZOMBIE_HELMET, "particles/ZombieHelmet.xml" },
    { PARTICLE_ZOMBIE_FLAG, "particles/ZombieFlag.xml" },
    { PARTICLE_ZOMBIE_DOOR, "particles/ZombieDoor.xml" },
    { PARTICLE_ZOMBIE_TRASHCAN, "particles/ZombieTrashcan.xml" },
    { PARTICLE_ZOMBIE_NEWSPAPER, "particles/ZombieNewspaper.xml" },
    { PARTICLE_ZOMBIE_HEAD_LIGHT, "particles/ZombieHeadLight.xml" },
    { PARTICLE_POW, "particles/Pow.xml" },
    { PARTICLE_ZOMBIE_POGO, "particles/ZombiePogo.xml" },
    { PARTICLE_ZOMBIE_NEWSPAPER_HEAD, "particles/ZombieNewspaperHead.xml" },
    { PARTICLE_ZOMBIE_BALLOON_HEAD, "particles/ZombieBalloonHead.xml" },
    { PARTICLE_SOD_ROLL, "particles/SodRoll.xml" },
    { PARTICLE_GRAVE_STONE, "particles/GraveStoneRise.xml" },
    { PARTICLE_PLANTING, "particles/Planting.xml" },
    { PARTICLE_PLANTING_POOL, "particles/PlantingPool.xml" },
    { PARTICLE_ZOMBIE_RISE, "particles/ZombieRise.xml" },
    { PARTICLE_GRAVE_BUSTER, "particles/GraveBuster.xml" },
    { PARTICLE_GRAVE_BUSTER_DIE, "particles/GraveBusterDie.xml" },
    { PARTICLE_POOL_SPLASH, "particles/PoolSplash.xml" },
    { PARTICLE_ICE_SPARKLE, "particles/IceSparkle.xml" },
    { PARTICLE_SEED_PACKET, "particles/SeedPacket.xml" },
    { PARTICLE_TALL_NUT_BLOCK, "particles/TallNutBlock.xml" },
    { PARTICLE_DOOM, "particles/Doom.xml" },
    { PARTICLE_DIGGER_RISE, "particles/DiggerRise.xml" },
    { PARTICLE_DIGGER_TUNNEL, "particles/DiggerTunnel.xml" },
    { PARTICLE_DANCER_RISE, "particles/DancerRise.xml" },
    { PARTICLE_POOL_SPARKLY, "particles/PoolSparkly.xml" },
    { PARTICLE_WALLNUT_EAT_SMALL, "particles/WallnutEatSmall.xml" },
    { PARTICLE_WALLNUT_EAT_LARGE, "particles/WallnutEatLarge.xml" },
    { PARTICLE_PEA_SPLAT, "particles/PeaSplat.xml" },
    { PARTICLE_BUTTER_SPLAT, "particles/ButterSplat.xml" },
    { PARTICLE_CABBAGE_SPLAT, "particles/CabbageSplat.xml" },
    { PARTICLE_PUFF_SPLAT, "particles/PuffSplat.xml" },
    { PARTICLE_STAR_SPLAT, "particles/StarSplat.xml" },
    { PARTICLE_ICE_TRAP, "particles/IceTrap.xml" },
    { PARTICLE_SNOWPEA_SPLAT, "particles/SnowPeaSplat.xml" },
    { PARTICLE_SNOW_PEA_PUFF, "particles/SnowPeaPuff.xml" },
    { PARTICLE_SNOWPEA_TRAIL, "particles/SnowPeaTrail.xml" },
    { PARTICLE_LANTERN_SHINE, "particles/LanternShine.xml" },
    { PARTICLE_SEED_PACKET_PICKUP, "particles/Award.xml" },
    { PARTICLE_POTATO_MINE, "particles/PotatoMine.xml" },
    { PARTICLE_POTATO_MINE_RISE, "particles/PotatoMineRise.xml" },
    { PARTICLE_PUFFSHROOM_TRAIL, "particles/PuffShroomTrail.xml" },
    { PARTICLE_PUFF_SHROOM_MUZZLE, "particles/PuffShroomMuzzle.xml" },
    { PARTICLE_SEED_PACKET_FLASH, "particles/SeedPacketFlash.xml" },
    { PARTICLE_WHACK_AZOMBIE_RISE, "particles/WhackAZombieRise.xml" },
    { PARTICLE_ZOMBIE_LADDER, "particles/ZombieLadder.xml" },
    { PARTICLE_UMBRELLA_REFLECT, "particles/UmbrellaReflect.xml" },
    { PARTICLE_SEED_PACKET_PICK, "particles/SeedPacketPick.xml" },
    { PARTICLE_ICE_TRAP_ZOMBIE, "particles/IceTrapZombie.xml" },
    { PARTICLE_ICE_TRAP_RELEASE, "particles/IceTrapRelease.xml" },
    { PARTICLE_ZAMBONI_SMOKE, "particles/ZamboniSmoke.xml" },
    { PARTICLE_GLOOM_CLOUD, "particles/GloomCloud.xml" },
    { PARTICLE_ZOMBIE_POGO_HEAD, "particles/ZombiePogoHead.xml" },
    { PARTICLE_ZAMBONI_TIRE, "particles/ZamboniTire.xml" },
    { PARTICLE_ZAMBONI_EXPLOSION, "particles/ZamboniExplosion.xml" },
    { PARTICLE_ZAMBONI_EXPLOSION2, "particles/ZamboniExplosion2.xml" },
    { PARTICLE_CATAPULT_EXPLOSION, "particles/CatapultExplosion.xml" },
    { PARTICLE_MOWER_CLOUD, "particles/MowerCloud.xml" },
    { PARTICLE_BOSS_ICE_BALL_TRAIL, "particles/BossIceBallTrail.xml" },
    { PARTICLE_BLASTMARK, "particles/BlastMark.xml" },
    { PARTICLE_COIN_PICKUP_ARROW, "particles/CoinPickupArrow.xml" },
    { PARTICLE_PRESENT_PICKUP, "particles/PresentPickup.xml" },
    { PARTICLE_IMITATER_MORPH, "particles/ImitaterMorph.xml" },
    { PARTICLE_MOWERED_ZOMBIE_HEAD, "particles/MoweredZombieHead.xml" },
    { PARTICLE_MOWERED_ZOMBIE_ARM, "particles/MoweredZombieArm.xml" },
    { PARTICLE_ZOMBIE_HEAD_POOL, "particles/ZombieHeadPool.xml" },
    { PARTICLE_ZOMBIE_BOSS_FIREBALL, "particles/Zombie_boss_fireball.xml" },
    { PARTICLE_FIRE_BALL_DEATH, "particles/FireBallDeath.xml" },
    { PARTICLE_ICE_BALL_DEATH, "particles/IceBallDeath.xml" },
    { PARTICLE_ICEBALL_TRAIL, "particles/Iceball_Trail.xml" },
    { PARTICLE_FIREBALL_TRAIL, "particles/Fireball_Trail.xml" },
    { PARTICLE_BOSS_EXPLOSION, "particles/BossExplosion.xml" },
    { PARTICLE_SCREEN_FLASH, "particles/ScreenFlash.xml" },
    { PARTICLE_TROPHY_SPARKLE, "particles/TrophySparkle.xml" },
    { PARTICLE_PORTAL_CIRCLE, "particles/PortalCircle.xml" },
    { PARTICLE_PORTAL_SQUARE, "particles/PortalSquare.xml" },
    { PARTICLE_POTTED_PLANT_GLOW, "particles/PottedPlantGlow.xml" },
    { PARTICLE_POTTED_WATER_PLANT_GLOW, "particles/PottedWaterPlantGlow.xml" },
    { PARTICLE_POTTED_ZEN_GLOW, "particles/PottedZenGlow.xml" },
    { PARTICLE_MIND_CONTROL, "particles/MindControl.xml" },
    { PARTICLE_VASE_SHATTER, "particles/VaseShatter.xml" },
    { PARTICLE_VASE_SHATTER_LEAF, "particles/VaseShatterLeaf.xml" },
    { PARTICLE_VASE_SHATTER_ZOMBIE, "particles/VaseShatterZombie.xml" },
    { PARTICLE_AWARD_PICKUP_ARROW, "particles/AwardPickupArrow.xml" },
    { PARTICLE_ZOMBIE_SEAWEED, "particles/Zombie_seaweed.xml" },
    { PARTICLE_ZOMBIE_MUSTACHE, "particles/ZombieMustache.xml" },
    { PARTICLE_ZOMBIE_FUTURE_GLASSES, "particles/ZombieFutureGlasses.xml" },
    { PARTICLE_PINATA, "particles/Pinata.xml" },
    { PARTICLE_DUST_SQUASH, "particles/Dust_Squash.xml" },
    { PARTICLE_FOOTBALL_RUNNING, "particles/Dust_Foot.xml" },
    { PARTICLE_DAISY, "particles/Daisy.xml" },
    { PARTICLE_CREDITS_STROBE, "particles/Credits_Strobe.xml" },
    { PARTICLE_CREDITS_RAYS_WIPE, "particles/Credits_RaysWipe.xml" },
    { PARTICLE_CREDITS_ZOMBIE_HEAD_WIPE, "particles/Credits_ZombieHeadWipe.xml" },
    { PARTICLE_STARBURST, "particles/Starburst.xml" },
    { PARTICLE_CREDITS_FOG, "particles/Credits_fog.xml" },
    { PARTICLE_UPSELL_ARROW, "particles/UpsellArrow.xml" },
    { PARTICLE_TROPHY_DROP_DUST, "particles/TrophyDropDust.xml" },
};
TrailParamRecord gLawnTrailArray[1] = {{ 0, "particles/IceTrail.trail" }};
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
