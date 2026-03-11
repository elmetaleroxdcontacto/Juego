#include "engine/models.h"

#include "competition/competition.h"
#include "io/io.h"
#include "utils/utils.h"
#include "validators/validators.h"

#include <algorithm>
#include <sstream>
#include <vector>

using namespace std;

namespace {

vector<vector<pair<int, int>>> buildRoundRobinSchedule(const vector<int>& teamIdx, bool doubleRound) {
    vector<vector<pair<int, int>>> out;
    if (teamIdx.size() < 2) return out;

    vector<int> idx = teamIdx;
    if (idx.size() % 2 == 1) idx.push_back(-1);

    int size = static_cast<int>(idx.size());
    int rounds = size - 1;
    for (int round = 0; round < rounds; ++round) {
        vector<pair<int, int>> matches;
        for (int i = 0; i < size / 2; ++i) {
            int a = idx[static_cast<size_t>(i)];
            int b = idx[static_cast<size_t>(size - 1 - i)];
            if (a == -1 || b == -1) continue;
            matches.push_back(round % 2 == 0 ? pair<int, int>{a, b} : pair<int, int>{b, a});
        }
        out.push_back(matches);
        int last = idx.back();
        for (int i = size - 1; i > 1; --i) idx[static_cast<size_t>(i)] = idx[static_cast<size_t>(i - 1)];
        idx[1] = last;
    }

    if (doubleRound) {
        int base = static_cast<int>(out.size());
        for (int i = 0; i < base; ++i) {
            vector<pair<int, int>> reverseMatches;
            for (const auto& match : out[static_cast<size_t>(i)]) {
                reverseMatches.push_back({match.second, match.first});
            }
            out.push_back(reverseMatches);
        }
    }
    return out;
}

LeagueTable buildBoardTable(const vector<Team*>& teams, const string& ruleId) {
    LeagueTable table;
    table.ruleId = ruleId;
    for (Team* team : teams) {
        if (team) table.addTeam(team);
    }
    table.sortTable();
    return table;
}

vector<Team*> boardRelevantTeams(const Career& career) {
    vector<Team*> teams;
    if (!career.myTeam) return teams;
    if (career.usesGroupFormat()) {
        const auto& idx = [&career]() -> const vector<int>& {
            for (int i : career.groupNorthIdx) {
                if (i >= 0 && i < static_cast<int>(career.activeTeams.size()) &&
                    career.activeTeams[static_cast<size_t>(i)] == career.myTeam) {
                    return career.groupNorthIdx;
                }
            }
            return career.groupSouthIdx;
        }();
        for (int i : idx) {
            if (i >= 0 && i < static_cast<int>(career.activeTeams.size())) {
                teams.push_back(career.activeTeams[static_cast<size_t>(i)]);
            }
        }
        if (!teams.empty()) return teams;
    }
    return career.activeTeams;
}

int youthPlayersUsed(const Team& team) {
    int count = 0;
    for (const auto& player : team.players) {
        if (player.age <= 20 && player.matchesPlayed > 0) count++;
    }
    return count;
}

bool promiseCurrentlyAtRisk(const Player& player, int currentWeek) {
    if (player.promisedRole == "Titular") {
        return player.startsThisSeason + 2 < max(2, currentWeek * 2 / 3);
    }
    if (player.promisedRole == "Rotacion") {
        return player.startsThisSeason + 1 < max(1, currentWeek / 3);
    }
    if (player.promisedRole == "Proyecto") {
        return player.age <= 22 && player.startsThisSeason < max(1, currentWeek / 4);
    }
    return false;
}

int promisesAtRiskForBoard(const Team& team, int currentWeek) {
    int total = 0;
    for (const auto& player : team.players) {
        if (promiseCurrentlyAtRisk(player, currentWeek)) total++;
    }
    return total;
}

}  // namespace

Career::Career()
    : myTeam(nullptr),
      currentSeason(1),
      currentWeek(1),
      saveFile("saves/career_save.txt"),
      managerName("Manager"),
      managerReputation(50),
      boardConfidence(60),
      boardExpectedFinish(0),
      boardBudgetTarget(0),
      boardYouthTarget(1),
      boardWarningWeeks(0),
      boardMonthlyTarget(0),
      boardMonthlyProgress(0),
      boardMonthlyDeadlineWeek(0),
      cupActive(false),
      cupRound(0),
      initialized(false) {}

