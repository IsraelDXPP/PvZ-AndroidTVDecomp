#pragma once
#include <vector>
class JSONNode;

class jsonChildren
{
public:
    std::vector<JSONNode *> mChildren;
    void deleteAll();
};
