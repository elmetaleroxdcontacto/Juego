#pragma once

#include "simulation/match_context.h"
#include "simulation/match_stats.h"

#include <string>
#include <vector>

namespace match_event_generator {

void playChances(Team& attacking,
                 Team& defending,
                 const std::vector<int>& attackingXI,
                 const std::vector<int>& defendingXI,
                 const TeamMatchSnapshot& attackingSnapshot,
                 const TeamMatchSnapshot& defendingSnapshot,
                 bool attackingIsHome,
                 int minuteStart,
                 int minuteEnd,
                 int chanceCount,
                 double attackingEdge,
                 MatchTimeline& timeline,
                 MatchStats& stats,
                 std::vector<GoalContribution>& goals);

void registerDiscipline(Team& team,
                        std::vector<int>& activeXI,
                        bool teamIsHome,
                        double intensity,
                        MatchTimeline& timeline,
                        MatchStats& stats,
                        std::vector<std::string>& cautionedPlayers,
                        std::vector<std::string>& sentOffPlayers,
                        std::vector<std::string>& yellowCardPlayers,
                        std::vector<std::string>& redCardPlayers);

void maybeInjure(const Team& team,
                 const std::vector<int>& activeXI,
                 double injuryRisk,
                 int minuteStart,
                 int minuteEnd,
                 MatchTimeline& timeline,
                 std::vector<std::string>& injuredPlayers);

}  // namespace match_event_generator
