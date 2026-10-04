// BaseAppDriver.h -- the hand-built platform driver tables.
//
// These classes used to live inside SexyAppBase.cpp, which meant no other
// translation unit could call them. InputManager dispatches GetInputInfo.
//
// They are not C++ virtuals. The binary is Itanium, where a virtual destructor
// spends two vtable slots and MSVC spends one, so a class declared with virtual
// methods would put every entry one slot early. The objects are hand-built
// tables (Platform/Win32AppDriver.cpp), and the named forwarders below reach
// them through the binary's own slot number.
//
// Only the vtable pointer is contractual: everything past the first word
// belongs to the platform implementation.

#pragma once

#include "ReconTypes.h"
#include "ReconHelpers.h"
#include "SexyString.h"

namespace Sexy {

// The device description BaseAppDriver::GetInputInfo fills in. Defined in
// IInputDriver.h; only a reference is passed, so the declaration is enough.
struct InputInfo;

class BaseAppDriver
{
public:
    void* mVtable;

    static BaseAppDriver* CreateAppDriver(Sexy::SexyAppBase* theApp);

    // The deleting destructor -- the slot `delete` dispatches and the one
    // Cleanup calls. A real delete cannot be used: this is a hand-built table.
    int Destroy()                                   { return Invoke(1); }

