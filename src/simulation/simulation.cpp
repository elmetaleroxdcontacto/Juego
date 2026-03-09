#include "simulation.h"

#include "ai/team_ai.h"
#include "development/player_progression_system.h"
#include "io.h"
#include "simulation/match_engine.h"
#include "simulation/match_engine_internal.h"
#include "simulation/morale_engine.h"
#include "utils.h"

#include <algorithm>
#include <cctype>
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

static string compactToken(string value) {
    value = toLower(trim(value));
    value.erase(remove_if(value.begin(), value.end(), [](unsigned char ch) {
                    return std::isspace(ch) || ch == '-' || ch == '_';
                }),
                value.end());
    return value;
}

static void applyRoleModifier(const Player& p, int& attack, int& defense) {
    string role = compactToken(p.role);
    if (role == "stopper") {
        defense += 5;
        attack -= 1;
    } else if (role == "sweeperkeeper") {
        attack += 2;
        defense += 3;
    } else if (role == "ballplaying") {
        attack += 2;
        defense += 3;
    } else if (role == "carrilero") {
        attack += 4;
        defense -= 1;
    } else if (role == "enganche") {
        attack += 5;
        defense -= 3;
    } else if (role == "boxtobox") {
        attack += 2;
        defense += 2;
    } else if (role == "organizador") {
        attack += 4;
        defense += 1;
    } else if (role == "pivote") {
        defense += 5;
        attack -= 2;
    } else if (role == "interior") {
        attack += 4;
        defense += 1;
    } else if (role == "poacher") {
        attack += 5;
        defense -= 1;
    } else if (role == "falso9") {
        attack += 3;
        defense += 2;
    } else if (role == "pressing") {
        attack += 2;
        defense += 1;
    } else if (role == "objetivo") {
        attack += 4;
        defense += 1;
    }
    attack = clampInt(attack, 1, 120);
    defense = clampInt(defense, 1, 120);
}

static void applyTraitModifier(const Player& p, int& attack, int& defense) {
    if (playerHasTrait(p, "Lider")) defense += 1;
    if (playerHasTrait(p, "Competidor")) {
        attack += 1;
        defense += 1;
    }
    if (playerHasTrait(p, "Pase riesgoso")) {
        attack += 3;
        defense -= 1;
    }
    if (playerHasTrait(p, "Llega al area")) attack += 2;
    if (playerHasTrait(p, "Presiona")) {
        attack += 1;
        defense += 2;
    }
    if (playerHasTrait(p, "Muralla")) defense += 3;
    if (playerHasTrait(p, "Cita grande")) {
        attack += 1;
        defense += 1;
    }
    if (playerHasTrait(p, "Versatil")) defense += 1;
    if (playerHasTrait(p, "Caliente")) {
        attack += 1;
        defense -= 1;
    }
    attack = clampInt(attack, 1, 125);
    defense = clampInt(defense, 1, 125);
}

static void applyPlayerStateModifier(const Player& p, const Team& team, int& attack, int& defense) {
    attack = attack * clampInt(92 + p.currentForm / 4 + p.consistency / 8, 72, 128) / 100;
    defense = defense * clampInt(92 + p.currentForm / 5 + p.tacticalDiscipline / 5, 74, 130) / 100;
    if (p.preferredFoot == "Ambos") {
        if (normalizePosition(p.position) == "MED" || normalizePosition(p.position) == "DEL") attack += 2;
        else defense += 1;
    } else if (p.preferredFoot == "Izquierdo" && team.width >= 4) {
        attack += 1;
    }
    if (p.currentForm <= 35) {
        attack -= 2;
        defense -= 2;
    } else if (p.currentForm >= 78) {
        attack += 2;
        defense += 1;
    }
    if (p.tacticalDiscipline >= 75 && (team.tactics == "Defensive" || team.matchInstruction == "Bloque bajo")) {
        defense += 2;
    }
    attack = clampInt(attack, 1, 130);
    defense = clampInt(defense, 1, 130);
}

static void applyMatchInstruction(const Team& team, int& attack, int& defense) {
    if (team.matchInstruction == "Laterales altos") {
        attack += 6;
        defense -= 3;
    } else if (team.matchInstruction == "Bloque bajo") {
        attack -= 4;
        defense += 8;
    } else if (team.matchInstruction == "Balon parado") {
        attack += 4;
        defense += 2;
    } else if (team.matchInstruction == "Presion final") {
        attack += 8;
        defense -= 2;
    } else if (team.matchInstruction == "Por bandas") {
        attack += 6;
        defense -= 1;
    } else if (team.matchInstruction == "Juego directo") {
        attack += 5;
        defense += 1;
    } else if (team.matchInstruction == "Contra-presion") {
        attack += 4;
        defense += 4;
    } else if (team.matchInstruction == "Pausar juego") {
        attack -= 3;
        defense += 6;
    }
}

static void applyFormationBias(const Team& team, const vector<int>& xi, int& attack, int& defense) {
    int defenders = 0;
    int midfielders = 0;
    int forwards = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        string pos = normalizePosition(team.players[idx].position);
        if (pos == "DEF") defenders++;
        else if (pos == "MED") midfielders++;
        else if (pos == "DEL") forwards++;
    }
    if (defenders >= 5) defense += 8;
    if (midfielders >= 4) {
        attack += 4;
        defense += 3;
    }
    if (forwards >= 3) {
        attack += 8;
        defense -= 4;
    }
    if (forwards <= 1) attack -= 5;
    if (team.formation.find("4-3-3") != string::npos || team.formation.find("3-4-3") != string::npos) attack += 3;
    if (team.formation.find("5-") == 0 || team.formation.find("4-5-1") != string::npos) defense += 4;
}

static void applyStyleMatchup(const Team& home,
                              const Team& away,
                              int& homeAttack,
                              int& homeDefense,
                              int& awayAttack,
                              int& awayDefense) {
    if (home.tactics == "Pressing" && away.tactics == "Counter") {
        awayAttack += 7;
        homeDefense -= 4;
    }
    if (away.tactics == "Pressing" && home.tactics == "Counter") {
        homeAttack += 7;
        awayDefense -= 4;
    }
    if (home.tactics == "Offensive" && away.tactics == "Defensive") {
        awayDefense += 6;
        homeAttack -= 2;
    }
    if (away.tactics == "Offensive" && home.tactics == "Defensive") {
        homeDefense += 6;
        awayAttack -= 2;
    }
    if (home.markingStyle == "Hombre" && away.width >= 4) {
        awayAttack += 4;
        homeDefense -= 2;
    }
    if (away.markingStyle == "Hombre" && home.width >= 4) {
        homeAttack += 4;
        awayDefense -= 2;
    }
}

