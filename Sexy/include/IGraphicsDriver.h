// IGraphicsDriver is the platform graphics driver SexyAppBase stores as
// mDDInterface, plus the surface and 3D device that driver hands back.
//
// IGraphicsDriver is a standalone interface. The concrete driver fills the
// same slots and then adds its own. Slot names below are the ones that
// driver implements. Six of them have empty default bodies in the binary
// (EnableCursor, PreDrawScreen, Pause, Resume, GetRenderEffectFormat,
// GetShaderLanguage); they stay pure here because nothing in this tree
// instantiates the interface.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_IGRAPHICSDRIVER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_IGRAPHICSDRIVER_H

#include "ReconTypes.h"
// RenderDevice3D derives from Sexy::RenderDevice, which is declared in
// MemoryImage.h. The base has to be complete to lay the derived vtable out, so
// the include is required rather than a forward declaration.
#include "MemoryImage.h"

namespace Sexy
{

class DeviceImage;
class Mesh;
class MeshListener;
// Defined further down this header; IGraphicsDriver::GetRenderDevice3D returns
// it by pointer, so the declaration has to come first.
class RenderDevice3D;

// ---------------------------------------------------------------------------
// IDeviceSurface
//
// The object IGraphicsDriver::CreateDeviceSurface() returns. DeviceImage
// holds it. This build's driver returns null from CreateDeviceSurface, so
// the slot list is the one DeviceImage.cpp calls through. The reserved
// slots are never called; they exist so GetBits, SetSurface and GetSize
// stay at the indices DeviceImage uses.
// ---------------------------------------------------------------------------
class IDeviceSurface
{
public:
    // The surface's second word. DeviceImage::GenerateDeviceSurface seeds it
    // from the image's create flags, the same word the image stores at its
    // own +0x04. It is part of the interface's layout.
    int mCreateFlags;

    virtual ~IDeviceSurface(); // [ 0]

    // Lock(theLockInfo, theFlags) -- hand the caller the surface's pixels. The
    // description DeviceImage passes is its +0xF4 block, which the driver fills
    // in with the pitch, the pixel pointer and the format.
    virtual bool Lock(void *theLockInfo, int theFlags) = 0; // [ 2]
    virtual void Unlock() = 0; // [ 3]
    // Both sides of a rebind compare versions to decide whether the surface is
    // still usable: DeviceImage::SetSurface against IGraphicsDriver::GetVersion.
    virtual int GetVersion() = 0; // [ 4]
    // Bind the surface to the image whose pixels it backs.
    virtual int Attach(DeviceImage *theImage) = 0; // [ 5]
    virtual void ReservedLockSlot() = 0; // [ 6]
    virtual void ReservedUnlockSlot() = 0; // [ 7]
    virtual void ReservedBindSlot() = 0; // [ 8]
    virtual void ReservedReleaseSlot() = 0; // [ 9]
    virtual unsigned int *GetBits(DeviceImage *theImage) = 0; // [10]
    virtual void ReservedBitsSlot() = 0; // [11]
    virtual void ReservedFormatSlot() = 0; // [12]
    // SetSurface(theNativeSurface) -- point the surface at the native handle
    // DeviceImage::SetSurface was given.
    virtual void SetSurface(void *theNativeSurface) = 0; // [13]
    // GetSize(theWidth, theHeight) -- report the surface's geometry back into
    // the image, so GetWidth/GetHeight stay in step with the native allocation.
    virtual int GetSize(int &theWidth, int &theHeight) = 0; // [14]
};

// ---------------------------------------------------------------------------
// IGraphicsDriver
//
// The interface behind SexyAppBase::mDDInterface. 39 slots, named from the
// concrete driver. The six slots that have empty default bodies in the
// binary stay pure here: nothing instantiates this interface, and a pure
// declaration keeps the table from being emitted with a null entry.
// ---------------------------------------------------------------------------
class IGraphicsDriver
{
public:
    // The render-mode selector. The binary's parameter type is
    // `IGraphicsDriver::ERenderMode`; no enumerator of it has a symbol, and the
    // one function that could have revealed the values
    // (GetRenderModeString) formats a string instead of switching on
    // them, so the values are not recoverable. A typedef keeps the 4-byte ABI
    // and the name, and lets call sites keep passing plain integers.
    typedef int ERenderMode;

    virtual ~IGraphicsDriver(); // [ 0]

