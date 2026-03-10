#pragma once

#include "simulation/match_engine.h"

#include <vector>

namespace match_postprocess {

void applySimulationOutcome(Team& home,
                            Team& away,
                            const match_engine::MatchSimulationData& simulation,
                            const std::vector<int>& homeStartXI,
                            const std::vector<int>& awayStartXI,
                            bool keyMatch);

}  // namespace match_postprocess