static int pressingRecoveryBonus(const Team& team, const vector<int>& xi) {
    if (xi.empty()) return 0;
    int discipline = 0;
    int pressers = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[idx];
        discipline += player.tacticalDiscipline;
        if (playerHasTrait(player, "Presiona") || compactToken(player.role) == "pressing") pressers++;
    }
    int avgDiscipline = discipline / static_cast<int>(xi.size());
    int bonus = max(0, team.pressingIntensity - 2) * 3 + max(0, avgDiscipline - 60) / 10 + pressers;
    if (team.tactics == "Pressing") bonus += 3;
    if (team.matchInstruction == "Contra-presion") bonus += 3;
    return clampInt(bonus, 0, 16);
}

static int directPlayBonus(const Team& attacking, const Team& defending, const vector<int>& xi) {
    bool direct = attacking.matchInstruction == "Juego directo" || attacking.tactics == "Counter" || attacking.tempo >= 4;
    if (!direct || defending.defensiveLine <= 3) return 0;
    int runners = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(attacking.players.size())) continue;
        const Player& player = attacking.players[idx];
        if (normalizePosition(player.position) == "DEL" || playerHasTrait(player, "Competidor") ||
            compactToken(player.role) == "poacher" || compactToken(player.role) == "objetivo") {
            runners++;
        }
    }
    int bonus = (defending.defensiveLine - 3) * 4 + min(4, runners);
    return clampInt(bonus, 0, 15);
}

static int crowdSupportBonus(const Team& home, const Team& away, bool neutralVenue) {
    if (neutralVenue) return 0;
    int bonus = home.fanBase / 8 + home.stadiumLevel * 2 + teamPrestigeScore(home) / 18;
    bonus -= away.fanBase / 12;
    if (areRivalClubs(home, away)) bonus += 3;
    return clampInt(bonus, 0, 12);
}

static int clutchModifier(const Team& team, const vector<int>& xi, bool keyMatch) {
    if (xi.empty()) return 0;
    int nerve = team.morale;
    int leaders = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[idx];
        nerve += player.bigMatches / 2;
        if (!team.captain.empty() && player.name == team.captain) nerve += player.leadership / 2;
        if (playerHasTrait(player, "Cita grande")) nerve += 6;
        if (playerHasTrait(player, "Caliente")) nerve -= 3;
        if (playerHasTrait(player, "Lider")) leaders++;
    }
    nerve += leaders * 2;
    if (keyMatch) nerve += 5;
    return clampInt((nerve / static_cast<int>(xi.size()) - 45) / 3, -4, 12);
}

static void pushTacticalEvent(vector<string>* events, int minute, const string& text) {
    if (!events || text.empty()) return;
    events->push_back(to_string(minute) + "' " + text);
}

