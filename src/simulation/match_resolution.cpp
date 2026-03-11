#include "simulation/match_resolution.h"

#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace match_resolution {

double opportunityProbability(double effectiveAttack,
                              double effectiveDefense,
                              double possessionFactor,
                              double mentalityFactor,
                              double matchMomentFactor) {
    const double ratio = effectiveAttack / max(1.0, effectiveDefense);
    double probability = 0.04 + ratio * 0.10 + possessionFactor * 0.08 + mentalityFactor * 0.06 +
                         matchMomentFactor * 0.04;
    return clampValue(probability, 0.04, 0.54);
}

double progressionProbability(double buildUpQuality,
                              double pressResistance,
                              double opponentPressingLoad,
                              double possessionFactor,
                              double matchMomentFactor) {
    const double ratio = (buildUpQuality * 0.62 + pressResistance * 0.38) / max(45.0, opponentPressingLoad);
    const double probability = 0.18 + ratio * 0.20 + possessionFactor * 0.10 + matchMomentFactor * 0.04;
    return clampValue(probability, 0.16, 0.80);
}

double attackConversionProbability(double lineBreakThreat,
                                   double opponentDefensiveShape,
                                   double tacticalRisk,
                                   double mentalityFactor,
                                   double fatigueFactor) {
    const double ratio = lineBreakThreat / max(48.0, opponentDefensiveShape);
    const double probability = 0.12 + ratio * 0.18 + tacticalRisk * 0.12 + mentalityFactor * 0.10 + fatigueFactor * 0.08;
    return clampValue(probability, 0.10, 0.68);
}

double shotCreationProbability(double chanceCreation,
                               double finishingQuality,
                               double opponentDefense,
                               double defensiveRisk,
                               double setPieceThreat,
                               double fatigueFactor) {
    const double ratio = (chanceCreation * 0.65 + finishingQuality * 0.25 + setPieceThreat * 0.10) / max(52.0, opponentDefense);
    const double probability = 0.04 + ratio * 0.14 + defensiveRisk * 0.10 + fatigueFactor * 0.06;
    return clampValue(probability, 0.04, 0.38);
}

}  // namespace match_resolution
