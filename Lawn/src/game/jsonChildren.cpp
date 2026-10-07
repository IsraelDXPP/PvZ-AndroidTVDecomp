#include "jsonChildren.h"
#include "JSONNode.h"

void jsonChildren::deleteAll()
{
    for (JSONNode *aChild : mChildren)
        aChild->Release();
    mChildren.clear();
}
