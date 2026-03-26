#pragma once

#include "engine/models.h"

#include <string>

namespace player_condition {

struct InjuryProfile {
    std::string type;
    int weeks = 0;
};

int workloadRisk(const Player& player, const Team& team);
int relapseRisk(const Player& player, const Team& team);
int readinessScore(const Player& player, const Team& team);
int developmentStability(const Player& player, const Team& team, bool congestedWeek);
InjuryProfile buildInjuryProfile(const Player& player, const Team& team, int severityBias = 0);
void applyInjury(Player& player, const Team& team, int severityBias = 0);

}  // namespace player_condition
