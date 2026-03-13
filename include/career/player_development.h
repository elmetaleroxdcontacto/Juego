#pragma once

#include "engine/models.h"

namespace player_dev {

void applyWeeklyTrainingPlan(Team& team, bool congestedWeek = false);
void addYouthPlayers(Team& team, int count);

}  // namespace player_dev