    int InitAppDriver()                             { return Invoke(2); }
    int Startup()                                   { return Invoke(3); }
    int Terminate()                                 { return Invoke(4); }
    int Start()                                     { return Invoke(5); }
    int Init()                                      { return Invoke(6); }
    int UpdateAppStep(bool* theContinue)            { return Invoke(7, theContinue); }
    int ClearUpdateBacklog(bool theClear)           { return Invoke(8, theClear); }
    int DrawDirtyStuff(bool theForce)               { return Invoke(9, theForce); }
    int Redraw(void* theClipRect)                   { return Invoke(10, theClipRect); }
    int Process(bool theAll)                        { return Invoke(11, theAll); }
    int Shutdown()                                  { return Invoke(12); }
    int DoExit(int theCode)                         { return Invoke(13, theCode); }
    int Remove3DData(Sexy::MemoryImage* theImage)   { return Invoke(14, theImage); }
    int BeginPopup()                                { return Invoke(15); }
    int EndPopup()                                  { return Invoke(16); }
    int MsgBox(const Sexy::String& theText, const Sexy::String& theTitle, int theFlags)
    {
        return Invoke(17, &theText, &theTitle, theFlags);
    }
    int MsgBoxIntString(const Sexy::String& theText, const Sexy::String& theTitle, int theFlags)
    {
        return Invoke(18, &theText, &theTitle, theFlags);
    }
    int Popup(const Sexy::String& theText)          { return Invoke(19, &theText); }
    int PopupIntString(const Sexy::String& theText) { return Invoke(20, &theText); }
    int OpenURL(const Sexy::String& theURL, bool theShutdownOnOpen)
    {
        return Invoke(21, &theURL, theShutdownOnOpen);
    }
    int GetGameSEHInfo()                            { return Invoke(22); }
    int SEHOccured()                                { return Invoke(23); }
    int GetSEHWebParams(void* theParameters)        { return Invoke(24, theParameters); }
    int DoParseCmdLine()                            { return Invoke(25); }
    int ParseCmdLine(const Sexy::String& theCommandLine) { return Invoke(26, &theCommandLine); }
    int HandleCmdLineParam(const Sexy::String& theName, const Sexy::String& theValue)
    {
        return Invoke(27, &theName, &theValue);
    }
    int StartLoadingThread()                        { return Invoke(28); }
    int WaitForLoadingThread()                      { return Invoke(29); }
    // A double, so it needs its own body.
    double GetLoadingThreadProgress() { return InvokeSlot<double>(this, 30 * 4); }
    int CopyToClipboard(const Sexy::String& theText) { return Invoke(31, &theText); }
    int GetClipboard(Sexy::String* theResult)       { return Invoke(32, theResult); }
    int SetCursor(int theCursorNum)                 { return Invoke(33, theCursorNum); }
    int GetCursor()                                 { return Invoke(34); }
    int EnableCustomCursors(bool theEnable)         { return Invoke(35, theEnable); }
    int SetCursorImage(int theCursorNum, Sexy::Image* theImage)
    {
        return Invoke(36, theCursorNum, theImage);
    }
    int EnforceCursor()                             { return Invoke(37); }
    int SwitchScreenMode()                          { return Invoke(38); }
    int SwitchScreenModeWindowed(bool theWindowed)  { return Invoke(39, theWindowed); }
    int SwitchScreenModeFull(bool theWindowed, bool theUse3D, bool theForce)
    {
        return Invoke(40, theWindowed, theUse3D, theForce);
    }
    int KeyDown(int theKeyCode)                     { return Invoke(41, theKeyCode); }
    int DebugKeyDown(int theKeyCode)                { return Invoke(42, theKeyCode); }
    int DebugKeyDownAsync(int theKeyCode, bool theControlDown, bool theAltDown)
    {
        return Invoke(43, theKeyCode, theControlDown, theAltDown);
    }
    int Is3DAccelerated()                           { return Invoke(44); }
    int Is3DAccelerationSupported()                 { return Invoke(45); }
    int Is3DAccelerationRecommended()               { return Invoke(46); }
    int Set3DAcclerated(bool theIs3D, bool theDoCheck) { return Invoke(47, theIs3D, theDoCheck); }
    int IsUIOrientationAllowed(int theOrientation)  { return Invoke(48, theOrientation); }
    int GetUIOrientation()                          { return Invoke(49); }
    int IsSystemUIShowing()                         { return Invoke(50); }
    int PreDrawScreen()                             { return Invoke(51); }
    int CheckSignature(const Sexy::Buffer& theBuffer, const Sexy::String& theFileName)
    {
        return Invoke(52, &theBuffer, &theFileName);
    }
    int ReloadAllResources()                        { return Invoke(53); }
    int ConfigGetSubKeys(const Sexy::String& theKey, void* theSubKeys)
    {
        return Invoke(54, &theKey, theSubKeys);
    }
    int ConfigReadString(const Sexy::String& theKey, Sexy::String* theValue)
    {
        return Invoke(55, &theKey, theValue);
    }
    int ConfigReadInteger(const Sexy::String& theKey, int* theValue)
    {
        return Invoke(56, &theKey, theValue);
    }
    int ConfigReadBoolean(const Sexy::String& theKey, bool* theValue)
    {
        return Invoke(57, &theKey, theValue);
    }
    int ConfigReadData(const Sexy::String& theKey, unsigned char* theData, unsigned long* theLength)
    {
        return Invoke(58, &theKey, theData, theLength);
    }
    int ConfigRead(const Sexy::String& theKey, unsigned long* theType, unsigned char* theData,
                   unsigned char* theDefault)
    {
        return Invoke(59, &theKey, theType, theData, theDefault);
    }
    int ConfigWriteString(const Sexy::String& theKey, const Sexy::String& theValue)
    {
        return Invoke(60, &theKey, &theValue);
    }
    int ConfigWriteInteger(const Sexy::String& theKey, int theValue)
    {
        return Invoke(61, &theKey, theValue);
    }
    int ConfigWriteBoolean(const Sexy::String& theKey, bool theValue)
    {
        return Invoke(62, &theKey, theValue);
    }
    int ConfigWriteData(const Sexy::String& theKey, unsigned char* theData, unsigned long theLength)
    {
        return Invoke(63, &theKey, theData, theLength);
    }
    int ConfigWrite(const Sexy::String& theKey, unsigned long theType, unsigned char* theData,
                    unsigned long theLength)
    {
        return Invoke(64, &theKey, theType, theData, theLength);
    }
    int ConfigEraseKey(const Sexy::String& theKey)  { return Invoke(65, &theKey); }
    int ConfigEraseValue(const Sexy::String& theKey) { return Invoke(66, &theKey); }
    int ReadFromConfig()                            { return Invoke(67); }
    int WriteToConfig()                             { return Invoke(68); }
    int ConfigFlush()                               { return Invoke(69); }
    int WriteBufferToFile(const Sexy::String& theFileName, Sexy::Buffer* theBuffer)
    {
        return Invoke(70, &theFileName, theBuffer);
    }
    int ReadBufferFromFile(const Sexy::String& theFileName, Sexy::Buffer* theBuffer,
                           bool theIsCompressed)
    {
        return Invoke(71, &theFileName, theBuffer, theIsCompressed);
    }
    int WriteBytesToFile(const Sexy::String& theFileName, const void* theData, unsigned long theLength)
    {
        return Invoke(72, &theFileName, theData, theLength);
    }
    int GetOptimizedImage(const Sexy::String& theFileName, bool theAllowTriReps, bool theCommitBits)
    {
        return Invoke(73, &theFileName, theAllowTriReps, theCommitBits);
    }
    int ShouldPauseUpdates()                        { return Invoke(74); }
    int RehupFocus()                                { return Invoke(75); }
    int StartSensor(int theSensorType)              { return Invoke(76, theSensorType); }
    int StopSensor(int theSensorType)               { return Invoke(77, theSensorType); }
    int Pause()                                     { return Invoke(78); }
    int Resume()                                    { return Invoke(79); }
    int GetInputInfo(Sexy::InputInfo& theInfo)      { return Invoke(80, &theInfo); }
    int GetEditSource()                             { return Invoke(81); }
    int ShowKeyboard(int theMode, const Sexy::String& theTitle, const Sexy::String& theDescription,
                     const Sexy::String& theDefault)
    {
        return Invoke(82, theMode, &theTitle, &theDescription, &theDefault);
    }
    int HideKeyboard()                              { return Invoke(83); }
    int UpdateCursor(int theX, int theY)            { return Invoke(84, theX, theY); }
    int IsCustomCursorSupported()                   { return Invoke(85); }
    int GetMemoryInfo(void* theMemoryInfo)          { return Invoke(86, theMemoryInfo); }
    int TakeScreenshot(Sexy::MemoryImage* theImage) { return Invoke(87, theImage); }
    int DoMainLoop()                                { return Invoke(88); }
    int UpdateFTimeAcc()                            { return Invoke(89); }
    int DoUpdateFramesF(float theFTime)             { return Invoke(90, theFTime); }
    int DoUpdateFrames()                            { return Invoke(91); }
    int MakeWindow()                                { return Invoke(92); }
    int CreateGraphics()                            { return Invoke(93); }
    int InitGraphics()                              { return Invoke(94); }
    int DestroyGraphics()                           { return Invoke(95); }
    int UpdateInput()                               { return Invoke(96); }

private:
    template <class... Args>
    int Invoke(int theSlot, Args... theArgs)
    {
        return InvokeSlot<int>(this, theSlot * 4, theArgs...);
    }
};

class IAppDriver
{
public:
    static IAppDriver* CreateAppDriver(Sexy::SexyAppBase* theApp);
};

class IResStreamsDriver
{
public:
    void* mVtable;

