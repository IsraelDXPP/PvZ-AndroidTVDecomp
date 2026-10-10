#ifndef LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_CRASHDIAGNOSTICS_H
#define LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_CRASHDIAGNOSTICS_H

namespace Sexy
{
void InstallCrashDiagnostics();
void SetCrashContext(
    int theGameMode, int theLevel, int theScene, int theFrame, int theIntroTime, int theGraves, bool theGravesEnabled);
void SetCrashPhase(const char *thePhase);
void RecordCrashContext(const char *theEvent);
}

#endif // LAWNPROJECTTV_SEXY_INCLUDE_PLATFORM_CRASHDIAGNOSTICS_H
