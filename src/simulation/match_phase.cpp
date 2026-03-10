#include "simulation/match_phase.h"

#include "simulation/fatigue_engine.h"
#include "simulation/match_resolution.h"
#include "simulation/tactics_engine.h"
#include "utils/utils.h"

#include <algorithm>
#include <cmath>

using namespace std;

namespace match_phase {

MatchPhaseEvaluation evaluatePhase(const MatchSetup& setup,
                                   const Team& home,
                                   const Team& away,
                                   const TeamMatchSnapshot& homeSnapshot,
                                   const TeamMatchSnapshot& awaySnapshot,
                                   int phaseIndex,
                                   int minuteStart,
                                   int minuteEnd) {
    MatchPhaseEvaluation evaluation;
    MatchPhaseReport& phase = evaluation.report;
    phase.minuteStart = minuteStart;
    phase.minuteEnd = minuteEnd;

    const int homePossessionShare = clampInt(
        static_cast<int>(round(50.0 + (homeSnapshot.midfieldControl - awaySnapshot.midfieldControl) / 3.4 +
                               (tactics_engine::possessionWeight(homeSnapshot.tacticalProfile) -
                                tactics_engine::possessionWeight(awaySnapshot.tacticalProfile)) *
                                   10.0)),
        32, 68);
    const int awayPossessionShare = 100 - homePossessionShare;
    const double intensity = clampValue(0.92 + phaseIndex * 0.04 +
                                            (homeSnapshot.tacticalProfile.tempo + awaySnapshot.tacticalProfile.tempo) * 0.22 +
                                            (homeSnapshot.tacticalProfile.pressing + awaySnapshot.tacticalProfile.pressing) *
                                                0.15,
                                        0.90, 1.65);
    const double homeSecurity = tactics_engine::defensiveSecurityWeight(homeSnapshot.tacticalProfile);
    const double awaySecurity = tactics_engine::defensiveSecurityWeight(awaySnapshot.tacticalProfile);
    const double homeTransition = tactics_engine::transitionThreatWeight(homeSnapshot.tacticalProfile);
    const double awayTransition = tactics_engine::transitionThreatWeight(awaySnapshot.tacticalProfile);

    const double homeAttack =
        max(12.0, (homeSnapshot.attackPower * homeSnapshot.moraleFactor * homeSnapshot.fatigueFactor +
                   homeSnapshot.midfieldControl * 0.55 + homeSnapshot.tacticalAdvantage * 20.0) *
                      setup.context.homeAdvantage * setup.context.weatherModifier);
    const double awayAttack =
        max(12.0, (awaySnapshot.attackPower * awaySnapshot.moraleFactor * awaySnapshot.fatigueFactor +
                   awaySnapshot.midfieldControl * 0.55 + awaySnapshot.tacticalAdvantage * 20.0) *
                      setup.context.weatherModifier);
    const double homeDefense =
        max(10.0, homeSnapshot.defensePower * (0.94 + homeSnapshot.fatigueFactor * 0.08) +
                      homeSnapshot.midfieldControl * 0.28 +
                      tactics_engine::defensiveSecurityWeight(homeSnapshot.tacticalProfile) * 12.0);
    const double awayDefense =
        max(10.0, awaySnapshot.defensePower * (0.94 + awaySnapshot.fatigueFactor * 0.08) +
                      awaySnapshot.midfieldControl * 0.28 +
                      tactics_engine::defensiveSecurityWeight(awaySnapshot.tacticalProfile) * 12.0);

    const double homeOpportunity = match_resolution::opportunityProbability(
        homeAttack, awayDefense, homePossessionShare / 100.0,
        0.95 + homeSnapshot.tacticalProfile.mentality * 0.10, intensity);
    const double awayOpportunity = match_resolution::opportunityProbability(
        awayAttack, homeDefense, awayPossessionShare / 100.0,
        0.95 + awaySnapshot.tacticalProfile.mentality * 0.10, intensity);

    phase.dominantTeam = homePossessionShare >= awayPossessionShare ? home.name : away.name;
    phase.homePossessionShare = homePossessionShare;
    phase.awayPossessionShare = awayPossessionShare;
    phase.intensity = intensity;
    phase.homeChanceProbability = homeOpportunity;
    phase.awayChanceProbability = awayOpportunity;
    phase.homeDefensiveRisk = clampValue(0.14 + awayTransition * 0.32 + awaySnapshot.tacticalProfile.directness * 0.12 +
                                             homeSnapshot.tacticalProfile.risk * 0.16 - homeSecurity * 0.18,
                                         0.06, 0.82);
    phase.awayDefensiveRisk = clampValue(0.14 + homeTransition * 0.32 + homeSnapshot.tacticalProfile.directness * 0.12 +
                                             awaySnapshot.tacticalProfile.risk * 0.16 - awaySecurity * 0.18,
                                         0.06, 0.82);
    phase.homeFatigueGain =
        static_cast<int>(round(intensity * fatigue_engine::phaseFatigueGain(home, phaseIndex)));
    phase.awayFatigueGain =
        static_cast<int>(round(intensity * fatigue_engine::phaseFatigueGain(away, phaseIndex)));
    phase.injuryRisk =
        clampValue(0.05 + intensity * 0.04 + (phase.homeFatigueGain + phase.awayFatigueGain) / 160.0, 0.05, 0.22);

    const int homePossessionChains = clampInt(
        static_cast<int>(round(3.0 + homePossessionShare / 11.0 + intensity * 1.6 +
                               homeSnapshot.tacticalProfile.tempo * 1.1 + homeSnapshot.tacticalProfile.width * 0.8)),
        3, 11);
    const int awayPossessionChains = clampInt(
        static_cast<int>(round(3.0 + awayPossessionShare / 11.0 + intensity * 1.6 +
                               awaySnapshot.tacticalProfile.tempo * 1.1 + awaySnapshot.tacticalProfile.width * 0.8)),
        3, 11);

    const double homeProgressionRate = clampValue(0.34 + homeSnapshot.tacticalProfile.width * 0.10 +
                                                      homeSnapshot.tacticalProfile.directness * 0.08 +
                                                      homeOpportunity * 0.18 - awaySecurity * 0.08,
                                                  0.20, 0.88);
    const double awayProgressionRate = clampValue(0.34 + awaySnapshot.tacticalProfile.width * 0.10 +
                                                      awaySnapshot.tacticalProfile.directness * 0.08 +
                                                      awayOpportunity * 0.18 - homeSecurity * 0.08,
                                                  0.20, 0.88);

    const int homeProgressions = clampInt(
        static_cast<int>(round(homePossessionChains * homeProgressionRate + rand01() * 1.4 - 0.3)),
        1, homePossessionChains);
    const int awayProgressions = clampInt(
        static_cast<int>(round(awayPossessionChains * awayProgressionRate + rand01() * 1.4 - 0.3)),
        1, awayPossessionChains);

    const double homeAttackRate = clampValue(0.30 + homeSnapshot.tacticalProfile.mentality * 0.14 +
                                                 homeTransition * 0.16 + max(0.0, homeAttack - awayDefense) / 320.0 -
                                                 awaySecurity * 0.12,
                                             0.16, 0.86);
    const double awayAttackRate = clampValue(0.30 + awaySnapshot.tacticalProfile.mentality * 0.14 +
                                                 awayTransition * 0.16 + max(0.0, awayAttack - homeDefense) / 320.0 -
                                                 homeSecurity * 0.12,
                                             0.16, 0.86);

    const int homeAttacks = clampInt(
        static_cast<int>(round(homeProgressions * homeAttackRate + rand01() * 1.2 - 0.2)),
        0, homeProgressions);
    const int awayAttacks = clampInt(
        static_cast<int>(round(awayProgressions * awayAttackRate + rand01() * 1.2 - 0.2)),
        0, awayProgressions);

    const double homeChanceRate = clampValue(0.10 + homeOpportunity * 0.22 +
                                                 max(0.0, homeAttack - awayDefense) / 430.0 +
                                                 homeSnapshot.tacticalProfile.directness * 0.05 -
                                                 awaySecurity * 0.06,
                                             0.04, 0.65);
    const double awayChanceRate = clampValue(0.10 + awayOpportunity * 0.22 +
                                                 max(0.0, awayAttack - homeDefense) / 430.0 +
                                                 awaySnapshot.tacticalProfile.directness * 0.05 -
                                                 homeSecurity * 0.06,
                                             0.04, 0.65);

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
                 0, min(4, homeAttacks));
    evaluation.awayChanceCount =
        clampInt(static_cast<int>(round(awayAttacks * awayChanceRate + rand01() * 1.1 - 0.2)),
                 0, min(4, awayAttacks));
    return evaluation;
}

}  // namespace match_phase
