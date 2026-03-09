#include "development/youth_generation_system.h"

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
        const string pos = normalizePosition(player.position);
        if (pos == "N/A") continue;
        counts[pos]++;
        totalSkill[pos] += player.skill;
    }

    string weakest = "MED";
    int weakestScore = 1000000;
    for (const auto& pos : positions) {
        const int count = counts[pos];
        const int avgSkill = count > 0 ? totalSkill[pos] / count : 0;
        const int score = count * 18 + avgSkill;
        if (score < weakestScore) {
            weakestScore = score;
            weakest = pos;
        }
    }
    return weakest;
}

vector<string> youthPositionPool(const Team& team) {
    vector<string> pool = {"ARQ", "DEF", "MED", "DEL"};
    if (team.youthRegion == "Norte") pool.insert(pool.end(), {"DEL", "MED", "MED"});
    else if (team.youthRegion == "Sur") pool.insert(pool.end(), {"DEF", "ARQ", "MED"});
    else if (team.youthRegion == "Patagonia") pool.insert(pool.end(), {"ARQ", "DEF", "DEF"});
    else if (team.youthRegion == "Centro") pool.insert(pool.end(), {"MED", "MED", "DEL"});
    else pool.insert(pool.end(), {"DEF", "MED", "DEL"});

    const string need = weakestYouthPosition(team);
    pool.push_back(need);
    pool.push_back(need);
    return pool;
}

}  // namespace

namespace development {

void generateYouthIntake(Team& team, int count) {
    int minSkill, maxSkill;
    getDivisionSkillRange(team.division, minSkill, maxSkill);
    const vector<string> pool = youthPositionPool(team);
    const int maxSquad = getCompetitionConfig(team.division).maxSquadSize;
    const int youthBonus = max(0, team.youthFacilityLevel - 1) + max(0, team.youthCoach - 55) / 12;

    for (int i = 0; i < count; ++i) {
        if (maxSquad > 0 && static_cast<int>(team.players.size()) >= maxSquad) return;
        const string pos = pool[static_cast<size_t>(randInt(0, static_cast<int>(pool.size()) - 1))];
        Player youth = makeRandomPlayer(pos, minSkill + youthBonus, maxSkill + youthBonus, 16, 18);
        const bool eliteProspect = randInt(1, 100) <= clampInt(6 + team.youthFacilityLevel * 3 +
                                                                    max(0, team.youthCoach - 60) / 5,
                                                                4, 30);
        const int identityBonus = (team.youthIdentity == "Cantera estructurada") ? 4
                                  : (team.youthIdentity == "Desarrollo mixto") ? 2
                                                                               : 0;
        const int potFloor = eliteProspect ? 14 + youthBonus + identityBonus : 9 + youthBonus + identityBonus / 2;
        const int potTop = eliteProspect ? 24 + youthBonus + identityBonus : 17 + youthBonus + identityBonus;
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

}  // namespace development
