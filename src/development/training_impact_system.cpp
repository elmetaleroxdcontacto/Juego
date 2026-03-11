#include "development/training_impact_system.h"

#include "utils/utils.h"

using namespace std;

namespace development {

void applyWeeklyTrainingImpact(Team& team) {
    const string focus = team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus;
    const bool technicalFocus = focus == "Tecnico" || focus == "Ataque";
    const bool tacticalFocus = focus == "Tactico" || focus == "Preparacion partido" || focus == "Defensa";
    const bool attackFocus = focus == "Ataque";
    const bool defenseFocus = focus == "Defensa";
    const bool enduranceFocus = focus == "Fisico" || focus == "Resistencia";
    const int facilityBonus = max(0, team.trainingFacilityLevel - 1);
    const int assistantBonus = max(0, team.assistantCoach - 55) / 15;
    const int fitnessBonus = max(0, team.fitnessCoach - 55) / 15;
    const int medicalBonus = max(0, team.medicalTeam - 55) / 15;
    const int youthBonus = max(0, team.youthCoach - 55) / 15;

    for (auto& player : team.players) {
        if (focus == "Recuperacion") {
            if (player.injured) {
                player.injuryWeeks = max(0, player.injuryWeeks - (1 + max(0, team.medicalTeam - 60) / 18));
                if (player.injuryWeeks == 0) {
                    player.injured = false;
                    player.injuryType.clear();
                }
            }
            player.fatigueLoad = max(0, player.fatigueLoad - (8 + fitnessBonus + medicalBonus));
            player.fitness = clampInt(player.fitness + 3 + facilityBonus + fitnessBonus + medicalBonus, 15, player.stamina);
            player.moraleMomentum = clampInt(player.moraleMomentum + 1, -25, 25);
            continue;
        }

        if (player.injured) continue;
        int recovery = 1 + facilityBonus + fitnessBonus + medicalBonus / 2;
        if (enduranceFocus) recovery += 2;
        if (focus == "Tactico" || defenseFocus) recovery += 1;
        recovery -= player.fatigueLoad / 22;
        if (player.age <= 21) recovery += youthBonus / 2;
        recovery = max(1, recovery);
        player.fitness = clampInt(player.fitness + recovery, 15, player.stamina);
        player.fatigueLoad = max(0, player.fatigueLoad - max(2, recovery));

        if (focus == "Preparacion partido") {
            player.currentForm = clampInt(player.currentForm + 1 + assistantBonus / 2, 1, 99);
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        } else if (focus == "Tactico" || defenseFocus) {
            player.tacticalDiscipline = clampInt(player.tacticalDiscipline + 1, 1, 99);
        }

        const int baseGrowth = clampInt(8 + max(0, player.potential - player.skill) / 4 +
                                            max(0, 23 - player.age) +
                                            max(0, player.professionalism - 55) / 10 +
                                            max(0, player.happiness - 50) / 12 +
                                            facilityBonus * 2 + assistantBonus + youthBonus +
                                            (technicalFocus ? 4 : 0) + (tacticalFocus ? 2 : 0) -
                                            player.fatigueLoad / 14,
                                        5, 30);
        if (player.skill < player.potential && randInt(1, 100) <= baseGrowth) {
            player.skill = min(100, player.skill + 1);
            if (technicalFocus || normalizePosition(player.position) == "DEL") player.attack = min(100, player.attack + 1);
            if (tacticalFocus || normalizePosition(player.position) == "DEF") player.defense = min(100, player.defense + 1);
            if (attackFocus && normalizePosition(player.position) != "ARQ") player.attack = min(100, player.attack + 1);
            if (defenseFocus) player.defense = min(100, player.defense + 1);
            player.moraleMomentum = clampInt(player.moraleMomentum + 1, -25, 25);
        }

        if (enduranceFocus && randInt(1, 100) <= clampInt(10 + facilityBonus * 2 + fitnessBonus * 2, 8, 28)) {
            player.stamina = min(100, player.stamina + 1);
            player.fitness = min(player.fitness, player.stamina);
        }
        if (attackFocus && randInt(1, 100) <= clampInt(8 + assistantBonus + max(0, player.professionalism - 55) / 15, 6, 24)) {
            player.setPieceSkill = min(99, player.setPieceSkill + 1);
        }
        if (player.age <= 23) {
            int potentialShift = 0;
            if (player.matchesPlayed >= max(1, team.points / 6) || player.startsThisSeason >= 2) potentialShift += 1;
            if (player.happiness >= 62 && player.fatigueLoad <= 30) potentialShift += 1;
            if (player.happiness <= 40 || player.fatigueLoad >= 70) potentialShift -= 1;
            if (player.professionalism >= 74) potentialShift += 1;
            if (potentialShift >= 2 && randInt(1, 100) <= 12 + youthBonus * 2) {
                player.potential = min(99, player.potential + 1);
            } else if (potentialShift <= -1 && player.potential > player.skill && randInt(1, 100) <= 8) {
                player.potential = max(player.skill, player.potential - 1);
            }
        }
    }

    if (focus == "Tactico" || defenseFocus) team.morale = clampInt(team.morale + 2 + assistantBonus, 0, 100);
    else if (focus == "Preparacion partido" || focus == "Recuperacion") team.morale = clampInt(team.morale + 1, 0, 100);
    else if (attackFocus) team.morale = clampInt(team.morale + 1, 0, 100);
}

}  // namespace development