bool Career::usesSegundaFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    return config.seasonHandler == CompetitionSeasonHandler::SegundaGroups &&
           (config.expectedTeamCount <= 0 || static_cast<int>(activeTeams.size()) == config.expectedTeamCount);
}

bool Career::usesTerceraBFormat() const {
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    return config.seasonHandler == CompetitionSeasonHandler::TerceraB &&
           (config.expectedTeamCount <= 0 || static_cast<int>(activeTeams.size()) == config.expectedTeamCount);
}

bool Career::usesGroupFormat() const {
    return competitionUsesGroupStage(activeDivision, static_cast<int>(activeTeams.size()));
}

void Career::buildSegundaGroups() {
    buildRegionalGroups();
}

void Career::buildRegionalGroups() {
    groupNorthIdx.clear();
    groupSouthIdx.clear();
    if (!usesGroupFormat()) return;
    const CompetitionConfig& config = getCompetitionConfig(activeDivision);
    int split = config.groups.groupSize;
    if (split <= 0) return;
    for (int i = 0; i < static_cast<int>(activeTeams.size()); ++i) {
        if (i < split) groupNorthIdx.push_back(i);
        else groupSouthIdx.push_back(i);
    }
}

void Career::initializeLeague(bool forceReload) {
    if (initialized && !forceReload) return;
    myTeam = nullptr;
    allTeams.clear();
    activeTeams.clear();
    schedule.clear();
    leagueTable.clear();
    groupNorthIdx.clear();
    groupSouthIdx.clear();
    divisions.clear();
    activeDivision.clear();
    loadWarnings.clear();

    reloadCompetitionConfigs();
    const vector<string>& configWarnings = competitionConfigWarnings();
    loadWarnings.insert(loadWarnings.end(), configWarnings.begin(), configWarnings.end());

    for (const auto& div : kDivisions) {
        if (!isDirectory(div.folder)) continue;
        DivisionLoadResult divisionLoad = loadDivisionFromFolder(div.folder, div.id, allTeams);
        loadWarnings.insert(loadWarnings.end(), divisionLoad.warnings.begin(), divisionLoad.warnings.end());
        if (!divisionLoad.teams.empty()) {
            divisions.push_back(div);
        }
    }
    for (auto& team : allTeams) ensureTeamIdentity(team);

    const StartupValidationSummary startupValidation = buildStartupValidationSummary(6);
    for (const string& line : startupValidation.lines) {
        if (line == "Auditoria automatica de datos externos") continue;
        loadWarnings.push_back(line);
    }

    const RuntimeValidationSummary validation = validateLoadedCareerData(*this, 10);
    for (const string& line : validation.lines) {
        if (line == "Validacion automatica de carga") continue;
        loadWarnings.push_back(line);
    }
    initialized = true;
}

vector<Team*> Career::getDivisionTeams(const string& id) {
    vector<Team*> out;
    for (auto& team : allTeams) {
        if (team.division == id) out.push_back(&team);
    }
    return out;
}

void Career::buildSchedule() {
    schedule.clear();
    int n = static_cast<int>(activeTeams.size());
    if (n < 2) return;

    if (usesGroupFormat()) {
        const CompetitionConfig& config = getCompetitionConfig(activeDivision);
        if (groupNorthIdx.empty() || groupSouthIdx.empty()) buildRegionalGroups();
        auto north = buildRoundRobinSchedule(groupNorthIdx, config.groups.doubleRoundRobin);
        auto south = buildRoundRobinSchedule(groupSouthIdx, config.groups.doubleRoundRobin);
        int rounds = static_cast<int>(max(north.size(), south.size()));
        for (int r = 0; r < rounds; ++r) {
            vector<pair<int, int>> matches;
            if (r < static_cast<int>(north.size())) {
                matches.insert(matches.end(), north[static_cast<size_t>(r)].begin(), north[static_cast<size_t>(r)].end());
            }
            if (r < static_cast<int>(south.size())) {
                matches.insert(matches.end(), south[static_cast<size_t>(r)].begin(), south[static_cast<size_t>(r)].end());
            }
            schedule.push_back(matches);
        }
        return;
    }

    vector<int> idx;
    idx.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) idx.push_back(i);
    schedule = buildRoundRobinSchedule(idx, true);
}

