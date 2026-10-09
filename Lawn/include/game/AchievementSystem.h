// Console achievements adapted from PlantsVsZombies-AndroidTV.
// Copyright (C) 2023-2026 PvZ TV Touch Team. GPL-3.0-or-later.
#pragma once

#include "ReconTypes.h"

enum AchievementType
{
    ACHIEVEMENT_HOME_SECURITY,
    ACHIEVEMENT_MORTICULTURALIST,
    ACHIEVEMENT_IMMORTAL,
    ACHIEVEMENT_SOILPLANTS,
    ACHIEVEMENT_CLOSESHAVE,
    ACHIEVEMENT_CHOMP,
    ACHIEVEMENT_VERSUS,
    ACHIEVEMENT_GARG,
    ACHIEVEMENT_COOP,
    ACHIEVEMENT_SHOP,
    ACHIEVEMENT_EXPLODONATOR,
    ACHIEVEMENT_TREE,
    NUM_ACHIEVEMENT_TYPES
};

struct AchievementDefinition
{
    const char* mStringKey;
    const char* mIconName;
    const char* mDefaultTitle;
    const char* mDefaultDescription;
    ReanimationType mReanimationType;
};

const AchievementDefinition& GetAchievementDefinition(AchievementType theAchievement);
Sexy::String GetAchievementTitle(AchievementType theAchievement);
Sexy::String GetAchievementDescription(AchievementType theAchievement);
Sexy::Image* GetAchievementIcon(AchievementType theAchievement);