static void applyIntensityInjuryRisk(Team& team, const vector<int>& participants, vector<string>* events) {
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

static const Player* lineupPlayerByName(const Team& team, const vector<int>& xi, const string& playerName) {
    if (playerName.empty()) return nullptr;
    for (int idx : xi) {
        if (idx >= 0 && idx < static_cast<int>(team.players.size()) && team.players[idx].name == playerName) {
            return &team.players[idx];
        }
    }
    return nullptr;
}

static vector<int> uniqueParticipants(const vector<int>& startXi, const vector<int>& finalXi) {
    vector<int> participants = startXi;
    for (int idx : finalXi) {
        if (find(participants.begin(), participants.end(), idx) == participants.end()) {
            participants.push_back(idx);
        }
    }
    return participants;
}

static void applyMatchFatigue(Team& team, const vector<int>& xi, const string& tactics) {
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

static int cardsForTactics(const Team& team, int minVal, int maxVal) {
    int base = randInt(minVal, maxVal);
    const string& tactics = team.tactics;
    if (tactics == "Pressing") base += 1;
    else if (tactics == "Offensive") base += 1;
    else if (tactics == "Defensive") base -= 1;
    if (team.matchInstruction == "Presion final") base += 1;
    else if (team.matchInstruction == "Contra-presion") base += 1;
    base += max(0, team.pressingIntensity - 3);
    if (team.markingStyle == "Hombre") base += 1;
    return clampInt(base, 0, 6);
}

static int redChanceForTactics(const Team& team) {
    int chance = 5;
    const string& tactics = team.tactics;
    if (tactics == "Pressing") chance += 4;
    else if (tactics == "Offensive") chance += 2;
    else if (tactics == "Defensive") chance -= 1;
    if (team.matchInstruction == "Presion final") chance += 2;
    else if (team.matchInstruction == "Contra-presion") chance += 1;
    chance += max(0, team.pressingIntensity - 3);
    if (team.markingStyle == "Hombre") chance += 1;
    return clampInt(chance, 1, 15);
}

static vector<int> cardCandidates(const Team& team, const vector<int>& xi) {
    vector<int> candidates;
    for (int idx : xi) {
        if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
            candidates.push_back(idx);
            if (playerHasTrait(team.players[idx], "Caliente") || playerHasTrait(team.players[idx], "Presiona")) {
                candidates.push_back(idx);
            }
        }
    }
    if (!candidates.empty()) return candidates;
    for (size_t i = 0; i < team.players.size(); ++i) {
        candidates.push_back(static_cast<int>(i));
    }
    return candidates;
}

static void registerCards(Team& team, const vector<int>& xi, int yellowCards, int redCards, vector<string>* events) {
    vector<int> candidates = cardCandidates(team, xi);
    if (candidates.empty()) return;

    for (int i = 0; i < yellowCards; ++i) {
        int idx = candidates[randInt(0, static_cast<int>(candidates.size()) - 1)];
        Player& player = team.players[idx];
        player.seasonYellowCards++;
        player.yellowAccumulation++;
        if (events) {
            events->push_back(to_string(randInt(1, 90)) + "' Amarilla: " + player.name);
        }
        if (player.yellowAccumulation >= 5) {
            player.yellowAccumulation -= 5;
            player.matchesSuspended++;
            if (events) {
                events->push_back(to_string(randInt(1, 90)) + "' Suspension por acumulacion: " + player.name);
            }
        }
    }

    for (int i = 0; i < redCards; ++i) {
        int idx = candidates[randInt(0, static_cast<int>(candidates.size()) - 1)];
        Player& player = team.players[idx];
        player.seasonRedCards++;
        player.matchesSuspended++;
        if (events) {
            events->push_back(to_string(randInt(1, 90)) + "' Roja: " + player.name);
        }
    }
}

static void registerNamedCards(Team& team,
                               const vector<string>& yellowNames,
                               const vector<string>& redNames,
                               vector<string>* events) {
    for (const string& name : yellowNames) {
        for (auto& player : team.players) {
            if (player.name != name) continue;
            player.seasonYellowCards++;
            player.yellowAccumulation++;
            if (player.yellowAccumulation >= 5) {
                player.yellowAccumulation -= 5;
                player.matchesSuspended++;
                if (events) {
                    events->push_back("Sancion por acumulacion: " + player.name);
                }
            }
            break;
        }
    }

    for (const string& name : redNames) {
        for (auto& player : team.players) {
            if (player.name != name) continue;
            player.seasonRedCards++;
            player.matchesSuspended++;
            break;
        }
    }
}

static int applyGoalContributions(Team& team,
                                  const vector<GoalContribution>& contributions,
                                  vector<string>* events) {
    int applied = 0;
    for (const auto& contribution : contributions) {
        bool scorerApplied = false;
        for (auto& player : team.players) {
            if (player.name != contribution.scorerName) continue;
            player.goals++;
            scorerApplied = true;
            applied++;
            break;
        }
        if (!scorerApplied) continue;
        if (!contribution.assisterName.empty() && contribution.assisterName != contribution.scorerName) {
            for (auto& player : team.players) {
                if (player.name == contribution.assisterName) {
                    player.assists++;
                    break;
                }
            }
        }
        if (events) {
            string text = to_string(contribution.minute) + "' Gol confirmado: " + contribution.scorerName;
            if (!contribution.assisterName.empty() && contribution.assisterName != contribution.scorerName) {
                text += " (asist: " + contribution.assisterName + ")";
            }
            events->push_back(text);
        }
    }
    return applied;
}

static void applyNamedInjuries(Team& team, const vector<string>& injuredNames) {
    for (const string& name : injuredNames) {
        for (auto& player : team.players) {
            if (player.name != name || player.injured) continue;
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
            break;
        }
    }
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
    int chemistry = 0;
    int professionalism = 0;
    int form = 0;
    int discipline = 0;
    for (int idx : ts.xi) {
        int att = team.players[idx].attack;
        int def = team.players[idx].defense;
        match_internal::applyRoleModifier(team.players[idx], att, def);
        match_internal::applyTraitModifier(team.players[idx], att, def);
        match_internal::applyPlayerStateModifier(team.players[idx], team, att, def);
        ts.attack += att;
        ts.defense += def;
        skill += team.players[idx].skill;
        stamina += clampInt(team.players[idx].fitness, 0, 100);
        chemistry += team.players[idx].chemistry;
        professionalism += team.players[idx].professionalism;
        form += team.players[idx].currentForm;
        discipline += team.players[idx].tacticalDiscipline;
    }
    match_internal::applyFormationBias(team, ts.xi, ts.attack, ts.defense);
    ts.avgSkill = skill / static_cast<int>(ts.xi.size());
    ts.avgStamina = stamina / static_cast<int>(ts.xi.size());
    ts.attack += chemistry / static_cast<int>(ts.xi.size()) / 4;
    ts.defense += professionalism / static_cast<int>(ts.xi.size()) / 5;
    ts.attack += form / static_cast<int>(ts.xi.size()) / 5;
    ts.defense += discipline / static_cast<int>(ts.xi.size()) / 5;
    return ts;
}

static TeamStrength computeStrengthFromXI(const Team& team, const vector<int>& xi) {
    TeamStrength ts;
    ts.xi = xi;
    ts.attack = 0;
    ts.defense = 0;
    ts.avgSkill = 0;
    ts.avgStamina = 0;
    if (ts.xi.empty()) return ts;
    int skill = 0;
    int stamina = 0;
    int chemistry = 0;
    int professionalism = 0;
    int form = 0;
    int discipline = 0;
    for (int idx : ts.xi) {
        int att = team.players[idx].attack;
        int def = team.players[idx].defense;
        match_internal::applyRoleModifier(team.players[idx], att, def);
        match_internal::applyTraitModifier(team.players[idx], att, def);
        match_internal::applyPlayerStateModifier(team.players[idx], team, att, def);
        ts.attack += att;
        ts.defense += def;
        skill += team.players[idx].skill;
        stamina += clampInt(team.players[idx].fitness, 0, 100);
        chemistry += team.players[idx].chemistry;
        professionalism += team.players[idx].professionalism;
        form += team.players[idx].currentForm;
        discipline += team.players[idx].tacticalDiscipline;
    }
    match_internal::applyFormationBias(team, ts.xi, ts.attack, ts.defense);
    ts.avgSkill = skill / static_cast<int>(ts.xi.size());
    ts.avgStamina = stamina / static_cast<int>(ts.xi.size());
    ts.attack += chemistry / static_cast<int>(ts.xi.size()) / 4;
    ts.defense += professionalism / static_cast<int>(ts.xi.size()) / 5;
    ts.attack += form / static_cast<int>(ts.xi.size()) / 5;
    ts.defense += discipline / static_cast<int>(ts.xi.size()) / 5;
    return ts;
}

static int lineupPerformanceScore(const Team& team, int idx, const string& targetPos) {
    if (idx < 0 || idx >= static_cast<int>(team.players.size())) return -100000;
    const Player& player = team.players[idx];
    int score = player.skill * 3 + player.fitness * 2 + player.attack + player.defense +
                player.currentForm * 2 + player.consistency + player.tacticalDiscipline;
    score += positionFitScore(player, targetPos) * 4;
    if (player.injured) score -= 60;
    if (player.matchesSuspended > 0) score -= 1000;
    return score;
}

static int performInMatchSubstitutions(Team& team, vector<int>& xi, vector<string>* events) {
    if (xi.empty()) return 0;
    vector<int> bench = team.getBenchIndices(7);
    if (bench.empty()) return 0;

    vector<bool> benchUsed(team.players.size(), false);
    int substitutions = 0;
    const int maxSubs = 3;
    while (substitutions < maxSubs) {
        int outSlot = -1;
        int outNeed = 0;
        for (size_t slot = 0; slot < xi.size(); ++slot) {
            int idx = xi[slot];
            if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
            const Player& player = team.players[idx];
            int need = 0;
            if (player.injured) need += 100;
            if (player.fitness < 58) need += (58 - player.fitness) * 2;
            if (player.yellowAccumulation >= 4) need += 6;
            if (team.rotationPolicy == "Rotacion" && player.fitness < 65) need += 4;
            if (need > outNeed) {
                outNeed = need;
                outSlot = static_cast<int>(slot);
            }
        }
        if (outSlot < 0 || outNeed <= 0) break;

        int outIdx = xi[outSlot];
        string targetPos = normalizePosition(team.players[outIdx].position);
        int bestBench = -1;
        int bestGain = -100000;
        int currentScore = lineupPerformanceScore(team, outIdx, targetPos);
        for (int idx : bench) {
            if (idx < 0 || idx >= static_cast<int>(team.players.size()) || benchUsed[idx]) continue;
            const Player& candidate = team.players[idx];
            if (candidate.matchesSuspended > 0 || candidate.injured) continue;
            int candidateScore = lineupPerformanceScore(team, idx, targetPos);
            int gain = candidateScore - currentScore;
            if (gain > bestGain) {
                bestGain = gain;
                bestBench = idx;
            }
        }
        if (bestBench < 0 || (bestGain < -6 && outNeed < 40)) break;

        benchUsed[bestBench] = true;
        substitutions++;
        if (events) {
            events->push_back(to_string(randInt(55, 88)) + "' Cambio: " + team.players[outIdx].name +
                              " por " + team.players[bestBench].name);
        }
        xi[outSlot] = bestBench;
    }
    return substitutions;
}

bool hasInjuredInXI(const Team& team, const vector<int>& xi) {
    for (int idx : xi) {
        if (idx >= 0 && idx < static_cast<int>(team.players.size()) && team.players[idx].injured) {
            return true;
        }
    }
    return false;
}

void applyTactics(const Team& team, int& attack, int& defense) {
    const string& tactics = team.tactics;
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

    attack += (team.tempo - 3) * 6;
    defense -= max(0, team.tempo - 3) * 2;
    defense += (team.defensiveLine - 3) * 4;
    attack += (team.width - 3) * 4;
    if (team.width <= 2) defense += 2;
    defense -= max(0, team.width - 3) * 2;
    attack += max(0, team.pressingIntensity - 3) * 3;
    defense += min(0, team.defensiveLine - 3) * 2;
    if (team.pressingIntensity >= 4 && team.defensiveLine >= 4) {
        attack += 3;
        defense -= 2;
    }
    if (team.defensiveLine <= 2 && team.tactics == "Counter") {
        defense += 3;
        attack += 1;
    }
    if (team.markingStyle == "Hombre") {
        defense += 4;
        attack -= 2;
    }
    match_internal::applyMatchInstruction(team, attack, defense);
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
    risk -= max(0, player.tacticalDiscipline - 70) / 12;
    if (tactics == "Pressing") risk += 3;
    else if (tactics == "Offensive") risk += 2;
    else if (tactics == "Counter") risk += 1;
    else if (tactics == "Defensive") risk -= 1;
    if (playerHasTrait(player, "Fragil")) risk += 4;
    if (playerHasTrait(player, "Competidor")) risk += 1;
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
        (void)verbose;
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
            int progress = 1;
            if (team.medicalTeam >= 70 && randInt(1, 100) <= 35) progress++;
            if (team.medicalTeam >= 85 && randInt(1, 100) <= 20) progress++;
            player.injuryWeeks -= progress;
            if (player.injuryWeeks <= 0) {
                player.injured = false;
                player.injuryType.clear();
                player.injuryWeeks = 0;
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
        base += team.trainingFacilityLevel - 1;
        base += max(0, (team.fitnessCoach - 55) / 15);
        if (base < 1) base = 1;
        if (player.fitness < player.stamina) {
            player.fitness = clampInt(player.fitness + base, 15, player.stamina);
        } else if (player.fitness > player.stamina) {
            player.fitness = player.stamina;
        }
    }
}

static void applyDevelopment(Team& team, const vector<int>& xi, vector<string>* events) {
    development::applyMatchExperience(team, xi, events);
}

static int keyMatchModifier(const Team& team, const vector<int>& xi) {
    if (xi.empty()) return 0;
    int total = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[idx];
        total += player.bigMatches;
        if (playerHasTrait(player, "Cita grande")) total += 8;
        if (!team.captain.empty() && player.name == team.captain) total += player.leadership / 6;
    }
    int avg = total / static_cast<int>(xi.size());
    return clampInt((avg - 55) / 4, -8, 10);
}

