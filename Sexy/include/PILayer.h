// PILayer is a named list of particle emitters plus a colour and a transform.

#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PILAYER_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PILAYER_H

#include "ReconTypes.h"
#include <vector>

namespace Sexy
{

struct PILayerEmitter
{
    Sexy::String mName;
};

class PILayer
{
public:
    std::vector<PILayerEmitter> mEmitters;
    bool mVisible;
    Sexy::Color mColor;
    Sexy::SexyMatrix3 mTransform;

    void SetVisible(bool visible);
    PILayerEmitter *GetEmitter(int theIndex);
    PILayerEmitter *GetEmitter(Sexy::String const &theName);
    PILayer();
};

} // namespace Sexy

using Sexy::PILayer;
using Sexy::PILayerEmitter;

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PILAYER_H
