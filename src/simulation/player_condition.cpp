#include "simulation/simulation.h"

#include "simulation/player_condition.h"
#include "utils/utils.h"

#include <algorithm>
#include <iostream>

using namespace std;

namespace {

int tacticalIntensityScore(const Team& team) {
    int intensity = max(0, team.pressingIntensity - 3) * 6 + max(0, team.tempo - 3) * 5;
    if (team.tactics == "Pressing") intensity += 8;
    else if (team.tactics == "Offensive") intensity += 5;
    else if (team.tactics == "Counter") intensity += 2;
    else if (team.tactics == "Defensive") intensity -= 3;
    if (team.matchInstruction == "Contra-presion" || team.matchInstruction == "Presion final") intensity += 4;
    if (team.matchInstruction == "Pausar juego" || team.matchInstruction == "Bloque bajo") intensity -= 3;
    return intensity;
}

string fallbackInstruction(const string& tactics) {
    if (tactics == "Pressing") return "Contra-presion";
    if (tactics == "Counter") return "Juego directo";
    if (tactics == "Defensive") return "Bloque bajo";
    if (tactics == "Offensive") return "Por bandas";
    return "Equilibrado";
}

}  // namespace

namespace player_condition {

int workloadRisk(const Player& player, const Team& team) {
    int risk = max(0, 72 - player.fitness);
    risk += player.fatigueLoad;
    risk += max(0, 62 - player.stamina) / 2;
    risk += max(0, player.age - 29) * 2;
    risk += tacticalIntensityScore(team);
    risk -= max(0, team.fitnessCoach - 60) / 2;
    if (team.rotationPolicy == "Rotacion") risk -= 4;
    if (team.trainingFocus == "Recuperacion") risk -= 5;
    if (player.injured) risk += 14;
    if (playerHasTrait(player, "Fragil")) risk += 12;
    if (playerHasTrait(player, "Presiona")) risk += 4;
    if (playerHasTrait(player, "Llega al area")) risk += 3;
    return clampInt(risk, 0, 99);
}

int relapseRisk(const Player& player, const Team& team) {
    int risk = player.injuryHistory * 12;
    risk += max(0, 68 - team.medicalTeam);
    risk += workloadRisk(player, team) / 3;
    risk += player.injured ? 18 : 0;
    if (playerHasTrait(player, "Fragil")) risk += 12;
    if (player.age >= 31) risk += (player.age - 30) * 2;
    return clampInt(risk, 0, 99);
}

int readinessScore(const Player& player, const Team& team) {
    int readiness = 36;
    readiness += player.fitness / 2;
    readiness += player.currentForm / 4;
    readiness += player.consistency / 5;
    readiness += player.chemistry / 6;
    readiness += player.tacticalDiscipline / 6;
    readiness += player.moraleMomentum / 2;
    readiness += max(0, team.assistantCoach - 55) / 3;
    readiness -= workloadRisk(player, team) / 3;
    readiness -= relapseRisk(player, team) / 4;
    if (player.matchesSuspended > 0 || player.injured) readiness -= 28;
    return clampInt(readiness, 0, 100);
}

int developmentStability(const Player& player, const Team& team, bool congestedWeek) {
    int stability = 28;
    stability += player.professionalism / 2;
    stability += player.happiness / 3;
    stability += player.chemistry / 5;
    stability += max(0, team.trainingFacilityLevel - 1) * 6;
    stability += max(0, team.assistantCoach - 55) / 3;
    stability += max(0, team.youthCoach - 55) / 3;
    stability += max(0, team.performanceAnalyst - 55) / 4;
    stability -= workloadRisk(player, team) / 4;
    stability -= relapseRisk(player, team) / 5;
    stability -= congestedWeek ? 7 : 0;
    if (player.matchesPlayed == 0 && player.age <= 22) stability -= 5;
    if (player.promisedRole == "Proyecto") stability += 4;
    if (playerHasTrait(player, "Proyecto")) stability += 4;
    if (playerHasTrait(player, "Competidor")) stability += 3;
    return clampInt(stability, 0, 100);
}

InjuryProfile buildInjuryProfile(const Player& player, const Team& team, int severityBias) {
    const int workload = workloadRisk(player, team);
    const int relapse = relapseRisk(player, team);
    int severity = 3 + workload / 22 + relapse / 24 + max(0, player.age - 30) / 2 + severityBias + randInt(0, 4);
    if (team.matchInstruction == "Contra-presion" || team.tactics == "Pressing") severity += 1;

    InjuryProfile profile;
    if (severity <= 5) {
        profile.type = "Sobrecarga";
        profile.weeks = randInt(1, 2);
    } else if (severity <= 8) {
        profile.type = "Muscular";
        profile.weeks = randInt(2, 5);
    } else if (severity <= 12) {
        profile.type = "Ligamentos";
        profile.weeks = randInt(5, 10);
    } else {
        profile.type = "Fractura";
        profile.weeks = randInt(8, 14);
    }
    if (player.age >= 34 && randInt(1, 100) <= 25) profile.weeks++;
    return profile;
}

void applyInjury(Player& player, const Team& team, int severityBias) {
    const InjuryProfile profile = buildInjuryProfile(player, team, severityBias);
    player.injured = true;
    player.injuryHistory++;
    player.fatigueLoad = clampInt(player.fatigueLoad + 10 + max(0, severityBias), 0, 100);
    player.injuryType = profile.type;
    player.injuryWeeks = profile.weeks;
}

}  // namespace player_condition

