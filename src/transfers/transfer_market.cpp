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

}  // namespace

namespace transfer_market {

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
        if (left.totalScore != right.totalScore) return left.totalScore > right.totalScore;
        if (left.expectedFee != right.expectedFee) return left.expectedFee < right.expectedFee;
        return left.playerName < right.playerName;
    });

    if (shortlist.size() > maxTargets) shortlist.resize(maxTargets);
    return shortlist;
}

void processCpuTransfers(Career& career) {
    for (auto& teamRef : career.allTeams) {
        Team* team = &teamRef;
        if (team == career.myTeam) continue;
        ensureTeamIdentity(*team);
        if (randInt(1, 100) > 18) continue;

        const SquadNeedReport squad = ai_squad_planner::analyzeSquad(*team);
        const ClubTransferStrategy strategy = ai_transfer_manager::buildClubTransferStrategy(career, *team);
        const int maxSquad = getCompetitionConfig(team->division).maxSquadSize;

        if ((maxSquad > 0 && static_cast<int>(team->players.size()) > maxSquad) || strategy.needsLiquidity) {
            int sellIdx = -1;
            int sellScore = -100000;
            for (size_t i = 0; i < team->players.size(); ++i) {
                const Player& current = team->players[i];
                const bool wrongProfile = normalizePosition(current.position) == squad.surplusPosition || current.wantsToLeave;
                if (!wrongProfile) continue;
                int score = static_cast<int>(current.value / 1000) + current.age * 3 + (current.wantsToLeave ? 25 : 0);
                if (score > sellScore) {
                    sellScore = score;
                    sellIdx = static_cast<int>(i);
                }
            }
            if (sellIdx >= 0) {
                const long long saleValue = team->players[static_cast<size_t>(sellIdx)].value;
                team_mgmt::detachPlayerFromSelections(*team, team->players[static_cast<size_t>(sellIdx)].name);
                team->players.erase(team->players.begin() + sellIdx);
                team->budget += saleValue;
            }
        }

        if (team->players.size() >= 24) continue;

        const vector<TransferTarget> shortlist = buildTransferShortlist(career, *team, static_cast<size_t>(strategy.maxTargets));
        if (!shortlist.empty()) {
            const TransferTarget& top = shortlist.front();
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
                if (top.expectedFee > team->budget) continue;
                team->budget -= top.expectedFee;
                seller->budget += top.expectedFee;
                newPlayer.onLoan = false;
                newPlayer.parentClub.clear();
                newPlayer.loanWeeksRemaining = 0;
                newPlayer.releaseClause = max(newPlayer.value * 2, top.expectedFee * 2);
            }
            newPlayer.wantsToLeave = false;
            team->addPlayer(newPlayer);
            team_mgmt::detachPlayerFromSelections(*seller, newPlayer.name);
            seller->players.erase(seller->players.begin() + sellerIdx);
        } else if (!strategy.needsLiquidity) {
            int minSkill, maxSkill;
            getDivisionSkillRange(team->division, minSkill, maxSkill);
            Player generated = makeRandomPlayer(strategy.weakestPosition.empty() ? squad.weakestPosition : strategy.weakestPosition,
                                                minSkill, maxSkill, 18, 29);
            const long long fee = max(generated.value, generated.releaseClause * 55 / 100);
            if (team->budget >= fee) {
                team->budget -= fee;
                generated.releaseClause = max(generated.value * 2, fee * 2);
                team->addPlayer(generated);
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
