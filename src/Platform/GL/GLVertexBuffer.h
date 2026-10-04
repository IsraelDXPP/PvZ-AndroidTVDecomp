// GLVertexBuffer.h -- batched 2D quad submission.
//
// GL 3.3 core requires a VAO and a buffer object; GLES2 (what the Android build
// targeted) allowed client-side vertex arrays. This class hides that difference
// and additionally batches quads, which matters because the game issues a very
// large number of small DrawImage calls per frame (every plant, zombie, particle
// and UI element).
//
// The original's equivalent is Sexy::GLTexture::BltTriangles, which assembled
// triangles into a scratch buffer and issued one draw call per batch. We follow
// the same structure with a dynamic VBO and an explicit flush.

#ifndef __GLVERTEXBUFFER_H__
#define __GLVERTEXBUFFER_H__

#include "GLHeaders.h"
#include "CoreTypes.h"

namespace Sexy
{

// Matches the vertex shader's attribute layout in GLShader.cpp.
struct GLVertex
{
    float mX, mY;       // pixel-space position
    float mU, mV;       // texture coordinate
};

class GLVertexBuffer
{
public:
    unsigned int    mVAO;
    unsigned int    mVBO;

    GLVertex*       mVertices;      // CPU staging buffer
    int             mCapacity;      // vertices the staging buffer holds
    int             mCount;         // vertices queued right now

    unsigned int    mCurrentTexture;    // all queued quads share one texture
    bool            mHasTexture;

public:
    GLVertexBuffer();
    ~GLVertexBuffer();

    bool    Create(int theCapacity = 4096 * 6);
    void    Destroy();

    // Begin a batch. Any previously queued geometry must have been flushed.
    void    Begin(unsigned int theTexture);

    // Queue one textured quad (two triangles, 6 vertices).
    void    AddQuad(float x0, float y0, float x1, float y1,
                    float u0, float v0, float u1, float v1);

    // Queue one untextured quad (solid fill).
    void    AddSolidQuad(float x0, float y0, float x1, float y1);

    // Upload and draw everything queued, then reset.
    void    Flush();

    // True when the batch is full and must be flushed before more geometry.
    bool    IsFull() const { return mCount + 6 > mCapacity; }
    int     GetCount() const { return mCount; }

    // Draw calls issued since Create() -- useful to confirm batching works.
    unsigned int GetDrawCalls() const { return mDrawCalls; }

private:
    unsigned int    mDrawCalls;
};

} // namespace Sexy



#endif // __GLVERTEXBUFFER_H__

