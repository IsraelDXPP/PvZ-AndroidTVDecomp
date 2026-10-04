// Definition maps -- the tables DefinitionFreeMap walks.
//
// A map is a 12-byte header: the field list, the byte size of one record, and
// an unused constructor pointer. Each field is 16 bytes: name, offset, type,
// and an extra pointer (the nested map, for an array field). An empty name
// ends the list. Freeing only acts on strings, float tracks and arrays; the
// other types are skipped.

#pragma once

struct DefField
{
    const char *mName;
    int         mOffset;
    int         mType;
    void       *mExtra;
};

struct DefMap
{
    const DefField *mFields;
    int             mSize;
    void           *mConstructor;
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
// One row of the parameter table the loaders walk. The first word is unused;
// the file name sits at +4, and the row is 8 bytes.
struct ParticleParamRecord
{
    int mUnused;
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
    int mReanimationType;
    const char *mFileName;
    int mFlags;
};

extern ReanimationParamRecord *gReanimationParamArray;
extern int gReanimationParamArraySize;
