#pragma once

#include "simulation/match_types.h"

#include <string>
#include <vector>

namespace match_stats {

void applyImpact(MatchStats& stats, const MatchEventImpact& impact);
void pushEvent(MatchTimeline& timeline, MatchStats& stats, const MatchEvent& event);
std::vector<std::string> buildLegacyTimeline(const MatchTimeline& timeline);
int countSubstitutions(const MatchTimeline& timeline, const std::string& teamName);

}  // namespace match_stats
