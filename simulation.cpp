#include "simulation.h"

#include "io.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

TeamStrength computeStrength(Team& team) {
    TeamStrength ts;
    ts.xi = team.getStartingXIIndices();
    ts.attack = 0;
    ts.defense = 0;
    ts.avgSkill = 0;
    ts.avgStamina = 0;
    if (ts.xi.empty()) return ts;
    int skill = 0;
    int stamina = 0;
    for (int idx : ts.xi) {
        ts.attack += team.players[idx].attack;
        ts.defense += team.players[idx].defense;
        skill += team.players[idx].skill;
        stamina += team.players[idx].stamina;
    }
    ts.avgSkill = skill / static_cast<int>(ts.xi.size());
    ts.avgStamina = stamina / static_cast<int>(ts.xi.size());
    return ts;
}

bool hasInjuredInXI(const Team& team, const vector<int>& xi) {
    for (int idx : xi) {
        if (idx >= 0 && idx < static_cast<int>(team.players.size()) && team.players[idx].injured) {
            return true;
        }
    }
    return false;
}

void applyTactics(const string& tactics, int& attack, int& defense) {
    if (tactics == "Defensive") {
        attack = attack * 90 / 100;
        defense = defense * 110 / 100;
    } else if (tactics == "Offensive") {
        attack = attack * 110 / 100;
        defense = defense * 90 / 100;
    }
}

double calcLambda(int attack, int defense) {
    if (attack <= 0 || defense <= 0) return 0.2;
    double ratio = static_cast<double>(attack) / static_cast<double>(defense);
    double lambda = 1.2 * ratio;
    if (lambda < 0.2) lambda = 0.2;
    if (lambda > 3.5) lambda = 3.5;
    return lambda;
}

int samplePoisson(double lambda) {
    double L = exp(-lambda);
    int k = 0;
    double p = 1.0;
    do {
        k++;
        p *= rand01();
    } while (p > L);
    return k - 1;
}

void simulateInjury(Player& player, bool verbose) {
    if (player.injured) return;
    int risk = 5;
    if (player.stamina < 60) risk = 7;
    if (randInt(1, 100) <= risk) {
        player.injured = true;
        player.injuryWeeks = randInt(1, 4);
        if (verbose) {
            cout << player.name << " se lesiono y estara fuera por " << player.injuryWeeks << " semanas." << endl;
        }
    }
}

void healInjuries(Team& team, bool verbose) {
    for (auto& player : team.players) {
        if (player.injured) {
            player.injuryWeeks--;
            if (player.injuryWeeks <= 0) {
                player.injured = false;
                if (verbose) {
                    cout << player.name << " se recupero de su lesion." << endl;
                }
            }
        }
    }
}

void assignGoalsAndAssists(Team& team, int goals, const vector<int>& xi) {
    if (goals <= 0 || xi.empty()) return;
    vector<int> attackers;
    for (int idx : xi) {
        string pos = normalizePosition(team.players[idx].position);
        if (pos == "DEL" || pos == "MED") attackers.push_back(idx);
    }
    if (attackers.empty()) attackers = xi;

    for (int i = 0; i < goals; ++i) {
        int scorerIdx = attackers[randInt(0, static_cast<int>(attackers.size()) - 1)];
        team.players[scorerIdx].goals++;

        if (xi.size() > 1) {
            int assistIdx = scorerIdx;
            int guard = 0;
            while (assistIdx == scorerIdx && guard < 10) {
                assistIdx = xi[randInt(0, static_cast<int>(xi.size()) - 1)];
                guard++;
            }
            if (assistIdx != scorerIdx) team.players[assistIdx].assists++;
        }
    }
}

MatchResult playMatch(Team& home, Team& away, bool verbose) {
    ensureMinimumSquad(home, 11);
    ensureMinimumSquad(away, 11);

    auto h = computeStrength(home);
    auto a = computeStrength(away);
    bool homeInj = hasInjuredInXI(home, h.xi);
    bool awayInj = hasInjuredInXI(away, a.xi);
    if (homeInj || awayInj) {
        cout << "[AVISO] Equipos sin suficientes jugadores sanos: ";
        if (homeInj) cout << home.name;
        if (homeInj && awayInj) cout << " y ";
        if (awayInj) cout << away.name;
        cout << ". Se usan lesionados en el XI." << endl;
    }

    int homeAttack = h.attack;
    int homeDefense = h.defense;
    int awayAttack = a.attack;
    int awayDefense = a.defense;

    applyTactics(home.tactics, homeAttack, homeDefense);
    applyTactics(away.tactics, awayAttack, awayDefense);

    homeAttack += h.avgSkill;
    homeDefense += h.avgSkill;
    awayAttack += a.avgSkill;
    awayDefense += a.avgSkill;

    homeAttack = homeAttack * max(30, h.avgStamina) / 100;
    homeDefense = homeDefense * max(30, h.avgStamina) / 100;
    awayAttack = awayAttack * max(30, a.avgStamina) / 100;
    awayDefense = awayDefense * max(30, a.avgStamina) / 100;

    int homeGoals = samplePoisson(calcLambda(homeAttack, awayDefense));
    int awayGoals = samplePoisson(calcLambda(awayAttack, homeDefense));
    homeGoals = min(homeGoals, 7);
    awayGoals = min(awayGoals, 7);

    if (verbose) {
        cout << "\n--- Partido ---" << endl;
        cout << home.name << " vs " << away.name << endl;
        cout << "Tacticas: " << home.tactics << " vs " << away.tactics << endl;
        cout << "Resultado Final: " << home.name << " " << homeGoals << " - " << awayGoals << " " << away.name << endl;
    }

    home.goalsFor += homeGoals;
    home.goalsAgainst += awayGoals;
    away.goalsFor += awayGoals;
    away.goalsAgainst += homeGoals;

    if (homeGoals > awayGoals) {
        home.points += 3;
        home.wins++;
        away.losses++;
        if (verbose) cout << "Ganaste!" << endl;
    } else if (homeGoals < awayGoals) {
        away.points += 3;
        away.wins++;
        home.losses++;
        if (verbose) cout << "Perdiste." << endl;
    } else {
        home.points += 1;
        away.points += 1;
        home.draws++;
        away.draws++;
        if (verbose) cout << "Empate." << endl;
    }

    for (int idx : h.xi) home.players[idx].matchesPlayed++;
    for (int idx : a.xi) away.players[idx].matchesPlayed++;
    assignGoalsAndAssists(home, homeGoals, h.xi);
    assignGoalsAndAssists(away, awayGoals, a.xi);

    for (int idx : h.xi) simulateInjury(home.players[idx], verbose);
    for (int idx : a.xi) simulateInjury(away.players[idx], verbose);

    return {homeGoals, awayGoals};
}
