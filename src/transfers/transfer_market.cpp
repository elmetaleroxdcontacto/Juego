#include "transfers/transfer_market.h"

#include "competition/competition.h"
#include "career/team_management.h"
#include "transfers/negotiation_system.h"
#include "utils/utils.h"

#include <algorithm>
#include <unordered_map>

using namespace std;

namespace {

int squadNeedScore(const Team& team, const string& pos) {
    int count = 0;
    int skill = 0;
    int oldPlayers = 0;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != pos) continue;
        count++;
        skill += player.skill;
        if (player.age >= 30) oldPlayers++;
    }
    int avgSkill = count > 0 ? skill / count : 0;
    return 100 - (count * 16 + avgSkill + oldPlayers * 3);
}

string surplusSquadPosition(const Team& team) {
    static const vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    string surplus = "MED";
    int bestScore = -100000;
    for (const auto& pos : positions) {
        int score = -squadNeedScore(team, pos);
        if (score > bestScore) {
            bestScore = score;
            surplus = pos;
        }
    }
    return surplus;
}

}  // namespace

namespace transfer_market {

string weakestSquadPosition(const Team& team) {
    static const vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    string weakest = "MED";
    int weakestScore = -1000000;
    for (const auto& pos : positions) {
        int score = squadNeedScore(team, pos);
        if (score > weakestScore) {
            weakestScore = score;
            weakest = pos;
        }
    }
    return weakest;
}

void processCpuTransfers(Career& career) {
    for (auto& teamRef : career.allTeams) {
        Team* team = &teamRef;
        if (team == career.myTeam) continue;
        ensureTeamIdentity(*team);
        if (randInt(1, 100) > 18) continue;

        int maxSquad = getCompetitionConfig(team->division).maxSquadSize;
        if ((maxSquad > 0 && static_cast<int>(team->players.size()) > maxSquad) ||
            (team->budget < 120000 && team->players.size() > 18)) {
            string surplus = surplusSquadPosition(*team);
            int sellIdx = -1;
            int sellScore = -100000;
            for (size_t i = 0; i < team->players.size(); ++i) {
                const Player& current = team->players[i];
                if (normalizePosition(current.position) != surplus && !current.wantsToLeave) continue;
                int score = current.value / 1000 + current.age * 3 + (current.wantsToLeave ? 25 : 0);
                if (score > sellScore) {
                    sellIdx = static_cast<int>(i);
                    sellScore = score;
                }
            }
            if (sellIdx >= 0) {
                long long saleValue = team->players[sellIdx].value;
                team_mgmt::detachPlayerFromSelections(*team, team->players[sellIdx].name);
                team->players.erase(team->players.begin() + sellIdx);
                team->budget += saleValue;
            }
        }

        if (team->players.size() >= 24) continue;
        string needPos = weakestSquadPosition(*team);
        bool lowBudget = team->budget < 150000;
        Team* seller = nullptr;
        int sellerIdx = -1;
        bool loanMove = false;
        int bestScore = -100000;

        for (auto& club : career.allTeams) {
            if (&club == team || &club == career.myTeam) continue;
            ensureTeamIdentity(club);
            if (club.players.size() <= 18) continue;
            for (size_t i = 0; i < club.players.size(); ++i) {
                const Player& target = club.players[i];
                if (target.onLoan) continue;
                if (normalizePosition(target.position) != needPos) continue;

                string reason;
                if (playerRejectsMove(career, *team, club, target, NegotiationPromise::Rotation, reason)) continue;

                long long fee = max(target.value, target.releaseClause * 55 / 100);
                fee += rivalrySurcharge(*team, club, fee);
                bool affordableTransfer = fee <= team->budget * 65 / 100;
                bool affordableLoan = lowBudget && club.players.size() >= 22 &&
                                      target.age <= 24 && target.contractWeeks > 18 &&
                                      target.skill >= team->getAverageSkill() - 4;
                if (!affordableTransfer && !affordableLoan) continue;

                int score = target.skill * 3 + target.potential - target.age * 2;
                score += squadNeedScore(*team, needPos);
                if (target.contractWeeks <= 20) score += 6;
                if (affordableLoan) score += 4;
                if (club.players.size() >= 24) score += 4;
                if (score > bestScore) {
                    bestScore = score;
                    seller = &club;
                    sellerIdx = static_cast<int>(i);
                    loanMove = !affordableTransfer && affordableLoan;
                }
            }
        }

        if (seller && sellerIdx >= 0) {
            Player newP = seller->players[static_cast<size_t>(sellerIdx)];
            if (loanMove) {
                long long loanFee = max(12000LL, newP.value / 10);
                long long wageShare = max(newP.wage / 2, wageDemandFor(newP) * 55 / 100);
                if (team->budget < loanFee + wageShare * 8) continue;
                team->budget -= loanFee;
                seller->budget += loanFee;
                newP.onLoan = true;
                newP.parentClub = seller->name;
                newP.loanWeeksRemaining = randInt(26, 52);
                newP.wantsToLeave = false;
                team->addPlayer(newP);
            } else {
                long long fee = max(newP.value, newP.releaseClause * 55 / 100);
                fee += rivalrySurcharge(*team, *seller, fee);
                if (fee > team->budget) continue;
                team->budget -= fee;
                seller->budget += fee;
                newP.releaseClause = max(newP.value * 2, fee * 2);
                newP.wantsToLeave = false;
                newP.onLoan = false;
                newP.parentClub.clear();
                newP.loanWeeksRemaining = 0;
                team->addPlayer(newP);
            }
            team_mgmt::detachPlayerFromSelections(*seller, newP.name);
            seller->players.erase(seller->players.begin() + sellerIdx);
        } else if (!lowBudget) {
            int minSkill, maxSkill;
            getDivisionSkillRange(team->division, minSkill, maxSkill);
            Player newP = makeRandomPlayer(needPos, minSkill, maxSkill, 18, 29);
            long long fee = max(newP.value, newP.releaseClause * 55 / 100);
            if (team->budget >= fee) {
                team->budget -= fee;
                newP.releaseClause = max(newP.value * 2, fee * 2);
                team->addPlayer(newP);
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
