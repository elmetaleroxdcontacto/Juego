#include "simulation/match_phase.h"

#include "simulation/fatigue_engine.h"
#include "simulation/match_resolution.h"
#include "simulation/tactics_engine.h"
#include "utils/utils.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace {

double scoreUrgency(int goalsFor, int goalsAgainst, int phaseIndex) {
    const int goalDiff = goalsFor - goalsAgainst;
    if (goalDiff < 0) {
        return clampValue((-goalDiff) * (0.10 + phaseIndex * 0.03) + (phaseIndex >= 4 ? 0.04 : 0.0), 0.0, 0.42);
    }
    if (goalDiff > 0) {
        return -clampValue(goalDiff * (0.06 + phaseIndex * 0.02), 0.0, 0.24);
    }
    if (phaseIndex >= 5) return 0.04;
    if (phaseIndex >= 3) return 0.02;
    return 0.0;
}

double availabilityFactor(int playersAvailable) {
    return clampValue(1.0 - max(0, 11 - playersAvailable) * 0.08, 0.70, 1.00);
}

}  // namespace

namespace match_phase {

MatchPhaseEvaluation evaluatePhase(const MatchSetup& setup,
                                   const Team& home,
                                   const Team& away,
                                   const TeamMatchSnapshot& homeSnapshot,
                                   const TeamMatchSnapshot& awaySnapshot,
                                   int phaseIndex,
                                   int minuteStart,
                                   int minuteEnd,
                                   int homeGoals,
                                   int awayGoals,
                                   int homePlayersAvailable,
                                   int awayPlayersAvailable) {
    MatchPhaseEvaluation evaluation;
    MatchPhaseReport& phase = evaluation.report;
    phase.minuteStart = minuteStart;
    phase.minuteEnd = minuteEnd;
    phase.homePlayersAvailable = homePlayersAvailable;
    phase.awayPlayersAvailable = awayPlayersAvailable;
    phase.homeUrgency = scoreUrgency(homeGoals, awayGoals, phaseIndex);
    phase.awayUrgency = scoreUrgency(awayGoals, homeGoals, phaseIndex);

    const double homeAvailability = availabilityFactor(homePlayersAvailable);
    const double awayAvailability = availabilityFactor(awayPlayersAvailable);

    const int homePossessionShare = clampInt(
        static_cast<int>(round(50.0 + (homeSnapshot.midfieldControl - awaySnapshot.midfieldControl) / 4.4 +
                               (homeSnapshot.pressResistance - awaySnapshot.pressingLoad) / 8.0 +
                               (tactics_engine::possessionWeight(homeSnapshot.tacticalProfile) -
                                tactics_engine::possessionWeight(awaySnapshot.tacticalProfile)) *
                                   10.0 +
                               (homeSnapshot.tacticalCompatibility - awaySnapshot.tacticalCompatibility) * 8.0 +
                               (phase.homeUrgency - phase.awayUrgency) * 18.0 +
                               (awayPlayersAvailable - homePlayersAvailable) * 2.5)),
        34, 66);
    const int awayPossessionShare = 100 - homePossessionShare;
    const double fatigueDrag = (2.0 - homeSnapshot.fatigueFactor - awaySnapshot.fatigueFactor) * 0.12;
    const double intensity = clampValue(0.88 + phaseIndex * 0.035 +
                                            (homeSnapshot.tacticalProfile.tempo + awaySnapshot.tacticalProfile.tempo) * 0.18 +
                                            (homeSnapshot.tacticalProfile.pressing + awaySnapshot.tacticalProfile.pressing) *
                                                0.14 -
                                            fatigueDrag +
                                            abs(phase.homeUrgency - phase.awayUrgency) * 0.12 +
                                            (abs(homeGoals - awayGoals) <= 1 && phaseIndex >= 3 ? 0.03 : 0.0),
                                        0.86, 1.42);
    const double homeSecurity = tactics_engine::defensiveSecurityWeight(homeSnapshot.tacticalProfile);
    const double awaySecurity = tactics_engine::defensiveSecurityWeight(awaySnapshot.tacticalProfile);
    const double homeTransition = tactics_engine::transitionThreatWeight(homeSnapshot.tacticalProfile);
    const double awayTransition = tactics_engine::transitionThreatWeight(awaySnapshot.tacticalProfile);
    const double homeIntent = 1.0 + max(0.0, phase.homeUrgency) * 0.30;
    const double awayIntent = 1.0 + max(0.0, phase.awayUrgency) * 0.30;
    const double homeProtection = 1.0 + max(0.0, -phase.homeUrgency) * 0.14;
    const double awayProtection = 1.0 + max(0.0, -phase.awayUrgency) * 0.14;

    const double homeAttack =
        max(12.0, (homeSnapshot.attackPower * homeSnapshot.moraleFactor * homeSnapshot.fatigueFactor +
                   homeSnapshot.chanceCreation * 0.32 +
                   homeSnapshot.lineBreakThreat * 0.18 +
                   homeSnapshot.tacticalAdvantage * 18.0) *
                      setup.context.homeAdvantage * setup.context.weatherModifier * homeAvailability * homeIntent);
    const double awayAttack =
        max(12.0, (awaySnapshot.attackPower * awaySnapshot.moraleFactor * awaySnapshot.fatigueFactor +
                   awaySnapshot.chanceCreation * 0.32 +
                   awaySnapshot.lineBreakThreat * 0.18 +
                   awaySnapshot.tacticalAdvantage * 18.0) *
                      setup.context.weatherModifier * awayAvailability * awayIntent);
    const double homeDefense =
        max(10.0, homeSnapshot.defensePower * (0.95 + homeSnapshot.fatigueFactor * 0.07) +
                      homeSnapshot.defensiveShape * 0.22 +
                      tactics_engine::defensiveSecurityWeight(homeSnapshot.tacticalProfile) * 14.0) *
        homeAvailability * (1.0 - max(0.0, phase.homeUrgency) * 0.08) * homeProtection;
    const double awayDefense =
        max(10.0, awaySnapshot.defensePower * (0.95 + awaySnapshot.fatigueFactor * 0.07) +
                      awaySnapshot.defensiveShape * 0.22 +
                      tactics_engine::defensiveSecurityWeight(awaySnapshot.tacticalProfile) * 14.0) *
        awayAvailability * (1.0 - max(0.0, phase.awayUrgency) * 0.08) * awayProtection;

    const double homeOpportunity = match_resolution::opportunityProbability(
        homeAttack, awayDefense, homePossessionShare / 100.0,
        0.95 + homeSnapshot.tacticalProfile.mentality * 0.10 + max(0.0, phase.homeUrgency) * 0.18,
        intensity);
    const double awayOpportunity = match_resolution::opportunityProbability(
        awayAttack, homeDefense, awayPossessionShare / 100.0,
        0.95 + awaySnapshot.tacticalProfile.mentality * 0.10 + max(0.0, phase.awayUrgency) * 0.18,
        intensity);

    phase.dominantTeam = homePossessionShare >= awayPossessionShare ? home.name : away.name;
    phase.homePossessionShare = homePossessionShare;
    phase.awayPossessionShare = awayPossessionShare;
    phase.intensity = intensity;
    phase.homeChanceProbability = homeOpportunity;
    phase.awayChanceProbability = awayOpportunity;
    phase.homeDefensiveRisk = clampValue(0.12 + awayTransition * 0.30 + awaySnapshot.tacticalProfile.directness * 0.10 +
                                             homeSnapshot.tacticalProfile.risk * 0.20 -
                                             homeSecurity * 0.16 -
                                             homeSnapshot.defensiveShape / 900.0 +
                                             max(0.0, static_cast<double>(home.defensiveLine - 3)) * 0.04 +
                                             max(0.0, phase.homeUrgency) * 0.10 +
                                             max(0, 11 - homePlayersAvailable) * 0.04 -
                                             max(0.0, -phase.homeUrgency) * 0.05,
                                         0.05, 0.72);
    phase.awayDefensiveRisk = clampValue(0.12 + homeTransition * 0.30 + homeSnapshot.tacticalProfile.directness * 0.10 +
                                             awaySnapshot.tacticalProfile.risk * 0.20 -
                                             awaySecurity * 0.16 -
                                             awaySnapshot.defensiveShape / 900.0 +
                                             max(0.0, static_cast<double>(away.defensiveLine - 3)) * 0.04 +
                                             max(0.0, phase.awayUrgency) * 0.10 +
                                             max(0, 11 - awayPlayersAvailable) * 0.04 -
                                             max(0.0, -phase.awayUrgency) * 0.05,
                                         0.05, 0.72);
    phase.homeFatigueGain =
        static_cast<int>(round(intensity * (1.0 + max(0.0, phase.homeUrgency) * 0.35) *
                               fatigue_engine::phaseFatigueGain(home, phaseIndex)));
    phase.awayFatigueGain =
        static_cast<int>(round(intensity * (1.0 + max(0.0, phase.awayUrgency) * 0.35) *
                               fatigue_engine::phaseFatigueGain(away, phaseIndex)));
    phase.injuryRisk =
        clampValue(0.05 + intensity * 0.04 + (phase.homeFatigueGain + phase.awayFatigueGain) / 160.0 +
                       max(0, 22 - homePlayersAvailable - awayPlayersAvailable) / 140.0,
                   0.05,
                   0.24);

    const int homePossessionChains = clampInt(
        static_cast<int>(round(3.0 + homePossessionShare / 11.0 + intensity * 1.6 +
                               homeSnapshot.tacticalProfile.tempo * 1.1 + homeSnapshot.tacticalProfile.width * 0.8)),
        3, 10);
    const int awayPossessionChains = clampInt(
        static_cast<int>(round(3.0 + awayPossessionShare / 11.0 + intensity * 1.6 +
                               awaySnapshot.tacticalProfile.tempo * 1.1 + awaySnapshot.tacticalProfile.width * 0.8)),
        3, 10);

    const double homeProgressionRate = match_resolution::progressionProbability(
        homeSnapshot.chanceCreation,
        homeSnapshot.pressResistance,
        awaySnapshot.pressingLoad,
        homePossessionShare / 100.0,
        intensity);
    const double awayProgressionRate = match_resolution::progressionProbability(
        awaySnapshot.chanceCreation,
        awaySnapshot.pressResistance,
        homeSnapshot.pressingLoad,
        awayPossessionShare / 100.0,
        intensity);

    const int homeProgressions = clampInt(
        static_cast<int>(round(homePossessionChains * homeProgressionRate + rand01() * 1.4 - 0.3)),
        1, homePossessionChains);
    const int awayProgressions = clampInt(
        static_cast<int>(round(awayPossessionChains * awayProgressionRate + rand01() * 1.4 - 0.3)),
        1, awayPossessionChains);

    const double homeAttackRate = match_resolution::attackConversionProbability(
        homeSnapshot.lineBreakThreat,
        awaySnapshot.defensiveShape,
        phase.awayDefensiveRisk,
        0.95 + homeSnapshot.tacticalProfile.mentality * 0.10 + max(0.0, phase.homeUrgency) * 0.16,
        homeSnapshot.fatigueFactor);
    const double awayAttackRate = match_resolution::attackConversionProbability(
        awaySnapshot.lineBreakThreat,
        homeSnapshot.defensiveShape,
        phase.homeDefensiveRisk,
        0.95 + awaySnapshot.tacticalProfile.mentality * 0.10 + max(0.0, phase.awayUrgency) * 0.16,
        awaySnapshot.fatigueFactor);

    const int homeAttacks = clampInt(
        static_cast<int>(round(homeProgressions * homeAttackRate + rand01() * 1.2 - 0.2)),
        0, homeProgressions);
    const int awayAttacks = clampInt(
        static_cast<int>(round(awayProgressions * awayAttackRate + rand01() * 1.2 - 0.2)),
        0, awayProgressions);

    const double homeChanceRate = match_resolution::shotCreationProbability(
        homeSnapshot.chanceCreation,
        homeSnapshot.finishingQuality,
        awayDefense,
        phase.awayDefensiveRisk,
        homeSnapshot.setPieceThreat,
        homeSnapshot.fatigueFactor);
    const double awayChanceRate = match_resolution::shotCreationProbability(
        awaySnapshot.chanceCreation,
        awaySnapshot.finishingQuality,
        homeDefense,
        phase.homeDefensiveRisk,
        awaySnapshot.setPieceThreat,
        awaySnapshot.fatigueFactor);

    evaluation.homeAttack = homeAttack;
    evaluation.awayAttack = awayAttack;
    evaluation.homeDefense = homeDefense;
    evaluation.awayDefense = awayDefense;
    evaluation.homePossessionChains = homePossessionChains;
    evaluation.awayPossessionChains = awayPossessionChains;
    evaluation.homeProgressions = homeProgressions;
    evaluation.awayProgressions = awayProgressions;
    evaluation.homeAttacks = homeAttacks;
    evaluation.awayAttacks = awayAttacks;
    phase.homePossessionChains = homePossessionChains;
    phase.awayPossessionChains = awayPossessionChains;
    phase.homeProgressions = homeProgressions;
    phase.awayProgressions = awayProgressions;
    phase.homeAttacks = homeAttacks;
    phase.awayAttacks = awayAttacks;
    evaluation.homeChanceCount =
        clampInt(static_cast<int>(round(homeAttacks * homeChanceRate + rand01() * 1.1 - 0.2)),
                 0, min(3, homeAttacks));
    evaluation.awayChanceCount =
        clampInt(static_cast<int>(round(awayAttacks * awayChanceRate + rand01() * 1.1 - 0.2)),
                 0, min(3, awayAttacks));
    if (homeGoals < awayGoals && phaseIndex >= 4 && homeProgressions > 0 && evaluation.homeAttacks == 0) {
        evaluation.homeAttacks = 1;
        phase.homeAttacks = 1;
    }
    if (awayGoals < homeGoals && phaseIndex >= 4 && awayProgressions > 0 && evaluation.awayAttacks == 0) {
        evaluation.awayAttacks = 1;
        phase.awayAttacks = 1;
    }
    return evaluation;
}

}  // namespace match_phase
