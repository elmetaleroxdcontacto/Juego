#include "simulation/simulation.h"

#include "utils/utils.h"

#include <iostream>

using namespace std;

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
    if (randInt(1, 100) > risk) return false;

    player.injured = true;
    player.injuryHistory++;
    const int roll = randInt(1, 100);
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
        if (player.injuryWeeks > 0) continue;
        player.injured = false;
        player.injuryType.clear();
        player.injuryWeeks = 0;
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
        if (base < 1) base = 1;
        if (player.fitness < player.stamina) {
            player.fitness = clampInt(player.fitness + base, 15, player.stamina);
        } else if (player.fitness > player.stamina) {
            player.fitness = player.stamina;
        }
    }
}
