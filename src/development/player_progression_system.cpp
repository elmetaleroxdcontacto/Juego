#include "development/player_progression_system.h"

#include "simulation/player_condition.h"
#include "utils/utils.h"

using namespace std;

namespace development {

void applyMatchExperience(Team& team, const vector<int>& participants, vector<string>* events) {
    for (int idx : participants) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& player = team.players[static_cast<size_t>(idx)];
        if (player.age > 34 || player.injured) continue;
        const int workload = player_condition::workloadRisk(player, team);
        const int stability = player_condition::developmentStability(player, team, false);
        const int readiness = player_condition::readinessScore(player, team);

        int growthChance = 2;
        growthChance += max(0, player.potential - player.skill) / 3;
        growthChance += max(0, 23 - player.age);
        growthChance += max(0, player.professionalism - 55) / 10;
        growthChance += max(0, player.currentForm - 50) / 12;
        growthChance += max(0, player.happiness - 50) / 14;
        growthChance += max(0, team.trainingFacilityLevel - 1) * 2;
        growthChance += max(0, team.assistantCoach - 55) / 12;
        growthChance += max(0, team.youthCoach - 55) / 14;
        growthChance += (player.matchesPlayed > player.startsThisSeason) ? 1 : 0;
        growthChance += stability / 16;
        growthChance += readiness / 18;
        if (player.age <= 21) growthChance += 2;
        if (player.developmentPlan == "Finalizacion" && normalizePosition(player.position) == "DEL") growthChance += 3;
        if (player.developmentPlan == "Creatividad" && normalizePosition(player.position) == "MED") growthChance += 3;
        if (player.developmentPlan == "Defensa" &&
            (normalizePosition(player.position) == "DEF" || normalizePosition(player.position) == "ARQ")) {
            growthChance += 3;
        }
        growthChance -= workload / 14;
        growthChance = clampInt(growthChance, 2, 32);

        if (player.skill < player.potential && randInt(1, 100) <= growthChance) {
            player.skill = min(100, player.skill + 1);
            const string pos = normalizePosition(player.position);
            if (pos == "ARQ" || pos == "DEF") {
                player.defense = min(100, player.defense + 1);
            } else if (pos == "MED") {
                player.attack = min(100, player.attack + 1);
                player.defense = min(100, player.defense + 1);
            } else {
                player.attack = min(100, player.attack + 1);
            }
            player.currentForm = clampInt(player.currentForm + 1, 1, 99);
            player.moraleMomentum = clampInt(player.moraleMomentum + 1, -25, 25);
            if (events) {
                events->push_back("[Desarrollo] " + player.name + " eleva su nivel a " + to_string(player.skill) + ".");
            }
        } else if (player.age >= 31 &&
                   randInt(1, 100) <= clampInt(player.age - 28 + max(0, 62 - player.fitness) / 4 + workload / 20, 2, 18)) {
            player.stamina = max(35, player.stamina - 1);
            if (player.age >= 33 && player.attack > 35) player.attack = max(35, player.attack - 1);
        }

        if (player.age <= 22) {
            int potentialDelta = 0;
            if (player.matchesPlayed >= 6) potentialDelta++;
            if (player.startsThisSeason >= 4) potentialDelta++;
            if (player.happiness >= 60 && player.currentForm >= 58) potentialDelta++;
            if (player.happiness <= 42 || workload >= 72) potentialDelta--;
            if (potentialDelta >= 2 && randInt(1, 100) <= 10) {
                player.potential = min(99, player.potential + 1);
            } else if (potentialDelta <= -1 && player.potential > player.skill && randInt(1, 100) <= 8) {
                player.potential = max(player.skill, player.potential - 1);
            }
        }
    }
}

}  // namespace development
