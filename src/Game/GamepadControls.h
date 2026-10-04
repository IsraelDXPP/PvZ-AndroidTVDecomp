// GamepadControls -- reconstructed from libGameMain.so.
//
// Inheritance: `class GamepadControls : public BaseGamepadControls`
// (BaseGamepadControls : public Sexy::GamepadListener, public GameObject).
// The authoritative header PvZ\Lawn\GamepadControls.h declares the chain, and the
// destructor body (GamepadControls.cpp:72-77) restores the GameObject sub-object's
// vptr at +4 and then calls `~GameObject()` on it -- proof that the GameObject
// sub-object sits at +0x04, exactly as BaseGamepadControls lays it out.
//
// SIZE 0x108 -- NOT the transcriber's 0x900:
//   * Board.cpp:18114 allocates `(GamepadControls *)operator new(0x108u)`;
//   * the authoritative header closes with "66 ints" = 264 = 0x108;
//   * BaseGamepadControls (0xAC) + the 23 own words below add up to 0x108.
//
// ---------------------------------------------------------------------------
// VTABLE, 18 slots (the authoritative GamepadControlsVTable, which is
// BaseGamepadControlsVTable with the two unnamed pure virtuals filled in):
//   [  0] 0x000  OnButtonDown             [  9] 0x024  MakeParentGraphicsFrame
//   [  1] 0x004  OnButtonUp               [ 10] 0x028  EnterState
//   [  2] 0x008  OnGameAxisMove           [ 11] 0x02C  ExitState
//   [  3] 0x00C  OnKeyDown                [ 12] 0x030  UpdateStates
//   [  4] 0x010  OnKeyUp                  [ 13] 0x034  GotoState
//   [  5] 0x014  Update                   [ 14] 0x038  GetSnapToGridPos
//   [  6] 0x018  BeginDraw                [ 15] 0x03C  GetSnapToGridXY
//   [  7] 0x01C  Draw                     [ 16] 0x040  ~GamepadControls (D1)
//   [  8] 0x020  EndDraw                  [ 17] 0x044  ~GamepadControls (D0; MSVC: 1 slot)
//
// SLOT ALIGNMENT STATUS -- read this before "fixing" the declarations below.
// BaseGamepadControls.h used to declare slots [0] OnButtonDown, [3] OnKeyDown and
// [6] BeginDraw with `Board **`, `Challenge *` and `int` returns, which no longer
// matched this class's bodies and made every same-named declaration here an
// implicit override with a different return type (C2555). An erroneous class
// declaration collapses sizeof() to 1, so the layout check at the bottom of this
// file could not pass; the member table itself was never wrong (measured with the
// conflicting declarations neutralised, sizeof(GamepadControls) is exactly 0x108).
// The base now carries the authoritative return types
// (PvZ\Lawn\BaseGamepadControls.h:68-74: slot [0] void, slot [3] bool, slot [6] bool)
// and this class declares the four real overrides, so no C2555 is emitted here.
// The bodies in GamepadControls.cpp still spell OnKeyDown `int` and EndDraw `int`
// and must be retyped to `bool` / `void` (EndDraw's `void` comes from
// GameObject::EndDraw, which takes Sexy::Graphics *).

#pragma once

#include "ReconTypes.h"
// Classes this one calls a method on: the call needs the
// complete type, so they are included rather than declared.
#include "BaseGamepadControls.h"
#include "Board.h"
#include "Challenge.h"
#include "Coin.h"
#include "CursorObject.h"
#include "DeviceImage.h"
#include "GameObject.h"
#include "GamepadApp.h"
#include "Graphics.h"
#include "Graphics3D.h"
#include "GridItem.h"
#include "Image.h"
#include "Reanimation.h"
#include "SeedBank.h"
#include "SeedPacket.h"
#include "SexyAppBase.h"
#include "SexyTransform2D.h"
#include "TodParticleSystem.h"
#include "ZenGarden.h"
#include "Zombie.h"

// LawnApp is not named in this file, so the include was pure cycle
// (LawnApp.h <-> GamepadControls.h). Sexy\ReconTypes.h already declares it.
class LawnApp;

class GamepadControls : public BaseGamepadControls
{
public:
    // ---- own members, +0xAC..+0x107 (authoritative ordinals 43..65) ----
    float mCursorLabelLiftOffset;         // +0xAC
    ParticleSystemID mSelectorParticleID; // +0xB0
    int mSelectedSeedIndex;               // +0xB4
    SeedType mSelectedSeedType;           // +0xB8
    bool mIsZombie;                       // +0xBC (188)
    bool mCanPickUp;                      // +0xBD (189)
    int mSelectedUpgradableType;          // +0xC0
    int mCobCannonPlantIndexInList;       // +0xC4
    bool mIsCobCannonSelected;            // +0xC8 (200)
    float mCanPickUpFade;                 // +0xCC
    ReanimationID mPreviewReanimID1;      // +0xD0
    ReanimationID mCobCannonReanimID;     // +0xD4
    ReanimationID mPreviewReanimID3;      // +0xD8
    int mCobCannonAnimCounter;            // +0xDC
    ReanimationID mPreviewReanimID4;      // +0xE0
    SeedType mPreviewingSeedType;         // +0xE4
    Sexy::Image * mPreviewImage;          // +0xE8
    int mButterZombieID;                  // +0xEC, TODO(recon): ZombieID in the authoritative header
    int mDigIndicatorStartCounter;        // +0xF0
    bool mIsShowingDigIndicator;          // +0xF4 (244)
    bool mIsInShopSeedBank;               // +0xF5 (245)
    int mSelectedShopSeedIndex;           // +0xF8

