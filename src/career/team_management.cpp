#include "career/team_management.h"

#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace team_mgmt {

int playerIndexByName(const Team& team, const string& name) {
    for (size_t i = 0; i < team.players.size(); ++i) {
        if (team.players[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

void detachPlayerFromSelections(Team& team, const string& playerName) {
    team.preferredXI.erase(remove(team.preferredXI.begin(), team.preferredXI.end(), playerName), team.preferredXI.end());
    team.preferredBench.erase(remove(team.preferredBench.begin(), team.preferredBench.end(), playerName), team.preferredBench.end());
    if (team.captain == playerName) team.captain.clear();
    if (team.penaltyTaker == playerName) team.penaltyTaker.clear();
    if (team.freeKickTaker == playerName) team.freeKickTaker.clear();
    if (team.cornerTaker == playerName) team.cornerTaker.clear();
}

void applyDepartureShock(Team& team, const Player& player) {
    if (player.skill < team.getAverageSkill()) return;
    team.morale = clampInt(team.morale - 3, 0, 100);
    for (auto& mate : team.players) {
        if (mate.name == player.name) continue;
        mate.chemistry = clampInt(mate.chemistry - 1, 1, 99);
        if (player.leadership >= 70) {
            mate.happiness = clampInt(mate.happiness - 1, 1, 99);
        }
    }
}

}  // namespace team_mgmt
