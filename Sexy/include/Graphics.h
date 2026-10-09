// Graphics is the draw context widgets hold: destination image, clip,
// translation, scale, colour, font, and the blit and string helpers.
//
// Every paint goes through the destination image; transform hooks forward to
// that image's render device, or to the bound 3D device when one is available.

#pragma once

#include "Font.h"
#include "GraphicsState.h"
#include "Image.h"
#include "SexyTransform2D.h"
#include "Transform.h"
#include <vector>

namespace Sexy {

class Graphics3D;

class Graphics : public GraphicsState
{
public:
    enum DrawMode
    {
        DRAWMODE_NORMAL = 0,
        DRAWMODE_ADDITIVE = 1,
    };

    // The 3D context built for this destination, or null on the software path.
    // Get3D returns it; callers test the pointer and then call Graphics3D.
    Graphics3D *mGraphics3D;
    std::vector<GraphicsState> mStateStack;

    static bool mDefaultGlobalScale;
    static bool mDefaultTrackDeviceState;
    static Sexy::Image mStaticImage;
    static Sexy::String mPreferedEncoding;

    virtual ~Graphics();
    virtual void PushTransform(SexyMatrix3 const& theTransform, bool theConcatenate);
    virtual void PopTransform();
    virtual void PopTransform(SexyMatrix3 & theTransform);

    static void SetPreferedEncoding(Sexy::String const& theEncoding);
    static Sexy::String GetPreferedEncoding();

    Graphics3D * Get3D();
    void SetAsCurrentContext();

    void CalcFinalColor();
    Color const& GetImageColor() const;

    Graphics* SetTrackingDeviceState(bool theTrack);
    Font * GetFont();
    Graphics* SetFont(Sexy::Font * theFont);
    Graphics* SetColor(Color const& theColor);
    const Color & GetColor();
    void PopColorMult();
    Graphics* SetDrawMode(int theDrawMode);
    int GetDrawMode();
    Graphics* SetColorizeImages(bool theColorize);
    bool GetColorizeImages();
    Graphics* SetFastStretch(bool theFastStretch);
    bool GetFastStretch();
    Graphics* SetLinearBlend(bool theLinear);
    bool GetLinearBlend();
    Graphics* SetDefaultGlobalScale(bool theScale);
    Graphics* SetGlobalScale(bool theScale);
    bool GetGlobalScale();

    Graphics* ClearRect(int theX, int theY, int theWidth, int theHeight);
    Graphics* ClearRect(TRect<int> const& theRect);
    Graphics* FillRect(int theX, int theY, int theWidth, int theHeight);
    Graphics* FillRect(TRect<int> const& theRect);
    Graphics* DrawRect(int theX, int theY, int theWidth, int theHeight);
    Graphics* DrawRect(TRect<int> const& theRect);

    void PolyFill(TPoint<int> const* thePoints, int theCount, bool theConvex);
    void PolyFillAA(TPoint<int> const* thePoints, int theCount, bool theConvex);

    Graphics* DrawLine(int theStartX, int theStartY, int theEndX, int theEndY);
    Graphics* DrawLineAA(int theStartX, int theStartY, int theEndX, int theEndY);

    Graphics* DrawImage(Sexy::Image * theImage, int theX, int theY, TRect<int> const& theSrcRect);
    Graphics* DrawImage(Sexy::Image * theImage, int theX, int theY);
    Graphics* DrawImageMirror(Sexy::Image * theImage, int theX, int theY, TRect<int> const& theSrcRect, bool theMirror);
    Graphics* DrawImageMirror(Sexy::Image * theImage, int theX, int theY, bool theMirror);
    Graphics* DrawImage(Sexy::Image * theImage, int theX, int theY, int theWidth, int theHeight);
    Graphics* DrawImage(Sexy::Image * theImage, TRect<int> const& theDestRect, TRect<int> const& theSrcRect);
    Graphics* DrawImageMirror(Sexy::Image * theImage, TRect<int> const& theDestRect, TRect<int> const& theSrcRect, bool theMirror);
    Graphics* DrawImageF(Sexy::Image * theImage, float theX, float theY, TRect<int> const& theSrcRect);
    Graphics* DrawImageF(Sexy::Image * theImage, float theX, float theY);
    void DrawImageRotatedF(Sexy::Image * theImage, float theX, float theY, double theRot, float theCenterX, float theCenterY, TRect<int> const* theSrcRect);
    void DrawImageRotated(Sexy::Image * theImage, int theX, int theY, double theRot, TRect<int> const* theSrcRect);
    void DrawImageRotatedF(Sexy::Image * theImage, float theX, float theY, double theRot, TRect<int> const* theSrcRect);
    void DrawImageRotated(Sexy::Image * theImage, int theX, int theY, double theRot, int theCenterX, int theCenterY, TRect<int> const* theSrcRect);
    Graphics* DrawImageMatrix(Sexy::Image * theImage, SexyMatrix3 const& theMatrix, float theX, float theY, bool theMirror);
    Graphics* DrawImageMatrix(Sexy::Image * theImage, SexyMatrix3 const& theMatrix, TRect<int> const& theSrcRect, float theX, float theY, bool theMirror);
    void DrawImageTransform(Sexy::Image * theImage, Sexy::Transform const& theTransform, float theX, float theY);
    void DrawImageTransformF(Sexy::Image * theImage, Sexy::Transform const& theTransform, float theX, float theY);
    void DrawImageTransform(Sexy::Image * theImage, Sexy::Transform const& theTransform, TRect<int> const& theSrcRect, float theX, float theY);
    void DrawImageTransformF(Sexy::Image * theImage, Sexy::Transform const& theTransform, TRect<int> const& theSrcRect, float theX, float theY);
    void DrawTriangleTex(Sexy::Image * theImage, SexyVertex2D const& theA, SexyVertex2D const& theB, SexyVertex2D const& theC);
    void DrawTrianglesTex(Sexy::Image * theImage, SexyVertex2D const(*theTriangles)[3], int theCount, Color const& theColor, int theDrawMode, float theTx, float theTy, bool theBlend, TRect<int> const* theClipRect);
    void DrawTrianglesTexStrip(Sexy::Image * theImage, SexyVertex2D const* theVertices, int theCount, Color const& theColor, int theDrawMode, float theTx, float theTy, bool theBlend);
    void DrawTrianglesTexStrip(Sexy::Image * theImage, SexyVertex2D const* theVertices, int theCount);

