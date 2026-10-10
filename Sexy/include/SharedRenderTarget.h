// SharedRenderTarget::Pool -- the render-target cache the app builds on demand.
//
// Three words. The app holds one pointer and creates the pool the first time
// something asks for it.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_SHAREDRENDERTARGET_H
#define LAWNPROJECTTV_SEXY_INCLUDE_SHAREDRENDERTARGET_H

namespace Sexy
{
namespace SharedRenderTarget
{

class Pool
{
public:
    Pool() :
        mCount(0),
        mHead(nullptr),
        mTail(nullptr)
    {
    }
    ~Pool();

    int mCount;
    void *mHead;
    void *mTail;
};

} // namespace SharedRenderTarget
} // namespace Sexy

#endif // LAWNPROJECTTV_SEXY_INCLUDE_SHAREDRENDERTARGET_H
