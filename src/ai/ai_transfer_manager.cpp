#include "ai/ai_transfer_manager.h"

#include "finance/finance_system.h"
#include "simulation/player_condition.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

bool onScoutingShortlist(const Career& career, const Team& seller, const Player& player) {
    const string entry = seller.name + "|" + player.name;
    return find(career.scoutingShortlist.begin(), career.scoutingShortlist.end(), entry) != career.scoutingShortlist.end();
}

int assignmentKnowledgeBoost(const Career& career, const Team& seller, const Player& player) {
    int boost = 0;
    const string normalizedPos = normalizePosition(player.position);
    for (const auto& assignment : career.scoutingAssignments) {
        if (!assignment.region.empty() && assignment.region != "Todas" && assignment.region != seller.youthRegion) continue;
        if (!assignment.focusPosition.empty() && normalizePosition(assignment.focusPosition) != normalizedPos) continue;
        boost = max(boost, assignment.knowledgeLevel / 4 + max(0, assignment.weeksRemaining - 1) * 2);
    }
    return clampInt(boost, 0, 22);
}

int projectStyleFit(const Team& buyer, const Player& player) {
    int fit = 0;
    if (buyer.headCoachStyle == "Intensidad" && playerHasTrait(player, "Presiona")) fit += 8;
    if (buyer.headCoachStyle == "Transicion" && normalizePosition(player.position) == "DEL") fit += 5;
    if (buyer.clubStyle == "Ataque por bandas" && (normalizePosition(player.position) == "DEF" || normalizePosition(player.position) == "DEL")) fit += 4;
    if (buyer.clubStyle == "Bloque ordenado" && normalizePosition(player.position) == "DEF") fit += 5;
    if (buyer.youthIdentity == "Cantera estructurada" && player.age <= 21) fit += 8;
    if (buyer.transferPolicy == "Cantera y valor futuro" && player.age <= 22) fit += 6;
    if (buyer.transferPolicy == "Competir por titulares hechos" && player.skill >= buyer.getAverageSkill()) fit += 5;
    return fit;
}

}  // namespace

