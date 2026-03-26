#include "development/monthly_development.h"

#include "competition/competition.h"
#include "development/youth_generation_system.h"
#include "simulation/player_condition.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

int planAlignmentBonus(const Player& player) {
    const string position = normalizePosition(player.position);
    if (player.developmentPlan == "Finalizacion" && position == "DEL") return 4;
    if (player.developmentPlan == "Creatividad" && position == "MED") return 4;
    if (player.developmentPlan == "Defensa" && (position == "DEF" || position == "ARQ")) return 4;
    if (player.developmentPlan == "Reflejos" && position == "ARQ") return 5;
    if (player.developmentPlan == "Fisico") return 3;
    if (player.developmentPlan == "Liderazgo") return 2;
    return 1;
}

void applyPlanSpecificGrowth(Player& player) {
    const string position = normalizePosition(player.position);
    if (player.developmentPlan == "Liderazgo") {
        player.leadership = min(99, player.leadership + 1);
        player.professionalism = min(99, player.professionalism + 1);
        return;
    }
    if (player.developmentPlan == "Creatividad") {
        player.setPieceSkill = min(99, player.setPieceSkill + 1);
    } else if (player.developmentPlan == "Fisico") {
        player.stamina = min(100, player.stamina + 1);
        player.fitness = min(player.stamina, player.fitness + 1);
    }

    if (position == "ARQ" || position == "DEF") {
        player.defense = min(100, player.defense + 1);
    } else if (position == "MED") {
        player.attack = min(100, player.attack + 1);
        player.defense = min(100, player.defense + 1);
    } else {
        player.attack = min(100, player.attack + 1);
    }
}

}  // namespace

namespace development {

MonthlyDevelopmentSummary runMonthlyDevelopmentCycle(Team& team, int currentWeek) {
    MonthlyDevelopmentSummary summary;

    for (auto& player : team.players) {
        const int stability = player_condition::developmentStability(player, team, false);
        const int readiness = player_condition::readinessScore(player, team);
        const int workload = player_condition::workloadRisk(player, team);

        if (player.age >= 29 && player.professionalism >= 78 && player.leadership < 99 &&
            randInt(1, 100) <= 10 + max(0, team.assistantCoach - 60) / 4) {
            player.leadership = min(99, player.leadership + 1);
        }

        if (player.age > 24 || player.skill >= player.potential || player.injured) continue;

        int growthChance = 5;
        growthChance += max(0, player.potential - player.skill) / 3;
        growthChance += max(0, 22 - player.age);
        growthChance += min(10, player.matchesPlayed + player.startsThisSeason * 2);
        growthChance += stability / 9;
        growthChance += readiness / 14;
        growthChance += planAlignmentBonus(player);
        growthChance += max(0, team.youthCoach - 55) / 8;
        growthChance += max(0, team.trainingFacilityLevel - 1) * 2;
        growthChance += currentWeek >= 20 ? 2 : 0;
        growthChance -= workload / 18;
        growthChance = clampInt(growthChance, 4, 34);

        if (randInt(1, 100) > growthChance) continue;

        int gain = 1;
        if (player.age <= 19 && player.potential - player.skill >= 10 && stability >= 64 && randInt(1, 100) <= 28) {
            gain++;
        }

        player.skill = min(100, player.skill + gain);
        applyPlanSpecificGrowth(player);
        player.currentForm = clampInt(player.currentForm + 1, 1, 99);
        player.moraleMomentum = clampInt(player.moraleMomentum + 1, -25, 25);
        summary.improvedPlayers++;
        if (gain > 1) summary.acceleratedProspects++;

        if (player.age <= 20 && stability >= 70 && player.potential < 99 && randInt(1, 100) <= 12) {
            player.potential = min(99, player.potential + 1);
        }
    }

    const int maxSquad = getCompetitionConfig(team.division).maxSquadSize;
    const bool hasRoom = maxSquad <= 0 || static_cast<int>(team.players.size()) < maxSquad;
    const int intakeChance = 4 + team.youthFacilityLevel * 3 + max(0, team.youthCoach - 60) / 5;
    if (hasRoom && randInt(1, 100) <= intakeChance) {
        const size_t before = team.players.size();
        generateYouthIntake(team, team.youthFacilityLevel >= 4 && randInt(1, 100) <= 22 ? 2 : 1);
        summary.newYouthPlayers = static_cast<int>(team.players.size() - before);
    }

    return summary;
}

}  // namespace development
