#include "engine/rival_ai.h"

#include "engine/models.h"
#include "engine/team_personality.h"
#include "utils/utils.h"

#include <algorithm>
#include <cmath>
#include <sstream>

using namespace std;

RivalAI createRivalAI(const std::string& teamName, int prestigeLevel) {
    RivalAI ai;
    ai.personality.teamName = teamName;

    if (prestigeLevel < 40) {
        ai.personality.playstyle = "defensive";
        ai.personality.adaptability = 40;
        ai.personality.tactical_awareness = 35;
    } else if (prestigeLevel < 60) {
        ai.personality.playstyle = "balanced";
        ai.personality.adaptability = 55;
        ai.personality.tactical_awareness = 50;
    } else if (prestigeLevel < 80) {
        ai.personality.playstyle = "aggressive";
        ai.personality.adaptability = 65;
        ai.personality.tactical_awareness = 65;
    } else {
        ai.personality.playstyle = "possession";
        ai.personality.adaptability = 75;
        ai.personality.tactical_awareness = 80;
    }

    ai.personality.unpredictability = 100 - prestigeLevel;
    return ai;
}

RivalAI createRivalAI(const Team& team) {
    RivalAI ai;
    ai.personality.teamName = team.name;
    const TeamPersonalityProfile profile = buildTeamPersonalityProfile(team);

    if (profile.pressBias >= std::max(profile.transitionBias, std::max(profile.blockBias, profile.controlBias))) {
        ai.personality.playstyle = "aggressive";
    } else if (profile.blockBias >= std::max(profile.transitionBias, profile.controlBias)) {
        ai.personality.playstyle = "defensive";
    } else if (profile.transitionBias >= std::max(profile.widthBias, profile.controlBias)) {
        ai.personality.playstyle = "counter-attack";
    } else if (profile.controlBias >= profile.widthBias) {
        ai.personality.playstyle = "possession";
    } else {
        ai.personality.playstyle = "balanced";
    }

    ai.personality.adaptability = clampInt(profile.adaptability, 35, 90);
    ai.personality.tactical_awareness =
        clampInt(42 + team.headCoachReputation / 2 + team.performanceAnalyst / 4, 35, 92);
    ai.personality.unpredictability =
        clampInt(72 - team.headCoachReputation / 3 - team.performanceAnalyst / 5 +
                     std::abs(50 - profile.riskAppetite) / 3 + std::max(0, 45 - team.jobSecurity) / 2,
                 12, 78);
    return ai;
}

std::string RivalAI::decideTactics(const std::string& playerTeamFormation, const std::string& playerTactics) const {
    std::string decidedTactics = toLower(personality.playstyle.empty() ? std::string("balanced") : personality.playstyle);
    const std::string playerTacticsLower = toLower(playerTactics);
    const std::string playerFormationLower = toLower(playerTeamFormation);

    if (!memoryBank.empty()) {
        const auto lastMemory = memoryBank.back();
        if (lastMemory.lastMatchOutcome == 1 && lastMemory.consecutiveVsThisTeam > 0) {
            return lastMemory.favoredTactics.empty() ? decidedTactics : toLower(lastMemory.favoredTactics.front());
        }
        if (lastMemory.lastMatchOutcome == -1 && personality.adaptability > 60) {
            if (playerTacticsLower == "pressing" || playerTacticsLower == "offensive" ||
                playerTacticsLower == "aggressive") {
                decidedTactics = "defensive";
            } else if (playerTacticsLower == "defensive") {
                decidedTactics = "counter-attack";
            } else {
                decidedTactics = "aggressive";
            }
        }
    }

    if (personality.tactical_awareness >= 72 && playerFormationLower.find("4-3-3") != std::string::npos &&
        decidedTactics == "balanced") {
        decidedTactics = "counter-attack";
    }

    return decidedTactics;
}