    virtual bool Is3D() = 0; // [ 2]
    virtual int GetVersion() = 0; // [ 3]
    virtual unsigned int GetRenderModeFlags() = 0; // [ 4]
    virtual void SetRenderModeFlags(unsigned int theFlags) = 0; // [ 5]
    virtual int GetRenderMode() = 0; // [ 6]
    virtual void SetRenderMode(ERenderMode theMode) = 0; // [ 7]
    virtual const char *GetRenderModeString(
        ERenderMode theMode, unsigned int theFlags, bool theFlagA, bool theFlagB) = 0; // [ 8]
    virtual void AddDeviceImage(DeviceImage *theImage) = 0; // [ 9]
    virtual void RemoveDeviceImage(DeviceImage *theImage) = 0; // [10]
    virtual void Remove3DData(MemoryImage *theImage) = 0; // [11]
    virtual DeviceImage *GetScreenImage() = 0; // [12]
    virtual void Init() = 0; // [13]
    virtual void WindowResize(int theWidth, int theHeight) = 0; // [14]
    virtual void Redraw(TRect<int> *theRect) = 0; // [15]
    virtual void RemapMouse(int &theX, int &theY) = 0; // [16]
    virtual void SetCursorImage(Image *theImage, int theHotX, int theHotY) = 0; // [17]
    virtual void SetCursorPos(int theX, int theY) = 0; // [18]
    virtual void EnableCursor(bool theEnabled) = 0; // [19]
    virtual void RemoveShader(const void *theShader) = 0; // [20]
    virtual IDeviceSurface *CreateDeviceSurface() = 0; // [21]
    virtual void *GetNativeDisplayInfo() = 0; // [22]
    virtual RenderDevice *GetRenderDevice() = 0; // [23]
    virtual RenderDevice3D *GetRenderDevice3D() = 0; // [24]
    virtual float GetAspectRatio() = 0; // [25]
    virtual int GetDisplayWidth() = 0; // [26]
    virtual int GetDisplayHeight() = 0; // [27]
    virtual TRect<int> GetPresentationRect() = 0; // [28]
    virtual void PreDrawScreen() = 0; // [29]
    virtual void *GetCritSect() = 0; // [30]
    virtual void LoadMesh(const String &theFile, MeshListener *theListener) = 0; // [31]
    virtual void AddMesh(Mesh *theMesh) = 0; // [32]
    virtual void RemoveMesh(Mesh *theMesh) = 0; // [33]
    virtual void *GetMetrics() = 0; // [34]
    virtual void Pause() = 0; // [35]
    virtual void Resume() = 0; // [36]
    // Both return a Sexy::String by value: the platform effect-file extension
    // (".popfx") and the platform path prefix (empty on this build).
    virtual Sexy::String GetRenderEffectFormat() = 0; // [37]
    virtual Sexy::String GetShaderLanguage() = 0; // [38]
};

// ---------------------------------------------------------------------------
// RenderDevice3D
//
// The 3D half of the render device, returned by
// IGraphicsDriver::GetRenderDevice3D(). It derives from Sexy::RenderDevice
// (declared in MemoryImage.h), whose 29 slots come first. The four methods
// RenderDevice itself implements (IsPixelFormatSupported, DrawRect,
// FillScanLines, TakeScreenshot) sit at those same indices, which is what
// fixes RenderDevice as the primary base.
//
// Slots 29..78 are RenderDevice3D's own, named from the concrete device.
// ReloadEffects, SetBltFilter and SetDrawPrimFilter are the three the
// interface itself implements. Everything else is pure.
//
// Two parameter types have no declaration anywhere in this tree and are spelled
// as the 4-byte values they are, with the binary's name in the comment:
//
//   [33] EInfoString  -- the nested enum RenderDevice3D::EInfoString
//   [41][42] RenderSurface -- the second out-parameter's pointee
//
// Everything else uses the real type: SexyVertex / SexyMatrix4 /
// RenderEffectDefinition / Graphics3D_LightColors are forward-declared by
// ReconTypes.h and only ever appear behind a pointer or reference, and the
// Graphics3D_E* typedefs and SexyVector3 / SexyVertex2D are already defined
// there and in CoreTypes.h.
// ---------------------------------------------------------------------------
class RenderDevice3D : public RenderDevice
{
public:
    typedef int EInfoString; // RenderDevice3D::EInfoString

