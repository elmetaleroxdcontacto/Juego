#include "simulation.h"

#include "io.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

struct WeatherEffect {
    string name;
    double attackMul;
    double defenseMul;
    double staminaMul;
};

static WeatherEffect rollWeather() {
    int roll = randInt(1, 100);
    if (roll <= 55) return {"Despejado", 1.00, 1.00, 1.00};
    if (roll <= 75) return {"Lluvia", 0.93, 1.02, 0.95};
    if (roll <= 85) return {"Viento", 0.95, 1.00, 0.98};
    if (roll <= 95) return {"Calor", 0.98, 0.98, 0.90};
    return {"Frio", 0.98, 1.00, 0.93};
}

static int effectiveStamina(int avgStamina, double mul) {
    int val = static_cast<int>(round(static_cast<double>(avgStamina) * mul));
    return clampInt(val, 20, 100);
}

static void applyRoleModifier(const Player& p, int& attack, int& defense) {
    string role = toLower(trim(p.role));
    if (role == "stopper") {
        defense += 4;
    } else if (role == "carrilero") {
        attack += 3;
        defense -= 2;
    } else if (role == "enganche") {
        attack += 5;
        defense -= 3;
    } else if (role == "boxtobox") {
        attack += 2;
        defense += 2;
    } else if (role == "pivote") {
        defense += 5;
        attack -= 3;
    } else if (role == "poacher") {
        attack += 5;
    } else if (role == "falso9") {
        attack += 3;
        defense += 2;
    } else if (role == "pressing") {
        attack += 2;
        defense += 1;
    }
    attack = clampInt(attack, 1, 120);
    defense = clampInt(defense, 1, 120);
}

static void applyMatchFatigue(Team& team, const vector<int>& xi, const string& tactics) {
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& p = team.players[idx];
        int drain = randInt(6, 14);
        if (p.stamina < 60) drain += 2;
        if (p.age > 30) drain += (p.age - 30) / 3;
        if (p.injured) drain += 2;
        if (tactics == "Pressing") drain += 3;
        else if (tactics == "Offensive") drain += 1;
        else if (tactics == "Defensive") drain -= 1;
        else if (tactics == "Counter") drain -= 1;
        p.fitness = clampInt(p.fitness - drain, 15, p.stamina);
    }
}

static int cardsForTactics(const string& tactics, int minVal, int maxVal) {
    int base = randInt(minVal, maxVal);
    if (tactics == "Pressing") base += 1;
    else if (tactics == "Offensive") base += 1;
    else if (tactics == "Defensive") base -= 1;
    return clampInt(base, 0, 6);
}

static int redChanceForTactics(const string& tactics) {
    int chance = 5;
    if (tactics == "Pressing") chance += 4;
    else if (tactics == "Offensive") chance += 2;
    else if (tactics == "Defensive") chance -= 1;
    return clampInt(chance, 1, 15);
}

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
        int att = team.players[idx].attack;
        int def = team.players[idx].defense;
        applyRoleModifier(team.players[idx], att, def);
        ts.attack += att;
        ts.defense += def;
        skill += team.players[idx].skill;
        stamina += clampInt(team.players[idx].fitness, 0, 100);
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
    } else if (tactics == "Pressing") {
        attack = attack * 105 / 100;
        defense = defense * 105 / 100;
    } else if (tactics == "Counter") {
        attack = attack * 107 / 100;
        defense = defense * 97 / 100;
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

bool simulateInjury(Player& player, const string& tactics, bool verbose, vector<string>* events) {
    if (player.injured) return false;
    int risk = 4;
    if (player.fitness < 60) risk += 3;
    if (player.stamina < 60) risk += 2;
    if (player.age >= 32) risk += (player.age - 31) / 3;
    risk += player.injuryHistory / 3;
    if (tactics == "Pressing") risk += 3;
    else if (tactics == "Offensive") risk += 2;
    else if (tactics == "Counter") risk += 1;
    else if (tactics == "Defensive") risk -= 1;
    risk = clampInt(risk, 2, 15);
    if (randInt(1, 100) <= risk) {
        player.injured = true;
        player.injuryHistory++;
        int roll = randInt(1, 100);
        if (roll <= 65) {
            player.injuryType = "Leve";
            player.injuryWeeks = randInt(1, 2);
        } else if (roll <= 90) {
            player.injuryType = "Media";
            player.injuryWeeks = randInt(3, 6);
        } else {
            player.injuryType = "Grave";
            player.injuryWeeks = randInt(7, 12);
        }
        if (player.age >= 34 && randInt(1, 100) <= 25) player.injuryWeeks++;
        if (verbose) {
            cout << player.name << " se lesiono (" << player.injuryType << ") y estara fuera por "
                 << player.injuryWeeks << " semanas." << endl;
        }
        if (events) {
            int minute = randInt(1, 90);
            events->push_back(to_string(minute) + "' Lesion: " + player.name +
                              " [" + player.injuryType + "]" +
                              " (" + to_string(player.injuryWeeks) + " sem)");
        }
        return true;
    }
    return false;
}

void healInjuries(Team& team, bool verbose) {
    for (auto& player : team.players) {
        if (player.injured) {
            player.injuryWeeks--;
            if (player.injuryWeeks <= 0) {
                player.injured = false;
                player.injuryType.clear();
                if (verbose) {
                    cout << player.name << " se recupero de su lesion." << endl;
                }
            }
        }
    }
}

void recoverFitness(Team& team, int days) {
    if (days <= 0) return;
    for (auto& player : team.players) {
        int base = 4 + days / 2;
        if (player.injured) base = max(1, base / 2);
        if (player.age > 30) base -= (player.age - 30) / 6;
        if (base < 1) base = 1;
        if (player.fitness < player.stamina) {
            player.fitness = clampInt(player.fitness + base, 15, player.stamina);
        } else if (player.fitness > player.stamina) {
            player.fitness = player.stamina;
        }
    }
}

static void applyDevelopment(Team& team, const vector<int>& xi, bool verbose) {
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& p = team.players[idx];
        if (p.age > 23) continue;
        if (p.skill >= p.potential) continue;
        int chance = clampInt(12 - (p.age - 17), 3, 12);
        if (randInt(1, 100) <= chance) {
            p.skill = min(100, p.skill + 1);
            string pos = normalizePosition(p.position);
            if (pos == "ARQ" || pos == "DEF") {
                p.defense = min(100, p.defense + 1);
            } else if (pos == "MED") {
                p.attack = min(100, p.attack + 1);
                p.defense = min(100, p.defense + 1);
            } else {
                p.attack = min(100, p.attack + 1);
            }
            if (verbose) {
                cout << "[Progreso] " << p.name << " mejoro su habilidad a " << p.skill << "." << endl;
            }
        }
    }
}

