#include "simulation/match_engine_internal.h"

#include "utils.h"

using namespace std;

namespace match_internal {

void pushTacticalEvent(vector<string>* events, int minute, const string& text) {
    if (!events || text.empty()) return;
    events->push_back(to_string(minute) + "' " + text);
}

void applyMatchFatigue(Team& team, const vector<int>& xi, const string& tactics) {
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& p = team.players[idx];
        int drain = randInt(6, 14);
        if (p.stamina < 60) drain += 2;
        if (p.age > 30) drain += (p.age - 30) / 3;
        if (p.injured) drain += 2;
        if (playerHasTrait(p, "Presiona")) drain += 1;
        if (playerHasTrait(p, "Llega al area")) drain += 1;
        if (tactics == "Pressing") drain += 3;
        else if (tactics == "Offensive") drain += 1;
        else if (tactics == "Defensive") drain -= 1;
        else if (tactics == "Counter") drain -= 1;
        if (team.matchInstruction == "Presion final") drain += 2;
        else if (team.matchInstruction == "Contra-presion") drain += 2;
        else if (team.matchInstruction == "Juego directo") drain -= 1;
        else if (team.matchInstruction == "Pausar juego") drain -= 2;
        drain += team.pressingIntensity - 3;
        drain += team.tempo - 3;
        drain -= max(0, p.tacticalDiscipline - 70) / 12;
        if (team.rotationPolicy == "Rotacion") drain -= 1;
        p.fitness = clampInt(p.fitness - drain, 15, p.stamina);
    }
}

void assignGoalsAndAssists(Team& team, int goals, const vector<int>& xi, const string& teamName, vector<string>* events) {
    if (goals <= 0 || xi.empty()) return;
    vector<int> attackers;
    for (int idx : xi) {
        string pos = normalizePosition(team.players[idx].position);
        string role = compactToken(team.players[idx].role);
        if (pos == "DEL" || pos == "MED") {
            attackers.push_back(idx);
            if (playerHasTrait(team.players[idx], "Llega al area") ||
                playerHasTrait(team.players[idx], "Competidor")) {
                attackers.push_back(idx);
            }
            if (role == "poacher" || role == "objetivo" || role == "interior") attackers.push_back(idx);
        }
    }
    if (attackers.empty()) attackers = xi;

    for (int i = 0; i < goals; ++i) {
        int scorerIdx = attackers[randInt(0, static_cast<int>(attackers.size()) - 1)];
        team.players[scorerIdx].goals++;

        string assistName;
        if (xi.size() > 1) {
            int assistIdx = scorerIdx;
            int guard = 0;
            while (assistIdx == scorerIdx && guard < 10) {
                assistIdx = xi[randInt(0, static_cast<int>(xi.size()) - 1)];
                guard++;
            }
            if (assistIdx != scorerIdx) {
                if (playerHasTrait(team.players[assistIdx], "Pase riesgoso") && xi.size() > 2 && randInt(1, 100) <= 35) {
                    int retry = xi[randInt(0, static_cast<int>(xi.size()) - 1)];
                    if (retry != scorerIdx) assistIdx = retry;
                }
                string assistRole = compactToken(team.players[assistIdx].role);
                if (assistRole != "enganche" && assistRole != "organizador" && xi.size() > 2) {
                    for (int candidate : xi) {
                        if (candidate == scorerIdx) continue;
                        string candidateRole = compactToken(team.players[candidate].role);
                        if (candidateRole == "enganche" || candidateRole == "organizador") {
                            assistIdx = candidate;
                            break;
                        }
                    }
                }
                team.players[assistIdx].assists++;
                assistName = team.players[assistIdx].name;
            }
        }
        if (events) {
            int minute = randInt(1, 90);
            string text = to_string(minute) + "' " + teamName + ": " + team.players[scorerIdx].name;
            if (!assistName.empty()) text += " (asist: " + assistName + ")";
            string cause;
            string scorerRole = compactToken(team.players[scorerIdx].role);
            if (playerHasTrait(team.players[scorerIdx], "Cita grande")) cause = "aparece en la cita grande";
            else if (playerHasTrait(team.players[scorerIdx], "Llega al area")) cause = "rompe desde segunda linea";
            else if (scorerRole == "objetivo") cause = "gana arriba";
            else if (scorerRole == "poacher") cause = "ataca el espacio";
            else if (scorerRole == "enganche" || scorerRole == "organizador") cause = "culmina una jugada asociada";
            if (!cause.empty()) text += " [" + cause + "]";
            events->push_back(text);
        }
    }
}

void applyIntensityInjuryRisk(Team& team, const vector<int>& participants, vector<string>* events) {
    int extraRisk = max(0, team.pressingIntensity - 3) * 3;
    if (team.matchInstruction == "Presion final") extraRisk += 4;
    if (team.matchInstruction == "Contra-presion") extraRisk += 2;
    if (extraRisk <= 0) return;
    for (int idx : participants) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& player = team.players[idx];
        if (player.injured || player.fitness > 56) continue;
        int risk = extraRisk + max(0, 58 - player.fitness) / 4;
        if (playerHasTrait(player, "Fragil")) risk += 3;
        if (randInt(1, 100) > risk) continue;
        player.injured = true;
        player.injuryHistory++;
        player.injuryType = "Leve";
        player.injuryWeeks = max(player.injuryWeeks, randInt(1, 2));
        pushTacticalEvent(events, randInt(70, 90), "La intensidad rompe a " + player.name + " en " + team.name);
        break;
    }
}

}  // namespace match_internal