std::vector<TacticalAdjustment> RivalAI::generateInMatchAdjustments(
    int currentMinute, int myScore, int oppScore,
    const std::string& myTactics, const std::string& oppTactics) const {

    std::vector<TacticalAdjustment> adjustments;
    const std::string myTacticsLower = toLower(myTactics);
    const std::string oppTacticsLower = toLower(oppTactics);

    if (myScore > oppScore) {
        if (personality.playstyle == "aggressive") {
            TacticalAdjustment adj;
            adj.adjustmentType = "intensity_boost";
            adj.intensity = 15;
            adj.minuteToApply = currentMinute + 5;
            adj.reactive = true;
            adjustments.push_back(adj);
        } else if (personality.playstyle == "defensive") {
            TacticalAdjustment adj;
            adj.adjustmentType = "defensive_focus";
            adj.intensity = 10;
            adj.minuteToApply = currentMinute + 3;
            adj.reactive = true;
            adjustments.push_back(adj);
        }
    } else if (oppScore > myScore && personality.adaptability > 50) {
        TacticalAdjustment adj;
        adj.adjustmentType =
            (oppTacticsLower == "pressing" || myTacticsLower == "defensive") ? "counter_setup" : "formation_change";
        adj.intensity = 20;
        adj.minuteToApply = currentMinute + 10;
        adj.reactive = true;
        adjustments.push_back(adj);
    }

    if (currentMinute == 45 || currentMinute == 80) {
        if (myScore == oppScore && personality.playstyle != "balanced") {
            TacticalAdjustment adj;
            adj.adjustmentType = "intensity_boost";
            adj.intensity = 10;
            adj.minuteToApply = currentMinute;
            adj.reactive = false;
            adjustments.push_back(adj);
        }
    }

    return adjustments;
}

void RivalAI::analyzeOpponentPattern(const std::string& opponentTeam, int goalsScoredByOpp, int goalsConceded) {
    auto memIt = std::find_if(memoryBank.begin(), memoryBank.end(),
        [&opponentTeam](const RivalMemory& m) { return m.opponentName == opponentTeam; });

    RivalMemory* memory;
    if (memIt != memoryBank.end()) {
        memory = &(*memIt);
    } else {
        memoryBank.push_back(RivalMemory{});
        memory = &memoryBank.back();
        memory->opponentName = opponentTeam;
    }

    const int previousOutcome = memory->lastMatchOutcome;
    const int newOutcome = goalsScoredByOpp > goalsConceded ? 1 : (goalsScoredByOpp < goalsConceded ? -1 : 0);
    memory->matchesPlayed++;
    if (newOutcome > 0) {
        memory->wins++;
    } else if (newOutcome == 0) {
        memory->draws++;
    } else {
        memory->losses++;
    }
    memory->lastMatchOutcome = newOutcome;
    memory->consecutiveVsThisTeam =
        (newOutcome != 0 && previousOutcome == newOutcome) ? memory->consecutiveVsThisTeam + 1 : (newOutcome == 0 ? 0 : 1);

    if (goalsConceded > 2) {
        memory->identifiedWeaknesses.push_back("defensive_fragility");
    }
    if (goalsScoredByOpp >= 2) {
        memory->identifiedStrengths.push_back("sharp_attack");
    }
}

int RivalAI::getErrorProbability(int minuteOfMatch) {
    int baseErrorChance = personality.unpredictability / 2;
    if (minuteOfMatch > 75) {
        baseErrorChance += 10;
    }
    if (minuteOfMatch < 45) {
        baseErrorChance -= 5;
    }
    return std::max(0, std::min(100, baseErrorChance));
}

void applyRivalAIAdjustments(std::string& rivalTactics, const std::vector<TacticalAdjustment>& adjustments) {
    for (const auto& adj : adjustments) {
        if (adj.adjustmentType == "defensive_focus") {
            rivalTactics = "defensive";
        } else if (adj.adjustmentType == "intensity_boost") {
            if (rivalTactics != "aggressive") {
                rivalTactics = "aggressive";
            }
        } else if (adj.adjustmentType == "counter_setup") {
            rivalTactics = "counter-attack";
        }
    }
}
