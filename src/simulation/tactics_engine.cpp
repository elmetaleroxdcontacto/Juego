#include "simulation/tactics_engine.h"

#include "simulation/match_engine_internal.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

int countMatchingRoles(const Team& team, const vector<int>& xi, const vector<string>& roles) {
    int count = 0;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        string role = match_internal::compactToken(team.players[idx].role);
        if (find(roles.begin(), roles.end(), role) != roles.end()) count++;
    }
    return count;
}

}  // namespace

namespace tactics_engine {

TacticalProfile buildTacticalProfile(const Team& team) {
    TacticalProfile profile;
    profile.mentality = 0.50;
    if (team.tactics == "Offensive") profile.mentality += 0.22;
    else if (team.tactics == "Pressing") profile.mentality += 0.14;
    else if (team.tactics == "Counter") profile.mentality += 0.08;
    else if (team.tactics == "Defensive") profile.mentality -= 0.18;

    profile.pressing = 0.26 + team.pressingIntensity * 0.13;
    profile.tempo = 0.30 + team.tempo * 0.13;
    profile.width = 0.32 + team.width * 0.11;
    profile.defensiveBlock = 0.30 + (6 - team.defensiveLine) * 0.11;
    profile.directness = (team.matchInstruction == "Juego directo" ? 0.34 : 0.0) +
                         (team.tactics == "Counter" ? 0.18 : 0.0) +
                         max(0, team.tempo - 3) * 0.05;
    profile.transitionThreat = (team.tactics == "Counter" ? 0.30 : 0.0) +
                               (team.matchInstruction == "Contra-presion" ? 0.18 : 0.0) +
                               max(0, team.pressingIntensity - 3) * 0.05;
    profile.setPieceThreat = (team.matchInstruction == "Balon parado" ? 0.34 : 0.0) +
                             max(0, team.width - 3) * 0.04;
    profile.risk = profile.mentality * 0.33 +
                   profile.pressing * 0.22 +
                   profile.tempo * 0.22 +
                   max(0.0, profile.width - 0.60) * 0.12 +
                   max(0.0, static_cast<double>(team.defensiveLine - 3)) * 0.13;
    return profile;
}

double tacticalCompatibility(const Team& team, const vector<int>& xi) {
    if (xi.empty()) return 0.80;
    int defenders = 0;
    int midfielders = 0;
    int forwards = 0;
    int pressers = countMatchingRoles(team, xi, {"pressing", "boxtobox"});
    int creators = countMatchingRoles(team, xi, {"enganche", "organizador", "interior"});
    int runners = countMatchingRoles(team, xi, {"poacher", "objetivo", "carrilero"});

    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const string pos = normalizePosition(team.players[idx].position);
        if (pos == "DEF") defenders++;
        else if (pos == "MED") midfielders++;
        else if (pos == "DEL") forwards++;
    }

    double fit = 1.0;
    if (team.tactics == "Pressing") {
        fit += min(0.12, pressers * 0.03);
        if (team.pressingIntensity >= 4 && defenders <= 3) fit -= 0.05;
    }
    if (team.tactics == "Counter") {
        fit += min(0.12, runners * 0.03);
        if (team.defensiveLine <= 2) fit += 0.03;
    }
    if (team.tactics == "Offensive") {
        fit += min(0.10, creators * 0.02 + forwards * 0.02);
        if (forwards <= 1) fit -= 0.08;
    }
    if (team.tactics == "Defensive" && defenders >= 4) fit += 0.06;
    if (team.matchInstruction == "Por bandas" && team.width >= 4) fit += 0.06;
    if (team.matchInstruction == "Juego directo" && runners >= 2) fit += 0.05;
    if (midfielders < 3 && team.tempo <= 2) fit -= 0.05;
    return clampValue(fit, 0.78, 1.18);
}

double tacticalAdvantage(const Team& team,
                         const Team& opponent,
                         const TacticalProfile& teamProfile,
                         const TacticalProfile& opponentProfile,
                         const vector<int>& xi) {
    double advantage = 0.0;
    advantage += (teamProfile.pressing - opponentProfile.directness) * 0.20;
    advantage += (teamProfile.width - opponentProfile.defensiveBlock) * 0.10;
    advantage += (teamProfile.transitionThreat - opponentProfile.risk) * 0.14;
    advantage += (teamProfile.setPieceThreat - opponentProfile.defensiveBlock * 0.60) * 0.12;
    advantage += (tacticalCompatibility(team, xi) - tacticalCompatibility(opponent, opponent.getStartingXIIndices())) * 0.30;

    if (team.matchInstruction == "Juego directo" && opponent.defensiveLine >= 4) advantage += 0.10;
    if (team.matchInstruction == "Bloque bajo" && opponent.tactics == "Offensive") advantage += 0.08;
    if (team.matchInstruction == "Contra-presion" && opponent.tempo >= 4) advantage += 0.07;
    if (team.tactics == "Counter" && opponent.tactics == "Pressing") advantage += 0.10;
    if (team.pressingIntensity >= 4 && team.defensiveLine >= 4 && opponent.tempo <= 3) advantage += 0.06;
    if (team.defensiveLine >= 4 && opponent.matchInstruction == "Juego directo") advantage -= 0.07;
    if (team.tactics == "Offensive" && opponent.tactics == "Defensive") advantage -= 0.03;
    if (team.markingStyle == "Hombre" && opponent.width >= 4) advantage -= 0.05;
    return clampValue(advantage, -0.40, 0.40);
}

double possessionWeight(const TacticalProfile& profile) {
    return profile.tempo * 0.26 + profile.width * 0.14 + profile.mentality * 0.20 - profile.directness * 0.18;
}

double transitionThreatWeight(const TacticalProfile& profile) {
    return profile.transitionThreat * 0.60 + profile.directness * 0.25 + profile.tempo * 0.15;
}

double defensiveSecurityWeight(const TacticalProfile& profile) {
    return profile.defensiveBlock * 0.55 - profile.risk * 0.25 + profile.pressing * 0.10;
}

}  // namespace tactics_engine
