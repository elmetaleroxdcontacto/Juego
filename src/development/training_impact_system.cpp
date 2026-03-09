#include "development/training_impact_system.h"

#include "utils/utils.h"

using namespace std;

namespace development {

void applyWeeklyTrainingImpact(Team& team) {
    const string focus = team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus;
    const bool technicalFocus = focus == "Tecnico";
    const bool tacticalFocus = focus == "Tactico" || focus == "Preparacion partido";
    const int facilityBonus = max(0, team.trainingFacilityLevel - 1);
    const int assistantBonus = max(0, team.assistantCoach - 55) / 15;
    const int fitnessBonus = max(0, team.fitnessCoach - 55) / 15;

    for (auto& player : team.players) {
        if (focus == "Recuperacion") {
            if (player.injured) {
                player.injuryWeeks = max(0, player.injuryWeeks - (1 + max(0, team.medicalTeam - 60) / 20));
                if (player.injuryWeeks == 0) {
                    player.injured = false;
                    player.injuryType.clear();
                }
            }
            player.fitness = clampInt(player.fitness + 3 + facilityBonus + fitnessBonus, 15, player.stamina);
            continue;
        }

        if (player.injured) continue;
        int recovery = 1 + facilityBonus + fitnessBonus;
        if (focus == "Fisico") recovery += 2;
        if (focus == "Tactico") recovery += 1;
        player.fitness = clampInt(player.fitness + recovery, 15, player.stamina);

        if (focus == "Preparacion partido") {
            player.currentForm = clampInt(player.currentForm + 1 + assistantBonus / 2, 1, 99);
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        } else if (focus == "Tactico") {
            player.tacticalDiscipline = clampInt(player.tacticalDiscipline + 1, 1, 99);
        }

        const int baseGrowth = clampInt(8 + max(0, player.potential - player.skill) / 4 +
                                            max(0, 23 - player.age) +
                                            max(0, player.professionalism - 55) / 10 +
                                            facilityBonus * 2 + assistantBonus +
                                            (technicalFocus ? 4 : 0) + (tacticalFocus ? 2 : 0),
                                        5, 30);
        if (player.skill < player.potential && randInt(1, 100) <= baseGrowth) {
            player.skill = min(100, player.skill + 1);
            if (technicalFocus || normalizePosition(player.position) == "DEL") player.attack = min(100, player.attack + 1);
            if (tacticalFocus || normalizePosition(player.position) == "DEF") player.defense = min(100, player.defense + 1);
        }

        if (focus == "Fisico" && randInt(1, 100) <= clampInt(10 + facilityBonus * 2 + fitnessBonus * 2, 8, 28)) {
            player.stamina = min(100, player.stamina + 1);
            player.fitness = min(player.fitness, player.stamina);
        }
    }

    if (focus == "Tactico") team.morale = clampInt(team.morale + 2 + assistantBonus, 0, 100);
    else if (focus == "Preparacion partido" || focus == "Recuperacion") team.morale = clampInt(team.morale + 1, 0, 100);
}

}  // namespace development