void assignGoalsAndAssists(Team& team, int goals, const vector<int>& xi, const string& teamName, vector<string>* events) {
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

        string assistName;
        if (xi.size() > 1) {
            int assistIdx = scorerIdx;
            int guard = 0;
            while (assistIdx == scorerIdx && guard < 10) {
                assistIdx = xi[randInt(0, static_cast<int>(xi.size()) - 1)];
                guard++;
            }
            if (assistIdx != scorerIdx) {
                team.players[assistIdx].assists++;
                assistName = team.players[assistIdx].name;
            }
        }
        if (events) {
            int minute = randInt(1, 90);
            string text = to_string(minute) + "' " + teamName + ": " + team.players[scorerIdx].name;
            if (!assistName.empty()) text += " (asist: " + assistName + ")";
            events->push_back(text);
        }
    }
}

MatchResult playMatch(Team& home, Team& away, bool verbose, bool keyMatch, bool neutralVenue) {
    ensureMinimumSquad(home, 11);
    ensureMinimumSquad(away, 11);

    auto h = computeStrength(home);
    auto a = computeStrength(away);
    WeatherEffect weather = rollWeather();
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

    double homeMoraleFactor = 0.9 + (home.morale / 500.0);
    double awayMoraleFactor = 0.9 + (away.morale / 500.0);
    homeAttack = static_cast<int>(round(homeAttack * homeMoraleFactor));
    homeDefense = static_cast<int>(round(homeDefense * homeMoraleFactor));
    awayAttack = static_cast<int>(round(awayAttack * awayMoraleFactor));
    awayDefense = static_cast<int>(round(awayDefense * awayMoraleFactor));

    const double homeAttackBonus = neutralVenue ? 1.00 : 1.05;
    const double homeDefenseBonus = neutralVenue ? 1.00 : 1.03;
    homeAttack = static_cast<int>(round(homeAttack * homeAttackBonus * weather.attackMul));
    homeDefense = static_cast<int>(round(homeDefense * homeDefenseBonus * weather.defenseMul));
    awayAttack = static_cast<int>(round(awayAttack * weather.attackMul));
    awayDefense = static_cast<int>(round(awayDefense * weather.defenseMul));

    int homeStam = effectiveStamina(h.avgStamina, weather.staminaMul);
    int awayStam = effectiveStamina(a.avgStamina, weather.staminaMul);
    homeAttack = homeAttack * max(30, homeStam) / 100;
    homeDefense = homeDefense * max(30, homeStam) / 100;
    awayAttack = awayAttack * max(30, awayStam) / 100;
    awayDefense = awayDefense * max(30, awayStam) / 100;

    int homeGoals = samplePoisson(calcLambda(homeAttack, awayDefense));
    int awayGoals = samplePoisson(calcLambda(awayAttack, homeDefense));
    homeGoals = min(homeGoals, 7);
    awayGoals = min(awayGoals, 7);

    int homeShots = clampInt(static_cast<int>(round(8 * (static_cast<double>(homeAttack) / max(1, awayDefense)))) + randInt(-2, 3), 3, 22);
    int awayShots = clampInt(static_cast<int>(round(8 * (static_cast<double>(awayAttack) / max(1, homeDefense)))) + randInt(-2, 3), 3, 22);
    int homeShotsOn = clampInt(homeGoals + randInt(0, max(0, homeShots - homeGoals)), homeGoals, homeShots);
    int awayShotsOn = clampInt(awayGoals + randInt(0, max(0, awayShots - awayGoals)), awayGoals, awayShots);
    int homeCorners = clampInt(homeShots / 3 + randInt(0, 2), 0, 12);
    int awayCorners = clampInt(awayShots / 3 + randInt(0, 2), 0, 12);
    int homePoss = clampInt(static_cast<int>(round(50 + ((static_cast<double>(homeAttack) / max(1, homeAttack + awayAttack)) - 0.5) * 30 + randInt(-3, 3))), 35, 65);
    int awayPoss = 100 - homePoss;

    if (verbose) {
        cout << "\n--- Partido ---" << endl;
        cout << home.name << " vs " << away.name << endl;
        cout << "Tacticas: " << home.tactics << " vs " << away.tactics << endl;
        if (keyMatch) cout << "[PARTIDO CLAVE]" << endl;
        if (neutralVenue) cout << "Sede: Cancha neutral" << endl;
        cout << "Clima: " << weather.name << endl;
        cout << "Condicion promedio: " << homeStam << " vs " << awayStam << endl;
        if (homeStam < 60 || awayStam < 60) {
            cout << "[AVISO] Condicion baja puede afectar el rendimiento." << endl;
        }
        cout << "Resultado Final: " << home.name << " " << homeGoals << " - " << awayGoals << " " << away.name << endl;
        cout << "Estadisticas: Tiros " << homeShots << "-" << awayShots
             << ", Tiros al arco " << homeShotsOn << "-" << awayShotsOn
             << ", Posesion " << homePoss << "%-" << awayPoss << "%"
             << ", Corners " << homeCorners << "-" << awayCorners << endl;
    }

    home.goalsFor += homeGoals;
    home.goalsAgainst += awayGoals;
    away.goalsFor += awayGoals;
    away.goalsAgainst += homeGoals;
    away.awayGoals += awayGoals;

    int yellowHome = cardsForTactics(home.tactics, 0, 3);
    int yellowAway = cardsForTactics(away.tactics, 0, 3);
    int redHome = (randInt(1, 100) <= redChanceForTactics(home.tactics)) ? 1 : 0;
    int redAway = (randInt(1, 100) <= redChanceForTactics(away.tactics)) ? 1 : 0;
    home.yellowCards += yellowHome;
    away.yellowCards += yellowAway;
    home.redCards += redHome;
    away.redCards += redAway;

    if (homeGoals > awayGoals) {
        home.points += 3;
        home.wins++;
        away.losses++;
        home.addHeadToHeadPoints(away.name, 3);
        away.addHeadToHeadPoints(home.name, 0);
        if (verbose) cout << "Ganaste!" << endl;
    } else if (homeGoals < awayGoals) {
        away.points += 3;
        away.wins++;
        home.losses++;
        home.addHeadToHeadPoints(away.name, 0);
        away.addHeadToHeadPoints(home.name, 3);
        if (verbose) cout << "Perdiste." << endl;
    } else {
        home.points += 1;
        away.points += 1;
        home.draws++;
        away.draws++;
        home.addHeadToHeadPoints(away.name, 1);
        away.addHeadToHeadPoints(home.name, 1);
        if (verbose) cout << "Empate." << endl;
    }

    int deltaHome = 0;
    int deltaAway = 0;
    if (homeGoals > awayGoals) {
        deltaHome = 4;
        deltaAway = -3;
    } else if (homeGoals < awayGoals) {
        deltaHome = -3;
        deltaAway = 4;
    } else {
        deltaHome = 1;
        deltaAway = 1;
    }
    if (keyMatch) {
        deltaHome *= 2;
        deltaAway *= 2;
    }
    home.morale = clampInt(home.morale + deltaHome, 0, 100);
    away.morale = clampInt(away.morale + deltaAway, 0, 100);

    for (int idx : h.xi) home.players[idx].matchesPlayed++;
    for (int idx : a.xi) away.players[idx].matchesPlayed++;
    vector<string> events;
    vector<string>* eventsPtr = verbose ? &events : nullptr;
    assignGoalsAndAssists(home, homeGoals, h.xi, home.name, eventsPtr);
    assignGoalsAndAssists(away, awayGoals, a.xi, away.name, eventsPtr);

    for (int idx : h.xi) simulateInjury(home.players[idx], home.tactics, verbose, eventsPtr);
    for (int idx : a.xi) simulateInjury(away.players[idx], away.tactics, verbose, eventsPtr);

    applyDevelopment(home, h.xi, verbose);
    applyDevelopment(away, a.xi, verbose);

    applyMatchFatigue(home, h.xi, home.tactics);
    applyMatchFatigue(away, a.xi, away.tactics);

    if (verbose && !events.empty()) {
        cout << "\nEventos:" << endl;
        for (const auto& e : events) {
            cout << "- " << e << endl;
        }
    }

    return {homeGoals, awayGoals};
}
