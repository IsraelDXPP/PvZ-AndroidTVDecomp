// Each body makes this context current, then forwards one device call.
// Capability queries only read the device flags.

#include "Graphics3D.h"

#include "Graphics.h"
#include "IGraphicsDriver.h"

namespace Sexy
{

namespace
{

// Capability bits the device reports in GetCapsFlags.
const int kCapPixelShaders = 2;
const int kCapVertexShaders = 4;
const int kCapImageRenderTargets = 8;
const int kCapCubeMaps = 0x20;
const int kCapVolumeMaps = 0x40;

// Compare and blend literals the masking pass installs. They are the
// original device constants, not named enumerators.
const int kCompareAlways = 8;
const int kCompareGreater = 5;
const int kCompareLessEqual = 4;
const int kCompareGreaterEqual = 7;
const int kBlendOne = 1;
const int kBlendZero = 2;
const int kBlendNone = 0xFFFF;

} // namespace

void Graphics3D::SetAsCurrentContext()
{
    mRenderDevice->SetCurrentContext(mRenderContext.mValue);
}

Graphics3D::Graphics3D(Graphics *theGraphics, RenderDevice3D *theDevice, HRenderContext const &theContext) :
    mGraphics2D(theGraphics),
    mRenderDevice(theDevice),
    mRenderContext(theContext)
{
}

Graphics *Graphics3D::Get2D()
{
    return mGraphics2D;
}

bool Graphics3D::SupportsPixelShaders()
{
    return (mRenderDevice->GetCapsFlags() & kCapPixelShaders) != 0;
}

bool Graphics3D::SupportsVertexShaders()
{
    return (mRenderDevice->GetCapsFlags() & kCapVertexShaders) != 0;
}

bool Graphics3D::SupportsCubeMaps()
{
    return (mRenderDevice->GetCapsFlags() & kCapCubeMaps) != 0;
}

bool Graphics3D::SupportsVolumeMaps()
{
    return (mRenderDevice->GetCapsFlags() & kCapVolumeMaps) != 0;
}

bool Graphics3D::SupportsImageRenderTargets()
{
    return (mRenderDevice->GetCapsFlags() & kCapImageRenderTargets) != 0;
}

int Graphics3D::GetMaxTextureStages()
{
    return mRenderDevice->GetMaxTextureStages();
}

void Graphics3D::DrawPrimitiveEx(unsigned long theFlags, Graphics3D_EPrimitiveType theType,
    SexyVertex const *theVertices, int theVertexCount, Color const &theColor, int theDrawMode, float theX, float theY,
    bool theBlend, unsigned int theTexCoord)
{
    SetAsCurrentContext();
    mRenderDevice->DrawPrimitiveEx(
        theFlags, theType, theVertices, theVertexCount, theColor, theDrawMode, theX, theY, theBlend, theTexCoord);
}

void Graphics3D::RenderMesh(Mesh *theMesh, SexyMatrix4 const &theTransform, Color const &theColor, bool theBlend)
{
    SetAsCurrentContext();
    mRenderDevice->RenderMesh(theMesh, theTransform, theColor, theBlend);
}

void Graphics3D::SetBltDepth(float theDepth)
{
    SetAsCurrentContext();
    mRenderDevice->SetBltDepth(theDepth);
}

void Graphics3D::PushTransform(SexyMatrix3 const &theTransform, bool theConcatenate)
{
    SetAsCurrentContext();
    mRenderDevice->PushTransform(theTransform, theConcatenate);
}

void Graphics3D::PopTransform()
{
    SetAsCurrentContext();
    mRenderDevice->PopTransform();
}

void Graphics3D::PopTransform(SexyMatrix3 &theTransform)
{
    SetAsCurrentContext();
    mRenderDevice->PopTransform(theTransform);
}

void Graphics3D::ClearColorBuffer(Color const &theColor)
{
    SetAsCurrentContext();
    mRenderDevice->ClearColorBuffer(theColor);
}

void Graphics3D::ClearDepthBuffer()
{
    SetAsCurrentContext();
    mRenderDevice->ClearDepthBuffer();
}

void Graphics3D::SetDepthState(Graphics3D_ECompareFunc theFunc, bool theWriteEnabled)
{
    SetAsCurrentContext();
    mRenderDevice->SetDepthState(theFunc, theWriteEnabled);
}

void Graphics3D::SetAlphaTest(Graphics3D_ECompareFunc theFunc, int theRef)
{
    SetAsCurrentContext();
    mRenderDevice->SetAlphaTest(theFunc, theRef);
}

void Graphics3D::SetWireframe(bool theWireframe)
{
    SetAsCurrentContext();
    mRenderDevice->SetWireframe(theWireframe);
}

void Graphics3D::SetBlend(Graphics3D_EBlendMode theSrcBlend, Graphics3D_EBlendMode theDestBlend)
{
    SetAsCurrentContext();
    mRenderDevice->SetBlend(theSrcBlend, theDestBlend);
}

void Graphics3D::SetBackfaceCulling(bool theCulling, bool theClockwise)
{
    SetAsCurrentContext();
    mRenderDevice->SetBackfaceCulling(theCulling, theClockwise);
}

void Graphics3D::SetLightEnabled(int theLight, bool theEnabled)
{
    SetAsCurrentContext();
    mRenderDevice->SetLightEnabled(theLight, theEnabled);
}

void Graphics3D::SetPointLight(int theLight, SexyVector3 const &thePosition, Graphics3D_LightColors const &theColors,
    float theRange, SexyVector3 const &theAttenuation)
{
    SetAsCurrentContext();
    mRenderDevice->SetPointLight(theLight, thePosition, theColors, theRange, theAttenuation);
}

void Graphics3D::SetDirectionalLight(
    int theLight, SexyVector3 const &theDirection, Graphics3D_LightColors const &theColors)
{
    SetAsCurrentContext();
    mRenderDevice->SetDirectionalLight(theLight, theDirection, theColors);
}

void Graphics3D::SetGlobalAmbient(Color const &theColor)
{
    SetAsCurrentContext();
    mRenderDevice->SetGlobalAmbient(theColor);
}

void Graphics3D::SetMaterialAmbient(Color const &theColor, int theLight)
{
    SetAsCurrentContext();
    mRenderDevice->SetMaterialAmbient(theColor, theLight);
}

void Graphics3D::SetMaterialDiffuse(Color const &theColor, int theLight)
{
    SetAsCurrentContext();
    mRenderDevice->SetMaterialDiffuse(theColor, theLight);
}

void Graphics3D::SetMaterialSpecular(Color const &theColor, int theLight, float theShininess)
{
    SetAsCurrentContext();
    mRenderDevice->SetMaterialSpecular(theColor, theLight, theShininess);
}

void Graphics3D::SetMaterialEmissive(Color const &theColor, int theLight)
{
    SetAsCurrentContext();
    mRenderDevice->SetMaterialEmissive(theColor, theLight);
}

void Graphics3D::SetLightingEnabled(bool theEnabled, bool theApplyDefaultMaterial)
{
    SetAsCurrentContext();
    mRenderDevice->SetLightingEnabled(theEnabled);
    if (theEnabled && theApplyDefaultMaterial)
    {
        SetMaterialAmbient(Color::White, -1);
        SetMaterialDiffuse(Color::White, 0);
        SetMaterialSpecular(Color::White, -1, 0.0f);
        SetMaterialEmissive(Color::Black, -1);
    }
}

void Graphics3D::SetWorldTransform(SexyMatrix4 const *theTransform)
{
    SetAsCurrentContext();
    mRenderDevice->SetWorldTransform(theTransform);
}

void Graphics3D::SetViewTransform(SexyMatrix4 const *theTransform)
{
    SetAsCurrentContext();
    mRenderDevice->SetViewTransform(theTransform);
}

void Graphics3D::SetProjectionTransform(SexyMatrix4 const *theTransform)
{
    SetAsCurrentContext();
    mRenderDevice->SetProjectionTransform(theTransform);
}

void Graphics3D::SetTextureTransform(int theStage, SexyMatrix4 const *theTransform, int theFlags)
{
    SetAsCurrentContext();
    mRenderDevice->SetTextureTransform(theStage, theTransform, theFlags);
}

void Graphics3D::SetTexture(int theStage, Image *theTexture)
{
    SetAsCurrentContext();
    mRenderDevice->SetTexture(theStage, theTexture);
}

void Graphics3D::SetTextureWrap(int theStage, bool theWrap)
{
    SetTextureWrap(theStage, theWrap, theWrap);
}

void Graphics3D::SetTextureWrap(int theStage, bool theWrapS, bool theWrapT)
{
    SetAsCurrentContext();
    mRenderDevice->SetTextureWrap(theStage, theWrapS, theWrapT);
}

void Graphics3D::SetTextureLinearFilter(int theStage, bool theLinear)
{
    SetAsCurrentContext();
    mRenderDevice->SetTextureLinearFilter(theStage, theLinear);
}

void Graphics3D::SetTextureCoordSource(int theStage, int theSource, Graphics3D_ETexCoordGen theGen)
{
    SetAsCurrentContext();
    mRenderDevice->SetTextureCoordSource(theStage, theSource, theGen);
}

void Graphics3D::SetTextureFactor(int theFactor)
{
    SetAsCurrentContext();
    mRenderDevice->SetTextureFactor(theFactor);
}

void Graphics3D::GetEffect(RenderEffectDefinition *theDefinition)
{
    if (theDefinition == nullptr)
        return;
    mRenderDevice->GetEffect(theDefinition);
}

void Graphics3D::SetViewport(int theX, int theY, int theWidth, int theHeight, float theNear, float theFar)
{
    SetAsCurrentContext();
    int aViewX = (int)mGraphics2D->mTransX + theX;
    int aViewY = (int)mGraphics2D->mTransY + theY;
    mRenderDevice->SetViewport(aViewX, aViewY, theWidth, theHeight, theNear, theFar);
}

void Graphics3D::SetMasking(Graphics3D_EMaskMode theMode, int theAlphaRef, float theWriteDepth, float theClearDepth)
{
    SetAsCurrentContext();
    switch (theMode)
    {
        case 0:
            mRenderDevice->SetAlphaTest(kCompareAlways, 0);
            mRenderDevice->SetDepthState(kCompareAlways, false);
            mRenderDevice->SetBltDepth(theClearDepth);
            mRenderDevice->SetBlend(kBlendNone, kBlendNone);
            break;
        case 1:
        case 2:
            mRenderDevice->SetAlphaTest(kCompareGreater, theAlphaRef);
            mRenderDevice->SetDepthState(kCompareAlways, true);
            mRenderDevice->SetBltDepth(theWriteDepth);
            if (theMode == 1)
                mRenderDevice->SetBlend(kBlendOne, kBlendZero);
            else
                mRenderDevice->SetBlend(kBlendNone, kBlendNone);
            break;
        case 3:
        case 4:
            mRenderDevice->SetAlphaTest(kCompareGreater, 0);
            mRenderDevice->SetDepthState(theMode == 4 ? kCompareLessEqual : kCompareGreaterEqual, false);
            mRenderDevice->SetBltDepth(theClearDepth);
            mRenderDevice->SetBlend(kBlendNone, kBlendNone);
            break;
        default:
            break;
    }
}

void Graphics3D::ClearMask()
{
    SetAsCurrentContext();
    mRenderDevice->ClearDepthBuffer();
}

} // namespace Sexy