namespace ai_transfer_manager {

ClubTransferStrategy buildClubTransferStrategy(const Career& career, const Team& team) {
    ClubTransferStrategy strategy;
    const SquadNeedReport squad = ai_squad_planner::analyzeSquad(team);
    strategy.weakestPosition = squad.weakestPosition;
    strategy.surplusPosition = squad.surplusPosition;
    strategy.needsLiquidity = team.budget < 120000 || team.debt > team.sponsorWeekly * 18;
    strategy.youthFocus = team.youthFacilityLevel >= 3 || team.youthCoach >= 75 || team.youthIdentity == "Cantera estructurada" ||
                         team.transferPolicy == "Cantera y valor futuro";
    strategy.promotionPush = teamPrestigeScore(team) >= 58 || career.currentSeason <= 2 || team.headCoachStyle == "Presion";
    strategy.needsStarter = squad.rotationRisk >= 5 || !squad.thinPositions.empty();
    strategy.trustYouthCover = strategy.youthFocus && !squad.youthCoverPositions.empty();
    strategy.rotationRisk = squad.rotationRisk;
    strategy.salePressure = squad.salePressure;
    strategy.maxTargets = strategy.promotionPush ? 6 : 4;
    if (team.transferPolicy == "Vender antes de comprar") strategy.maxTargets = min(strategy.maxTargets, 3);
    strategy.maxTransferBudget = finance_system::calculateTransferBuffer(team);
    strategy.maxWageBudget = max(20000LL, finance_system::calculateWeeklyPayroll(team) / 4);
    strategy.averageStarterSkill = team.getAverageSkill();
    strategy.prestigeScore = teamPrestigeScore(team);
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
    target.expectedAgentFee = estimatedAgentFee(player, target.expectedFee);
    target.contractRunningOut = player.contractWeeks <= 20;
    target.availableForLoan = player.age <= 24 && player.contractWeeks > 18;
    target.squadNeedScore = ai_squad_planner::positionNeedScore(buyer, normalizePosition(player.position));
    target.onShortlist = onScoutingShortlist(career, seller, player);
    target.urgentNeed = find(strategy.thinPositions.begin(), strategy.thinPositions.end(), normalizePosition(player.position)) != strategy.thinPositions.end();
    target.readinessScore = player_condition::readinessScore(player, seller);
    target.medicalRisk = max(player_condition::workloadRisk(player, seller),
                             player_condition::relapseRisk(player, seller));
    target.upsideScore = max(0, player.potential - player.skill) +
                         player_condition::developmentStability(player, seller, false) / 5;
    target.fitScore = player.skill * 3 + player.potential * 2 - player.age * 2 + target.squadNeedScore;
    const int assignmentBoost = assignmentKnowledgeBoost(career, seller, player);
    const int styleFit = projectStyleFit(buyer, player);
    if (strategy.youthFocus) target.fitScore += max(0, 24 - player.age) * 3 + max(0, player.potential - player.skill) * 2;
    if (strategy.promotionPush) target.fitScore += player.skill * 2 + max(0, player.currentForm - 55);
    if (strategy.needsStarter && player.skill >= strategy.averageStarterSkill) target.fitScore += 12;
    if (strategy.trustYouthCover && player.age <= 21) target.fitScore += 8;
    if (target.contractRunningOut) target.fitScore += 8;
    if (target.urgentNeed) target.fitScore += 10;
    if (target.onShortlist) target.fitScore += 6;
    target.fitScore += styleFit;
    target.fitScore += target.readinessScore / 6;
    target.fitScore += target.upsideScore / 7;
    target.fitScore -= target.medicalRisk / 7;

    const long long loyaltyExpectation = max(15000LL, target.expectedWage * 2);
    const long long totalCost = target.expectedFee + target.expectedAgentFee + loyaltyExpectation + target.expectedWage * 8;
    if (totalCost <= strategy.maxTransferBudget) target.affordabilityScore = 24;
    else if (target.availableForLoan && target.expectedWage <= strategy.maxWageBudget) target.affordabilityScore = 16;
    else if (target.expectedFee + target.expectedAgentFee <= strategy.maxTransferBudget * 12 / 10) target.affordabilityScore = 8;
    else target.affordabilityScore = -12;
    if (target.medicalRisk >= 72) target.affordabilityScore -= 8;
    else if (target.medicalRisk >= 56) target.affordabilityScore -= 3;

    target.competitionScore = clampInt(max(0, player.potential - strategy.averageStarterSkill) / 4 +
                                           max(0, teamPrestigeScore(seller) - strategy.prestigeScore + 4),
                                       0, 28);
    target.scoutingConfidence = clampInt(36 + buyer.scoutingChief / 2 + (target.onShortlist ? 14 : 0) +
                                             (target.contractRunningOut ? 8 : 0) + (target.availableForLoan ? 5 : 0) +
                                             assignmentBoost,
                                         35, 92);
    target.scoutingNote = target.onShortlist
                              ? "seguimiento activo"
                              : target.contractRunningOut ? "ventana contractual"
                                                          : target.availableForLoan ? "opcion de prestamo"
                                                                                    : "perfil general";
    if (assignmentBoost > 0) {
        target.scoutingNote += " | cobertura especifica";
    }
    if (target.expectedAgentFee >= max(12000LL, target.expectedFee / 5)) {
        target.scoutingNote += " | agente exigente";
    }
    if (player.ambition >= 75) {
        target.scoutingNote += " | jugador ambicioso";
    }
    if (buyer.transferPolicy == "Cantera y valor futuro" && player.age <= 21) {
        target.scoutingNote += " | encaja con politica juvenil";
    } else if (buyer.transferPolicy == "Vender antes de comprar") {
        target.scoutingNote += " | el club vigila mucho el coste total";
    }
    if (styleFit >= 8) {
        target.scoutingNote += " | encaje fuerte con el proyecto";
    } else if (styleFit >= 4) {
        target.scoutingNote += " | encaje tactico razonable";
    }
    if (target.medicalRisk >= 72) {
        target.scoutingNote += " | riesgo fisico alto";
    } else if (target.medicalRisk >= 56) {
        target.scoutingNote += " | requiere control medico";
    }
    if (target.readinessScore >= 68) {
        target.scoutingNote += " | listo para competir";
    } else if (target.readinessScore <= 48) {
        target.scoutingNote += " | necesita puesta a punto";
    }
    if (buyer.headCoachStyle == "Presion" && playerHasTrait(player, "Presiona")) target.fitScore += 6;

    string reason;
    if (playerRejectsMove(career, buyer, seller, player, NegotiationPromise::Rotation, reason)) {
        target.affordabilityScore -= 18;
    }

    target.totalScore = target.fitScore * 0.55 + target.potentialScore * 0.18 +
                        target.affordabilityScore * 2.1 + target.scoutingConfidence * 0.08 +
                        target.readinessScore * 0.10 - target.medicalRisk * 0.09 + target.upsideScore * 0.12 -
                        target.competitionScore * 0.22 + (target.availableForLoan ? 6.0 : 0.0) +
                        (target.contractRunningOut ? 4.0 : 0.0) + (target.onShortlist ? 5.0 : 0.0) + styleFit * 0.35 +
                        assignmentBoost * 0.18;
    return target;
}

}  // namespace ai_transfer_manager
