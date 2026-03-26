#pragma once

#include "engine/models.h"

namespace development {

struct MonthlyDevelopmentSummary {
    int improvedPlayers = 0;
    int acceleratedProspects = 0;
    int newYouthPlayers = 0;
};

MonthlyDevelopmentSummary runMonthlyDevelopmentCycle(Team& team, int currentWeek);

}  // namespace development