    virtual void Flush(unsigned long theFlags) = 0; // [29]
    virtual void Present(const TRect<int> *theSourceRect, const TRect<int> *theDestRect) = 0; // [30]
    // The capability bits. GamepadControls tests bit 3 (value 8).
    // The concrete device answers it from the GL extension set.
    virtual int GetCapsFlags() = 0; // [31]
    virtual int GetMaxTextureStages() = 0; // [32]
    virtual const char *GetInfoString(EInfoString theInfo) = 0; // [33]
    virtual void GetBackBufferDimensions(unsigned long &theWidth, unsigned long &theHeight) = 0; // [34]
    virtual void SceneBegun() = 0; // [35]
    virtual void *CreateImageRenderData(MemoryImage *theImage) = 0; // [36]
    virtual void RemoveImageRenderData(MemoryImage *theImage) = 0; // [37]
    virtual void RecoverImageBitsFromRenderData(MemoryImage *theImage) = 0; // [38]
    virtual unsigned long GetTextureMemorySize(MemoryImage *theImage) = 0; // [39]
    virtual int GetTextureFormat(MemoryImage *theImage) = 0; // [40]
    // RenderSurface is the pointee of the second out-parameter; the type has no
    // declaration in this tree.
    virtual void SwapScreenImage(DeviceImage *&theImage, void *&theSurface) = 0; // [41]
    virtual void SwapScreenImage(DeviceImage *&theImage, void *&theSurface, unsigned int theFlags) = 0; // [42]
    virtual void CopyScreenImage(DeviceImage *theImage, unsigned int theFlags) = 0; // [43]
    virtual void DrawPrimitiveEx(unsigned long theFlags, Graphics3D_EPrimitiveType theType,
        const SexyVertex *theVertices, int theVertexCount, const Color &theColor, int theDrawMode, float theX,
        float theY, bool theBlend,
        unsigned int theTexCoord) = 0; // [44]
    virtual void SetBltDepth(float theDepth) = 0; // [45]
    virtual void ClearColorBuffer(const Color &theColor) = 0; // [46]
    virtual void ClearDepthBuffer() = 0; // [47]
    virtual void SetDepthState(Graphics3D_ECompareFunc theFunc, bool theWriteEnabled) = 0; // [48]
    virtual void SetAlphaTest(Graphics3D_ECompareFunc theFunc, int theRef) = 0; // [49]
    virtual void SetColorWriteState(bool theRed, bool theGreen, bool theBlue, bool theAlpha) = 0; // [50]
    virtual void SetWireframe(bool theWireframe) = 0; // [51]
    virtual void SetBlend(Graphics3D_EBlendMode theSrcBlend, Graphics3D_EBlendMode theDestBlend) = 0; // [52]
    virtual void SetBackfaceCulling(bool theCulling, bool theClockwise) = 0; // [53]
    virtual void SetLightingEnabled(bool theEnabled) = 0; // [54]
    virtual void SetLightEnabled(int theLight, bool theEnabled) = 0; // [55]
    virtual void SetPointLight(int theLight, const SexyVector3 &thePosition, const Graphics3D_LightColors &theColors,
        float theRange,
        const SexyVector3 &theAttenuation) = 0; // [56]
    virtual void SetDirectionalLight(int theLight, const SexyVector3 &theDirection,
        const Graphics3D_LightColors &theColors) = 0; // [57]
    virtual void SetGlobalAmbient(const Color &theColor) = 0; // [58]
    virtual void SetMaterialAmbient(const Color &theColor, int theLight) = 0; // [59]
    virtual void SetMaterialDiffuse(const Color &theColor, int theLight) = 0; // [60]
    virtual void SetMaterialSpecular(const Color &theColor, int theLight, float theShininess) = 0; // [61]
    virtual void SetMaterialEmissive(const Color &theColor, int theLight) = 0; // [62]
    virtual void SetWorldTransform(const SexyMatrix4 *theTransform) = 0; // [63]
    virtual void SetViewTransform(const SexyMatrix4 *theTransform) = 0; // [64]
    virtual void SetProjectionTransform(const SexyMatrix4 *theTransform) = 0; // [65]
    virtual void SetTextureTransform(int theStage, const SexyMatrix4 *theTransform, int theFlags) = 0; // [66]
    virtual void SetViewport(int theX, int theY, int theWidth, int theHeight, float theNear, float theFar) = 0; // [67]
    virtual void SetTexture(int theStage, Image *theTexture) = 0; // [68]
    virtual void SetTextureWrap(int theStage, bool theWrapS, bool theWrapT) = 0; // [69]
    virtual void SetTextureLinearFilter(int theStage, bool theLinear) = 0; // [70]
    virtual void SetTextureCoordSource(int theStage, int theSource,
        Graphics3D_ETexCoordGen theGen) = 0; // [71]
    virtual void SetTextureFactor(int theFactor) = 0; // [72]
    virtual void GetEffect(RenderEffectDefinition *theDefinition) = 0; // [73]
    // The three the interface itself implements.
    virtual void ReloadEffects() = 0; // [74]
    virtual void SetBltFilter(
        bool (*theFilter)(void *, int, unsigned int, const SexyVertex2D *, int, const TRect<int> **, void *),
        void *theData) = 0; // [75]
    virtual void SetDrawPrimFilter(bool (*theFilter)(void *, int, unsigned int, const SexyVertex2D *, int),
        void *theData) = 0; // [76]
    virtual void LoadMesh(Mesh *theMesh) = 0; // [77]
    virtual void RenderMesh(
        Mesh *theMesh, const SexyMatrix4 &theTransform, const Color &theColor, bool theBlend) = 0; // [78]
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_IGRAPHICSDRIVER_H
