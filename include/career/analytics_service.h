#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

namespace analytics_service {

struct TeamAnalyticsSnapshot {
    int attackIndex = 0;
    int controlIndex = 0;
    int defenseIndex = 0;
    int contractRisk = 0;
    int fatigueRisk = 0;
    int youthUpside = 0;
    int continuityScore = 0;
    int roleBalance = 0;
    int setPieceThreat = 0;
    int aerialThreat = 0;
    int xgForTenths = 0;
    int xgAgainstTenths = 0;
    int finishingDeltaTenths = 0;
    std::string pressureWindow;
    std::vector<std::string> alerts;
};

TeamAnalyticsSnapshot buildTeamAnalyticsSnapshot(const Career& career, const Team& team);
std::vector<std::string> buildTeamAnalyticsLines(const Career& career, const Team& team);
std::vector<std::string> buildMatchTrendLines(const Career& career);

}  // namespace analytics_service
