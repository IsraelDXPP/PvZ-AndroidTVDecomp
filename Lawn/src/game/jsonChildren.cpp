#include "jsonChildren.h"
#include "JSONNode.h"

void jsonChildren::deleteAll()
{
    for (JSONNode *aChild : mChildren)
    {
        if (aChild != nullptr)
            aChild->Release();
    }
    mChildren.clear();
}
