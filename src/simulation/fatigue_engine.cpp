#include "simulation/fatigue_engine.h"

#include "utils/utils.h"

using namespace std;

namespace fatigue_engine {

double collectiveFatigueFactor(const Team& team, const vector<int>& xi) {
    if (xi.empty()) return 0.94;
    int totalFitness = 0;
    int totalStamina = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[idx];
        totalFitness += clampInt(player.fitness, 1, 100);
        totalStamina += clampInt(player.stamina, 1, 100);
    }
    const double avgFitness = static_cast<double>(totalFitness) / max(1, static_cast<int>(xi.size()));
    const double avgStamina = static_cast<double>(totalStamina) / max(1, static_cast<int>(xi.size()));
    double factor = 0.82 + avgFitness / 190.0 + avgStamina / 500.0;
    if (team.rotationPolicy == "Rotacion") factor += 0.02;
    return clampValue(factor, 0.78, 1.08);
}

int phaseFatigueGain(const Team& team, int phaseIndex) {
    int gain = 2 + phaseIndex;
    gain += max(0, team.pressingIntensity - 3);
    gain += max(0, team.tempo - 3);
    if (team.tactics == "Pressing") gain += 1;
    if (team.tactics == "Counter") gain -= 1;
    if (team.matchInstruction == "Presion final" || team.matchInstruction == "Contra-presion") gain += 1;
    if (team.matchInstruction == "Pausar juego") gain -= 1;
    return clampInt(gain, 1, 8);
}

int substitutionNeedScore(const Team& team, int playerIndex, bool cautioned) {
    if (playerIndex < 0 || playerIndex >= static_cast<int>(team.players.size())) return 0;
    const Player& player = team.players[static_cast<size_t>(playerIndex)];
    int need = 0;
    if (player.injured) need += 100;
    need += max(0, 58 - player.fitness) * 2;
    need += max(0, player.age - 30);
    if (cautioned) need += 10;
    if (playerHasTrait(player, "Caliente")) need += 4;
    if (team.rotationPolicy == "Rotacion" && player.fitness < 66) need += 4;
    return need;
}

void applyPhaseFatigue(Team& team, const vector<int>& xi, int phaseIndex) {
    const int gain = phaseFatigueGain(team, phaseIndex);
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& player = team.players[static_cast<size_t>(idx)];
        int playerGain = gain;
        if (playerHasTrait(player, "Presiona")) playerGain += 1;
        if (playerHasTrait(player, "Llega al area")) playerGain += 1;
        if (player.age >= 31) playerGain += 1;
        player.fitness = clampInt(player.fitness - playerGain, 18, player.stamina);
    }
}

}  // namespace fatigue_engine
