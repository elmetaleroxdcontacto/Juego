#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

struct SquadNeedReport {
    std::string weakestPosition;
    std::string surplusPosition;
    int averageAge = 0;
    int averageFitness = 0;
    int weakestScore = 0;
    int surplusScore = 0;
    std::vector<std::string> priorityPositions;
};

namespace ai_squad_planner {

int positionNeedScore(const Team& team, const std::string& pos);
SquadNeedReport analyzeSquad(const Team& team);

}  // namespace ai_squad_planner
