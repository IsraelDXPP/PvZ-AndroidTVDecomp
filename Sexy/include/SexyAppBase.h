// SexyAppBase is the window, input, and resource root of the TV shell.
// ButtonListener, DialogListener, and InputConnectListener are its three bases.
// LawnApp adds the game-center and profile listeners; they do not live here.
// Packed to 4 so the volume doubles and the GamepadApp second base stay where
// the named members expect them. Collections are standard maps, lists, and sets.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SEXYAPPBASE_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SEXYAPPBASE_H

#include "ReconTypes.h"
#include <atomic>
// Classes this one calls a method on: the call needs the complete type, so they
// are included rather than declared.
#include "AuthManager.h"
#include "Buffer.h"
#include "ButtonListener.h"
#include "CritSect.h"
#include "Graphics.h"
#include "Image.h"
#include "InputManager.h"
#include "LogManager.h"
#include "MemoryImage.h"
#include "PixelUtil.h"
#include "ResourceRef.h"
#include "SharedImageRef.h"
#include "WidgetManager.h"
// DeviceImage.h and ResourceManager.h are deliberately NOT included.
//
// Neither type is used by value or called through here: DeviceImage does not
// appear in this header at all, and ResourceManager appears once, as the
// `ResourceManager *mResourceManager` member at +0x6CC, which needs no more
// than the `class ResourceManager;` ReconTypes.h already forward-declares.
//
// DeviceImage.h is the load-bearing one. It derives from MemoryImage, so it
// must be reached with MemoryImage already complete -- and the old include
// here made that impossible on one of the two entry orders:
//   MemoryImage.h -> ... -> SexyAppBase.h -> DeviceImage.h
// and DeviceImage.h's own `#include "MemoryImage.h"` was skipped because
// MemoryImage.h was still in progress, so `class DeviceImage : public
// MemoryImage` was parsed against an incomplete base. That is the C2504 that
// Sexy\MemoryImage.cpp reported and that Sexy\DeviceImage.cpp did not: the same
// header, two entry orders, two different states. Every file that actually
// needs a DeviceImage includes "DeviceImage.h" itself (Sexy\DeviceImage.cpp,
// Game\GamepadControls.h and six others).
#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Sexy
{

class Dialog;
class Widget;
// The gamepad-input bridge at +0x714, allocated by the constructor. Only a
// pointer to it is stored here and its methods are called from this class's own
// bodies (UpdateFrames, Cleanup), so the definition lives in Sexy\SexyAppBase.cpp
// beside those call sites -- a forward declaration is enough for the member.
class InputConnectManager;
// The platform driver at +0x2EC. Only a pointer to it is stored here, so the
// forward declaration is enough; its definition (and the reason its methods are
// not C++ `virtual`s) is in Sexy\SexyAppBase.cpp.
class BaseAppDriver;
// The platform display at +0x2E4. Its definition is Sexy\NativeDisplay.h; only a
// pointer to it is stored here.
class NativeDisplay;
// The rest of the platform drivers at +0x2E8..+0x2FC. Their definitions are in
// Sexy\BaseAppDriver.h; only pointers are stored here.
class IFileDriver;
class IAudioDriver;
class IResStreamsDriver;
class IHttpDriver;

// The sound manager at +0x4E8. ReconTypes.h has no declaration for it and the
// class is only ever used through this pointer, so a forward declaration is
// enough; Sexy\SexyAppBase.cpp defines it beside its use sites.
class SoundManager;

// ---------------------------------------------------------------------------
// The 2nd and 3rd polymorphic bases. Their vptrs are the words at +0x04 and
// +0x08 of SexyAppBase (see the header comment above). Neither interface has a
// header in this tree and neither file is in the refactor's list, so the two
// declarations live here rather than in files nothing owns.
//
// Both are one vptr and nothing else, so all this class needs from them is the
// right number of slots in the right order. What is declared below is what those
// sub-vtables actually hold; see each class.
// ---------------------------------------------------------------------------
// Sexy::DialogListener -- the base at +0x04.
//
// Its sub-vtable (@0x687880, offset-to-top -0x04 at 0x687878) starts with the two
// destructor thunks 0x2AFEED / 0x2AFF09, so a virtual destructor is correct here
// and stays. (The two entries after them, 0x336A19 / 0x336A4D at 0x687888 /
// 0x68788C, are DialogButtonPress / DialogButtonDepress override thunks. Those two
// are NOT declared here on purpose: MSVC would then treat SexyAppBase's own
// 0x1C0 / 0x1C4 entries as overrides of them and drop two slots out of the
// primary table, which the authoritative 147-entry SexyAppBaseVTable proves are
// new virtuals of SexyAppBase.)
class DialogListener
{
public:
    virtual ~DialogListener();
};

// InputConnectListener has three empty hooks and no destructor slot.
// InputConnectManager dispatches disconnect, connect, then connecting.
class InputConnectListener
{
public:
    virtual void InputDeviceDisconnected(int theDeviceIndex) {} // [0] 0x2AF6EC
    virtual void InputDeviceConnected(int theDeviceIndex) {} // [1] 0x2AF6F0
    virtual void InputDeviceConnecting(int theDeviceIndex) {} // [2] 0x2AF6F4
};

// Sexy::Ratio -- two words, 8 bytes, used three times below (+0x62C, +0x634,
// +0x63C). It already exists: Sexy\ReconHelpers.h:450 defines `class Ratio` with
// mNumerator/mDenominator and the Set(int, int) that the binary's
// _ZN4Sexy5Ratio3SetEii is, and ReconTypes.h pulls that header in.

// Thread id plus a valid bit. Thread.h owns a different thread type, so this
// stays a small record the app can copy from that type's public fields.
struct ThreadStorage
{
    SDL_ThreadID mThread;
    bool mValid; // +0x04
    unsigned char mPad[3]; // +0x05
};

// One -help row. The table is a C array of these, ended by a null name.
struct ParameterEntry
{
    const char *mName;
    const char *mValue;
    const char *mHelp;
    unsigned int mUnused;
};

class SexyAppBase : public ButtonListener, public DialogListener, public InputConnectListener
{
public:
    virtual ~SexyAppBase();

    // -----------------------------------------------------------------------
    // SexyAppBase's own new virtuals, slots [9]..[146], in binary order. The
    // left-hand numbers are the byte offsets of the authoritative
    // SexyAppBaseVTable, whose slot index is that offset / 4.
    // -----------------------------------------------------------------------
    virtual void MakeWindow(); // 0x024
    virtual void InitCursors(); // 0x028
    virtual void EnforceCursor(); // 0x02C
    virtual void ClearKeysDown(); // 0x030
    virtual void UpdateFrames(); // 0x034
    virtual void ReInitImages(); // 0x038
    virtual void DeleteNativeImageData(); // 0x03C
    virtual void DeleteExtraImageData(); // 0x040
    virtual void Delete3DImageData(); // 0x044
    virtual void LoadingThreadCompleted(); // 0x048
    virtual void Evict3DImageData(unsigned int theAmount); // 0x04C
    virtual void StartSensor(int theSensorType); // 0x050
    virtual void StopSensor(int theSensorType); // 0x054
    virtual void PauseApp(); // 0x058
    virtual void ResumeApp(); // 0x05C
    virtual MusicInterface *CreateMusicInterface(void *theContext); // 0x060
    virtual void InitHook(); // 0x064
    virtual void ShutdownHook(); // 0x068
    virtual void PreTerminate(); // 0x06C
    virtual void LoadingThreadProc(); // 0x070
    virtual void WriteToRegistry(); // 0x074
    virtual void ReadFromRegistry(); // 0x078
    virtual Dialog *NewDialog(int theDialogId, bool isModal, const Sexy::String &theHeader,
        const Sexy::String &theLines, const Sexy::String &theFooter, int theButtonMode); // 0x07C
    virtual void PreDisplayHook(); // 0x080
    virtual bool IsUIOrientationAllowed(int theOrientation); // 0x084
    virtual void UIOrientationChanged(int theOrientation); // 0x088
    virtual int GetUIOrientation(); // 0x08C
    virtual void OnFullVersionChange(); // 0x090
    virtual void LowMemoryWarning(); // 0x094
    virtual void AppEnteredBackground(); // 0x098
    virtual void AppEnteredForeground(); // 0x09C
    virtual void BeginPopup(); // 0x0A0
    virtual void EndPopup(); // 0x0A4
    virtual int MsgBox(const Sexy::String &theText, const Sexy::String &theTitle, int theFlags); // 0x0A8
    virtual int MsgBoxIntString(const Sexy::String &theText, const Sexy::String &theTitle, int theFlags); // 0x0AC
    virtual void Popup(const Sexy::String &theText); // 0x0B0
    virtual void PopupIntString(const Sexy::String &theText); // 0x0B4
    virtual void LogScreenSaverError(const Sexy::String &theError); // 0x0B8
    virtual void SafeDeleteWidget(Sexy::Widget *theWidget); // 0x0BC
    virtual void URLOpenFailed(const Sexy::String &theURL); // 0x0C0
    virtual void URLOpenSucceeded(const Sexy::String &theURL); // 0x0C4
    virtual bool OpenURL(const Sexy::String &theURL, bool shutdownOnOpen); // 0x0C8
    virtual void OpenRegisterPageWithParameters(void *theParameters); // 0x0CC
    virtual void OpenRegisterPage(); // 0x0D0
    virtual Sexy::String GetProductVersion(const Sexy::String &thePath); // 0x0D4
    virtual void SEHOccured(); // 0x0D8
    virtual Sexy::String GetGameSEHInfo(); // 0x0DC
    virtual void GetSEHWebParams(void *theParameters); // 0x0E0
    virtual void Shutdown(); // 0x0E4
    virtual void Quit(); // 0x0E8
    virtual Sexy::String FormatHelpString(); // 0x0EC
    virtual void AddParameterEntry(void *theEntry); // 0x0F0
    virtual void AddParameterEntries(void *theEntries); // 0x0F4
    virtual void DoParseCmdLine(); // 0x0F8
    virtual void ParseCmdLineVector(const std::vector<Sexy::String> &theArguments); // 0x0FC
    virtual void ParseCmdLineString(const Sexy::String &theCommandLine); // 0x100
    virtual void HandleCmdLineParam(const Sexy::String &theName, const Sexy::String &theValue); // 0x104
    virtual void HandleNotifyGameMessage(int theType, int theParameter); // 0x108
    virtual void HandleGameAlreadyRunning(); // 0x10C
    virtual void Startup(); // 0x110
    virtual void Start(); // 0x114
    virtual void Terminate(); // 0x118
    virtual void Init(); // 0x11C
    virtual void Cleanup(); // 0x120
    virtual void PreDDInterfaceInitHook(); // 0x124
    virtual void PostDDInterfaceInitHook(); // 0x128
    virtual bool ChangeDirHook(const char *theIntendedPath); // 0x12C
    virtual void PlaySample(int theSoundId); // 0x130
    virtual void PlaySampleWithPan(int theSoundId, int thePan); // 0x134
    virtual void PlaySampleSingle(int theSoundId); // 0x138
    virtual void PlaySampleSingleWithPan(int theSoundId, int thePan); // 0x13C
    virtual double GetMasterVolume(); // 0x140
    virtual double GetMusicVolume(); // 0x144
    virtual double GetSfxVolume(); // 0x148
    virtual bool IsMuted(); // 0x14C
    virtual void SetMasterVolume(double theVolume); // 0x150
    virtual void SetMusicVolume(double theVolume); // 0x154
    virtual void SetSfxVolume(double theVolume); // 0x158
    virtual void Mute(bool autoMute); // 0x15C
    virtual void Unmute(bool autoMute); // 0x160
    virtual double GetLoadingThreadProgress(); // 0x164
    virtual Image *GetTexImage(const Sexy::String &theFileName, bool commitBits); // 0x168
    virtual Image *GetImage(const Sexy::String &theFileName, bool commitBits); // 0x16C
    virtual Image *GetImageWithAttrs(const Sexy::String &theFileName, const Sexy::String &theVariant,
        const Sexy::String &theAttributes, bool commitBits); // 0x170
    virtual bool ReloadImage(Image *theImage); // 0x174
    // Both GetSharedImage overloads return a SharedImageRef by value.
    //
    // WHICH overload is which comes from the vtable, not from the old names:
    // `_ZTVN4Sexy11SexyAppBaseE` slot 94 (byte 0x178) is
    // `_ZN4Sexy11SexyAppBase14GetSharedImageERKSsS2_Pbbb` -- FIVE parameters --
    // and slot 95 (byte 0x17C) is `...ERKSsS2_S2_S2_Pb` -- SIX. There is no
    // `GetSharedImageWithAttrs` symbol anywhere in the binary; the old header had
    // the six-parameter entry at 0x178 under that invented name, which made both
    // bodies report C2511. The five-parameter form is at 0x178 and keeps the
    // name `GetSharedImage`; the six-parameter form is the attrs form at 0x17C.
    virtual SharedImageRef GetSharedImage(const Sexy::String &theFileName, const Sexy::String &theVariant, bool *isNew,
        bool commitBits, bool allowTriReps); // 0x178
    virtual SharedImageRef GetSharedImageWithAttrs(const Sexy::String &theFileName, const Sexy::String &theVariant,
        const Sexy::String &theAttributes, const Sexy::String &theAlphaFile,
        bool *isNew); // 0x17C
    virtual Image *GetImageForInput(const Sexy::String &theFileName, int theWidth, int theHeight,
        const Sexy::String &theVariant); // 0x180
    virtual void RemoveImageForInput(const Sexy::String &theFileName); // 0x184
    virtual void SwitchScreenMode(); // 0x188
    virtual int SwitchScreenMode(bool windowed); // 0x18C
    virtual void SwitchScreenModeFull(bool windowed, bool use3D, bool force); // 0x190
    virtual void SetAlphaDisabled(bool disabled); // 0x194
    virtual Dialog *DoDialog(int theDialogId, bool isModal, const Sexy::String &theHeader, const Sexy::String &theLines,
        const Sexy::String &theFooter, int theButtonMode); // 0x198
    virtual Dialog *GetDialog(int theDialogId); // 0x19C
    virtual void AddDialogWithId(int theDialogId, Dialog *theDialog); // 0x1A0
    virtual void AddDialog(Dialog *theDialog); // 0x1A4
    virtual bool KillDialogFull(int theDialogId, bool removeWidget, bool deleteWidget); // 0x1A8
    virtual bool KillDialogById(int theDialogId); // 0x1AC
    virtual bool KillDialogByPointer(Dialog *theDialog); // 0x1B0
    virtual int GetDialogCount(); // 0x1B4
    virtual void ModalOpen(); // 0x1B8
    virtual void ModalClose(); // 0x1BC
    virtual void DialogButtonPress(int theDialogId, int theButtonId); // 0x1C0
    virtual void DialogButtonDepress(int theDialogId, int theButtonId); // 0x1C4
    virtual void GotFocus(); // 0x1C8
    virtual void LostFocus(); // 0x1CC
    virtual bool IsAltKeyUsed(int theKeyCode); // 0x1D0
    virtual bool KeyDown(int theKeyCode); // 0x1D4
    virtual bool DebugKeyDown(int theKeyCode); // 0x1D8
    virtual bool DebugKeyDownAsync(int theKeyCode, bool controlDown, bool altDown); // 0x1DC
    virtual void CloseRequestAsync(); // 0x1E0
    virtual void Done3dTesting(); // 0x1E4
    virtual Sexy::String NotifyCrashHook(); // 0x1E8
    virtual bool CheckSignature(const Sexy::Buffer &theBuffer, const Sexy::String &theFileName); // 0x1EC
    virtual void PreDrawScreen(); // 0x1F0
    virtual bool DrawDirtyStuff(); // 0x1F4
    virtual void Redraw(TRect<int> *theClipRect); // 0x1F8
    virtual void InitPropertiesHook(); // 0x1FC
    virtual bool UpdateAppStep(bool *updated); // 0x200
    virtual bool UpdateApp(); // 0x204
    virtual void PopulateMessages(); // 0x208
    virtual bool ProcessMessages(std::vector<Sexy::Event> &theEvents); // 0x20C
    virtual bool ProcessMessage(Sexy::Event &theEvent); // 0x210
    virtual void CleanupHook(); // 0x214
    virtual void DrawOneFrame(); // 0x218
    virtual bool AppCanRestore(); // 0x21C
    virtual void InputStatusChanged(void *theStatus); // 0x220
    virtual void InitPreLoadWidget(); // 0x224
    virtual Sexy::Widget *CreatePreLoadWidget(); // 0x228
    virtual void FinishPreLoadWidget(); // 0x22C
    virtual void PostPreLoad(); // 0x230
    virtual void PreLoadResources(); // 0x234
    virtual void AuthFinished(bool success); // 0x238
    virtual void StartAuth(); // 0x23C
    virtual void GetMemoryInfo(int &theMemoryInfo); // 0x240
    virtual bool FrameNeedsSwapScreenImage(); // 0x244
    virtual bool TakeScreenshot(Sexy::MemoryImage &theImage); // 0x248
    // Frame counter the foley system reads. LawnApp stores the real counter.
    virtual int GetFrameCounter()
    {
        return 0;
    }

    // -----------------------------------------------------------------------
    // Methods that are NOT vtable slots. Sexy\SexyAppBase.cpp defines
    // SetCursor(int) (@0x339040), so the class has to declare it: eleven other
    // translation units call `mApp->SetCursor(n)` and were all reporting
    // C2039 "SetCursor is not a member of Sexy::SexyAppBase" against a
    // declaration that never existed. The body forwards to
    // `mAppDriver->vtable[33]`, which is the driver's own SetCursor.
    // -----------------------------------------------------------------------
    int SetCursor(int theCursorNum);

    // The two convenience overloads the binary carries beside the virtuals:
    // _ZN4Sexy11SexyAppBase10KillDialogEi @0x33628C forwards to the virtual
    // KillDialogFull(theDialogId, true, true), and
    // _ZN4Sexy11SexyAppBase10KillDialogEPNS_6DialogE @0x33629C to
    // KillDialogById(theDialog->mId). Neither is a vtable slot -- the
    // authoritative 147-entry SexyAppBaseVTable carries KillDialogFull,
    // KillDialogById and KillDialogByPointer, not these two.
    bool KillDialog(int theDialogId);
    bool KillDialog(Dialog *theDialog);

    // -----------------------------------------------------------------------
    // The rest of the class's non-virtual API. Every one of these already has a
    // body in Sexy\SexyAppBase.cpp but was never declared here, so the
    // definitions were reported as C2511 "no overloaded member function found"
    // and no other translation unit could call them. The signatures are the
    // binary's own, taken from the mangled names in the transcription banners.
    //
    // TODO(recon): a few of the .cpp definitions still spell a different
    // parameter list than the declaration below -- the transcriber lost
    // arguments in the 16-byte tail-call forwarders -- so those bodies have to
    // be retyped to match before they compile.
    // -----------------------------------------------------------------------
    void ParseCmdLine(const Sexy::String &theCommandLine);
    IFileDriver *InitFileDriver();
    bool IsScreenSaver();
    void ReadDemoBuffer(Sexy::String &theBuffer);
    void WriteDemoBuffer();
    void DemoSyncBuffer(Sexy::Buffer *theBuffer);
    void DemoSyncString(Sexy::String *theString);
    void DemoSyncInt(int *theInt);
    void DemoSyncBool(bool *theBool);
    void DemoAssertStringEqual(const Sexy::String &theString);
    void DemoAddMarker(const Sexy::String &theMarker);
    void DemoAssertIntEqual(int theInt);
    int SetCursorImage(int theCursorNum, Sexy::Image *theImage);
    int TakeScreenshot();
    void DumpProgramInfo();
    int StartLoadingThread();
    int WaitForLoadingThread();

    int RegistryWrite(
        const Sexy::String &theKey, unsigned long theType, const unsigned char *theData, unsigned long theLength);
    int RegistryWriteString(const Sexy::String &theKey, const Sexy::String &theValue);
    int RegistryWriteInteger(const Sexy::String &theKey, int theValue);
    int RegistryWriteBoolean(const Sexy::String &theKey, bool theValue);
    int RegistryWriteData(const Sexy::String &theKey, const unsigned char *theData, unsigned long theLength);
    int RegistryEraseKey(const Sexy::String &theKey);
    int RegistryEraseValue(const Sexy::String &theKey);
    int RegistryRead(
        const Sexy::String &theKey, unsigned long *theType, unsigned char *theData, unsigned long *theLength);
    int RegistryReadString(const Sexy::String &theKey, Sexy::String *theValue);
    int RegistryReadInteger(const Sexy::String &theKey, int *theValue);
    int RegistryReadBoolean(const Sexy::String &theKey, bool *theValue);
    int RegistryReadData(const Sexy::String &theKey, unsigned char *theData, unsigned long *theLength);
    bool WriteBufferToFile(const Sexy::String &theFileName, const Sexy::Buffer *theBuffer);
    bool EraseFile(const Sexy::String &theFileName);
    void RestoreScreenResolution();
    int DoExit(int theCode);
    int RehupFocus();
    int ShowMemoryUsage();
    int ProcessDeferredMessages(bool theAll);
    MemoryImage *CreateCursorFromAndMask(
        unsigned char *theAndMask, unsigned char *theXorMask, int theWidth, int theHeight);
    void ProcessSafeDeleteList();
    char *GetCurrentEvent();
    int InitDDInterface();
    void LoadResourceManifest();
    std::vector<Sexy::String> GetStringVector(const Sexy::String &theId);
    void SetBoolean(const Sexy::String &theId, bool theValue);
    void SetInteger(const Sexy::String &theId, int theValue);
    void SetDouble(const Sexy::String &theId, double theValue);
    int GetCmdLine();
    int CopyToClipboard(const Sexy::String &theText);
    Sexy::String GetClipboard();
    int GetCursor();
    int EnableCustomCursors(bool theEnable);
    int IsCustomCursorSupported();
    int CreateImage(int theWidth, int theHeight);
    MemoryImage *CreateCrossfadeImage(Sexy::Image *lpsrc, const TRect<int> &theSrcRect, Sexy::Image *theImage2,
        const TRect<int> &theDestRect, double theAlpha);
    void ColorizeImage(Sexy::Image *lpsrc, const Color &theColor);
    MemoryImage *CreateColorizedImage(Sexy::Image *lpsrc, const Color &theColor);
    int MirrorImage(Sexy::Image *lpsrc);
    int FlipImage(Sexy::Image *lpsrc);
    int RotateImageHue(Sexy::MemoryImage *theImage, int theHue);
    unsigned int HSLToRGB(int theHue, int theSaturation, int theLightness);
    unsigned int RGBToHSL(int theRed, int theGreen, int theBlue);
    void PrecacheAdditive(Sexy::MemoryImage *theImage);
    void PrecacheAlpha(Sexy::MemoryImage *theImage);
    void PrecacheNative(Sexy::MemoryImage *theImage);
    int RemoveMemoryImage(Sexy::MemoryImage *theImage);
    int Is3DAccelerationSupported();
    int Is3DAccelerationRecommended();
    void DemoSyncRefreshRate();
    int Set3DAcclerated(bool theIs3D, bool theDoCheck);
    SharedRenderTarget::Pool *GetSharedRenderTargetPool();
    int GetInputManager();
    int GetInputConnectManager();
    int CheckControllerStatus();
    int HasTouchScreen();
    void SetLocale(const Sexy::String &theLanguage, const Sexy::String &theEncoding);
    Sexy::String GetLocaleEncoding();
    int IsMainThread();
    Sexy::String GetAppDataDir();
    // GetInteger() -- `_ZN4Sexy11SexyAppBase10GetIntegerERKSs` @0x33BA0C, 0x1A
    // bytes. It looks theId up in mIntProperties and answers 0 when the key is
    // absent. Not a vtable slot. The two-argument overloads are separate symbols
    // (`...ERKSsi` @0x33BA28, `...ERKSsb` @0x33B9F0, `...ERKSsd` @0x33BACC).
    int GetInteger(const Sexy::String &theId);
    int GetInteger(const Sexy::String &theId, int theDefault);
    // GetBoolean() -- `_ZN4Sexy11SexyAppBase10GetBooleanERKSs` @0x33B9D4, the
    // same shape over mBoolProperties. Not a vtable slot.
    int GetBoolean(const Sexy::String &theId);
    int GetBoolean(const Sexy::String &theId, bool theDefault);
    // GetDouble() -- `_ZN4Sexy11SexyAppBase9GetDoubleERKSs` @0x33BAAC, 0x20 bytes,
    // over mDoubleProperties, answering 0.0 when the key is absent. Not a vtable
    // slot.
    double GetDouble(const Sexy::String &theId);
    double GetDouble(const Sexy::String &theId, double theDefault);

    // GetString returns the stored text, or the default when the key is absent.
    Sexy::String GetString(const Sexy::String &theId);
    Sexy::String GetString(const Sexy::String &theId, const Sexy::String &theDefault);
    // SetString -- `_ZN4Sexy11SexyAppBase9SetStringERKSsRKSbIiSt11char_traitsIiESaIiEE`
    // @0x33D74C. Stores the text under theId in mStringProperties. A key that is
    // an integer is also recorded in mPopLoc, and an id already present there is
    // left alone.
    void SetString(const Sexy::String &theId, const Sexy::String &theValue);
    void InitCmdLine(int argc, char **argv);
    void CleanSharedImages();
    int LoadProperties(const Sexy::String &theFileName, bool required, bool checkSig);
    // LoadProperties() -- `_ZN4Sexy11SexyAppBase14LoadPropertiesEv` @0x341A5C, the
    // no-argument form that loads mDefaultPropertiesPath. A separate symbol from
    // the three-argument overload above, and not a vtable slot.
    int LoadProperties();

    // GetLocale() -- `_ZN4Sexy11SexyAppBase9GetLocaleEv` @0x33A2E8, 48 bytes: it
    // builds a String from setLocale(nullptr), falling back to the empty string.
    // Sexy\SexyAppBase.cpp:5902 already defines it, but the class never declared
    // it, so MessageWidget.cpp's `mApp->GetLocale()` reported C2039. The
    // transcribed definition types the ARM sret buffer as `SexyAppBase *`; the
    // return value is a Sexy::String, so that definition has to be retyped.
    Sexy::String GetLocale();

    // ClearUpdateBacklog(bool relaxTime) -- `_ZN4Sexy11SexyAppBase18ClearUpdateBacklogEb`
    // @0x337C5C, not a vtable slot (the authoritative 147-entry table does not
    // carry it). Game\LawnApp.cpp's KillStoreScreen calls it.
    bool ClearUpdateBacklog(bool relaxTime);

    // FileExists(Sexy::String const&) -- `_ZN4Sexy11SexyAppBase10FileExistsERKSs`,
    // not a vtable slot. Game\LawnApp.cpp's SaveFileExists calls it.
    bool FileExists(Sexy::String const &theFileName);

    // ShowResourceError(bool) -- `_ZN4Sexy11SexyAppBase17ShowResourceErrorEb`,
    // not a vtable slot either: LawnApp adds its own at table byte 0x2B0 and the
    // binary's body is a thunk that forwards here.
    void ShowResourceError(bool showError);

    // Is3DAccelerated() -- `_ZN4Sexy11SexyAppBase15Is3DAcceleratedEv` @0x339FD8,
    // 18 bytes, already defined by Sexy\SexyAppBase.cpp:5628. Same story as
    // SetCursor and GetLocale above: the body exists but the class never declared
    // it, so every `mApp->Is3DAccelerated()` reported C2039.
    //
    // It is NOT a vtable slot, and the disassembly is what proves that: the body
    // reads the app driver out of +0x2EC and dispatches the DRIVER's own slot 44
    // (`LDR.W R0,[R0,#0x2EC] ; LDR R3,[R0] ; LDR.W R3,[R3,#0xB0] ; BLX R3`), and
    // no data word anywhere holds 0x339FD8, so it is never in a vtable. The two
    // siblings (Is3DAccelerationSupported @0x33A028, Is3DAccelerationRecommended
    // @0x33A03C) forward to the driver's slots 45 and 46 the same way and are
    // deliberately not declared yet: nothing in this tree calls them.
    int Is3DAccelerated();

    // GetDouble() -- `_ZN4Sexy11SexyAppBase9GetDoubleERKSsd` @0x33BACC, 0x20 bytes.
    // Declared with the other two overloads above; the entry here used to carry a
    // duplicate. It is NOT a vtable slot, so nothing about the layout changes.

    // HSLToRGB(uint const*, uint*, int) @0x339CD4 and RGBToHSL(uint const*, uint*, int)
    // @0x339D1C -- the array forms of the two colour converters, 0x46 bytes each.
    // They are separate symbols from the (int,int,int) forms above, which is why
    // both pairs exist. Neither is a vtable slot.
    unsigned int HSLToRGB(const unsigned int *theInput, unsigned int *theOutput, int theCount);
    unsigned int RGBToHSL(const unsigned int *theInput, unsigned int *theOutput, int theCount);

    // ReadBufferFromFile() @0x338248 and WriteBytesToFile() @0x338228 -- the same
    // story once more: both bodies already exist in Sexy\SexyAppBase.cpp (:3490
    // and :3511) but the class never declared them, so DefaultProfileMgr.cpp's
    // Load and Save had nothing to call. Neither is a vtable slot; the binary
    // reaches both with a direct BL.
    int ReadBufferFromFile(const Sexy::String &theFileName, Sexy::Buffer *theBuffer, bool dontWriteToDemo);
    bool WriteBytesToFile(const Sexy::String &theFileName, void const *theData, unsigned long theLength);

    // AddParameterEntry(Sexy::ParameterEntry const*) @0x33C358 and
    // AddParameterEntries(Sexy::ParameterEntry const*) @0x33C388 -- a SECOND pair
    // with the same names as the virtuals at 0x0F0 / 0x0F4 but taking a
    // `ParameterEntry const *` instead of `void *`, and returning int. Neither is
    // a vtable slot: the authoritative table's 0x0F0/0x0F4 entries are the
    // `void *` forms. Both bodies already exist in Sexy\SexyAppBase.cpp and
    // append to the vector at +0x728.
    int AddParameterEntry(const Sexy::ParameterEntry *theEntry);
    int AddParameterEntries(const Sexy::ParameterEntry *theEntry);

    // The constructor -- `_ZN4Sexy11SexyAppBaseC1Ev` @0x33F288, 4878 bytes, the
    // largest body in the file. Declaring it does not move the layout.
    SexyAppBase();

    // CopyImage() -- `_ZN4Sexy11SexyAppBase9CopyImageEPNS_5ImageE` @0x3415A0 and
    // its TRect overload @0x341538, 50 and 82 bytes. Both bodies already exist in
    // Sexy\SexyAppBase.cpp but the class never declared them, so ActiveFontLayer's
    // copy constructor reported C2039 "CopyImage is not a member". Neither is a
    // vtable slot: the binary reaches them with a direct BL, not a slot load.
    Sexy::MemoryImage *CopyImage(Sexy::Image *theImage);
    Sexy::MemoryImage *CopyImage(Sexy::Image *theImage, TRect<int> const &theRect);

    // AddMemoryImage() @0x339D70 and Remove3DData() @0x339E20 -- both bodies
    // already exist in Sexy\SexyAppBase.cpp but the class never declared them, so
    // MemoryImage's constructor and MemoryImage::Delete3DBuffers reported C2039
    // "is not a member". Neither is a vtable slot: the binary reaches both with a
    // direct BL, not a slot load.
    void AddMemoryImage(Sexy::MemoryImage *theMemoryImage);
    void Remove3DData(Sexy::MemoryImage *theImage);

    // -----------------------------------------------------------------------
    // Members. Offsets are the authoritative table's; the table is corroborated
    // by the raw accesses in _recon/ida/SexyAppBase.cpp, e.g. +660 = mWidgetManager,
    // +772 = mMusicInterface, +1740 = mResourceManager, +944 = mLawnMouseMode,
    // +1296 = mMouseX, +1556 = mWidescreenAware.
    // -----------------------------------------------------------------------
    int mRandSeed; // +0x00C

    Sexy::String mCompanyName; // +0x010
    Sexy::String mFullCompanyName; // +0x014
    Sexy::String mProdName; // +0x018
    Sexy::String mTitle; // +0x01C
    Sexy::String mRegKey; // +0x020
    Sexy::String mChangeDirTo; // +0x024
    Sexy::String mResourceManifestPath; // +0x028
    Sexy::String mDefaultPropertiesPath; // +0x02C

    int mRelaxUpdateBacklogCount; // +0x030
    int mPreferredX; // +0x034
    int mPreferredY; // +0x038
    int mPreferredX2; // +0x03C
    int mPreferredY2; // +0x040
    // The TV layout's stage, 1280 by 720. mWidth and mHeight stay 800 by 600,
    // which is the window the art was drawn for; these two say how far the
    // layout extends past that window.
    int mLayoutWidth; // +0x044
    int mLayoutHeight; // +0x048

    int mWidth; // +0x04C
    int mHeight; // +0x050
    int mFullscreenBits; // +0x054
    double mMusicVolume; // +0x058
    double mSfxVolume; // +0x060
    double mDemoMusicVolume; // +0x068
    double mDemoSfxVolume; // +0x070

    bool mNoSoundNeeded; // +0x078
    bool mWantFMod; // +0x079
    bool mCmdLineParsed; // +0x07A
    bool mSkipSignatureChecks; // +0x07B
    bool mStandardWordWrap; // +0x07C
    bool mAllowExtendedChars; // +0x07D
    bool mOnlyAllowOneCopyToRun; // +0x07E
    bool mInitialized; // +0x07F

    void *mMutex; // +0x080
    CritSect mCritSect; // +0x084, 8 bytes
    CritSect mSecondaryCritSect; // +0x08C, Android addition
    int mNotifyGameMessage; // +0x094
    bool mBetaValidate; // +0x098
    unsigned char mAdd8BitMaxTable[512]; // +0x099, values 0..255 then 0xFF

    WidgetManager *mWidgetManager; // +0x294

    std::map<int, Dialog *> mDialogMap; // dialogs keyed by id
    std::list<Dialog *> mDialogList; // dialogs in the order they were added
    unsigned int mPrimaryThreadId; // +0x2B8
    ThreadStorage mPrimaryThread; // +0x2BC, Android addition

    bool mSEHOccured; // +0x2C4
    bool mShutDown; // +0x2C5
    bool mExitToTop; // +0x2C6  Quit asks UpdateApp to shut down
    bool mCloseRequested; // +0x2C7  DrawOneFrame clears this before a step
    // The byte at +0x2C8 is NOT a third state flag: it is the windowed flag.
    // LawnApp's constructor sets it to 1, `WriteToRegistry` and the Fullscreen
    // line of `GetGameSEHInfo` both read it, and Board.cpp's
    // `SwitchScreenMode(..., !Is3DAccelerated(), false)` passes it as the
    // windowed argument -- all three agree it is 0 for fullscreen, 1 for
    // windowed. It is spelled out here rather than left as index 2 of a longer
    // array so the reads cannot be mistaken for state flags.
    bool mWindowed; // +0x2C8
    unsigned char mReservedAppFlags[7]; // bytes between the windowed flag and the load time
    unsigned int mTimeLoaded; // +0x2D0
    void *mPlatformWindow1; // +0x2D4
    void *mPlatformWindow2; // +0x2D8
    bool mIsScreenSaver; // +0x2DC
    bool mAllowMonitorPowersave; // +0x2DD
    bool mNoDefer; // +0x2DE
    bool mFullScreenPageFlip; // +0x2DF

    bool mTabletPC; // +0x2E0

    // The platform display driver, the object every image and font is created
    // through. Its interface is Sexy\NativeDisplay.h, which the bodies include;
    // only a pointer is stored here, so the forward declaration below is enough.
    NativeDisplay *mPlatformDriverOrQueue; // +0x2E4

    // The platform drivers. Each is one of the hand-built tables in
    // Sexy\BaseAppDriver.h, so they are released through their own slot 4 (the
    // Itanium deleting destructor), which that header spells Destroy().
    IFileDriver *mFileDriver; // +0x2E8
    BaseAppDriver *mAppDriver; // +0x2EC
    // The platform graphics driver. Its interface is Sexy\IGraphicsDriver.h;
    // only a pointer is stored here, so the forward declaration ReconTypes.h
    // already carries is enough. It is 4 bytes either way, so nothing below
    // this line moves.
    IGraphicsDriver *mDDInterface; // +0x2F0
    IAudioDriver *mAudioDriver; // +0x2F4
    IResStreamsDriver *mResStreamsDriver; // +0x2F8
    IHttpDriver *mHttpDriver; // +0x2FC
    bool mAlphaDisabled; // +0x300
    MusicInterface *mMusicInterface; // +0x304
    bool mReadFromRegistry; // +0x308
    Sexy::String mRegisterLink; // +0x30C
    Sexy::String mProductVersion; // +0x310
    Image *mCursorImages[13]; // +0x314
    int mCursorPlatformData[26]; // platform cursor words beside the 13 cursor images

    bool mLawnMouseMode; // +0x3B0
    bool mIsOpeningURL; // +0x3B1
    bool mShutdownOnURLOpen; // +0x3B2

    Sexy::String mOpeningURL; // +0x3B4
    unsigned int mOpeningURLTime; // +0x3B8
    unsigned int mLastTimerTime; // +0x3BC
    unsigned int mLastBigDelayTime; // +0x3C0
    int mVolumeAlign; // keeps the unmuted volume doubles 8-aligned under pack(4)
    double mUnmutedMusicVolume; // +0x3C8
    double mUnmutedSfxVolume; // +0x3D0
    int mMuteCount; // +0x3D8
    int mAutoMuteCount; // +0x3DC
    bool mDemoMute; // +0x3E0
    bool mMuteOnLostFocus; // +0x3E1

    std::set<MemoryImage *> mMemoryImages; // every live memory image
    CritSect mMemoryImageCritSect; // guards mMemoryImages
    std::set<PIEffect *> mPIEffects; // live particle effects
    std::set<PopAnim *> mPopAnims; // live PopAnims
    std::map<std::pair<Sexy::String, Sexy::String>, SharedImage> mSharedImages;
    bool mCleanupSharedImages; // +0x448
    std::map<Sexy::String, std::map<Sexy::String, Image *>> mImageGroups;
    std::map<int, Sexy::String> mImageIdStrings;

    int mNonDrawCount; // +0x47C
    int mFrameTime; // +0x480
    bool mIsDrawing; // +0x484
    bool mLastDrawWasEmpty; // +0x485
    bool mHasPendingDraw; // +0x486
    double mPendingUpdatesAcc; // +0x488
    double mUpdateFTimeAcc; // +0x490
    unsigned int mLastTimeCheck; // +0x498
    unsigned int mLastTime; // +0x49C
    unsigned int mLastUserInputTick; // +0x4A0
    int mSleepCount; // +0x4A4
    int mDrawCount; // +0x4A8
    int mUpdateCount; // +0x4AC
    int mUpdateAppState; // +0x4B0
    int mUpdateAppDepth; // +0x4B4
    int mUpdateDepth; // +0x4B8  how deep UpdateApp is nested
    int mUpdateFrameTime; // +0x4BC, initialised to 10
    double mUpdateMultiplier; // +0x4C0
    bool mPaused; // +0x4C8
    int mFastForwardToUpdateNum; // +0x4CC
    bool mFastForwardToMarker; // +0x4D0
    bool mFastForwardStep; // +0x4D1
    unsigned int mLastDrawTick; // +0x4D4
    unsigned int mNextDrawTick; // +0x4D8
    int mStepMode; // +0x4DC
    int mCursorNum; // +0x4E0
    int mCursorState; // +0x4E4, cursor mode beside mCursorNum

    // TODO(recon): the authoritative table types this as `AudiereSoundManager *`,
    // a subclass of Sexy::SoundManager that this tree does not declare; the base
    // interface in Sexy\SexyAppBase.cpp carries the slots this file reaches.
    SoundManager *mSoundManager; // +0x4E8

    Image *mHandCursor; // +0x4EC
    Image *mDraggingCursor; // +0x4F0
    // InitCursors stores the pointer image, its hotspot, the dragging image
    // and that image's hotspot. The words stay in the order the binary writes.
    Image *mPointerCursor; // +0x4F4
    int mPointerHotX; // +0x4F8
    int mPointerHotY; // +0x4FC
    Image *mDragCursor; // +0x500
    int mDragHotX; // +0x504
    int mDragHotY; // +0x508
    int mCursorHotY; // +0x50C
    int mMouseX; // +0x510
    int mMouseY; // +0x514
    std::list<Widget *> mSafeDeleteList; // widgets deleted after the current update

    bool mMouseIn; // +0x520
    bool mRunning; // +0x521
    bool mActive; // +0x522
    bool mMinimized; // +0x523
    bool mPhysMinimized; // +0x524
    bool mIsDisabled; // +0x525
    bool mHasFocus; // +0x526
    bool mFocusLatch; // set once focus has been delivered to the window

    int mDrawTime; // +0x528
    unsigned int mFPSStartTick; // +0x52C
    int mFPSFlipCount; // +0x530
    int mFPSDirtyCount; // +0x534
    int mFPSTime; // +0x538
    int mFPSCount; // +0x53C
    bool mShowFPS; // +0x540
    int mShowFPSMode; // +0x544
    int mScreenBltTime; // +0x548
    int mPerfCounters[8]; // +0x54C..+0x56B, frame timing slots beside the FPS counters

    bool mAutoStartLoadingThread; // +0x56C
    bool mLoadingThreadStarted; // +0x56D
    std::atomic<bool> mLoadingThreadCompleted; // published by the loading worker
    bool mLoaded; // +0x56F

    bool mYieldMainThread; // +0x570
    bool mLoadingFailed; // +0x571
    bool mLoadingThreadYield; // the loading thread should yield the main thread
    bool mLoadingThreadFailedExtra; // extra failure bit beside mLoadingFailed

    ThreadStorage mLoadingThread; // +0x574, 8 bytes

    bool mCursorThreadRunning; // +0x57C
    bool mSysCursor; // +0x57D
    bool mCustomCursorsEnabled; // +0x57E
    bool mCustomCursorDirty; // +0x57F
    bool mLastShutdownWasGraceful; // +0x580
    bool mIsWideWindow; // +0x581
    bool mWriteToCache; // +0x582, constructor default: cache writes allowed
    bool mLoadLocale; // +0x583, constructor default: locale tables load
    bool mDeferLoad; // +0x584, constructor default: do not defer the load
    bool mCacheFonts; // +0x585, ImageFont writes its cache only while this is set
    bool mCacheBuffers; // image and font caches may be written
    bool mLoadingLocaleReady; // locale tables finished loading

    std::atomic<int> mNumLoadingThreadTasks;
    std::atomic<int> mCompletedLoadingThreadTasks;

    bool mRecordingDemoBuffer; // +0x590
    bool mPlayingDemoBuffer; // +0x591
    bool mManualShutdown; // +0x592
    Sexy::String mDemoPrefix; // +0x594
    Sexy::String mDemoFileName; // +0x598
    Sexy::Buffer mDemoBuffer; // the recorded demo stream
    int mDemoLength; // +0x5B8
    int mLastDemoMouseX; // +0x5BC
    int mLastDemoMouseY; // +0x5C0
    int mLastDemoUpdateCnt; // +0x5C4
    bool mDemoNeedsCommand; // +0x5C8
    bool mDemoIsShortCmd; // +0x5C9
    int mDemoCmdNum; // +0x5CC
    int mDemoCmdOrder; // +0x5D0
    int mDemoCmdBitPos; // +0x5D4
    bool mDemoLoadingComplete; // +0x5D8
    int mCurHandleNum; // +0x5DC
    std::list<Sexy::String> mDemoMarkers; // markers written into the demo stream

    bool mDebugKeysEnabled; // +0x5E8
    bool mEnableMaximizeButton; // +0x5E9
    bool mCtrlDown; // +0x5EA
    bool mAltDown; // +0x5EB
    bool mAltEnterAllowed; // Alt+Enter may switch screen mode
    bool mReservedKeyFlags[3]; // key-state bytes beside the modifier flags

    int mSyncRefreshRate; // +0x5F0
    bool mVSyncUpdates; // +0x5F4
    bool mSoftVSyncWait; // +0x5F5
    bool mVSyncBroken; // +0x5F6
    bool mVSyncTestRunning; // a vsync-broken test is in progress
    int mVSyncBrokenCount; // +0x5F8
    unsigned int mVSyncBrokenTestStartTick; // +0x5FC
    unsigned int mVSyncBrokenTestUpdates; // +0x600
    bool mWaitForVSync; // +0x604
    bool mUserChanged3DSetting; // +0x605
    bool mAutoEnable3D; // +0x606
    bool mTest3D; // +0x607
    bool mIs3DAccelerated; // +0x608
    bool mSavedIs3D; // +0x609, the Is3D value last read from the registry

    unsigned int mMinVidMemory3D; // +0x60C
    unsigned int mRecommendedVidMemory3D; // +0x610

    bool mWidescreenAware; // +0x614
    bool mComposeToScreen; // +0x615, constructor default: present the composed frame
    bool mDisplayWide; // the current mode is the wide layout
    bool mDisplayAspectLock; // presentation keeps the window aspect
    Rect mScreenBounds; // +0x618, 16 bytes

    bool mEnableWindowAspect; // +0x628
    Ratio mWindowAspect; // +0x62C, 8 bytes
    Ratio mPresentationAspect; // +0x634, Android addition
    Ratio mWideScreenAspect; // +0x63C, Android addition
    int mScreenModeIndex; // which display mode the shell last chose
    int mScreenWidth; // +0x648
    int mScreenHeight; // +0x64C
    bool mDisplayOptions[4]; // +0x650, display switches; [0] off, [1] and [2] on

    std::map<Sexy::String, Sexy::String> mStringProperties;
    std::map<Sexy::String, bool> mBoolProperties;
    std::map<Sexy::String, int> mIntProperties;
    std::map<Sexy::String, double> mDoubleProperties;
    std::map<Sexy::String, std::vector<Sexy::String>> mStringVectorProperties;
    ResourceManager *mResourceManager; // +0x6CC

    // Integer property ids recorded by SetString. An id already present is left
    // alone, so the map is only ever inserted into.
    std::map<int, Sexy::String> mPopLoc;
    AuthManager *mAuthManager; // +0x700
    // Three owned pointers the authoritative header leaves unnamed
    // (`unkMem5[62..64]`). Nothing in this tree ever assigns them -- the Android
    // layer does -- so all that is known is the release shape Cleanup uses: the
    // first through vtable slot 4 and the other two through slot 0. They keep a
    // word type and Cleanup names that shape with a file-local helper.
    SharedRenderTarget::Pool *mRenderTargetPool; // built on first use
    void *mOwnedService; // released through its complete destructor
    void *mOwnedServiceExtra; // released through its complete destructor
    // Both pointers are typed now. They were `void *`, which forced every use
    // site to cast the member back to its own type -- the redundant-cast shape
    // R10 exists to catch, and `InputManager` is a complete type here
    // (InputManager.h is included above), so nothing is gained by erasing it.
    // Four bytes either way, so no offset below this line moves.
    InputManager *mInputManager; // +0x710
    InputConnectManager *mInputConnectManager; // +0x714
    Sexy::String mCommandLineText; // the command line joined into one string
    std::vector<Sexy::String> mCommandLine; // command-line arguments
    std::vector<ParameterEntry> mParameters; // the -help parameter table
    float mMusicPercent; // +0x734, music slider at 100
    float mSfxPercent; // +0x738, sfx slider at 100
    int mMasterPercent; // +0x73C, master slider at 100
    int mMaxFPS; // +0x740
    int mCompatFps[2]; // compat-path frame limits beside mMaxFPS
    bool mKeepCompat; // +0x74C, constructor default: keep the compat path
    Sexy::String mAppDataDir; // the writable directory, or empty until set

    // This is an Event, not a generic parameter block. `PopulateMessages`
    // @0x33B7E4 copies it into the event vector with a hand-unrolled 13-word
    // block, source `this+0x754` -> destination `[R6]`, one `LDR/STR` pair per
    // word: +0x754->+0x00, +0x758->+0x04, +0x75C->+0x08, +0x760->+0x0C,
    // +0x764->+0x10, then `ADD.W R9, R4, #0x768` / `LDM.W R9!` / `STM.W R8!`
    // moves +0x768..+0x777 to +0x14..+0x20 and the closing `LDM.W R9` /
    // `STM.W R8` moves +0x778..+0x787 to +0x24..+0x30. That is exactly Event's
    // thirteen words, and the size is unchanged at 0x34, so naming it costs
    // nothing and lets PopulateMessages be written as a real `Event` copy.
    Sexy::Event mParameterStorage; // +0x754

    std::vector<Sexy::Event> mQueuedEvents; // events waiting to be dispatched
    bool mRuntimeFlags[4]; // +0x794, packed runtime switches; LawnApp sets [1]
    Widget *mPreLoadWidget; // +0x798, built by CreatePreLoadWidget
    bool mThreadedPreload; // +0x79C, true
    bool mPreLoadStarted; // +0x79D, the preload widget phase has already run
    bool mSafeReload; // +0x79E
    bool mPreLoadFailed; // +0x79F, constructor default: preload has not failed
    // size 0x7A0
};

} // namespace Sexy

// The global spelling, which the transcribed bodies use.
using Sexy::SexyAppBase;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SEXYAPPBASE_H
