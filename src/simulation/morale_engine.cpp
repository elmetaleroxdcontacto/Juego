#include "simulation/morale_engine.h"

#include "utils/utils.h"

#include <map>

using namespace std;

namespace morale_engine {

double collectiveMoraleFactor(const Team& team, const vector<int>& xi, bool keyMatch) {
    int total = team.morale * 2;
    int count = 2;
    int wantsOut = 0;
    int fatigueStress = 0;
    int positionPromiseConflicts = 0;
    map<string, int> socialMix;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[idx];
        total += player.happiness + player.currentForm / 2 + player.chemistry / 2;
        total += player.leadership / 8 + player.discipline / 10 + player.adaptation / 12;
        total += player.moraleMomentum / 2;
        if (playerHasTrait(player, "Lider")) total += 6;
        if (player.personality == "Temperamental" && player.happiness < 48) total -= 5;
        if (player.personality == "Profesional" && player.currentForm < 45) total += 2;
        if (playerHasTrait(player, "Cita grande") && keyMatch) total += 8;
        if (playerHasTrait(player, "Caliente") && keyMatch) total -= 3;
        if (player.wantsToLeave) wantsOut++;
        if (player.fitness < 60 || player.fatigueLoad >= 60) fatigueStress++;
        if (!player.promisedPosition.empty() && normalizePosition(player.promisedPosition) != normalizePosition(player.position)) {
            positionPromiseConflicts++;
        }
        if (!player.socialGroup.empty()) socialMix[player.socialGroup]++;
        count += 2;
    }
    int collective = count > 0 ? total / count : team.morale;
    double factor = 0.88 + collective / 230.0;
    int largestGroup = 0;
    int frustratedGroup = 0;
    for (const auto& entry : socialMix) {
        largestGroup = max(largestGroup, entry.second);
        if (entry.first == "Frustrados") frustratedGroup = entry.second;
    }
    if (largestGroup >= 4) factor += 0.02;
    if (socialMix.size() >= 4) factor -= 0.01;
    if (frustratedGroup >= 2) factor -= min(0.05, frustratedGroup * 0.015);
    factor -= wantsOut * 0.008;
    factor -= fatigueStress * 0.006;
    factor -= positionPromiseConflicts * 0.008;
    if (socialMix.count("Lideres") > 0) factor += min(0.03, socialMix["Lideres"] * 0.01);
    if (keyMatch) factor += 0.02;
    return clampValue(factor, 0.84, 1.18);
}

int postMatchMoraleDelta(const Team& team, int goalsFor, int goalsAgainst, bool keyMatch) {
    int delta = 0;
    if (goalsFor > goalsAgainst) delta = 4;
    else if (goalsFor < goalsAgainst) delta = -3;
    else delta = 1;

    const int expectation = teamPrestigeScore(team);
    if (goalsFor > goalsAgainst && expectation <= 48) delta += 1;
    if (goalsFor < goalsAgainst && expectation >= 68) delta -= 1;
    int positiveMomentum = 0;
    int negativeMomentum = 0;
    for (const auto& player : team.players) {
        if (player.moraleMomentum >= 8) positiveMomentum++;
        if (player.moraleMomentum <= -8) negativeMomentum++;
    }
    if (goalsFor > goalsAgainst && positiveMomentum >= 4) delta += 1;
    if (goalsFor < goalsAgainst && negativeMomentum >= 4) delta -= 1;
    if (keyMatch) delta += (delta >= 0) ? 1 : -1;
    return delta;
}

}  // namespace morale_engine
