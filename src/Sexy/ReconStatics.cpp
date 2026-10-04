// ReconStatics.cpp -- the class-scope statics the link was missing.
//
// These fourteen symbols are not `extern` globals, so the generator that writes
// ReconGlobals.cpp cannot emit them: each one is a static data member and needs
// an out-of-line definition with the class-qualified name. The declarations
// live in the classes:
//
//   Board::PLANT_WIN_TIME                 Game/Board.h:843
//   Challenge::gVSSuddenDeathMode         Game/Challenge.h:106
//   Challenge::gVSZombieSpawnMode         Game/Challenge.h:107
//   DeckChooserButton::gDeckDefs          Game/DeckChooserButton.h:118  (const array)
//   LawnApp::MP_FLOWERPOT_TEST            Game/LawnApp.h:373
//   LawnApp::MP_GRAVESTONE_TARGETS_TEST   Game/LawnApp.h:374
//   Mailbox::INVALID_MESSAGE              Game/Mailbox.h:66
//   PIParticleInstance::mCount            Sexy/ReconTypes.h:276
//   Sexy::Color::White / Black            Runtime/CoreTypes.h:164-165
//   VSResultsMenu::msPlayerRecords        Game/VSResultsMenu.h:112       (private)
//
// MainMenu::msMenuOffsetX, VSSetupMenu::GAMEPAD_X_POSITIONS and
// VSSetupMenu::msNextFirstPick used to be defined here too. Their own .cpp
// files now carry the definition with the real values out of the binary
// (Game\MainMenu.cpp:35, Game\VSSetupMenu.cpp:43 and :49), so the zero-filled
// copies here were both wrong and a duplicate symbol; they are gone.
//
// Like the globals in ReconGlobals.cpp these are runtime-filled storage in the
// TV build -- the binary keeps each one as a word in .bss and the game code
// writes it -- so zero-initialised storage is the correct starting state. The
// two exceptions are Sexy::Color::White and Black: those are framework
// constants with a known value, so they are given their real colours rather
// than a zero word (a zeroed Color would be transparent black, which would
// change what every default-coloured draw produces).

// Game/Board.h, not "Board.h": a quoted include is searched for in the
// including file's own directory first, and Sexy\Board.h -- a different class,
// Sexy::Board -- shadows the Game one from here. With the shadow in place
// `Board` was incomplete, so the definition below was reported as an incomplete
// type and then mis-recovered as a plain global that collided with the
// PLANT_WIN_TIME declaration in Sexy\ReconFreeFunctions.h.
#include "Game/Board.h"
#include "Challenge.h"
#include "CoreTypes.h"
#include "DeckChooserButton.h"
#include "LawnApp.h"
#include "Mailbox.h"
#include "MainMenu.h"
#include "ReconTypes.h"
#include "VSResultsMenu.h"
#include "VSSetupMenu.h"

// --- runtime-filled words: zero is what the binary's .bss holds ------------

int Board::PLANT_WIN_TIME;

int Challenge::gVSSuddenDeathMode;
int Challenge::gVSZombieSpawnMode;

unsigned char LawnApp::MP_FLOWERPOT_TEST;
unsigned char LawnApp::MP_GRAVESTONE_TARGETS_TEST;

int PIParticleInstance::mCount;

int VSResultsMenu::msPlayerRecords[2][5];

// --- const tables: a const object needs an initialiser ---------------------

const int DeckChooserButton::gDeckDefs[16] = {};

// --- the framework's two colours, with their real values -------------------
//
// CoreTypes.h declares them `static Color White;` / `static Color Black;` and
// Color's own constructors are (r, g, b, a) and (r, g, b). White is opaque
// white and Black is opaque black in the PC framework this port follows; the
// comment on Color in CoreTypes.h records the same 0xAARRGGBB packing.

Sexy::Color Sexy::Color::White(255, 255, 255, 255);
Sexy::Color Sexy::Color::Black(0, 0, 0, 255);
