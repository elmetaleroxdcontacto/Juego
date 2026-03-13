#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

namespace development {

struct TrainingSessionPlan {
    std::string day;
    std::string focus;
    std::string note;
    int load = 0;
};

std::vector<TrainingSessionPlan> buildWeeklyTrainingSchedule(const Team& team, bool congestedWeek = false);
std::string formatWeeklyTrainingSchedule(const Team& team, bool congestedWeek = false);
void applyWeeklyTrainingImpact(Team& team, bool congestedWeek = false);

}  // namespace development
