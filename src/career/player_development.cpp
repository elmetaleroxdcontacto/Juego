#include "career/player_development.h"

#include "competition/competition.h"
#include "utils/utils.h"

#include <unordered_map>
#include <vector>

using namespace std;

namespace {

string weakestYouthPosition(const Team& team) {
    static const vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    unordered_map<string, int> counts;
    unordered_map<string, int> totalSkill;
    for (const auto& player : team.players) {
        string pos = normalizePosition(player.position);
        if (pos == "N/A") continue;
        counts[pos]++;
        totalSkill[pos] += player.skill;
    }

    string weakest = "MED";
    int weakestScore = 1000000;
    for (const auto& pos : positions) {
        int count = counts[pos];
        int avgSkill = count > 0 ? totalSkill[pos] / count : 0;
        int score = count * 18 + avgSkill;
        if (score < weakestScore) {
            weakestScore = score;
            weakest = pos;
        }
    }
    return weakest;
}

vector<string> youthPositionPool(const Team& team) {
    vector<string> pool = {"ARQ", "DEF", "MED", "DEL"};
    if (team.youthRegion == "Norte") {
        pool.insert(pool.end(), {"DEL", "MED", "MED"});
    } else if (team.youthRegion == "Sur") {
        pool.insert(pool.end(), {"DEF", "ARQ", "MED"});
    } else if (team.youthRegion == "Patagonia") {
        pool.insert(pool.end(), {"ARQ", "DEF", "DEF"});
    } else if (team.youthRegion == "Centro") {
        pool.insert(pool.end(), {"MED", "MED", "DEL"});
    } else {
        pool.insert(pool.end(), {"DEF", "MED", "DEL"});
    }

    string need = weakestYouthPosition(team);
    pool.push_back(need);
    pool.push_back(need);
    return pool;
}

int developmentChance(const Team& team, const Player& player, bool technicalFocus, bool tacticalFocus) {
    int gap = max(0, player.potential - player.skill);
    int chance = 4 + gap / 3;
    chance += max(0, 23 - player.age);
    chance += max(0, player.professionalism - 55) / 10;
    chance += max(0, player.happiness - 50) / 14;
    chance += max(0, player.currentForm - 50) / 16;
    chance += max(0, team.assistantCoach - 55) / 10;
    chance += max(0, team.trainingFacilityLevel - 1) * 2;
    if (technicalFocus) chance += 4;
    if (tacticalFocus) chance += 2;
    return clampInt(chance, 4, 34);
}

void progressPlayerAttributes(Player& player) {
    string pos = normalizePosition(player.position);
    if (pos == "ARQ" || pos == "DEF") {
        player.defense = min(100, player.defense + 1);
    } else if (pos == "MED") {
        player.attack = min(100, player.attack + 1);
        player.defense = min(100, player.defense + 1);
    } else {
        player.attack = min(100, player.attack + 1);
    }
}

}  // namespace

