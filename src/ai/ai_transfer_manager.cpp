#include "ai/ai_transfer_manager.h"

#include "finance/finance_system.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

bool onScoutingShortlist(const Career& career, const Team& seller, const Player& player) {
    const string entry = seller.name + "|" + player.name;
    return find(career.scoutingShortlist.begin(), career.scoutingShortlist.end(), entry) != career.scoutingShortlist.end();
}

}  // namespace

namespace ai_transfer_manager {

ClubTransferStrategy buildClubTransferStrategy(const Career& career, const Team& team) {
    ClubTransferStrategy strategy;
    const SquadNeedReport squad = ai_squad_planner::analyzeSquad(team);
    strategy.weakestPosition = squad.weakestPosition;
    strategy.surplusPosition = squad.surplusPosition;
    strategy.needsLiquidity = team.budget < 120000 || team.debt > team.sponsorWeekly * 18;
    strategy.youthFocus = team.youthFacilityLevel >= 3 || team.youthCoach >= 75 || team.youthIdentity == "Cantera estructurada";
    strategy.promotionPush = teamPrestigeScore(team) >= 58 || career.currentSeason <= 2;
    strategy.needsStarter = squad.rotationRisk >= 5 || !squad.thinPositions.empty();
    strategy.trustYouthCover = strategy.youthFocus && !squad.youthCoverPositions.empty();
    strategy.rotationRisk = squad.rotationRisk;
    strategy.salePressure = squad.salePressure;
    strategy.maxTargets = strategy.promotionPush ? 6 : 4;
    strategy.maxTransferBudget = finance_system::calculateTransferBuffer(team);
    strategy.maxWageBudget = max(20000LL, finance_system::calculateWeeklyPayroll(team) / 4);
    strategy.priorityPositions = squad.priorityPositions;
    strategy.thinPositions = squad.thinPositions;
    strategy.youthCoverPositions = squad.youthCoverPositions;
    strategy.saleCandidates = squad.saleCandidates;
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
    target.onShortlist = onScoutingShortlist(career, seller, player);
    target.urgentNeed = find(strategy.thinPositions.begin(), strategy.thinPositions.end(), normalizePosition(player.position)) != strategy.thinPositions.end();
    target.fitScore = player.skill * 3 + player.potential * 2 - player.age * 2 + target.squadNeedScore;
    if (strategy.youthFocus) target.fitScore += max(0, 24 - player.age) * 3 + max(0, player.potential - player.skill) * 2;
    if (strategy.promotionPush) target.fitScore += player.skill * 2 + max(0, player.currentForm - 55);
    if (strategy.needsStarter && player.skill >= buyer.getAverageSkill()) target.fitScore += 12;
    if (strategy.trustYouthCover && player.age <= 21) target.fitScore += 8;
    if (target.contractRunningOut) target.fitScore += 8;
    if (target.urgentNeed) target.fitScore += 10;
    if (target.onShortlist) target.fitScore += 6;

    const long long totalCost = target.expectedFee + target.expectedWage * 8;
    if (totalCost <= strategy.maxTransferBudget) target.affordabilityScore = 24;
    else if (target.availableForLoan && target.expectedWage <= strategy.maxWageBudget) target.affordabilityScore = 16;
    else if (target.expectedFee <= strategy.maxTransferBudget * 12 / 10) target.affordabilityScore = 8;
    else target.affordabilityScore = -12;

    target.competitionScore = clampInt(max(0, player.potential - buyer.getAverageSkill()) / 4 +
                                           max(0, teamPrestigeScore(seller) - teamPrestigeScore(buyer) + 4),
                                       0, 28);
    target.scoutingConfidence = target.onShortlist ? 88 : target.contractRunningOut ? 74 : 58;
    target.scoutingNote = target.onShortlist
                              ? "seguimiento activo"
                              : target.contractRunningOut ? "ventana contractual"
                                                          : target.availableForLoan ? "opcion de prestamo"
                                                                                    : "perfil general";

    string reason;
    if (playerRejectsMove(career, buyer, seller, player, NegotiationPromise::Rotation, reason)) {
        target.affordabilityScore -= 18;
    }

    target.totalScore = target.fitScore * 0.55 + target.potentialScore * 0.18 +
                        target.affordabilityScore * 2.1 +
                        target.scoutingConfidence * 0.08 -
                        target.competitionScore * 0.22 +
                        (target.availableForLoan ? 6.0 : 0.0) +
                        (target.contractRunningOut ? 4.0 : 0.0) +
                        (target.onShortlist ? 5.0 : 0.0);
    return target;
}

}  // namespace ai_transfer_manager
