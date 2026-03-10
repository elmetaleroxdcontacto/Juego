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
    double probability = 0.08 + ratio * 0.14 + possessionFactor * 0.10 + mentalityFactor * 0.08 +
                         matchMomentFactor * 0.05;
    return clampValue(probability, 0.05, 0.72);
}

}  // namespace match_resolution
