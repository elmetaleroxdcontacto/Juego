#pragma once

#include "engine/models.h"

#include <vector>

struct TacticalProfile {
    double mentality = 0.0;
    double pressing = 0.0;
    double tempo = 0.0;
    double width = 0.0;
    double defensiveBlock = 0.0;
    double directness = 0.0;
    double transitionThreat = 0.0;
    double setPieceThreat = 0.0;
    double risk = 0.0;
};

namespace tactics_engine {

TacticalProfile buildTacticalProfile(const Team& team);
double tacticalCompatibility(const Team& team, const std::vector<int>& xi);
double tacticalAdvantage(const Team& team,
                         const Team& opponent,
                         const TacticalProfile& teamProfile,
                         const TacticalProfile& opponentProfile,
                         const std::vector<int>& xi);
double possessionWeight(const TacticalProfile& profile);
double transitionThreatWeight(const TacticalProfile& profile);
double defensiveSecurityWeight(const TacticalProfile& profile);

}  // namespace tactics_engine