static void updateMatchForm(Team& team,
                            const vector<int>& participants,
                            int goalsFor,
                            int goalsAgainst,
                            bool keyMatch) {
    int baseDelta = 0;
    if (goalsFor > goalsAgainst) baseDelta = 3;
    else if (goalsFor < goalsAgainst) baseDelta = -3;
    if (keyMatch) baseDelta += (baseDelta >= 0) ? 1 : -1;
    for (int idx : participants) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        Player& player = team.players[idx];
        int delta = baseDelta;
        string pos = normalizePosition(player.position);
        if (pos == "DEL" && goalsFor >= 2) delta += 1;
        if ((pos == "DEF" || pos == "ARQ") && goalsAgainst == 0) delta += 1;
        if (player.fitness < 50) delta -= 1;
        if (player.injured) delta -= 2;
        if (keyMatch && playerHasTrait(player, "Cita grande")) delta += 1;
        if (keyMatch && player.bigMatches <= 40) delta -= 1;
        player.currentForm = clampInt(player.currentForm + delta, 1, 99);
    }
}

void assignGoalsAndAssists(Team& team, int goals, const vector<int>& xi, const string& teamName, vector<string>* events) {
    match_internal::assignGoalsAndAssists(team, goals, xi, teamName, events);
}

static bool playerNamedInXI(const Team& team, const vector<int>& xi, const string& playerName) {
    return lineupPlayerByName(team, xi, playerName) != nullptr;
}

struct PhaseMatchSummary {
    int homeGoals = 0;
    int awayGoals = 0;
    int homeShots = 0;
    int awayShots = 0;
    int homeCorners = 0;
    int awayCorners = 0;
    int homeControl = 0;
};

static int phaseFatiguePenalty(const Team& team, int phaseIndex, int averageStamina) {
    int penalty = phaseIndex * max(1, team.pressingIntensity + team.tempo - 4);
    if (team.tactics == "Pressing") penalty += phaseIndex * 2;
    if (team.tactics == "Offensive") penalty += phaseIndex;
    if (team.matchInstruction == "Presion final" || team.matchInstruction == "Contra-presion") penalty += phaseIndex;
    if (team.matchInstruction == "Pausar juego") penalty -= phaseIndex;
    penalty += max(0, 60 - averageStamina) / 4;
    return max(0, penalty);
}

static void applyScoreStatePressure(const Team& team,
                                    int minute,
                                    int goalsFor,
                                    int goalsAgainst,
                                    int clutch,
                                    int& attack,
                                    int& defense) {
    int scoreDiff = goalsFor - goalsAgainst;
    if (scoreDiff < 0 && minute >= 60) {
        attack += 6 + abs(scoreDiff) * 3 + clutch / 2;
        defense -= 2 + abs(scoreDiff);
    } else if (scoreDiff > 0 && minute >= 75) {
        attack -= 2;
        defense += 4 + clutch / 3;
    } else if (scoreDiff == 0 && minute >= 75) {
        attack += clutch / 2;
    }
    if (team.morale <= 35 && minute >= 70 && scoreDiff < 0) {
        attack -= 2;
        defense -= 1;
    }
}

static void addPhaseNarrative(const Team& team,
                              const Team& opponent,
                              int minuteStart,
                              int minuteEnd,
                              int pressureEdge,
                              int directBonus,
                              vector<string>* events) {
    if (!events) return;
    int eventMinute = randInt(minuteStart, minuteEnd);
    if (pressureEdge >= 18 && randInt(1, 100) <= 45) {
        match_internal::pushTacticalEvent(events, eventMinute, team.name + " inclina el partido en este tramo");
    } else if (directBonus >= 7 && opponent.defensiveLine >= 4 && randInt(1, 100) <= 40) {
        match_internal::pushTacticalEvent(events, eventMinute, team.name + " encuentra espacio tras un balon largo");
    } else if (team.tactics == "Counter" && randInt(1, 100) <= 28) {
        match_internal::pushTacticalEvent(events, eventMinute, team.name + " sale rapido al contraataque");
    } else if (team.matchInstruction == "Balon parado" && randInt(1, 100) <= 26) {
        match_internal::pushTacticalEvent(events, eventMinute, team.name + " amenaza desde la pelota detenida");
    }
}

