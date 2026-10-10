// Definition tables describe native fields and the fixed TV file layout separately.
// Empty field names terminate a table.

#ifndef LAWNPROJECTTV_LAWN_INCLUDE_GAME_DEFINITION_H
#define LAWNPROJECTTV_LAWN_INCLUDE_GAME_DEFINITION_H

#include <cstddef>

enum ReanimationType : int;
enum ParticleEffect : int;

struct DefField
{
    const char *mName;
    size_t mOffset;
    int mType;
    void *mExtra;
    int mFileOffset;
};

struct DefMap
{
    const DefField *mFields;
    size_t mSize;
    int mFileSize;
    void *mConstructor;
};

class ReanimatorDefinition;

void DefinitionFreeMap(DefMap *theMap, void *theObject);
void ReanimationFreeDefinition(ReanimatorDefinition *theDefinition);
void ReanimatorFreeDefinitions();
void TodParticleFreeDefinitions();
void TrailFreeDefinitions();

extern DefMap gParticleDefMap;
extern DefMap gTrailDefMap;

// The loaders store the caller's parameter table here and the free functions
// clear both words. The record layout is filled in with the loaders.
// One row of the TV parameter table: effect type followed by resource path.
struct ParticleParamRecord
{
    ParticleEffect mParticleEffect;
    const char *mFileName;
};
struct TrailParamRecord
{
    int mUnused;
    const char *mFileName;
};

extern ParticleParamRecord *gParticleParamArray;
extern int gParticleParamArraySize;
extern TrailParamRecord *gTrailParamArray;
extern int gTrailParamArraySize;
extern int gParticleDefCount;
extern int gTrailDefCount;

// One row of the reanimation parameter table. The type, the file name and a
// flags word, 12 bytes. ReanimatorLoadDefinitions steps by 12.
struct ReanimationParamRecord
{
    ReanimationType mReanimationType;
    const char *mFileName;
    int mFlags;
};

extern ReanimationParamRecord *gReanimationParamArray;
extern int gReanimationParamArraySize;

#endif // LAWNPROJECTTV_LAWN_INCLUDE_GAME_DEFINITION_H