    Graphics* ClearClipRect();
    void SetClipRect(int theX, int theY, int theWidth, int theHeight);
    void SetClipRect(TRect<int> const& theRect);
    void ClipRect(int theX, int theY, int theWidth, int theHeight);
    void ClipRect(TRect<int> const& theRect);
    Graphics* Translate(int theX, int theY);
    Graphics* TranslateF(float theX, float theY);
    Graphics* SetScale(float theScaleX, float theScaleY, float theOrigX, float theOrigY);

    int StringWidth(Sexy::String const& theString);
    Graphics* DrawImageCel(Sexy::Image * theImage, int theX, int theY, int theCol, int theRow);
    Graphics* DrawImageCel(Sexy::Image * theImage, int theX, int theY, int theCel);
    Graphics* DrawImageAnim(Sexy::Image * theImage, int theX, int theY, int theTime);
    Graphics* DrawImageCel(Sexy::Image * theImage, TRect<int> const& theDestRect, int theCol, int theRow);
    Graphics* DrawImageCel(Sexy::Image * theImage, TRect<int> const& theDestRect, int theCel);
    Graphics* DrawString(Sexy::String const& theString, int theX, int theY);

    Graphics(Sexy::Image * theDestImage);
    Graphics(Sexy::Graphics const& theOther);
    Sexy::Graphics * Create();
    void DrawImageBox(TRect<int> const& theSrc, TRect<int> const& theDest, Sexy::Image * theImage);
    void DrawImageBox(TRect<int> const& theDest, Sexy::Image * theImage);
    Graphics* PushState();
    Graphics* PushState(bool theTrackDeviceState);
    Graphics* PopState();
    void PushColorMult();
    void DrawTrianglesTex(Sexy::Image * theImage, SexyVertex2D const(*theTriangles)[3], int theCount);
    int WriteString(Sexy::String const& theString, int theX, int theY, int theWidth, int theJustification, bool theDraw, int theOffset, int theLength, int theOldColor);
    int DrawStringColor(Sexy::String const& theString, int theX, int theY, int theOldColor);
    int WriteWordWrapped(TRect<int> const& theRect, Sexy::String const& theLine, int theLineSpacing, int theJustification, int *theMaxWidth, int theMaxChars, int *theLastWidth);
    int DrawStringWordWrapped(Sexy::String const& theLine, int theX, int theY, int theWrapWidth, int theLineSpacing, int theJustification, int *theMaxWidth);
    int GetWordWrappedHeight(int theWidth, Sexy::String const& theLine, int theLineSpacing, int *theMaxWidth);

protected:
    struct ActiveEdge
    {
        double mX;
        double mSlope;
        int mIndex;
        double mY;
    };

    static const TPoint<int> *mPolyPoints;
    ActiveEdge *mActiveEdges;
    int mActiveEdgeCount;
    int mPointCount;

    static int ComparePointY(void const* theLeft, void const* theRight);
    static int CompareEdgeX(void const* theLeft, void const* theRight);
    void DeleteEdge(int theIndex);
    void InsertEdge(int theIndex, int theScanY);
    bool ClipLine(double *theStartX, double *theStartY, double *theEndX, double *theEndY);
    void DrawImageTransformHelper(Sexy::Image * theImage, Sexy::Transform const& theTransform, TRect<int> const& theSrcRect, float theX, float theY, bool theUseFloat);
    void BindDestination(Sexy::Graphics const* theOther);
    Color const& DrawColor() const;
    bool ScaleIsIdentity() const;
    void ScalePoint(int & theX, int & theY, int & theWidth, int & theHeight) const;
};

} // namespace Sexy

using Sexy::Graphics;
