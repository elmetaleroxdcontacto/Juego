#include "transfers/transfer_market.h"

#include "ai/ai_squad_planner.h"
#include "ai/ai_transfer_manager.h"
#include "career/team_management.h"
#include "competition/competition.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>

using namespace std;

namespace {

int playerIndexByName(const Team& team, const string& name) {
    for (size_t i = 0; i < team.players.size(); ++i) {
        if (team.players[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

const TransferTarget* selectPreferredTarget(const vector<TransferTarget>& shortlist) {
    if (shortlist.empty()) return nullptr;
    const size_t limit = min<size_t>(3, shortlist.size());
    double total = 0.0;
    for (size_t i = 0; i < limit; ++i) {
        total += max(1.0, shortlist[i].totalScore);
    }
    double pick = rand01() * total;
    for (size_t i = 0; i < limit; ++i) {
        pick -= max(1.0, shortlist[i].totalScore);
        if (pick <= 0.0) return &shortlist[i];
    }
    return &shortlist.front();
}

int findSaleCandidateIndex(const Team& team, const vector<string>& candidateNames) {
    for (const string& name : candidateNames) {
        int idx = playerIndexByName(team, name);
        if (idx >= 0) return idx;
    }
    return -1;
}

long long totalDealCost(const NegotiationState& state) {
    return state.agreedFee + state.agreedBonus + state.agreedAgentFee + state.agreedLoyaltyBonus;
}

void adjustCpuSquadTactics(Team& team, const SquadNeedReport& squad, const ClubTransferStrategy& strategy) {
    int avgFitness = 0;
    int avgDiscipline = 0;
    int avgAdaptation = 0;
    for (const auto& player : team.players) {
        avgFitness += player.fitness;
        avgDiscipline += player.discipline > 0 ? player.discipline : player.tacticalDiscipline;
        avgAdaptation += player.adaptation;
    }
    const int count = max(1, static_cast<int>(team.players.size()));
    avgFitness /= count;
    avgDiscipline /= count;
    avgAdaptation /= count;

    if (strategy.needsLiquidity || team.debt > team.sponsorWeekly * 24) {
        team.transferPolicy = "Vender antes de comprar";
    }
    if (strategy.youthFocus && team.transferPolicy.empty()) {
        team.transferPolicy = "Cantera y valor futuro";
    }

    if (squad.rotationRisk >= 6 || avgFitness < 62) {
        team.tactics = "Defensive";
        team.matchInstruction = "Bloque bajo";
        team.pressingIntensity = max(1, team.pressingIntensity - 1);
        team.tempo = max(1, team.tempo - 1);
    } else if (avgDiscipline >= 68 && avgFitness >= 70) {
        team.tactics = "Pressing";
        team.matchInstruction = "Contra-presion";
        team.pressingIntensity = min(5, team.pressingIntensity + 1);
    } else if (strategy.needsStarter && avgAdaptation >= 60) {
        team.tactics = "Offensive";
        team.matchInstruction = "Por bandas";
        team.tempo = min(5, team.tempo + 1);
    } else if (strategy.needsLiquidity) {
        team.tactics = "Counter";
        team.matchInstruction = "Juego directo";
    } else {
        team.tactics = "Balanced";
        if (team.matchInstruction.empty()) team.matchInstruction = "Equilibrado";
    }
}

}  // namespace

namespace transfer_market {

bool isTransferWindowOpen(const Career& career) {
    if (career.currentWeek < 1) return true;
    const int seasonWeeks = static_cast<int>(career.schedule.size());
    if (seasonWeeks <= 0) return true;

    const int openingEnd = min(4, seasonWeeks);
    if (career.currentWeek <= openingEnd) return true;
    if (seasonWeeks < 10) return false;

    const int midStart = max(openingEnd + 1, seasonWeeks / 2);
    const int midEnd = min(seasonWeeks, midStart + 2);
    return career.currentWeek >= midStart && career.currentWeek <= midEnd;
}

int nextTransferWindowWeek(const Career& career) {
    const int seasonWeeks = static_cast<int>(career.schedule.size());
    if (seasonWeeks <= 0) return max(1, career.currentWeek);
    if (isTransferWindowOpen(career)) return max(1, career.currentWeek);

    const int openingEnd = min(4, seasonWeeks);
    if (career.currentWeek < 1) return 1;
    if (career.currentWeek <= openingEnd) return career.currentWeek;
    if (seasonWeeks >= 10) {
        const int midStart = max(openingEnd + 1, seasonWeeks / 2);
        if (career.currentWeek < midStart) return midStart;
    }
    return 1;
}

string transferWindowLabel(const Career& career) {
    const int seasonWeeks = static_cast<int>(career.schedule.size());
    if (seasonWeeks <= 0) return "Mercado abierto: calendario aun sin fijar.";
    if (isTransferWindowOpen(career)) {
        return "Mercado abierto: puedes cerrar fichajes, clausulas y cesiones inmediatas.";
    }

    const int nextWeek = nextTransferWindowWeek(career);
    if (nextWeek == 1 && career.currentWeek > 1) {
        return "Mercado cerrado: prepara shortlist y precontratos hasta la proxima temporada.";
    }
    return "Mercado cerrado: proxima ventana en semana " + to_string(nextWeek) +
           ". Puedes seguir scouteando, renovar o firmar precontratos elegibles.";
}

vector<string> buildTransferWindowLines(const Career& career, size_t limit) {
    vector<string> lines;
    if (limit == 0) return lines;
    lines.push_back(transferWindowLabel(career));
    if (lines.size() >= limit) return lines;

    if (isTransferWindowOpen(career)) {
        lines.push_back("Prioridad: cerrar solo operaciones que mejoren una necesidad real del XI o la rotacion.");
    } else {
        lines.push_back("Prioridad: crear informes, limpiar salarios y dejar listas las ofertas para la siguiente apertura.");
    }
    if (lines.size() >= limit) return lines;

    const int seasonWeeks = static_cast<int>(career.schedule.size());
    if (seasonWeeks >= 10) {
        const int midStart = max(min(4, seasonWeeks) + 1, seasonWeeks / 2);
        lines.push_back("Calendario: ventana inicial semanas 1-4; ventana media semanas " +
                        to_string(midStart) + "-" + to_string(min(seasonWeeks, midStart + 2)) + ".");
    } else {
        lines.push_back("Calendario: ventana inicial semanas 1-" + to_string(min(4, max(1, seasonWeeks))) + ".");
    }
    if (lines.size() > limit) lines.resize(limit);
    return lines;
}

string weakestSquadPosition(const Team& team) {
    return ai_squad_planner::analyzeSquad(team).weakestPosition;
}

vector<TransferTarget> buildTransferShortlist(const Career& career, const Team& team, size_t maxTargets) {
    vector<TransferTarget> shortlist;
    const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, team);

    for (const auto& club : career.allTeams) {
        if (&club == &team || club.players.size() <= 18) continue;
        for (const auto& player : club.players) {
            if (player.onLoan) continue;
            const string normalizedPos = normalizePosition(player.position);
            if (!strategy.priorityPositions.empty() &&
                find(strategy.priorityPositions.begin(), strategy.priorityPositions.end(), normalizedPos) == strategy.priorityPositions.end()) {
                continue;
            }
            TransferTarget target = ai_transfer_manager::evaluateTarget(career, team, club, player, strategy);
            if (target.affordabilityScore < 0 && !target.availableForLoan) continue;
            shortlist.push_back(target);
        }
    }

    sort(shortlist.begin(), shortlist.end(), [](const TransferTarget& left, const TransferTarget& right) {
        if (left.onShortlist != right.onShortlist) return left.onShortlist > right.onShortlist;
        if (left.urgentNeed != right.urgentNeed) return left.urgentNeed > right.urgentNeed;
        if (left.totalScore != right.totalScore) return left.totalScore > right.totalScore;
        if (left.expectedFee != right.expectedFee) return left.expectedFee < right.expectedFee;
        return left.playerName < right.playerName;
    });

    if (shortlist.size() > maxTargets) shortlist.resize(maxTargets);
    return shortlist;
}

void processCpuTransfers(Career& career) {
    const bool windowOpen = isTransferWindowOpen(career);
    for (auto& teamRef : career.allTeams) {
        Team* team = &teamRef;
        if (team == career.myTeam) continue;
        ensureTeamIdentity(*team);
        if (randInt(1, 100) > 18) continue;

        const SquadNeedReport squad = ai_squad_planner::analyzeSquad(*team);
        const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, *team);
        const int maxSquad = getCompetitionConfig(team->division).maxSquadSize;
        adjustCpuSquadTactics(*team, squad, strategy);

        if (!windowOpen) continue;

        if (team->players.size() > 18 &&
            ((maxSquad > 0 && static_cast<int>(team->players.size()) > maxSquad) ||
             strategy.needsLiquidity || strategy.salePressure >= 2)) {
            int sellIdx = findSaleCandidateIndex(*team, strategy.saleCandidates);
            int sellScore = -100000;
            if (sellIdx >= 0) {
                const Player& current = team->players[static_cast<size_t>(sellIdx)];
                sellScore = static_cast<int>(current.value / 1000) + current.age * 3 + (current.wantsToLeave ? 25 : 0);
                if (current.age >= 24 && current.startsThisSeason == 0 &&
                    current.matchesPlayed <= 1 && current.potential <= current.skill + 2) {
                    sellScore += 18;
                }
            }
            for (size_t i = 0; i < team->players.size(); ++i) {
                const Player& current = team->players[i];
                const bool wrongProfile = normalizePosition(current.position) == squad.surplusPosition || current.wantsToLeave;
                const bool expendableByUsage = current.age >= 24 && current.startsThisSeason == 0 &&
                                               current.matchesPlayed <= 1 && current.potential <= current.skill + 2;
                if (!wrongProfile && !expendableByUsage) continue;
                int score = static_cast<int>(current.value / 1000) + current.age * 3 + (current.wantsToLeave ? 25 : 0);
                if (expendableByUsage) score += 18;
                if (score > sellScore) {
                    sellScore = score;
                    sellIdx = static_cast<int>(i);
                }
            }
            if (sellIdx >= 0) {
                const string soldName = team->players[static_cast<size_t>(sellIdx)].name;
                const long long saleValue = team->players[static_cast<size_t>(sellIdx)].value;
                team_mgmt::detachPlayerFromSelections(*team, team->players[static_cast<size_t>(sellIdx)].name);
                team->players.erase(team->players.begin() + sellIdx);
                team->budget += saleValue;
                if (saleValue >= 250000 || strategy.needsLiquidity) {
                    career.addNews("[Mercado IA] " + team->name + " vende a " + soldName +
                                   " por $" + to_string(saleValue) + ".");
                }
            }
        }

        if (team->players.size() >= 24) continue;

        const vector<TransferTarget> shortlist = buildTransferShortlist(career, *team, static_cast<size_t>(strategy.maxTargets));
        if (!shortlist.empty()) {
            const TransferTarget* preferred = selectPreferredTarget(shortlist);
            if (!preferred) continue;
            const TransferTarget& top = *preferred;
            Team* seller = career.findTeamByName(top.clubName);
            if (!seller) continue;
            int sellerIdx = playerIndexByName(*seller, top.playerName);
            if (sellerIdx < 0 || sellerIdx >= static_cast<int>(seller->players.size())) continue;

            Player newPlayer = seller->players[static_cast<size_t>(sellerIdx)];
            if (top.availableForLoan && top.expectedFee + top.expectedWage * 8 > team->budget) {
                const long long loanFee = max(12000LL, newPlayer.value / 10);
                if (team->budget < loanFee + top.expectedWage * 8) continue;
                team->budget -= loanFee;
                seller->budget += loanFee;
                newPlayer.onLoan = true;
                newPlayer.parentClub = seller->name;
                newPlayer.loanWeeksRemaining = randInt(26, 52);
                newPlayer.wage = max(newPlayer.wage / 2, top.expectedWage * 55 / 100);
            } else {
                const NegotiationProfile profile = strategy.promotionPush ? NegotiationProfile::Safe
                                                  : strategy.needsLiquidity ? NegotiationProfile::Balanced
                                                                           : NegotiationProfile::Aggressive;
                const NegotiationPromise promise = strategy.youthFocus && newPlayer.age <= 21
                                                       ? NegotiationPromise::Prospect
                                                       : strategy.promotionPush
                                                             ? NegotiationPromise::Starter
                                                             : NegotiationPromise::Rotation;
                const NegotiationState negotiation =
                    runTransferNegotiation(career, *team, *seller, newPlayer, profile, promise);
                if (!negotiation.clubAccepted || !negotiation.playerAccepted) continue;
                if (team->budget < totalDealCost(negotiation)) continue;

                team->budget -= totalDealCost(negotiation);
                seller->budget += negotiation.agreedFee;
                newPlayer.onLoan = false;
                newPlayer.parentClub.clear();
                newPlayer.loanWeeksRemaining = 0;
                newPlayer.wage = negotiation.agreedWage;
                newPlayer.contractWeeks = negotiation.agreedContractWeeks;
                newPlayer.releaseClause = negotiation.agreedClause;
                applyNegotiatedPromise(newPlayer, promise);
            }
            newPlayer.wantsToLeave = false;
            team->addPlayer(newPlayer);
            team_mgmt::detachPlayerFromSelections(*seller, newPlayer.name);
            seller->players.erase(seller->players.begin() + sellerIdx);
            if (top.expectedFee >= 250000 || top.urgentNeed || top.availableForLoan) {
                career.addNews("[Mercado IA] " + team->name + " incorpora a " + newPlayer.name +
                               " desde " + seller->name + (newPlayer.onLoan ? " a prestamo." : "."));
            }
            if (!newPlayer.onLoan && top.expectedFee >= 300000) {
                career.historicalRecords.push_back({
                    "Fichaje importante T" + to_string(career.currentSeason) + " - " + newPlayer.name,
                    newPlayer.name,
                    team->name,
                    career.currentSeason,
                    static_cast<int>(min<long long>(top.expectedFee, 2000000000LL)),
                    seller->name + " -> " + team->name
                });
                if (career.historicalRecords.size() > 60) {
                    career.historicalRecords.erase(career.historicalRecords.begin(),
                                                   career.historicalRecords.begin() +
                                                       static_cast<long long>(career.historicalRecords.size() - 60));
                }
            }
        } else if (!strategy.needsLiquidity) {
            int minSkill, maxSkill;
            getDivisionSkillRange(team->division, minSkill, maxSkill);
            const string targetPosition = strategy.weakestPosition.empty() ? squad.weakestPosition : strategy.weakestPosition;
            if (strategy.youthFocus || strategy.trustYouthCover || squad.rotationRisk >= 5) {
                Player promoted = makeRandomPlayer(targetPosition, max(minSkill - 2, 38), maxSkill, 17, 20);
                promoted.potential = clampInt(promoted.skill + randInt(6, 14), promoted.skill, 95);
                promoted.contractWeeks = 156;
                promoted.wage = max(2500LL, promoted.wage / 2);
                promoted.releaseClause = max(promoted.value * 3, 120000LL);
                team->addPlayer(promoted);
                career.addNews("[Cantera IA] " + team->name + " promueve a " + promoted.name +
                               " para cubrir " + targetPosition + ".");
                continue;
            }

            Player generated = makeRandomPlayer(targetPosition, minSkill, maxSkill, 18, 29);
            const long long fee = max(generated.value, generated.releaseClause * 55 / 100);
            if (team->budget >= fee) {
                team->budget -= fee;
                generated.releaseClause = max(generated.value * 2, fee * 2);
                team->addPlayer(generated);
            } else if (team->players.size() < 21) {
                generated.value = max(10000LL, generated.value / 3);
                generated.wage = max(1500LL, generated.wage * 2 / 3);
                generated.releaseClause = max(50000LL, generated.value * 2);
                generated.contractWeeks = randInt(26, 78);
                team->addPlayer(generated);
                career.addNews("[Mercado IA] " + team->name + " suma agente libre: " + generated.name + ".");
            }
        }
    }
}

void processLoanReturns(Career& career) {
    for (auto& team : career.allTeams) {
        for (size_t i = 0; i < team.players.size();) {
            Player& player = team.players[i];
            if (!player.onLoan) {
                ++i;
                continue;
            }
            if (player.loanWeeksRemaining > 0) player.loanWeeksRemaining--;
            if (player.loanWeeksRemaining > 0) {
                ++i;
                continue;
            }
            Team* parent = career.findTeamByName(player.parentClub);
            if (!parent || parent == &team) {
                player.onLoan = false;
                player.parentClub.clear();
                ++i;
                continue;
            }
            Player returning = player;
            returning.onLoan = false;
            returning.parentClub.clear();
            returning.loanWeeksRemaining = 0;
            parent->addPlayer(returning);
            career.addNews(returning.name + " regresa desde prestamo a " + parent->name + ".");
            team.players.erase(team.players.begin() + static_cast<long long>(i));
        }
    }
}

}  // namespace transfer_market