void Career::setActiveDivision(const string& id) {
    activeDivision = id;
    activeTeams = getDivisionTeams(id);
    leagueTable.clear();
    leagueTable.title = divisionDisplay(id);
    leagueTable.ruleId = id;
    for (Team* team : activeTeams) leagueTable.addTeam(team);
    leagueTable.sortTable();
    groupNorthIdx.clear();
    groupSouthIdx.clear();
    buildRegionalGroups();
    buildSchedule();
}

void Career::resetSeason() {
    activePromises.clear();
    for (auto& team : allTeams) {
        team.resetSeasonStats();
        team.morale = 50;
        for (auto& player : team.players) {
            player.injured = false;
            player.injuryWeeks = 0;
            player.injuryType.clear();
            player.fitness = player.stamina;
            player.yellowAccumulation = 0;
            player.seasonYellowCards = 0;
            player.seasonRedCards = 0;
            player.matchesSuspended = 0;
            player.goals = 0;
            player.assists = 0;
            player.matchesPlayed = 0;
            player.startsThisSeason = 0;
            player.wantsToLeave = false;
            player.moraleMomentum = 0;
            player.fatigueLoad = 0;
            player.unhappinessWeeks = 0;
            player.happiness = clampInt(player.happiness + 5, 1, 99);
            player.currentForm = clampInt(44 + player.professionalism / 4 + randInt(-8, 8), 20, 88);
            ensurePlayerProfile(player, false);
        }
    }
    buildSchedule();
    initializeBoardObjectives();
    initializeSeasonCup();
    initializeDynamicObjective();
    lastMatchAnalysis.clear();
    lastMatchReportLines.clear();
    lastMatchEvents.clear();
    lastMatchPlayerOfTheMatch.clear();
    lastMatchCenter = MatchCenterSnapshot{};
    scoutInbox.clear();
}

void Career::agePlayers() {
    for (auto& team : allTeams) {
        for (auto& player : team.players) {
            player.age++;
            if (player.age > 30) {
                player.skill = max(1, player.skill - 1);
                player.stamina = max(1, player.stamina - 1);
            }
            if (player.fitness > player.stamina) player.fitness = player.stamina;
            if (player.potential < player.skill) player.potential = player.skill;
            player.moraleMomentum = clampInt(player.moraleMomentum / 2, -25, 25);
            player.fatigueLoad = max(0, player.fatigueLoad - 8);
            player.unhappinessWeeks = 0;
            player.currentForm = clampInt((player.currentForm + 50) / 2 + randInt(-4, 4), 15, 90);
            ensurePlayerProfile(player, false);
        }
    }
}

Team* Career::findTeamByName(const string& name) {
    for (auto& team : allTeams) {
        if (team.name == name) return &team;
    }
    return nullptr;
}

const Team* Career::findTeamByName(const string& name) const {
    for (const auto& team : allTeams) {
        if (team.name == name) return &team;
    }
    return nullptr;
}

void Career::addNews(const string& item) {
    if (item.empty()) return;
    string entry = "T" + to_string(currentSeason) + "-F" + to_string(currentWeek) + ": " + item;
    newsFeed.push_back(entry);
    if (newsFeed.size() > 40) {
        newsFeed.erase(newsFeed.begin(), newsFeed.begin() + static_cast<long long>(newsFeed.size() - 40));
    }
}

