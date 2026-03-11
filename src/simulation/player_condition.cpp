#include "simulation/simulation.h"

#include "utils/utils.h"

#include <iostream>

using namespace std;

bool simulateInjury(Player& player, const string& tactics, bool verbose, vector<string>* events) {
    if (player.injured) return false;
    int risk = 4;
    if (player.fitness < 60) risk += 3;
    if (player.stamina < 60) risk += 2;
    risk += player.fatigueLoad / 12;
    if (player.age >= 32) risk += (player.age - 31) / 3;
    risk += player.injuryHistory / 3;
    risk -= max(0, player.tacticalDiscipline - 70) / 12;
    risk += max(0, -player.moraleMomentum) / 8;
    if (tactics == "Pressing") risk += 3;
    else if (tactics == "Offensive") risk += 2;
    else if (tactics == "Counter") risk += 1;
    else if (tactics == "Defensive") risk -= 1;
    if (playerHasTrait(player, "Fragil")) risk += 4;
    if (playerHasTrait(player, "Competidor")) risk += 1;
    risk = clampInt(risk, 2, 15);
    if (randInt(1, 100) > risk) return false;

    player.injured = true;
    player.injuryHistory++;
    player.fatigueLoad = clampInt(player.fatigueLoad + 10, 0, 100);
    const int roll = randInt(1, 100);
    if (roll <= 45) {
        player.injuryType = "Sobrecarga";
        player.injuryWeeks = randInt(1, 2);
    } else if (roll <= 72) {
        player.injuryType = "Muscular";
        player.injuryWeeks = randInt(2, 5);
    } else if (roll <= 90) {
        player.injuryType = "Ligamentos";
        player.injuryWeeks = randInt(5, 10);
    } else {
        player.injuryType = "Fractura";
        player.injuryWeeks = randInt(8, 14);
    }
    if (player.age >= 34 && randInt(1, 100) <= 25) player.injuryWeeks++;
    (void)verbose;
    if (events) {
        const int minute = randInt(1, 90);
        events->push_back(to_string(minute) + "' Lesion: " + player.name +
                          " [" + player.injuryType + "]" +
                          " (" + to_string(player.injuryWeeks) + " sem)");
    }
    return true;
}

void healInjuries(Team& team, bool verbose) {
    for (auto& player : team.players) {
        if (!player.injured) continue;
        int progress = 1;
        if (team.medicalTeam >= 70 && randInt(1, 100) <= 35) progress++;
        if (team.medicalTeam >= 85 && randInt(1, 100) <= 20) progress++;
        player.injuryWeeks -= progress;
        player.fatigueLoad = max(0, player.fatigueLoad - progress * 6);
        if (player.injuryWeeks > 0) continue;
        player.injured = false;
        player.injuryType.clear();
        player.injuryWeeks = 0;
        player.fitness = clampInt(player.fitness + 6 + max(0, team.medicalTeam - 60) / 10, 15, player.stamina);
        if (verbose) {
            cout << player.name << " se recupero de su lesion." << endl;
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
        base += max(0, (team.medicalTeam - 55) / 20);
        if (player.fatigueLoad >= 40) base -= player.fatigueLoad / 18;
        if (base < 1) base = 1;
        if (player.fitness < player.stamina) {
            player.fitness = clampInt(player.fitness + base, 15, player.stamina);
        } else if (player.fitness > player.stamina) {
            player.fitness = player.stamina;
        }
        int fatigueRecovery = max(2, days / 2 + max(0, team.fitnessCoach - 60) / 10);
        if (player.injured) fatigueRecovery += 2;
        player.fatigueLoad = max(0, player.fatigueLoad - fatigueRecovery);
    }
}
