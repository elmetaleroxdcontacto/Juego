#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

struct SeasonTransitionSummary {
    std::string champion;
    std::vector<std::string> promotedTeams;
    std::vector<std::string> relegatedTeams;
    std::vector<std::string> lines;
    std::string note;
};

SeasonTransitionSummary endSeason(Career& career);
