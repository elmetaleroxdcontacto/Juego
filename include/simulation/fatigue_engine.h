#pragma once

#include "engine/models.h"

#include <vector>

namespace fatigue_engine {

double collectiveFatigueFactor(const Team& team, const std::vector<int>& xi);
int phaseFatigueGain(const Team& team, int phaseIndex);
int substitutionNeedScore(const Team& team, int playerIndex, bool cautioned);
void applyPhaseFatigue(Team& team, const std::vector<int>& xi, int phaseIndex);

}  // namespace fatigue_engine
