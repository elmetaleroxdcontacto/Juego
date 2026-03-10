#pragma once

#include "simulation/match_context.h"

struct MatchPhaseEvaluation {
    MatchPhaseReport report;
    int homePossessionChains = 0;
    int awayPossessionChains = 0;
    int homeProgressions = 0;
    int awayProgressions = 0;
    int homeAttacks = 0;
    int awayAttacks = 0;
    int homeChanceCount = 0;
    int awayChanceCount = 0;
    double homeAttack = 0.0;
    double awayAttack = 0.0;
    double homeDefense = 0.0;
    double awayDefense = 0.0;
};

namespace match_phase {

MatchPhaseEvaluation evaluatePhase(const MatchSetup& setup,
                                   const Team& home,
                                   const Team& away,
                                   const TeamMatchSnapshot& homeSnapshot,
                                   const TeamMatchSnapshot& awaySnapshot,
                                   int phaseIndex,
                                   int minuteStart,
                                   int minuteEnd);

}  // namespace match_phase
