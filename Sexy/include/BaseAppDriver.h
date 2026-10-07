// BaseAppDriver.h -- the platform driver interfaces.
//
// The binary dispatched these through an Itanium vtable stored in the first
// word. This build uses C++ virtual functions, so the compiler owns that word.
// SDLAppDriver.cpp supplies the SDL3 implementations.
// A method without a platform implementation returns the success or failure the old
// stub slot returned.

#pragma once

#include "ReconTypes.h"
#include "ReconHelpers.h"
#include "SexyString.h"

namespace Sexy {

struct InputInfo;
class MemoryImage;
class Image;
class Buffer;
class SexyAppBase;

class BaseAppDriver
{
public:
    virtual ~BaseAppDriver() {}

    static BaseAppDriver* CreateAppDriver(Sexy::SexyAppBase* theApp);

    virtual int Destroy()                                   { return 0; }
    virtual int InitAppDriver()                             { return 1; }
    virtual int Startup()                                   { return 1; }
    virtual int Terminate()                                 { return 1; }
    virtual int Start()                                     { return 1; }
    virtual int Init()                                      { return 1; }
    virtual int UpdateAppStep(bool* theContinue)            { (void)theContinue; return 1; }
    virtual int ClearUpdateBacklog(bool theClear)           { (void)theClear; return 1; }
    virtual int DrawDirtyStuff(bool theForce)               { (void)theForce; return 1; }
    virtual int Redraw(void* theClipRect)                   { (void)theClipRect; return 1; }
    virtual int Process(bool theAll)                        { (void)theAll; return 1; }
    virtual int Shutdown()                                  { return 1; }
    virtual int DoExit(int theCode)                         { (void)theCode; return 1; }
    virtual int Remove3DData(Sexy::MemoryImage* theImage)   { (void)theImage; return 1; }
    virtual int BeginPopup()                                { return 1; }
    virtual int EndPopup()                                  { return 1; }
    virtual int MsgBox(const Sexy::String& theText, const Sexy::String& theTitle, int theFlags)
    {
        (void)theText; (void)theTitle; (void)theFlags;
        return 0;
    }
    virtual int MsgBoxIntString(const Sexy::String& theText, const Sexy::String& theTitle, int theFlags)
    {
        (void)theText; (void)theTitle; (void)theFlags;
        return 0;
    }
    virtual int Popup(const Sexy::String& theText)          { (void)theText; return 0; }
    virtual int PopupIntString(const Sexy::String& theText) { (void)theText; return 0; }
    virtual int OpenURL(const Sexy::String& theURL, bool theShutdownOnOpen)
    {
        (void)theURL; (void)theShutdownOnOpen;
        return 0;
    }
    virtual int GetGameSEHInfo()                            { return 0; }
    virtual int SEHOccured()                                { return 0; }
    virtual int GetSEHWebParams(void* theParameters)        { (void)theParameters; return 0; }
    virtual int DoParseCmdLine()                            { return 0; }
    virtual int ParseCmdLine(const Sexy::String& theCommandLine) { (void)theCommandLine; return 0; }
    virtual int HandleCmdLineParam(const Sexy::String& theName, const Sexy::String& theValue)
    {
        (void)theName; (void)theValue;
        return 0;
    }
    virtual int StartLoadingThread()                        { return 1; }
    virtual int WaitForLoadingThread()                      { return 1; }
    virtual double GetLoadingThreadProgress()               { return 0.0; }
    virtual int CopyToClipboard(const Sexy::String& theText) { (void)theText; return 0; }
    virtual int GetClipboard(Sexy::String* theResult)       { (void)theResult; return 0; }
    virtual int SetCursor(int theCursorNum)                 { (void)theCursorNum; return 1; }
    virtual int GetCursor()                                 { return 0; }
    virtual int EnableCustomCursors(bool theEnable)         { (void)theEnable; return 1; }
    virtual int SetCursorImage(int theCursorNum, Sexy::Image* theImage)
    {
        (void)theCursorNum; (void)theImage;
        return 1;
    }
    virtual int EnforceCursor()                             { return 1; }
    virtual int SwitchScreenMode()                          { return 1; }
    virtual int SwitchScreenModeWindowed(bool theWindowed)  { (void)theWindowed; return 1; }
    virtual int SwitchScreenModeFull(bool theWindowed, bool theUse3D, bool theForce)
    {
        (void)theWindowed; (void)theUse3D; (void)theForce;
        return 1;
    }
    virtual int KeyDown(int theKeyCode)                     { (void)theKeyCode; return 0; }
    virtual int DebugKeyDown(int theKeyCode)                { (void)theKeyCode; return 0; }
    virtual int DebugKeyDownAsync(int theKeyCode, bool theControlDown, bool theAltDown)
    {
        (void)theKeyCode; (void)theControlDown; (void)theAltDown;
        return 0;
    }
    virtual int Is3DAccelerated()                           { return 1; }
    virtual int Is3DAccelerationSupported()                 { return 1; }
    virtual int Is3DAccelerationRecommended()               { return 1; }
    virtual int Set3DAcclerated(bool theIs3D, bool theDoCheck) { (void)theIs3D; (void)theDoCheck; return 1; }
    virtual int IsUIOrientationAllowed(int theOrientation)  { (void)theOrientation; return 1; }
    virtual int GetUIOrientation()                          { return 1; }
    virtual int IsSystemUIShowing()                         { return 0; }
    virtual int PreDrawScreen()                             { return 1; }
    virtual int CheckSignature(const Sexy::Buffer& theBuffer, const Sexy::String& theFileName)
    {
        (void)theBuffer; (void)theFileName;
        return 0;
    }
    virtual int ReloadAllResources()                        { return 1; }
    virtual int ConfigGetSubKeys(const Sexy::String& theKey, void* theSubKeys)
    {
        (void)theKey; (void)theSubKeys;
        return 0;
    }
    virtual int ConfigReadString(const Sexy::String& theKey, Sexy::String* theValue)
    {
        (void)theKey; (void)theValue;
        return 0;
    }
    virtual int ConfigReadInteger(const Sexy::String& theKey, int* theValue)
    {
        (void)theKey; (void)theValue;
        return 0;
    }
    virtual int ConfigReadBoolean(const Sexy::String& theKey, bool* theValue)
    {
        (void)theKey; (void)theValue;
        return 0;
    }
    virtual int ConfigReadData(const Sexy::String& theKey, unsigned char* theData, unsigned long* theLength)
    {
        (void)theKey; (void)theData; (void)theLength;
        return 0;
    }
    virtual int ConfigRead(const Sexy::String& theKey, unsigned long* theType, unsigned char* theData,
                           unsigned char* theDefault)
    {
        (void)theKey; (void)theType; (void)theData; (void)theDefault;
        return 0;
    }
    virtual int ConfigWriteString(const Sexy::String& theKey, const Sexy::String& theValue)
    {
        (void)theKey; (void)theValue;
        return 0;
    }
    virtual int ConfigWriteInteger(const Sexy::String& theKey, int theValue)
    {
        (void)theKey; (void)theValue;
        return 0;
    }
    virtual int ConfigWriteBoolean(const Sexy::String& theKey, bool theValue)
    {
        (void)theKey; (void)theValue;
        return 0;
    }
    virtual int ConfigWriteData(const Sexy::String& theKey, unsigned char* theData, unsigned long theLength)
    {
        (void)theKey; (void)theData; (void)theLength;
        return 0;
    }
    virtual int ConfigWrite(const Sexy::String& theKey, unsigned long theType, unsigned char* theData,
                            unsigned long theLength)
    {
        (void)theKey; (void)theType; (void)theData; (void)theLength;
        return 0;
    }
    virtual int ConfigEraseKey(const Sexy::String& theKey)  { (void)theKey; return 0; }
    virtual int ConfigEraseValue(const Sexy::String& theKey) { (void)theKey; return 0; }
    virtual int ReadFromConfig()                            { return 0; }
    virtual int WriteToConfig()                             { return 0; }
    virtual int ConfigFlush()                               { return 0; }
    virtual bool WriteBufferToFile(const Sexy::String& theFileName, const Sexy::Buffer* theBuffer)
    {
        (void)theFileName; (void)theBuffer;
        return 0;
    }
    virtual int ReadBufferFromFile(const Sexy::String& theFileName, Sexy::Buffer* theBuffer,
                                   bool theIsCompressed)
    {
        (void)theFileName; (void)theBuffer; (void)theIsCompressed;
        return 0;
    }
    virtual bool WriteBytesToFile(const Sexy::String& theFileName, const void* theData, unsigned long theLength)
    {
        (void)theFileName; (void)theData; (void)theLength;
        return 0;
    }
    virtual int GetOptimizedImage(const Sexy::String& theFileName, bool theAllowTriReps, bool theCommitBits)
    {
        (void)theFileName; (void)theAllowTriReps; (void)theCommitBits;
        return 0;
    }
    virtual int ShouldPauseUpdates()                        { return 0; }
    virtual int RehupFocus()                                { return 1; }
    virtual int StartSensor(int theSensorType)              { (void)theSensorType; return 1; }
    virtual int StopSensor(int theSensorType)               { (void)theSensorType; return 1; }
    virtual int Pause()                                     { return 1; }
    virtual int Resume()                                    { return 1; }
    virtual int GetInputInfo(Sexy::InputInfo& theInfo)      { (void)theInfo; return 0; }
    virtual int GetEditSource()                             { return 0; }
    virtual int ShowKeyboard(int theMode, const Sexy::String& theTitle, const Sexy::String& theDescription,
                             const Sexy::String& theDefault)
    {
        (void)theMode; (void)theTitle; (void)theDescription; (void)theDefault;
        return 0;
    }
    virtual int HideKeyboard()                              { return 1; }
    virtual int UpdateCursor(int theX, int theY)            { (void)theX; (void)theY; return 1; }
    virtual int IsCustomCursorSupported()                   { return 0; }
    virtual int GetMemoryInfo(void* theMemoryInfo)          { (void)theMemoryInfo; return 0; }
    virtual int TakeScreenshot(Sexy::MemoryImage* theImage) { (void)theImage; return 0; }
    virtual int DoMainLoop()                                { return 1; }
    virtual int UpdateFTimeAcc()                            { return 1; }
    virtual int DoUpdateFramesF(float theFTime)             { (void)theFTime; return 1; }
    virtual int DoUpdateFrames()                            { return 1; }
    virtual int MakeWindow()                                { return 1; }
    virtual int CreateGraphics()                            { return 1; }
    virtual int InitGraphics()                              { return 1; }
    virtual int DestroyGraphics()                           { return 1; }
    virtual int UpdateInput()                               { return 1; }
};

class IAppDriver : public BaseAppDriver
{
public:
    static IAppDriver* CreateAppDriver(Sexy::SexyAppBase* theApp);
};

class IResStreamsDriver
{
public:
    virtual ~IResStreamsDriver() {}

    static IResStreamsDriver* CreateResStreamsDriver();

    virtual int Destroy() { return 1; }
};

class IAudioDriver
{
public:
    virtual ~IAudioDriver() {}

    static IAudioDriver* CreateAudioDriver(Sexy::SexyAppBase* theApp);

    virtual int Destroy() { return 1; }
    virtual int InitAppDriver() { return 1; }
};

class IHttpDriver
{
public:
    virtual ~IHttpDriver() {}

    static IHttpDriver* CreateHttpDriver(Sexy::SexyAppBase* theApp);

    virtual int Destroy() { return 1; }
    virtual int Update() { return 1; }
};

class IFileDriver
{
public:
    virtual ~IFileDriver() {}

    virtual int Destroy() { return 1; }
    virtual int InitAppDriver(Sexy::SexyAppBase* theApp) { (void)theApp; return 1; }
};

} // namespace Sexy

using Sexy::BaseAppDriver;
using Sexy::IAppDriver;
using Sexy::IResStreamsDriver;
using Sexy::IAudioDriver;
using Sexy::IHttpDriver;
using Sexy::IFileDriver;