void Career::executePendingTransfers() {
    for (size_t i = 0; i < pendingTransfers.size();) {
        PendingTransfer& move = pendingTransfers[i];
        if (move.effectiveSeason > currentSeason) {
            ++i;
            continue;
        }
        Team* from = findTeamByName(move.fromTeam);
        Team* to = findTeamByName(move.toTeam);
        if (!from || !to) {
            pendingTransfers.erase(pendingTransfers.begin() + static_cast<long long>(i));
            continue;
        }
        int playerIdx = -1;
        for (size_t p = 0; p < from->players.size(); ++p) {
            if (from->players[p].name == move.playerName) {
                playerIdx = static_cast<int>(p);
                break;
            }
        }
        if (playerIdx < 0) {
            pendingTransfers.erase(pendingTransfers.begin() + static_cast<long long>(i));
            continue;
        }
        Player player = from->players[static_cast<size_t>(playerIdx)];
        if (move.preContract) {
            player.onLoan = false;
            player.parentClub.clear();
            player.loanWeeksRemaining = 0;
            if (move.wage > 0) player.wage = move.wage;
            if (move.contractWeeks > 0) player.contractWeeks = move.contractWeeks;
            player.releaseClause = max(player.value * 2, player.wage * 45);
            if (!move.promisedRole.empty()) {
                player.promisedRole = move.promisedRole;
                player.promisedPosition = player.position;
                if (move.promisedRole == "Titular") player.desiredStarts = max(player.desiredStarts, 4);
                else if (move.promisedRole == "Rotacion") player.desiredStarts = max(player.desiredStarts, 2);
                else if (move.promisedRole == "Proyecto") player.desiredStarts = max(player.desiredStarts, 2);
            }
            from->players.erase(from->players.begin() + playerIdx);
            to->addPlayer(player);
            addNews(player.name + " se incorpora a " + to->name + " por precontrato.");
        }
        pendingTransfers.erase(pendingTransfers.begin() + static_cast<long long>(i));
    }
}

void Career::initializeSeasonCup() {
    cupRemainingTeams.clear();
    cupChampion.clear();
    cupRound = 0;
    cupActive = activeTeams.size() >= 4;
    if (!cupActive) return;
    for (Team* team : activeTeams) {
        if (team) cupRemainingTeams.push_back(team->name);
    }
}

void Career::initializeDynamicObjective() {
    if (!myTeam) return;
    int rank = currentCompetitiveRank();
    int field = max(1, currentCompetitiveFieldSize());
    boardMonthlyProgress = 0;
    boardMonthlyTarget = 0;
    boardMonthlyDeadlineWeek = min(static_cast<int>(schedule.size()), currentWeek + 3);
    if (boardMonthlyDeadlineWeek < currentWeek) boardMonthlyDeadlineWeek = currentWeek;

    int objectiveType = randInt(1, 4);
    if (objectiveType == 1) {
        boardMonthlyTarget = 6;
        boardMonthlyObjective = "Sumar al menos 6 puntos en 4 semanas";
    } else if (objectiveType == 2) {
        boardMonthlyTarget = 2;
        boardMonthlyObjective = "Dar 2 titularidades a sub-20";
    } else if (objectiveType == 3 && rank > 1) {
        boardMonthlyTarget = max(1, rank - 1);
        boardMonthlyProgress = (rank > 0) ? rank : field;
        boardMonthlyObjective = "Mejorar la posicion liguera antes de 4 semanas";
    } else {
        boardMonthlyTarget = static_cast<int>(max(100000LL, myTeam->budget * 80 / 100));
        boardMonthlyObjective = "Mantener presupuesto por sobre $" + to_string(boardMonthlyTarget);
        boardMonthlyProgress = static_cast<int>(min<long long>(myTeam->budget, 2000000000LL));
    }
}

