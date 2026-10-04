// Sexy::Board -- the framework demo's effect viewer, not the game's Board.
//
// `class Board : public Widget, public ButtonListener`. Widget is the primary
// base; the ButtonListener sub-object follows it, and this class's own members
// start after that. The whole class is dead code in the shipped binary: the
// vtable and the constructor have no cross-references.
//
// Overrides, in the binary's numbering (Widget.h for the base, ButtonListener.h
// for its slots): AddedToManager, RemovedFromManager, Update, Draw, KeyChar,
// MouseDown, ButtonPress, ButtonDepress. The virtual destructor is the pair
// the compiler collapses to one slot.

#pragma once

#include "ReconTypes.h"
#include "ButtonListener.h"
#include "Widget.h"

// EffectSystem, Reanimation and the definition types are global classes.
// Forward-declaring them inside Sexy would create unrelated incomplete types.
class EffectSystem;
class Reanimation;
class ReanimatorDefinition;
class TodParticleDefinition;
class TodParticleSystem;
class TrailDefinition;

namespace Sexy {

class Font;
class HyperlinkWidget;
class SexyAppBase;
class WidgetManager;

class Board : public Widget, public ButtonListener
{
public:
    // The binary constructor takes a GameApp*, the framework demo's app class.
    // This tree does not model GameApp; every member the bodies read through
    // it is a SexyAppBase member, so the parameter is the base they really are.
    Board(Sexy::SexyAppBase* theApp);
    virtual ~Board();

    virtual void AddedToManager(WidgetManager* theWidgetManager);
    virtual void RemovedFromManager(WidgetManager* theWidgetManager);
    virtual void Update();
    virtual void Draw(Sexy::Graphics* g);
    virtual int KeyChar(char theChar);
    virtual void MouseDown(int x, int y, int theClickCount);

    virtual void ButtonPress(int theId);
    virtual void ButtonDepress(int theId);

    // Returns non-zero when the name resolved to an existing file, and rewrites
    // theFileName with the extension it found.
    int MakeFullImageFileName(Sexy::String theFileName);
    int TryToLoadParticle();
    int TryToLoadTrail();
    int TryToLoadReanimation();
    void ClearDefs();
    void TryToLoadEffect();
    int ClearEffects();
    void LoadEffectDef();
    void DrawPerfText(Sexy::Graphics* g);
    bool DefinitionHasChanged();
    int NextAnim();
    int Blink();
    void FadeOutParticles();
    Reanimation* GetBuddyReanimation();
    void MakeAllBuddyImages();
    void NewEffectDialog();
    double RunATest();
    void ScaleEffects(float theScale);
    int DoATestSet(float theScale);
    int DoReanimatorTest();
    void MakeBuddyImageStrip();
    void MakeBuddyImage();

    Sexy::HyperlinkWidget* mLoadNewEffectButton;
    Sexy::SexyAppBase* mApp;
    TodParticleDefinition* mParticleDef;
    TrailDefinition* mTrailDef;
    ReanimatorDefinition* mReanimatorDef;
    // [0] is mFileName's timestamp, [1 + i] is mImageNames[i]'s. The bodies
    // index it as one array.
    int mFileDates[101];
    int mResourceDate;
    int mNumImages;
    Sexy::String mFileName;
    Sexy::String mImageNames[100];
    EffectSystem* mEffectSystem;
    Sexy::Font* mFont;
    int mUnusedWord;
    int mFrameCounter;
    Sexy::String mTrackName;
};

} // namespace Sexy
