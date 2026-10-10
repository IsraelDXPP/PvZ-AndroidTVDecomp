#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONCHILDREN_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONCHILDREN_H
#include <vector>
class JSONNode;

class jsonChildren
{
public:
    std::vector<JSONNode *> mChildren;
    void deleteAll();
};

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_JSONCHILDREN_H