namespace player_dev {

void applyWeeklyTrainingPlan(Team& team) {
    string focus = team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus;
    bool technicalFocus = focus == "Tecnico";
    bool tacticalFocus = focus == "Tactico" || focus == "Preparacion partido";
    int facilityBonus = max(0, team.trainingFacilityLevel - 1);
    int assistantBonus = max(0, team.assistantCoach - 55) / 15;
    int fitnessBonus = max(0, team.fitnessCoach - 55) / 15;

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
            if (player.fitness >= player.stamina - 2) {
                player.currentForm = clampInt(player.currentForm + 1, 1, 99);
            }
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

        if (focus == "Fisico") {
            int staminaChance = clampInt(10 + facilityBonus * 2 + fitnessBonus * 2 +
                                             max(0, 25 - player.age) / 2,
                                         8, 28);
            if (randInt(1, 100) <= staminaChance) {
                player.stamina = min(100, player.stamina + 1);
                if (player.fitness > player.stamina) player.fitness = player.stamina;
            }
        }

        if (player.skill < player.potential && randInt(1, 100) <= developmentChance(team, player, technicalFocus, tacticalFocus)) {
            player.skill = min(100, player.skill + 1);
            progressPlayerAttributes(player);
            player.currentForm = clampInt(player.currentForm + 1, 1, 99);
        }

        int planChance = clampInt(10 + facilityBonus * 2 + assistantBonus + max(0, player.professionalism - 50) / 12, 6, 30);
        if (player.developmentPlan == "Fisico") {
            player.fitness = clampInt(player.fitness + 1, 15, player.stamina);
            if (randInt(1, 100) <= planChance) player.stamina = min(100, player.stamina + 1);
        } else if (player.developmentPlan == "Defensa" || player.developmentPlan == "Reflejos") {
            if (randInt(1, 100) <= planChance) player.defense = min(100, player.defense + 1);
        } else if (player.developmentPlan == "Creatividad") {
            if (randInt(1, 100) <= planChance) {
                player.attack = min(100, player.attack + 1);
                player.setPieceSkill = min(99, player.setPieceSkill + 1);
            }
        } else if (player.developmentPlan == "Finalizacion") {
            if (randInt(1, 100) <= planChance) player.attack = min(100, player.attack + 1);
        } else if (player.developmentPlan == "Liderazgo") {
            if (randInt(1, 100) <= max(8, planChance - 2)) {
                player.leadership = min(99, player.leadership + 1);
                if (randInt(1, 100) <= 40) player.professionalism = min(99, player.professionalism + 1);
            }
        }
    }

    if (focus == "Tactico") {
        team.morale = clampInt(team.morale + 2 + assistantBonus, 0, 100);
    } else if (focus == "Preparacion partido") {
        team.morale = clampInt(team.morale + 1 + assistantBonus, 0, 100);
    } else if (focus == "Recuperacion") {
        team.morale = clampInt(team.morale + 1, 0, 100);
    }
}

void addYouthPlayers(Team& team, int count) {
    int minSkill, maxSkill;
    getDivisionSkillRange(team.division, minSkill, maxSkill);
    vector<string> pool = youthPositionPool(team);
    int maxSquad = getCompetitionConfig(team.division).maxSquadSize;
    int youthBonus = max(0, team.youthFacilityLevel - 1) + max(0, team.youthCoach - 55) / 12;

    for (int i = 0; i < count; ++i) {
        if (maxSquad > 0 && static_cast<int>(team.players.size()) >= maxSquad) return;
        string pos = pool[randInt(0, static_cast<int>(pool.size()) - 1)];
        Player youth = makeRandomPlayer(pos, minSkill + youthBonus, maxSkill + youthBonus, 16, 18);
        bool eliteProspect = randInt(1, 100) <= clampInt(6 + team.youthFacilityLevel * 3 +
                                                             max(0, team.youthCoach - 60) / 5,
                                                         4, 30);
        int identityBonus = (team.youthIdentity == "Cantera estructurada") ? 4
                           : (team.youthIdentity == "Desarrollo mixto") ? 2
                                                                        : 0;
        int potFloor = eliteProspect ? 14 + youthBonus + identityBonus : 9 + youthBonus + identityBonus / 2;
        int potTop = eliteProspect ? 24 + youthBonus + identityBonus : 17 + youthBonus + identityBonus;
        youth.potential = clampInt(youth.skill + randInt(potFloor, potTop), youth.skill, 99);
        youth.chemistry = clampInt(youth.chemistry + max(0, team.youthCoach - 55) / 10, 1, 99);
        youth.professionalism = clampInt(youth.professionalism + max(0, team.youthCoach - 55) / 12 + identityBonus / 2, 1, 99);
        youth.happiness = clampInt(youth.happiness + identityBonus / 2, 1, 99);
        if (eliteProspect) {
            youth.value = max(youth.value, static_cast<long long>(youth.potential) * 12000LL);
            youth.role = (pos == "DEL") ? "Poacher" : (pos == "MED" ? "BoxToBox" : defaultRoleForPosition(pos));
        }
        team.addPlayer(youth);
    }
}

}  // namespace player_dev
