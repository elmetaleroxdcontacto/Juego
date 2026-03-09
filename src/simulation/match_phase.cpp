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
    phase.homeFatigueGain =
        static_cast<int>(round(intensity * fatigue_engine::phaseFatigueGain(home, phaseIndex)));
    phase.awayFatigueGain =
        static_cast<int>(round(intensity * fatigue_engine::phaseFatigueGain(away, phaseIndex)));
    phase.injuryRisk =
        clampValue(0.05 + intensity * 0.04 + (phase.homeFatigueGain + phase.awayFatigueGain) / 160.0, 0.05, 0.22);

    evaluation.homeAttack = homeAttack;
    evaluation.awayAttack = awayAttack;
    evaluation.homeDefense = homeDefense;
    evaluation.awayDefense = awayDefense;
    evaluation.homeChanceCount =
        clampInt(static_cast<int>(round(homeOpportunity * (homePossessionShare >= 54 ? 5.2 : 3.8) * intensity +
                                       rand01() * 1.4 - 0.3)),
                 0, 4);
    evaluation.awayChanceCount =
        clampInt(static_cast<int>(round(awayOpportunity * (awayPossessionShare >= 54 ? 5.2 : 3.8) * intensity +
                                       rand01() * 1.4 - 0.3)),
                 0, 4);
    return evaluation;
}

}  // namespace match_phase
