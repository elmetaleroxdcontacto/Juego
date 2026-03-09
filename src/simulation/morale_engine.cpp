#include "simulation/morale_engine.h"

#include "utils/utils.h"

using namespace std;

namespace morale_engine {

double collectiveMoraleFactor(const Team& team, const vector<int>& xi, bool keyMatch) {
    int total = team.morale * 2;
    int count = 2;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[idx];
        total += player.happiness + player.currentForm / 2 + player.chemistry / 2;
        if (playerHasTrait(player, "Lider")) total += 6;
        if (playerHasTrait(player, "Cita grande") && keyMatch) total += 8;
        if (playerHasTrait(player, "Caliente") && keyMatch) total -= 3;
        count += 2;
    }
    int collective = count > 0 ? total / count : team.morale;
    double factor = 0.88 + collective / 230.0;
    if (keyMatch) factor += 0.02;
    return clampValue(factor, 0.86, 1.18);
}

int postMatchMoraleDelta(const Team& team, int goalsFor, int goalsAgainst, bool keyMatch) {
    int delta = 0;
    if (goalsFor > goalsAgainst) delta = 4;
    else if (goalsFor < goalsAgainst) delta = -3;
    else delta = 1;

    const int expectation = teamPrestigeScore(team);
    if (goalsFor > goalsAgainst && expectation <= 48) delta += 1;
    if (goalsFor < goalsAgainst && expectation >= 68) delta -= 1;
    if (keyMatch) delta += (delta >= 0) ? 1 : -1;
    return delta;
}

}  // namespace morale_engine
