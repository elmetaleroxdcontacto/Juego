#include "career/player_development.h"

#include "development/training_impact_system.h"
#include "development/youth_generation_system.h"

namespace player_dev {

void applyWeeklyTrainingPlan(Team& team) {
    development::applyWeeklyTrainingImpact(team);
}

void addYouthPlayers(Team& team, int count) {
    development::generateYouthIntake(team, count);
}

}  // namespace player_dev
