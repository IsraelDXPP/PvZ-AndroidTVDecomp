// GLVertexBuffer.cpp -- dynamic vertex buffer with quad batching.

#include "GLVertexBuffer.h"
#include "SexyTypes.h"
#include <cstdint>

#include "GLShader.h"
#include "AndroidShims.h"
namespace Sexy
{

GLVertexBuffer::GLVertexBuffer()
    : mVAO(0), mVBO(0), mVertices(nullptr), mCapacity(0), mCount(0),
      mCurrentTexture(0), mHasTexture(false), mDrawCalls(0)
{
}

GLVertexBuffer::~GLVertexBuffer()
{
    Destroy();
}

bool GLVertexBuffer::Create(int theCapacity)
{
    Destroy();

    mCapacity = theCapacity;
    mVertices = static_cast<GLVertex *>(malloc(sizeof(GLVertex) * (size_t)mCapacity));
    if (!mVertices)
    {
        TOD_ERROR("GLVertexBuffer: out of memory (%d vertices)", mCapacity);
        return false;
    }
    mCount = 0;
    mDrawCalls = 0;

#ifndef LAWNTV_USE_GLES2
    // Core profile requires a bound VAO for any vertex submission.
    glGenVertexArrays(1, &mVAO);
    glBindVertexArray(mVAO);
#else
    mVAO = 0;
#endif

    glGenBuffers(1, &mVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mVBO);

    // Allocate immutable-ish storage once; contents are updated with
    // glBufferSubData on each flush.
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(sizeof(GLVertex) * (size_t)mCapacity),
                 nullptr, GL_DYNAMIC_DRAW);

    // Attribute 0: position, attribute 1: texcoord (see GLShader.cpp).
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          (const void *)(intptr_t)offsetof(GLVertex, mX));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex),
                          (const void *)(intptr_t)offsetof(GLVertex, mU));

#ifndef LAWNTV_USE_GLES2
    glBindVertexArray(0);
#endif

    GL_CHECK("GLVertexBuffer::Create");
    return true;
}

void GLVertexBuffer::Destroy()
{
    if (mVBO) { glDeleteBuffers(1, &mVBO); mVBO = 0; }
#ifndef LAWNTV_USE_GLES2
    if (mVAO) { glDeleteVertexArrays(1, &mVAO); mVAO = 0; }
#endif
    if (mVertices) { free(mVertices); mVertices = nullptr; }
    mCapacity = mCount = 0;
}

void GLVertexBuffer::Begin(unsigned int theTexture)
{
    // Callers must flush between texture changes; assert loudly in debug so a
    // missing flush shows up as a wrong-texture bug rather than silently.
    if (mCount > 0 && theTexture != mCurrentTexture)
    {
        TOD_WARN("GLVertexBuffer::Begin: texture changed with %d vertices "
                 "pending -- flushing", mCount);
        Flush();
    }
    mCurrentTexture = theTexture;
    mHasTexture = (theTexture != 0);
}

void GLVertexBuffer::AddQuad(float x0, float y0, float x1, float y1,
                             float u0, float v0, float u1, float v1)
{
    if (mCount + 6 > mCapacity)
        Flush();

    GLVertex* v = mVertices + mCount;

    // Triangle 1: top-left, bottom-left, bottom-right
    v[0] = { x0, y0, u0, v0 };
    v[1] = { x0, y1, u0, v1 };
    v[2] = { x1, y1, u1, v1 };

    // Triangle 2: top-left, bottom-right, top-right
    v[3] = { x0, y0, u0, v0 };
    v[4] = { x1, y1, u1, v1 };
    v[5] = { x1, y0, u1, v0 };

    mCount += 6;
}

void GLVertexBuffer::AddSolidQuad(float x0, float y0, float x1, float y1)
{
    // UVs are unused by the solid shader; pass zeros.
    AddQuad(x0, y0, x1, y1, 0.0f, 0.0f, 0.0f, 0.0f);
}

void GLVertexBuffer::Flush()
{
    if (mCount == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, mVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    (GLsizeiptr)(sizeof(GLVertex) * (size_t)mCount), mVertices);

#ifndef LAWNTV_USE_GLES2
    glBindVertexArray(mVAO);
#endif

    glDrawArrays(GL_TRIANGLES, 0, mCount);
    ++mDrawCalls;

    GL_CHECK("GLVertexBuffer::Flush");

    mCount = 0;
}

} // namespace Sexy