static PhaseMatchSummary simulateMatchByPhases(const Team& home,
                                               const Team& away,
                                               const vector<int>& homeXI,
                                               const vector<int>& awayXI,
                                               const WeatherEffect& weather,
                                               int homeCrowd,
                                               int awayCrowd,
                                               int homeBaseStamina,
                                               int awayBaseStamina,
                                               bool keyMatch,
                                               vector<string>* events) {
    PhaseMatchSummary summary;
    Team homeState = home;
    Team awayState = away;

    for (int phase = 0; phase < 6; ++phase) {
        int minuteEnd = (phase + 1) * 15;
        int minuteStart = minuteEnd - 14;

        team_ai::applyInMatchCpuAdjustment(homeState, awayState, minuteEnd, summary.homeGoals, summary.awayGoals, events);
        team_ai::applyInMatchCpuAdjustment(awayState, homeState, minuteEnd, summary.awayGoals, summary.homeGoals, events);

        TeamStrength homeStrength = computeStrengthFromXI(homeState, homeXI);
        TeamStrength awayStrength = computeStrengthFromXI(awayState, awayXI);
        int homeAttack = homeStrength.attack;
        int homeDefense = homeStrength.defense;
        int awayAttack = awayStrength.attack;
        int awayDefense = awayStrength.defense;
        int homeRecovery = match_internal::pressingRecoveryBonus(homeState, homeXI);
        int awayRecovery = match_internal::pressingRecoveryBonus(awayState, awayXI);
        int homeDirect = match_internal::directPlayBonus(homeState, awayState, homeXI);
        int awayDirect = match_internal::directPlayBonus(awayState, homeState, awayXI);
        int homeClutch = match_internal::clutchModifier(homeState, homeXI, keyMatch) + homeCrowd / 2;
        int awayClutch = match_internal::clutchModifier(awayState, awayXI, keyMatch) + awayCrowd / 2;

        applyTactics(homeState, homeAttack, homeDefense);
        applyTactics(awayState, awayAttack, awayDefense);
        match_internal::applyStyleMatchup(homeState, awayState, homeAttack, homeDefense, awayAttack, awayDefense);

        homeAttack += homeRecovery + homeDirect + homeCrowd + homeStrength.avgSkill;
        homeDefense += homeRecovery / 2 + homeCrowd / 2 + homeStrength.avgSkill;
        awayAttack += awayRecovery + awayDirect + awayCrowd + awayStrength.avgSkill;
        awayDefense += awayRecovery / 2 + awayCrowd / 2 + awayStrength.avgSkill;

        double homeMoraleFactor = 0.9 + (homeState.morale / 500.0);
        double awayMoraleFactor = 0.9 + (awayState.morale / 500.0);
        homeAttack = static_cast<int>(round(homeAttack * homeMoraleFactor * weather.attackMul));
        homeDefense = static_cast<int>(round(homeDefense * weather.defenseMul));
        awayAttack = static_cast<int>(round(awayAttack * awayMoraleFactor * weather.attackMul));
        awayDefense = static_cast<int>(round(awayDefense * weather.defenseMul));

        int homeStamina = clampInt(homeBaseStamina - phaseFatiguePenalty(homeState, phase, homeBaseStamina), 30, 100);
        int awayStamina = clampInt(awayBaseStamina - phaseFatiguePenalty(awayState, phase, awayBaseStamina), 30, 100);
        homeAttack = homeAttack * homeStamina / 100;
        homeDefense = homeDefense * homeStamina / 100;
        awayAttack = awayAttack * awayStamina / 100;
        awayDefense = awayDefense * awayStamina / 100;

        applyScoreStatePressure(homeState, minuteEnd, summary.homeGoals, summary.awayGoals, homeClutch, homeAttack, homeDefense);
        applyScoreStatePressure(awayState, minuteEnd, summary.awayGoals, summary.homeGoals, awayClutch, awayAttack, awayDefense);

        if (playerNamedInXI(homeState, homeXI, homeState.captain)) {
            if (const Player* captain = lineupPlayerByName(homeState, homeXI, homeState.captain)) {
                homeAttack += 1 + captain->leadership / 18;
                homeDefense += 1 + captain->leadership / 18;
            }
        }
        if (playerNamedInXI(awayState, awayXI, awayState.captain)) {
            if (const Player* captain = lineupPlayerByName(awayState, awayXI, awayState.captain)) {
                awayAttack += 1 + captain->leadership / 18;
                awayDefense += 1 + captain->leadership / 18;
            }
        }
        if (const Player* taker = lineupPlayerByName(homeState, homeXI, homeState.freeKickTaker)) {
            homeAttack += taker->setPieceSkill / 14;
        }
        if (const Player* taker = lineupPlayerByName(awayState, awayXI, awayState.freeKickTaker)) {
            awayAttack += taker->setPieceSkill / 14;
        }

        double homeLambda = calcLambda(homeAttack, awayDefense) / 6.0;
        double awayLambda = calcLambda(awayAttack, homeDefense) / 6.0;
        int phaseHomeGoals = min(3, samplePoisson(homeLambda));
        int phaseAwayGoals = min(3, samplePoisson(awayLambda));
        int phaseHomeShots = clampInt(static_cast<int>(round(1.0 + homeLambda * 3.2 + homeRecovery / 10.0 + homeDirect / 12.0 + randInt(-1, 2))),
                                      phaseHomeGoals, 7);
        int phaseAwayShots = clampInt(static_cast<int>(round(1.0 + awayLambda * 3.2 + awayRecovery / 10.0 + awayDirect / 12.0 + randInt(-1, 2))),
                                      phaseAwayGoals, 7);
        int phaseHomeCorners = clampInt(phaseHomeShots / 2 + (homeState.matchInstruction == "Laterales altos" ? 1 : 0) + randInt(0, 1), 0, 3);
        int phaseAwayCorners = clampInt(phaseAwayShots / 2 + (awayState.matchInstruction == "Laterales altos" ? 1 : 0) + randInt(0, 1), 0, 3);

        summary.homeGoals += phaseHomeGoals;
        summary.awayGoals += phaseAwayGoals;
        summary.homeShots += phaseHomeShots;
        summary.awayShots += phaseAwayShots;
        summary.homeCorners += phaseHomeCorners;
        summary.awayCorners += phaseAwayCorners;
        summary.homeControl += clampInt(50 + (homeAttack - awayAttack) / 6 + homeCrowd - awayCrowd, 30, 70);

        addPhaseNarrative(homeState, awayState, minuteStart, minuteEnd, homeAttack - awayDefense, homeDirect, events);
        addPhaseNarrative(awayState, homeState, minuteStart, minuteEnd, awayAttack - homeDefense, awayDirect, events);
    }

    return summary;
}

