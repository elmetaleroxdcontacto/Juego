#pragma once

#include "engine/models.h"
#include "simulation/match_context.h"

#include <vector>

namespace match_report {

MatchReport buildReport(const MatchSetup& setup,
                        const Team& home,
                        const Team& away,
                        const MatchTimeline& timeline,
                        const MatchStats& stats);

void appendSummaryLines(const MatchReport& report, std::vector<std::string>& lines);

}  // namespace match_report