bool simulateInjury(Player& player, const string& tactics, bool verbose, vector<string>* events) {
    if (player.injured) return false;

    Team context("Contexto");
    context.tactics = tactics;
    context.matchInstruction = fallbackInstruction(tactics);
    context.pressingIntensity = tactics == "Pressing" ? 5 : (tactics == "Offensive" ? 4 : 3);
    context.tempo = tactics == "Counter" ? 4 : 3;
    context.medicalTeam = 55;
    context.fitnessCoach = 55;

    const int triggerChance =
        clampInt(3 + player_condition::workloadRisk(player, context) / 12 + player_condition::relapseRisk(player, context) / 18,
                 2,
                 24);
    if (randInt(1, 100) > triggerChance) return false;

    player_condition::applyInjury(player, context);
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
        progress += max(0, team.medicalTeam - 58) / 12;
        progress += max(0, team.trainingFacilityLevel - 1) / 2;
        if (team.trainingFocus == "Recuperacion") progress++;
        if (player.age <= 23 && player.injuryWeeks <= 2 && randInt(1, 100) <= 30) progress++;
        progress = max(1, progress);

        player.injuryWeeks -= progress;
        player.fatigueLoad = max(0, player.fatigueLoad - progress * 6);
        if (player.injuryWeeks > 0) continue;

        player.injured = false;
        player.injuryType.clear();
        player.injuryWeeks = 0;
        player.fitness = clampInt(player.fitness + 6 + max(0, team.medicalTeam - 60) / 8, 15, player.stamina);
        player.currentForm = clampInt(player.currentForm - 1, 1, 99);
        if (verbose) {
            cout << player.name << " se recupero de su lesion." << endl;
        }
    }
}

void recoverFitness(Team& team, int days) {
    if (days <= 0) return;
    for (auto& player : team.players) {
        int base = 4 + days / 2;
        base += max(0, team.trainingFacilityLevel - 1);
        base += max(0, team.fitnessCoach - 55) / 14;
        base += max(0, team.medicalTeam - 55) / 16;
        if (team.trainingFocus == "Recuperacion") base += 2;
        if (player.injured) base = max(1, base / 2 + 1);
        if (player.age > 30) base -= (player.age - 30) / 6;
        base -= player_condition::workloadRisk(player, team) / 32;
        base = max(1, base);

        if (player.fitness < player.stamina) {
            player.fitness = clampInt(player.fitness + base, 15, player.stamina);
        } else if (player.fitness > player.stamina) {
            player.fitness = player.stamina;
        }

        int fatigueRecovery = max(2, days / 2 + max(0, team.fitnessCoach - 60) / 10);
        if (player.injured) fatigueRecovery += 2;
        if (team.trainingFocus == "Recuperacion") fatigueRecovery += 2;
        player.fatigueLoad = max(0, player.fatigueLoad - fatigueRecovery);
    }
}
