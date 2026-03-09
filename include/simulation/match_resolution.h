#pragma once

#include "engine/models.h"
#include "simulation/match_types.h"

struct ChanceResolutionInput {
    int minute = 0;
    bool bigChance = false;
    bool attackingTeamIsHome = true;
    double chanceQuality = 0.0;
    double attackingEdge = 0.0;
    double defensivePressure = 0.0;
};

struct ChanceResolutionOutput {
    bool scored = false;
    bool onTarget = false;
    bool wonCorner = false;
    double expectedGoals = 0.0;
    int shooterIndex = -1;
    int assisterIndex = -1;
    MatchEvent attemptEvent;
    std::vector<MatchEvent> outcomeEvents;
};

namespace match_resolution {

double opportunityProbability(double effectiveAttack,
                              double effectiveDefense,
                              double possessionFactor,
                              double mentalityFactor,
                              double matchMomentFactor);
ChanceResolutionOutput resolveChance(const Team& attacking,
                                     const Team& defending,
                                     const std::vector<int>& attackingXI,
                                     const std::vector<int>& defendingXI,
                                     const ChanceResolutionInput& input);

}  // namespace match_resolution