int teamPenaltyStrength(const Team& team) {
    auto xi = team.getStartingXIIndices();
    int total = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const Player& player = team.players[idx];
        total += player.skill;
        total += player.setPieceSkill / 4;
        total += player.professionalism / 10;
        if (!team.penaltyTaker.empty() && player.name == team.penaltyTaker) total += 12 + player.setPieceSkill / 5;
        if (!team.captain.empty() && player.name == team.captain) total += 4 + player.leadership / 6;
    }
    if (xi.empty()) return 50;
    return total / static_cast<int>(xi.size());
}

static MatchResult simulateMatchLegacy(Team& home, Team& away, bool keyMatch, bool neutralVenue) {
    ensureMinimumSquad(home, 11);
    ensureMinimumSquad(away, 11);
    ensureTeamIdentity(home);
    ensureTeamIdentity(away);

    auto h = computeStrength(home);
    auto a = computeStrength(away);
    vector<int> homeStartXI = h.xi;
    vector<int> awayStartXI = a.xi;
    for (int idx : homeStartXI) {
        if (idx >= 0 && idx < static_cast<int>(home.players.size())) {
            home.players[idx].startsThisSeason++;
        }
    }
    for (int idx : awayStartXI) {
        if (idx >= 0 && idx < static_cast<int>(away.players.size())) {
            away.players[idx].startsThisSeason++;
        }
    }

    vector<string> warnings;
    vector<string> reportLines;
    vector<string> events;
    vector<string>* eventsPtr = &events;
    int homeSubs = performInMatchSubstitutions(home, h.xi, eventsPtr);
    int awaySubs = performInMatchSubstitutions(away, a.xi, eventsPtr);
    if (homeSubs > 0) {
        TeamStrength adjusted = computeStrengthFromXI(home, h.xi);
        h.attack = (h.attack * 3 + adjusted.attack) / 4;
        h.defense = (h.defense * 3 + adjusted.defense) / 4;
        h.avgSkill = (h.avgSkill * 3 + adjusted.avgSkill) / 4;
        h.avgStamina = max(h.avgStamina, adjusted.avgStamina);
    }
    if (awaySubs > 0) {
        TeamStrength adjusted = computeStrengthFromXI(away, a.xi);
        a.attack = (a.attack * 3 + adjusted.attack) / 4;
        a.defense = (a.defense * 3 + adjusted.defense) / 4;
        a.avgSkill = (a.avgSkill * 3 + adjusted.avgSkill) / 4;
        a.avgStamina = max(a.avgStamina, adjusted.avgStamina);
    }
    WeatherEffect weather = rollWeather();
    bool homeInj = hasInjuredInXI(home, h.xi);
    bool awayInj = hasInjuredInXI(away, a.xi);
    if (homeInj || awayInj) {
        string warning = "Equipos sin suficientes jugadores sanos: ";
        if (homeInj) warning += home.name;
        if (homeInj && awayInj) warning += " y ";
        if (awayInj) warning += away.name;
        warning += ". Se usan lesionados en el XI.";
        warnings.push_back(warning);
    }

    int homeAttack = h.attack;
    int homeDefense = h.defense;
    int awayAttack = a.attack;
    int awayDefense = a.defense;
    int homeRecovery = match_internal::pressingRecoveryBonus(home, h.xi);
    int awayRecovery = match_internal::pressingRecoveryBonus(away, a.xi);
    int homeDirect = match_internal::directPlayBonus(home, away, h.xi);
    int awayDirect = match_internal::directPlayBonus(away, home, a.xi);
    int homeCrowd = match_internal::crowdSupportBonus(home, away, neutralVenue);
    int awayCrowd = match_internal::crowdSupportBonus(away, home, neutralVenue);

    applyTactics(home, homeAttack, homeDefense);
    applyTactics(away, awayAttack, awayDefense);
    match_internal::applyStyleMatchup(home, away, homeAttack, homeDefense, awayAttack, awayDefense);
    homeAttack += homeRecovery + homeDirect + homeCrowd;
    homeDefense += homeRecovery / 2 + homeCrowd / 2;
    awayAttack += awayRecovery + awayDirect + awayCrowd;
    awayDefense += awayRecovery / 2 + awayCrowd / 2;
    if (areRivalClubs(home, away)) {
        match_internal::pushTacticalEvent(eventsPtr, randInt(4, 18), "Se juega con clima de clasico entre " + home.name + " y " + away.name);
    }
    if (homeRecovery >= 8) {
        match_internal::pushTacticalEvent(eventsPtr, randInt(8, 24), home.name + " muerde alto y roba cerca del area rival");
    }
    if (awayRecovery >= 8) {
        match_internal::pushTacticalEvent(eventsPtr, randInt(8, 24), away.name + " activa la presion y corta la salida");
    }
    if (homeDirect >= 7) {
        match_internal::pushTacticalEvent(eventsPtr, randInt(16, 34), home.name + " amenaza con balones largos a la espalda de la linea alta");
    }
    if (awayDirect >= 7) {
        match_internal::pushTacticalEvent(eventsPtr, randInt(16, 34), away.name + " castiga la espalda de la defensa adelantada");
    }
    if (homeCrowd >= 6) {
        match_internal::pushTacticalEvent(eventsPtr, randInt(2, 12), "La localia empuja a " + home.name + " desde el inicio");
    }

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
    if (keyMatch) {
        int homeKey = keyMatchModifier(home, h.xi);
        int awayKey = keyMatchModifier(away, a.xi);
        homeAttack += homeKey;
        homeDefense += homeKey / 2;
        awayAttack += awayKey;
        awayDefense += awayKey / 2;
    }

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

    if (playerNamedInXI(home, h.xi, home.captain)) {
        const Player* captain = lineupPlayerByName(home, h.xi, home.captain);
        if (captain) {
            homeAttack += 2 + captain->leadership / 15;
            homeDefense += 2 + captain->leadership / 15;
        }
    }
    if (playerNamedInXI(away, a.xi, away.captain)) {
        const Player* captain = lineupPlayerByName(away, a.xi, away.captain);
        if (captain) {
            awayAttack += 2 + captain->leadership / 15;
            awayDefense += 2 + captain->leadership / 15;
        }
    }
    if (const Player* taker = lineupPlayerByName(home, h.xi, home.freeKickTaker)) {
        homeAttack += taker->setPieceSkill / 10;
    }
    if (const Player* taker = lineupPlayerByName(home, h.xi, home.cornerTaker)) {
        homeAttack += taker->setPieceSkill / 12;
    }
    if (const Player* taker = lineupPlayerByName(away, a.xi, away.freeKickTaker)) {
        awayAttack += taker->setPieceSkill / 10;
    }
    if (const Player* taker = lineupPlayerByName(away, a.xi, away.cornerTaker)) {
        awayAttack += taker->setPieceSkill / 12;
    }
    if (home.matchInstruction == "Balon parado") homeAttack += 6;
    if (away.matchInstruction == "Balon parado") awayAttack += 6;

    int homeGoals = samplePoisson(calcLambda(homeAttack, awayDefense));
    int awayGoals = samplePoisson(calcLambda(awayAttack, homeDefense));
    homeGoals = min(homeGoals, 7);
    awayGoals = min(awayGoals, 7);

    int homeShots = clampInt(static_cast<int>(round(8 * (static_cast<double>(homeAttack) / max(1, awayDefense)))) + randInt(-2, 3), 3, 22);
    int awayShots = clampInt(static_cast<int>(round(8 * (static_cast<double>(awayAttack) / max(1, homeDefense)))) + randInt(-2, 3), 3, 22);
    homeShots = clampInt(homeShots + homeRecovery / 3 + homeDirect / 4, 3, 24);
    awayShots = clampInt(awayShots + awayRecovery / 3 + awayDirect / 4, 3, 24);
    int homeShotsOn = clampInt(homeGoals + randInt(0, max(0, homeShots - homeGoals)), homeGoals, homeShots);
    int awayShotsOn = clampInt(awayGoals + randInt(0, max(0, awayShots - awayGoals)), awayGoals, awayShots);
    int homeCorners = clampInt(homeShots / 3 + randInt(0, 2), 0, 12);
    int awayCorners = clampInt(awayShots / 3 + randInt(0, 2), 0, 12);
    if (home.matchInstruction == "Laterales altos") homeCorners = clampInt(homeCorners + 2, 0, 14);
    if (away.matchInstruction == "Laterales altos") awayCorners = clampInt(awayCorners + 2, 0, 14);
    int homePoss = clampInt(static_cast<int>(round(50 + ((static_cast<double>(homeAttack) / max(1, homeAttack + awayAttack)) - 0.5) * 30 + randInt(-3, 3))), 35, 65);
    int awayPoss = 100 - homePoss;
    int homeClutch = match_internal::clutchModifier(home, h.xi, keyMatch) + homeCrowd / 2;
    int awayClutch = match_internal::clutchModifier(away, a.xi, keyMatch) + awayCrowd / 2;
    if (abs(homeGoals - awayGoals) <= 1) {
        if (homeClutch - awayClutch >= 5 && randInt(1, 100) <= 24) {
            homeGoals = clampInt(homeGoals + 1, 0, 7);
            homeShotsOn = clampInt(homeShotsOn + 1, homeGoals, homeShots);
            match_internal::pushTacticalEvent(eventsPtr, randInt(74, 90), home.name + " decide mejor en el cierre y golpea al final");
        } else if (awayClutch - homeClutch >= 5 && randInt(1, 100) <= 24) {
            awayGoals = clampInt(awayGoals + 1, 0, 7);
            awayShotsOn = clampInt(awayShotsOn + 1, awayGoals, awayShots);
            match_internal::pushTacticalEvent(eventsPtr, randInt(74, 90), away.name + " mantiene la calma y castiga en el tramo final");
        }
    }

    reportLines.push_back("--- Partido ---");
    reportLines.push_back(home.name + " vs " + away.name);
    reportLines.push_back("Tacticas: " + home.tactics + " (" + home.formation + ") vs " +
                          away.tactics + " (" + away.formation + ")");
    reportLines.push_back("Instrucciones: " + home.matchInstruction + " vs " + away.matchInstruction);
    if (keyMatch) reportLines.push_back("[PARTIDO CLAVE]");
    if (neutralVenue) reportLines.push_back("Sede: Cancha neutral");
    reportLines.push_back("Clima: " + weather.name);
    reportLines.push_back("Condicion promedio: " + to_string(homeStam) + " vs " + to_string(awayStam));
    reportLines.push_back("Contexto: prestigio " + to_string(teamPrestigeScore(home)) + "-" +
                          to_string(teamPrestigeScore(away)) +
                          " | apoyo local " + to_string(homeCrowd) + "-" + to_string(awayCrowd) +
                          " | recuperaciones " + to_string(homeRecovery) + "-" + to_string(awayRecovery));
    if (homeStam < 60 || awayStam < 60) {
        warnings.push_back("Condicion baja puede afectar el rendimiento.");
    }
    reportLines.push_back("Resultado Final: " + home.name + " " + to_string(homeGoals) + " - " +
                          to_string(awayGoals) + " " + away.name);
    reportLines.push_back("Estadisticas: Tiros " + to_string(homeShots) + "-" + to_string(awayShots) +
                          ", Tiros al arco " + to_string(homeShotsOn) + "-" + to_string(awayShotsOn) +
                          ", Posesion " + to_string(homePoss) + "%-" + to_string(awayPoss) + "%" +
                          ", Corners " + to_string(homeCorners) + "-" + to_string(awayCorners));

    home.goalsFor += homeGoals;
    home.goalsAgainst += awayGoals;
    away.goalsFor += awayGoals;
    away.goalsAgainst += homeGoals;
    away.awayGoals += awayGoals;

    int yellowHome = cardsForTactics(home, 0, 3);
    int yellowAway = cardsForTactics(away, 0, 3);
    if (areRivalClubs(home, away)) {
        yellowHome = clampInt(yellowHome + 1, 0, 6);
        yellowAway = clampInt(yellowAway + 1, 0, 6);
    }
    int redHome = (randInt(1, 100) <= redChanceForTactics(home)) ? 1 : 0;
    int redAway = (randInt(1, 100) <= redChanceForTactics(away)) ? 1 : 0;
    home.yellowCards += yellowHome;
    away.yellowCards += yellowAway;
    home.redCards += redHome;
    away.redCards += redAway;

    string verdict;
    if (homeGoals > awayGoals) {
        home.points += 3;
        home.wins++;
        away.losses++;
        home.addHeadToHeadPoints(away.name, 3);
        away.addHeadToHeadPoints(home.name, 0);
        verdict = "Ganaste!";
    } else if (homeGoals < awayGoals) {
        away.points += 3;
        away.wins++;
        home.losses++;
        home.addHeadToHeadPoints(away.name, 0);
        away.addHeadToHeadPoints(home.name, 3);
        verdict = "Perdiste.";
    } else {
        home.points += 1;
        away.points += 1;
        home.draws++;
        away.draws++;
        home.addHeadToHeadPoints(away.name, 1);
        away.addHeadToHeadPoints(home.name, 1);
        verdict = "Empate.";
    }
    reportLines.push_back(verdict);

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

    vector<int> homeParticipants = uniqueParticipants(homeStartXI, h.xi);
    vector<int> awayParticipants = uniqueParticipants(awayStartXI, a.xi);
    for (int idx : homeParticipants) {
        if (idx >= 0 && idx < static_cast<int>(home.players.size())) home.players[idx].matchesPlayed++;
    }
    for (int idx : awayParticipants) {
        if (idx >= 0 && idx < static_cast<int>(away.players.size())) away.players[idx].matchesPlayed++;
    }
    registerCards(home, homeParticipants, yellowHome, redHome, eventsPtr);
    registerCards(away, awayParticipants, yellowAway, redAway, eventsPtr);
    assignGoalsAndAssists(home, homeGoals, h.xi, home.name, eventsPtr);
    assignGoalsAndAssists(away, awayGoals, a.xi, away.name, eventsPtr);

    for (int idx : homeParticipants) {
        if (idx >= 0 && idx < static_cast<int>(home.players.size())) {
            simulateInjury(home.players[idx], home.tactics, false, eventsPtr);
        }
    }
    for (int idx : awayParticipants) {
        if (idx >= 0 && idx < static_cast<int>(away.players.size())) {
            simulateInjury(away.players[idx], away.tactics, false, eventsPtr);
        }
    }
    match_internal::applyIntensityInjuryRisk(home, homeParticipants, eventsPtr);
    match_internal::applyIntensityInjuryRisk(away, awayParticipants, eventsPtr);

    applyDevelopment(home, homeParticipants, eventsPtr);
    applyDevelopment(away, awayParticipants, eventsPtr);
    updateMatchForm(home, homeParticipants, homeGoals, awayGoals, keyMatch);
    updateMatchForm(away, awayParticipants, awayGoals, homeGoals, keyMatch);

    match_internal::applyMatchFatigue(home, homeParticipants, home.tactics);
    match_internal::applyMatchFatigue(away, awayParticipants, away.tactics);

    return {homeGoals, awayGoals, homeShots, awayShots, homePoss, awayPoss, homeSubs, awaySubs,
            homeCorners, awayCorners, weather.name, warnings, reportLines, events, verdict};
}