    static IResStreamsDriver* CreateResStreamsDriver();

    int Destroy() { return InvokeSlot<int>(this, 1 * 4); }
};

class IAudioDriver
{
public:
    void* mVtable;

    static IAudioDriver* CreateAudioDriver(Sexy::SexyAppBase* theApp);

    int Destroy() { return InvokeSlot<int>(this, 1 * 4); }

    // The app constructor dispatches this slot on the audio driver.
    int InitAppDriver() { return InvokeSlot<int>(this, 2 * 4); }
};

class IHttpDriver
{
public:
    void* mVtable;

    static IHttpDriver* CreateHttpDriver(Sexy::SexyAppBase* theApp);

    int Destroy() { return InvokeSlot<int>(this, 1 * 4); }

    // The per-frame poll UpdateFrames makes.
    int Update() { return InvokeSlot<int>(this, 2 * 4); }
};

// InitFileDriver's return type. The constructor and Cleanup dispatch through
// the same hand-built table the other platform drivers use.
class IFileDriver
{
public:
    void* mVtable;

    int Destroy() { return InvokeSlot<int>(this, 1 * 4); }
    int InitAppDriver(Sexy::SexyAppBase* theApp) { return InvokeSlot<int>(this, 2 * 4, theApp); }
};

} // namespace Sexy

using Sexy::BaseAppDriver;
using Sexy::IAppDriver;
using Sexy::IResStreamsDriver;
using Sexy::IAudioDriver;
using Sexy::IHttpDriver;
