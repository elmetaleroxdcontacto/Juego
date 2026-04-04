#pragma once

#include "engine/models.h"

namespace match_engine {

struct MatchSimulationData {
    MatchResult result;
    std::vector<int> homeParticipants;
    std::vector<int> awayParticipants;
    std::vector<std::string> homeYellowCardPlayers;
    std::vector<std::string> awayYellowCardPlayers;
    std::vector<std::string> homeRedCardPlayers;
    std::vector<std::string> awayRedCardPlayers;
    std::vector<std::string> homeInjuredPlayers;
    std::vector<std::string> awayInjuredPlayers;
    std::vector<GoalContribution> homeGoals;
    std::vector<GoalContribution> awayGoals;
};

MatchSimulationData simulate(const Team& home, const Team& away, bool keyMatch = false, bool neutralVenue = false);
MatchSimulationData simulate(const Team& home, const Team& away, const Career* career, bool keyMatch = false, bool neutralVenue = false);

}  // namespace match_engine
