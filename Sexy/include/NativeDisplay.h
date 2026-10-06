// NativeDisplay.h -- the platform display/graphics-driver interface.
//
// SexyAppBase stores this driver at +0x2E4. Until now the member was typed
// `void *` because no header in this tree declared the class, and the call sites
// spelled the calls as raw vtable-slot invocations. libGameMain115.h confirms
// the name (`struct Sexy::NativeDisplay;`), and the binary's own table gives the
// slot list:
//
//   _ZTVN4Sexy13NativeDisplayE  @0x68D700
//
// Slot k sits at `vtable + 8 + 4*k` (offset-to-top at +0, typeinfo at +4), and
// the ARM Thumb pointers have bit 0 set, so it is cleared before resolving a
// name. The table is 45 entries long; entry 45 is the next symbol, not a
// function.
//
// Three ranges are `__cxa_pure_virtual` in the class's own table AND in every
// derived table the binary emits, so the binary never reaches them. They are
// declared only to keep the later offsets right, and are named `UnkNN` after
// their byte offset, matching the convention the rest of this tree uses for
// slots whose purpose is not recoverable.
//
// The right-hand numbers below are the BINARY's byte offsets. Itanium spends
// two slots on a virtual destructor and MSVC spends one, so the declared order
// is one slot shorter than those numbers -- the same note Image.h carries. The
// relative order of every other slot is unchanged, and this class is only ever
// called through, never constructed here.
//
// This is an interface: the tree never constructs a NativeDisplay, it only
// calls through a pointer, so the slots are declared and not defined.

#pragma once

#include "ReconTypes.h"
#include "SexyString.h"

namespace Sexy {

class Graphics;
class Image;
class Font;
class ImageFont;
class MemoryImage;
class SexyAppBase;
class EditSink;

// The parameter and return types of three slots that no call site in this tree
// reaches. The binary names them but the tree has no reconstruction of them yet.
struct InputInfo;
class DelayedWork;

class NativeDisplay
{
public:
    virtual ~NativeDisplay() {}                                                   //   0

    virtual void Init() {}                                                      //   8
    virtual void Reinit() {}                                                    //  12
    // Returns a Sexy::FreeTypeFont*, not an ImageFont*: the body @0x4064E0
    // allocates 0x20 bytes and constructs a FreeTypeFont in them, and
    // _ZTIN4Sexy12FreeTypeFontE @0x68EA28 records Sexy::Font as its base. The
    // tree has no FreeTypeFont header, so the slot keeps Sexy::Font*, which is
    // the type every call site stores the result into.
    virtual Font * CreateFont(SexyAppBase * theApp, const String & theName,
                              int theSize, bool aBold, bool aItalic,
                              bool aUnderline) { return nullptr; }            //  16
    virtual Image * CreateImage(SexyAppBase * theApp, int theWidth,
                                int theHeight) { return nullptr; }              //  20

    // Slots 24..32 are pure here and implemented by GLDisplay / AndroidDisplay,
    // whose tables are `_ZTVN4Sexy9GLDisplayE` and `_ZTVN4Sexy14AndroidDisplayE`
    // @0x68EAE0. They are NOT dead: SexyAppBase::Cleanup calls 24 and 28 on this
    // pointer to blank the screen before shutting the display down.
    virtual Image * GetScreenImage() { return nullptr; }                                         //  24
    virtual void Redraw(TRect<int> * theRect) {}                                //  28
    virtual void RemapMouse(int & theX, int & theY) {}                          //  32

    virtual bool IsPixelFormatSupported(PixelFormat theFormat) { return false; }               //  36
    virtual void UpdateCursor(int theX, int theY) {}                            //  40
    virtual void DrawCursor(Graphics * theGraphics) {}                          //  44
    virtual void EnableCursor(bool theEnable) {}                                //  48
    virtual void SetCursorImage(Image * theImage, int theX, int theY) {}        //  52
    virtual void SetCursorPos(int theX, int theY) {}                            //  56
    virtual bool HasEvent() { return false; }                                                  //  60
    virtual bool GetEvent(Event & theEvent) { return false; }                                  //  64
    // InputManager::DumpStatus @0x3B74AC branches on the value left in r0.
    virtual int GetInputInfo(InputInfo & theInfo) { return 0; }                 //  68
    virtual bool Is3DAccelerated() { return false; }                                           //  72
    virtual bool Is3DAccelerationSupported() { return false; }                                 //  76
    virtual bool Is3DAccelerationRecommended() { return false; }                               //  80
    virtual bool IsFullcreenSupported() { return false; }                                      //  84
    virtual bool IsWindowModeSupported() { return false; }                                     //  88
    virtual bool CanFullscreen() { return false; }                                             //  92
    virtual bool CanWindowed() { return false; }                                               //  96

    virtual void Create3DData(MemoryImage * theImage) {}                        // 100
    virtual void Remove3DData(MemoryImage * theImage) {}                        // 104

    virtual void Ensure3DData(MemoryImage * theImage, bool a, bool b) {}        // 108
    // RecoverBits returns a bool: the base body @0x406444 is `MOVS R0,#0; BX LR`,
    // and its only call site (MemoryImage.cpp) tests the result.
    virtual bool RecoverBits(MemoryImage * theImage) { return false; }                         // 112
    virtual void Update() {}                                                    // 116
    virtual unsigned int AllocTexMemSpace(unsigned int theSize) { return 0; }              // 120
    virtual void FreeTexMemSpace(unsigned int theHandle) {}                     // 124
    virtual void EnsureTexMemSpace(unsigned int theSize) {}                     // 128
    virtual void ShrinkTexMemSpace() {}                                         // 132
    virtual void FreezeTextueReleasing() {}                                     // 136
    virtual void ThawTextureReleasing() {}                                      // 140
    virtual bool HasTouchScreen() { return false; }                                            // 144
    // `theMode` is the binary's `Sexy::KeyboardMode`; the tree has no
    // reconstruction of that enum yet, and no call site passes one.
    virtual void ShowKeyboard(int theMode, const String & theTitle,
                              const String & theText, const String & theHint) {} // 148
    virtual void HideKeyboard() {}                                              // 152
    virtual EditSink * GetEditSource() { return nullptr; }                                       // 156
    virtual void StartSensor(int theSensor) {}                                  // 160
    virtual void StopSensor(int theSensor) {}                                   // 164
    virtual void Pause() {}                                                     // 168
    virtual void Resume() {}                                                    // 172

    // The data the binary keeps past the vtable. Two CritSects occupy +0x04 and
    // +0x08 (NativeDisplay::NativeDisplay @0x406964). The byte at +0x0C is
    // stored 0 by that constructor (@0x4069C4) and 1 by
    // AndroidDisplay::AndroidDisplay (@0x43F874). PurgeBits @0x31DCA4 reads it:
    // non-zero means keep the software buffers unless the image can be
    // reloaded from its path. The port constructs this base, so the
    // member starts at the constructor's 0.
    unsigned char mReservedBeforeRetainBits[8];                                   // +0x04
    bool mRetainSoftwareBits = false;                                             // +0x0C
    // TitleScreen scales the logos against the screen height at byte 0x44.
    // 55 reserved bytes keep that word there.
    unsigned char mReservedToScreenHeight[55];                                    // +0x0D
    int mScreenHeight = 0;                                                        // +0x44
    // Byte +96 is the wide-screen flag: LawnApp::HardwareInit stores 1 there,
    // and every layout branch reads it back. 24 reserved bytes keep it there.
    unsigned char mReservedToWideScreen[24];                                      // +0x48
    bool mWideScreen = false;                                                     // 96
};

} // namespace Sexy
