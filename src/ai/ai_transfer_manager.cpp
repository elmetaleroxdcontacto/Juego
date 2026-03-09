#include "ai/ai_transfer_manager.h"

#include "finance/finance_system.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

using namespace std;

namespace ai_transfer_manager {

ClubTransferStrategy buildClubTransferStrategy(const Career& career, const Team& team) {
    ClubTransferStrategy strategy;
    const SquadNeedReport squad = ai_squad_planner::analyzeSquad(team);
    strategy.weakestPosition = squad.weakestPosition;
    strategy.surplusPosition = squad.surplusPosition;
    strategy.needsLiquidity = team.budget < 120000 || team.debt > team.sponsorWeekly * 18;
    strategy.youthFocus = team.youthFacilityLevel >= 3 || team.youthCoach >= 75 || team.youthIdentity == "Cantera estructurada";
    strategy.promotionPush = teamPrestigeScore(team) >= 58 || career.currentSeason <= 2;
    strategy.maxTargets = strategy.promotionPush ? 6 : 4;
    strategy.maxTransferBudget = finance_system::calculateTransferBuffer(team);
    strategy.maxWageBudget = max(20000LL, finance_system::calculateWeeklyPayroll(team) / 4);
    strategy.priorityPositions = squad.priorityPositions;
    return strategy;
}

TransferTarget evaluateTarget(const Career& career,
                              const Team& buyer,
                              const Team& seller,
                              const Player& player,
                              const ClubTransferStrategy& strategy) {
    TransferTarget target;
    target.clubName = seller.name;
    target.playerName = player.name;
    target.position = player.position;
    target.qualityScore = player.skill;
    target.potentialScore = player.potential;
    target.age = player.age;
    target.expectedFee = max(player.value, player.releaseClause * 55 / 100) + rivalrySurcharge(buyer, seller, player.value);
    target.expectedWage = max(wageDemandFor(player), player.wage);
    target.contractRunningOut = player.contractWeeks <= 20;
    target.availableForLoan = player.age <= 24 && player.contractWeeks > 18;
    target.squadNeedScore = ai_squad_planner::positionNeedScore(buyer, normalizePosition(player.position));
    target.fitScore = player.skill * 3 + player.potential * 2 - player.age * 2 + target.squadNeedScore;
    if (strategy.youthFocus) target.fitScore += max(0, 24 - player.age) * 3 + max(0, player.potential - player.skill) * 2;
    if (strategy.promotionPush) target.fitScore += player.skill * 2 + max(0, player.currentForm - 55);
    if (target.contractRunningOut) target.fitScore += 8;

    const long long totalCost = target.expectedFee + target.expectedWage * 8;
    if (totalCost <= strategy.maxTransferBudget) target.affordabilityScore = 24;
    else if (target.availableForLoan && target.expectedWage <= strategy.maxWageBudget) target.affordabilityScore = 16;
    else if (target.expectedFee <= strategy.maxTransferBudget * 12 / 10) target.affordabilityScore = 8;
    else target.affordabilityScore = -12;

    string reason;
    if (playerRejectsMove(career, buyer, seller, player, NegotiationPromise::Rotation, reason)) {
        target.affordabilityScore -= 18;
    }

    target.totalScore = target.fitScore * 0.55 + target.potentialScore * 0.18 +
                        target.affordabilityScore * 2.1 +
                        (target.availableForLoan ? 6.0 : 0.0) +
                        (target.contractRunningOut ? 4.0 : 0.0);
    return target;
}

}  // namespace ai_transfer_manager