void Career::updateDynamicObjectiveStatus() {
    if (!myTeam || boardMonthlyObjective.empty()) return;
    int youthStarts = 0;
    for (const auto& player : myTeam->players) {
        if (player.age <= 20) youthStarts += player.startsThisSeason;
    }
    if (boardMonthlyObjective.find("titularidades") != string::npos) {
        boardMonthlyProgress = youthStarts;
    } else if (boardMonthlyObjective.find("posicion") != string::npos) {
        boardMonthlyProgress = currentCompetitiveRank();
    } else if (boardMonthlyObjective.find("presupuesto") != string::npos) {
        boardMonthlyProgress = static_cast<int>(min<long long>(myTeam->budget, 2000000000LL));
    }

    if (currentWeek < boardMonthlyDeadlineWeek) return;
    bool success = false;
    if (boardMonthlyObjective.find("puntos") != string::npos) {
        success = boardMonthlyProgress >= boardMonthlyTarget;
    } else if (boardMonthlyObjective.find("titularidades") != string::npos) {
        success = boardMonthlyProgress >= boardMonthlyTarget;
    } else if (boardMonthlyObjective.find("posicion") != string::npos) {
        success = boardMonthlyProgress > 0 && boardMonthlyProgress <= boardMonthlyTarget;
    } else if (boardMonthlyObjective.find("presupuesto") != string::npos) {
        success = boardMonthlyProgress >= boardMonthlyTarget;
    }
    boardConfidence = clampInt(boardConfidence + (success ? 4 : -4), 0, 100);
    addNews(string("Objetivo mensual ") + (success ? "cumplido: " : "fallado: ") + boardMonthlyObjective);
    initializeDynamicObjective();
}

int Career::currentCompetitiveRank() const {
    if (!myTeam) return -1;
    vector<Team*> teams = boardRelevantTeams(*this);
    LeagueTable table = buildBoardTable(teams, activeDivision);
    for (size_t i = 0; i < table.teams.size(); ++i) {
        if (table.teams[i] == myTeam) return static_cast<int>(i) + 1;
    }
    return -1;
}

int Career::currentCompetitiveFieldSize() const {
    return static_cast<int>(boardRelevantTeams(*this).size());
}

void Career::initializeBoardObjectives() {
    if (!myTeam) return;
    vector<Team*> teams = boardRelevantTeams(*this);
    if (teams.empty()) return;
    vector<Team*> byValue = teams;
    sort(byValue.begin(), byValue.end(), [](Team* left, Team* right) {
        if (left->getSquadValue() != right->getSquadValue()) return left->getSquadValue() > right->getSquadValue();
        return left->name < right->name;
    });
    int valueRank = static_cast<int>(byValue.size());
    for (size_t i = 0; i < byValue.size(); ++i) {
        if (byValue[i] == myTeam) {
            valueRank = static_cast<int>(i) + 1;
            break;
        }
    }

    int fieldSize = static_cast<int>(teams.size());
    boardExpectedFinish = clampInt(valueRank + 1, 1, fieldSize);
    boardBudgetTarget = max(150000LL, myTeam->budget * 60 / 100);
    boardYouthTarget = (fieldSize >= 14) ? 2 : 1;
    boardConfidence = clampInt(55 + max(0, boardExpectedFinish - valueRank) * 4, 45, 75);
    boardWarningWeeks = 0;
}

void Career::updateBoardConfidence() {
    if (!myTeam || boardExpectedFinish <= 0) return;
    int rank = currentCompetitiveRank();
    int delta = 0;
    int prestige = teamPrestigeScore(*myTeam);
    if (rank > 0) {
        if (rank <= boardExpectedFinish) delta += 3;
        else delta -= min(5, rank - boardExpectedFinish);
        if (prestige >= 70 && rank > boardExpectedFinish) delta--;
    }
    if (myTeam->budget >= boardBudgetTarget) delta += 1;
    else delta -= 2;
    if (myTeam->budget < boardBudgetTarget * 8 / 10) delta -= 1;
    if (myTeam->debt > myTeam->sponsorWeekly * 16) delta -= 2;

    int youthUsed = youthPlayersUsed(*myTeam);
    if (youthUsed >= boardYouthTarget) delta += 1;
    else if (currentWeek > max(3, static_cast<int>(schedule.size()) / 4)) delta -= 1;

    if (myTeam->morale >= 65) delta += 1;
    else if (myTeam->morale <= 35) delta -= 1;
    if (myTeam->fanBase >= 55 && myTeam->morale <= 42) delta -= 1;

    int promiseRisk = promisesAtRiskForBoard(*myTeam, currentWeek);
    if (promiseRisk >= 2) delta -= min(3, promiseRisk);

    boardConfidence = clampInt(boardConfidence + delta, 0, 100);
    if (boardConfidence < 35) boardWarningWeeks++;
    else boardWarningWeeks = 0;
}