MatchResult simulateMatch(Team& home, Team& away, bool keyMatch, bool neutralVenue) {
    ensureMinimumSquad(home, 11);
    ensureMinimumSquad(away, 11);
    ensureTeamIdentity(home);
    ensureTeamIdentity(away);

    const vector<int> homeStartXI = home.getStartingXIIndices();
    const vector<int> awayStartXI = away.getStartingXIIndices();
    for (int idx : homeStartXI) {
        if (idx >= 0 && idx < static_cast<int>(home.players.size())) home.players[idx].startsThisSeason++;
    }
    for (int idx : awayStartXI) {
        if (idx >= 0 && idx < static_cast<int>(away.players.size())) away.players[idx].startsThisSeason++;
    }

    match_engine::MatchSimulationData simulation = match_engine::simulate(home, away, keyMatch, neutralVenue);
    MatchResult result = simulation.result;

    home.goalsFor += result.homeGoals;
    home.goalsAgainst += result.awayGoals;
    away.goalsFor += result.awayGoals;
    away.goalsAgainst += result.homeGoals;
    away.awayGoals += result.awayGoals;
    home.yellowCards += result.stats.homeYellowCards;
    away.yellowCards += result.stats.awayYellowCards;
    home.redCards += result.stats.homeRedCards;
    away.redCards += result.stats.awayRedCards;

    if (result.homeGoals > result.awayGoals) {
        home.points += 3;
        home.wins++;
        away.losses++;
        home.addHeadToHeadPoints(away.name, 3);
        away.addHeadToHeadPoints(home.name, 0);
    } else if (result.homeGoals < result.awayGoals) {
        away.points += 3;
        away.wins++;
        home.losses++;
        home.addHeadToHeadPoints(away.name, 0);
        away.addHeadToHeadPoints(home.name, 3);
    } else {
        home.points += 1;
        away.points += 1;
        home.draws++;
        away.draws++;
        home.addHeadToHeadPoints(away.name, 1);
        away.addHeadToHeadPoints(home.name, 1);
    }

    home.morale = clampInt(home.morale + morale_engine::postMatchMoraleDelta(home, result.homeGoals, result.awayGoals, keyMatch), 0, 100);
    away.morale = clampInt(away.morale + morale_engine::postMatchMoraleDelta(away, result.awayGoals, result.homeGoals, keyMatch), 0, 100);

    const vector<int> homeParticipants = simulation.homeParticipants.empty() ? homeStartXI : simulation.homeParticipants;
    const vector<int> awayParticipants = simulation.awayParticipants.empty() ? awayStartXI : simulation.awayParticipants;
    for (int idx : homeParticipants) {
        if (idx >= 0 && idx < static_cast<int>(home.players.size())) home.players[idx].matchesPlayed++;
    }
    for (int idx : awayParticipants) {
        if (idx >= 0 && idx < static_cast<int>(away.players.size())) away.players[idx].matchesPlayed++;
    }

    registerNamedCards(home, simulation.homeYellowCardPlayers, simulation.homeRedCardPlayers, nullptr);
    registerNamedCards(away, simulation.awayYellowCardPlayers, simulation.awayRedCardPlayers, nullptr);

    const int appliedHomeGoals = applyGoalContributions(home, simulation.homeGoals, nullptr);
    const int appliedAwayGoals = applyGoalContributions(away, simulation.awayGoals, nullptr);
    if (appliedHomeGoals < result.homeGoals) {
        assignGoalsAndAssists(home, result.homeGoals - appliedHomeGoals, homeStartXI, home.name, nullptr);
    }
    if (appliedAwayGoals < result.awayGoals) {
        assignGoalsAndAssists(away, result.awayGoals - appliedAwayGoals, awayStartXI, away.name, nullptr);
    }

    applyNamedInjuries(home, simulation.homeInjuredPlayers);
    applyNamedInjuries(away, simulation.awayInjuredPlayers);
    applyDevelopment(home, homeParticipants, nullptr);
    applyDevelopment(away, awayParticipants, nullptr);
    updateMatchForm(home, homeParticipants, result.homeGoals, result.awayGoals, keyMatch);
    updateMatchForm(away, awayParticipants, result.awayGoals, result.homeGoals, keyMatch);
    match_internal::applyMatchFatigue(home, homeParticipants, home.tactics);
    match_internal::applyMatchFatigue(away, awayParticipants, away.tactics);

    return result;
}

MatchResult playMatch(Team& home, Team& away, bool verbose, bool keyMatch, bool neutralVenue) {
    MatchResult result = simulateMatch(home, away, keyMatch, neutralVenue);
    if (!verbose) return result;

    cout << endl;
    for (const auto& warning : result.warnings) {
        cout << "[AVISO] " << warning << endl;
    }
    for (const auto& line : result.reportLines) {
        cout << line << endl;
    }
    if (!result.events.empty()) {
        cout << "\nEventos:" << endl;
        for (const auto& event : result.events) {
            cout << "- " << event << endl;
        }
    }
    return result;
}
