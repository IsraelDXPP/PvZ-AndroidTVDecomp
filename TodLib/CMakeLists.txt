set(lawn_module_sources
    src/FoleyTypeData.cpp
    src/SyncObject.cpp
    src/TodCurveWidgetAnimator.cpp
    src/TodDrawString.cpp
    src/TodFoley.cpp
    src/TodHesitationBracket.cpp
    src/TodParticle.cpp
    src/TodParticleDefinition.cpp
    src/TodParticleEmitter.cpp
    src/TodParticleHolder.cpp
    src/TodParticleSystem.cpp
)

add_library(TodLib OBJECT ${lawn_module_sources})
target_link_libraries(TodLib PUBLIC LawnTVBuildOptions LawnTVDependencies)
