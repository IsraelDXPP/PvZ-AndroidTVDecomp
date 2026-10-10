// Graphics3D makes its stored 3D context current, then forwards one call
// to the render device. It is not polymorphic.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_GRAPHICS3D_H
#define LAWNPROJECTTV_SEXY_INCLUDE_GRAPHICS3D_H

#include "ReconTypes.h"

namespace Sexy
{

class Graphics;
class RenderDevice3D;
class Image;
class Mesh;
class RenderEffectDefinition;

// One-word render-context handle. The constructor copies the caller's word.
// SetAsCurrentContext hands that word to the device.
class HRenderContext
{
public:
    int mValue;
};

class Graphics3D
{
public:
    Graphics *mGraphics2D;
    RenderDevice3D *mRenderDevice;
    HRenderContext mRenderContext;

    Graphics3D(Graphics *theGraphics, RenderDevice3D *theDevice, HRenderContext const &theContext);

    void SetAsCurrentContext();
    Graphics *Get2D();

    bool SupportsPixelShaders();
    bool SupportsVertexShaders();
    bool SupportsCubeMaps();
    bool SupportsVolumeMaps();
    bool SupportsImageRenderTargets();
    int GetMaxTextureStages();

    void DrawPrimitiveEx(unsigned long theFlags, Graphics3D_EPrimitiveType theType, SexyVertex const *theVertices,
        int theVertexCount, Color const &theColor, int theDrawMode, float theX, float theY, bool theBlend,
        unsigned int theTexCoord);
    void RenderMesh(Mesh *theMesh, SexyMatrix4 const &theTransform, Color const &theColor, bool theBlend);
    void SetBltDepth(float theDepth);
    void PushTransform(SexyMatrix3 const &theTransform, bool theConcatenate);
    void PopTransform();
    void PopTransform(SexyMatrix3 &theTransform);
    void ClearColorBuffer(Color const &theColor);
    void ClearDepthBuffer();
    void SetDepthState(Graphics3D_ECompareFunc theFunc, bool theWriteEnabled);
    void SetAlphaTest(Graphics3D_ECompareFunc theFunc, int theRef);
    void SetWireframe(bool theWireframe);
    void SetBlend(Graphics3D_EBlendMode theSrcBlend, Graphics3D_EBlendMode theDestBlend);
    void SetBackfaceCulling(bool theCulling, bool theClockwise);
    void SetLightEnabled(int theLight, bool theEnabled);
    void SetPointLight(int theLight, SexyVector3 const &thePosition, Graphics3D_LightColors const &theColors,
        float theRange, SexyVector3 const &theAttenuation);
    void SetDirectionalLight(int theLight, SexyVector3 const &theDirection, Graphics3D_LightColors const &theColors);
    void SetGlobalAmbient(Color const &theColor);
    void SetMaterialAmbient(Color const &theColor, int theLight);
    void SetMaterialDiffuse(Color const &theColor, int theLight);
    void SetMaterialSpecular(Color const &theColor, int theLight, float theShininess);
    void SetMaterialEmissive(Color const &theColor, int theLight);
    void SetLightingEnabled(bool theEnabled, bool theApplyDefaultMaterial);
    void SetWorldTransform(SexyMatrix4 const *theTransform);
    void SetViewTransform(SexyMatrix4 const *theTransform);
    void SetProjectionTransform(SexyMatrix4 const *theTransform);
    void SetTextureTransform(int theStage, SexyMatrix4 const *theTransform, int theFlags);
    void SetTexture(int theStage, Image *theTexture);
    void SetTextureWrap(int theStage, bool theWrap);
    void SetTextureWrap(int theStage, bool theWrapS, bool theWrapT);
    void SetTextureLinearFilter(int theStage, bool theLinear);
    void SetTextureCoordSource(int theStage, int theSource, Graphics3D_ETexCoordGen theGen);
    void SetTextureFactor(int theFactor);
    void GetEffect(RenderEffectDefinition *theDefinition);
    void SetViewport(int theX, int theY, int theWidth, int theHeight, float theNear, float theFar);
    void SetMasking(Graphics3D_EMaskMode theMode, int theAlphaRef, float theWriteDepth, float theClearDepth);
    void ClearMask();
};

} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_GRAPHICS3D_H
