#pragma once

#include "engine/models.h"

#include <cstddef>
#include <string>
#include <vector>

struct WorldPulseSummary {
    int resolvedPromises = 0;
    int brokenPromises = 0;
    int pressureClubs = 0;
    int newRecords = 0;
    std::vector<std::string> headlines;
};

namespace world_state_service {

void seedSeasonPromises(Career& career);
WorldPulseSummary processWeeklyWorldState(Career& career);
int resolveSeasonCarryover(Career& career, std::vector<std::string>* summaryLines = nullptr);
int updateSeasonRecords(Career& career, const LeagueTable& table);
int worldRuleValue(const std::string& key, int defaultValue);
std::vector<std::string> listConfiguredScoutingRegions();
std::string formatPromiseSummary(const Career& career, std::size_t maxLines = 5);
std::string formatHistoricalRecordSummary(const Career& career, std::size_t maxLines = 5);

}  // namespace world_state_service
