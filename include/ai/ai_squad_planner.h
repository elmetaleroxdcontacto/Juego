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
    int rotationRisk = 0;
    int unusedSeniorPlayers = 0;
    int salePressure = 0;
    std::vector<std::string> priorityPositions;
    std::vector<std::string> thinPositions;
    std::vector<std::string> youthCoverPositions;
    std::vector<std::string> saleCandidates;
};

namespace ai_squad_planner {

int positionNeedScore(const Team& team, const std::string& pos);
SquadNeedReport analyzeSquad(const Team& team);

}  // namespace ai_squad_planner
