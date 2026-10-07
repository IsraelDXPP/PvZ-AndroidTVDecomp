#pragma once

namespace Sexy
{
void InstallCrashDiagnostics();
void SetCrashContext(int theGameMode, int theLevel, int theScene, int theFrame,
                     int theIntroTime, int theGraves, bool theGravesEnabled);
void SetCrashPhase(const char* thePhase);
void RecordCrashContext(const char* theEvent);
}
