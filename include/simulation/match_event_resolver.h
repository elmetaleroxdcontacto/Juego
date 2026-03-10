#pragma once

#include "simulation/match_resolution.h"

namespace match_event_resolver {

ChanceResolutionOutput resolveChance(const Team& attacking,
                                     const Team& defending,
                                     const std::vector<int>& attackingXI,
                                     const std::vector<int>& defendingXI,
                                     const ChanceResolutionInput& input);

}  // namespace match_event_resolver