    // ---- the 18 slots, declared in binary order --------------------------
    // Every one of these is a real override of a BaseGamepadControls virtual
    // (or, for EndDraw, of GameObject's); the return types are the authoritative
    // ones from PvZ\Lawn\GamepadControls.h:34-53.
    virtual void OnButtonDown(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 0] 0x000
    virtual void OnButtonUp(int theButton, int thePlayerIndex, unsigned int theFlags); // [ 1] 0x004
    virtual void OnGameAxisMove(int theAxis, int theValue, int thePlayerIndex); // [ 2] 0x008
    virtual bool OnKeyDown(Sexy::KeyCode theKey, unsigned int theFlags);                 // [ 3] 0x00C
    virtual int OnKeyUp(Sexy::KeyCode theKey, unsigned int theFlags); // [ 4] 0x010
    virtual int Update(float theDeltaTime);                          // [ 5] 0x014
    virtual bool BeginDraw();                                        // [ 6] 0x018
    virtual void EndDraw(Sexy::Graphics * g);                        // [ 8] 0x020
    virtual void MakeParentGraphicsFrame();                          // [ 9] 0x024
    virtual int UpdateStates(float theDeltaTime);                    // [12] 0x030
    virtual ~GamepadControls();                                      // [16] 0x040

    // ---- methods the binary has that are NOT vtable overrides --------------
    // Draw takes a Sexy::Graphics * while the base's slot [7] Draw takes none,
    // so it hides the base name instead of replacing the slot.
    void Draw(Sexy::Graphics * g);

    // ---- methods the binary has but the authoritative header does not list ----
    // The transcription gave several of these the return type of whatever the
    // decompiler found in r0 at the end of the body (a stale pointer, a stale
    // int). None of the callers reads a result, so they are void here.
    void GotoSelectingState(int theSeedIndex);
    void InvalidatePreviewReanim();
    int DrawHighlightPolys(Sexy::Graphics * g);
    float DrawReticuleHorizontal(Sexy::Graphics *g, int theX, int theY);
    int DrawReticuleVertical(Sexy::Graphics *g, int theX, int theY);
    int DrawDigIndicator(Sexy::Graphics *g, float theX, float theY, float theProgress);
    SeedBank * GetSeedBank();
    int PlantShopPacket();
    int FreeUseShopPacketAndBack();
    void DrawPreview(Sexy::Graphics * g);
    int UpdateSeedSelect(float theDeltaTime);
    int IsMagnetized(CoinType theCoinType);
    int AddSelectorParticle(int theSeedIndex);
    unsigned int IncSelectedSeed();
    unsigned int DecSelectedSeed();
    int IncShopSeed();
    int DecShopSeed();
    int otherPlayerDownSelectedSeed();
    int DownSelectedSeed();
    int otherPlayerUpSelectedSeed();
    int UpSelectedSeed();
    // The binary returns the direction through the sret pointer (ARM r0), so the
    // real return type is the same 8-byte pair BaseGamepadControls's
    // GetSnapToGridPos hands back; the transcription mistook that destination for
    // `this` and wrote the two floats over the object's vptr. Call sites store the
    // pair into mGamepadVelocityLeftX/Y.
    SnapToGridPosition GetNextGridDirection(int theDirection, int theX, int theY);
    int ButtonDownPlantTest(int theButton, int thePlayerIndex, unsigned int theFlags);
    int ButtonDownFireCobcannonTest();
    // Pieces of OnButtonDown. Each one is the work of one button family.
    bool HandleDPadButtons(int theButton);
    void HandlePauseButton();
    void HandleCancelButton();
    void HandleButterButton();
    void MagnetizeNearbyCoins();
    bool HandleVersusZombiePlant(int thePlantAccepted);
    void PlaceVersusGrave(SeedPacket *thePacket, int theCost);
    bool HandleVersusMoundUpgrade(int thePlantAccepted);
    bool HandleNormalPlant(int thePlantAccepted);
    void HandleShovelButton();
    void UpdateCursorReanims();
    void AddCursorReanim();
    int PickupUsableSeedPacket(Coin * theCoin);
    void UpdatePreviewReanim();
    GamepadControls(Board * theBoard, int thePlayerIndex1, int thePlayerIndex2);
};

// Layout check: BaseGamepadControls (0xAC) + 0x5C of own members = 0x108.

