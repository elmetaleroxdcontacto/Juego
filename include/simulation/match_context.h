#pragma once

#include "simulation/match_types.h"
#include "simulation/tactics_engine.h"

#include <vector>

struct TeamMatchSnapshot {
    std::vector<int> xi;
    int attackPower = 0;
    int defensePower = 0;
    int midfieldControl = 0;
    int goalkeeperPower = 0;
    int chanceCreation = 0;
    int finishingQuality = 0;
    int pressResistance = 0;
    int defensiveShape = 0;
    int lineBreakThreat = 0;
    int pressingLoad = 0;
    int setPieceThreat = 0;
    int averageSkill = 0;
    int averageStamina = 0;
    int collectiveForm = 0;
    int collectiveMorale = 0;
    int collectiveFatigue = 0;
    double moraleFactor = 1.0;
    double fatigueFactor = 1.0;
    double tacticalCompatibility = 1.0;
    double tacticalAdvantage = 0.0;
    TacticalProfile tacticalProfile;
};

struct MatchSetup {
    MatchContext context;
    TeamMatchSnapshot home;
    TeamMatchSnapshot away;
};

namespace match_context {

MatchSetup buildMatchSetup(const Team& home, const Team& away, bool keyMatch, bool neutralVenue);
TeamMatchSnapshot rebuildSnapshot(const Team& team,
                                  const Team& opponent,
                                  const std::vector<int>& xi,
                                  bool keyMatch);

}  // namespace match_context
