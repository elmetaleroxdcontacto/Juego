#pragma once

#include "engine/models.h"

#include <vector>

namespace morale_engine {

double collectiveMoraleFactor(const Team& team, const std::vector<int>& xi, bool keyMatch);
int postMatchMoraleDelta(const Team& team, int goalsFor, int goalsAgainst, bool keyMatch);

}  // namespace morale_engine
