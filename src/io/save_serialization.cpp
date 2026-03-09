#include "engine/models.h"

#include "utils/utils.h"

#include <fstream>
#include <sstream>

using namespace std;

namespace {

static constexpr int kCareerSaveVersion = 5;

string encodeHeadToHead(const vector<HeadToHeadRecord>& records) {
    string out;
    for (size_t i = 0; i < records.size(); ++i) {
        if (i) out += ";";
        out += records[i].opponent + "=" + to_string(records[i].points);
    }
    return out;
}

void decodeHeadToHead(const string& encoded, Team& team) {
    team.headToHead.clear();
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, ';')) {
        token = trim(token);
        if (token.empty()) continue;
        size_t eq = token.find('=');
        if (eq == string::npos) continue;
        string opponent = trim(token.substr(0, eq));
        string pointsStr = trim(token.substr(eq + 1));
        if (opponent.empty() || pointsStr.empty()) continue;
        try {
            team.headToHead.push_back({opponent, stoi(pointsStr)});
        } catch (...) {
        }
    }
}

string encodeStringList(const vector<string>& items) {
    string out;
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) out += ";";
        out += items[i];
    }
    return out;
}

vector<string> decodeStringList(const string& encoded) {
    vector<string> items;
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, ';')) {
        token = trim(token);
        if (!token.empty()) items.push_back(token);
    }
    return items;
}

string encodeHistory(const vector<SeasonHistoryEntry>& entries) {
    string out;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i) out += "~";
        const auto& entry = entries[i];
        out += to_string(entry.season) + "^" + entry.division + "^" + entry.club + "^" + to_string(entry.finish) +
               "^" + entry.champion + "^" + entry.promoted + "^" + entry.relegated + "^" + entry.note;
    }
    return out;
}

vector<SeasonHistoryEntry> decodeHistory(const string& encoded) {
    vector<SeasonHistoryEntry> entries;
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, '~')) {
        auto parts = splitByDelimiter(token, '^');
        if (parts.size() < 8) continue;
        SeasonHistoryEntry entry;
        entry.season = stoi(parts[0]);
        entry.division = parts[1];
        entry.club = parts[2];
        entry.finish = stoi(parts[3]);
        entry.champion = parts[4];
        entry.promoted = parts[5];
        entry.relegated = parts[6];
        entry.note = parts[7];
        entries.push_back(entry);
    }
    return entries;
}

string encodePendingTransfers(const vector<PendingTransfer>& entries) {
    string out;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i) out += "~";
        const auto& entry = entries[i];
        out += entry.playerName + "^" + entry.fromTeam + "^" + entry.toTeam + "^" + to_string(entry.effectiveSeason) +
               "^" + to_string(entry.loanWeeks) + "^" + to_string(entry.fee) + "^" + to_string(entry.wage) + "^" +
               to_string(entry.contractWeeks) + "^" + to_string(entry.preContract ? 1 : 0) + "^" +
               to_string(entry.loan ? 1 : 0) + "^" + entry.promisedRole;
    }
    return out;
}

vector<PendingTransfer> decodePendingTransfers(const string& encoded) {
    vector<PendingTransfer> entries;
    stringstream ss(encoded);
    string token;
    while (getline(ss, token, '~')) {
        auto parts = splitByDelimiter(token, '^');
        if (parts.size() < 8) continue;
        PendingTransfer entry;
        entry.playerName = parts[0];
        entry.fromTeam = parts[1];
        entry.toTeam = parts[2];
        entry.effectiveSeason = stoi(parts[3]);
        entry.loanWeeks = stoi(parts[4]);
        entry.fee = stoll(parts[5]);
        if (parts.size() >= 10) {
            entry.wage = stoll(parts[6]);
            entry.contractWeeks = stoi(parts[7]);
            entry.preContract = (parts[8] == "1");
            entry.loan = (parts[9] == "1");
            entry.promisedRole = (parts.size() > 10) ? parts[10] : "Sin promesa";
        } else {
            entry.wage = 0;
            entry.contractWeeks = 104;
            entry.preContract = (parts[6] == "1");
            entry.loan = (parts[7] == "1");
            entry.promisedRole = "Sin promesa";
        }
        entries.push_back(entry);
    }
    return entries;
}

string encodePlayerFields(const Player& player) {
    stringstream ss;
    ss << player.name << "|" << player.position << "|" << player.attack << "|" << player.defense << "|" << player.stamina
       << "|" << player.fitness << "|" << player.skill << "|" << player.potential << "|" << player.age << "|"
       << player.value << "|" << player.onLoan << "|" << player.parentClub << "|" << player.loanWeeksRemaining << "|"
       << player.injured << "|" << player.injuryType << "|" << player.injuryWeeks << "|" << player.goals << "|"
       << player.assists << "|" << player.matchesPlayed << "|" << player.lastTrainedSeason << "|"
       << player.lastTrainedWeek << "|" << player.wage << "|" << player.releaseClause << "|" << player.contractWeeks
       << "|" << player.injuryHistory << "|" << player.yellowAccumulation << "|" << player.seasonYellowCards << "|"
       << player.seasonRedCards << "|" << player.matchesSuspended << "|" << player.role << "|" << player.setPieceSkill
       << "|" << player.leadership << "|" << player.professionalism << "|" << player.ambition << "|"
       << player.happiness << "|" << player.chemistry << "|" << player.desiredStarts << "|" << player.startsThisSeason
       << "|" << player.wantsToLeave << "|" << player.developmentPlan << "|" << player.promisedRole << "|"
       << encodeStringList(player.traits) << "|" << player.preferredFoot << "|"
       << encodeStringList(player.secondaryPositions) << "|" << player.consistency << "|" << player.bigMatches << "|"
       << player.currentForm << "|" << player.tacticalDiscipline << "|" << player.versatility;
    return ss.str();
}

Player decodePlayerFields(const vector<string>& fields) {
    Player player;
    size_t idx = 0;
    bool hasLoanState = fields.size() >= 30;
    if (idx < fields.size()) player.name = fields[idx++];
    if (idx < fields.size()) player.position = fields[idx++];
    if (idx < fields.size()) player.attack = stoi(fields[idx++]); else player.attack = 40;
    if (idx < fields.size()) player.defense = stoi(fields[idx++]); else player.defense = 40;
    if (idx < fields.size()) player.stamina = stoi(fields[idx++]); else player.stamina = 60;
    bool hasFitness = fields.size() >= 16;
    if (hasFitness && idx < fields.size()) player.fitness = stoi(fields[idx++]); else player.fitness = player.stamina;
    if (idx < fields.size()) player.skill = stoi(fields[idx++]); else player.skill = (player.attack + player.defense) / 2;
    size_t remaining = (idx <= fields.size()) ? (fields.size() - idx) : 0;
    bool hasPotential = remaining >= 11;
    if (hasPotential && idx < fields.size()) player.potential = stoi(fields[idx++]);
    else player.potential = clampInt(player.skill + randInt(0, 8), player.skill, 95);
    if (idx < fields.size()) player.age = stoi(fields[idx++]); else player.age = 24;
    if (idx < fields.size()) player.value = stoll(fields[idx++]); else player.value = static_cast<long long>(player.skill) * 10000;
    if (hasLoanState && idx < fields.size()) {
        string loanStr = toLower(fields[idx++]);
        player.onLoan = (loanStr == "1" || loanStr == "true");
    } else {
        player.onLoan = false;
    }
    if (hasLoanState && idx < fields.size()) player.parentClub = fields[idx++]; else player.parentClub.clear();
    if (hasLoanState && idx < fields.size()) player.loanWeeksRemaining = stoi(fields[idx++]); else player.loanWeeksRemaining = 0;
    string injuredStr = (idx < fields.size()) ? toLower(fields[idx++]) : "0";
    player.injured = (injuredStr == "1" || injuredStr == "true");
    if (hasPotential && idx < fields.size()) player.injuryType = fields[idx++]; else player.injuryType = player.injured ? "Leve" : "";
    if (idx < fields.size()) player.injuryWeeks = stoi(fields[idx++]); else player.injuryWeeks = 0;
    if (idx < fields.size()) player.goals = stoi(fields[idx++]); else player.goals = 0;
    if (idx < fields.size()) player.assists = stoi(fields[idx++]); else player.assists = 0;
    if (idx < fields.size()) player.matchesPlayed = stoi(fields[idx++]); else player.matchesPlayed = 0;
    if (idx + 1 < fields.size()) {
        player.lastTrainedSeason = stoi(fields[idx++]);
        player.lastTrainedWeek = stoi(fields[idx++]);
    } else {
        player.lastTrainedSeason = -1;
        player.lastTrainedWeek = -1;
    }
    if (idx < fields.size()) player.wage = stoll(fields[idx++]); else player.wage = static_cast<long long>(player.skill) * 150 + randInt(0, 800);
    if (idx < fields.size()) player.releaseClause = stoll(fields[idx++]); else player.releaseClause = max(50000LL, player.value * 2);
    if (idx < fields.size()) player.contractWeeks = stoi(fields[idx++]); else player.contractWeeks = randInt(52, 156);
    if (idx < fields.size()) player.injuryHistory = stoi(fields[idx++]); else player.injuryHistory = 0;
    if (idx < fields.size()) player.yellowAccumulation = stoi(fields[idx++]); else player.yellowAccumulation = 0;
    if (idx < fields.size()) player.seasonYellowCards = stoi(fields[idx++]); else player.seasonYellowCards = 0;
    if (idx < fields.size()) player.seasonRedCards = stoi(fields[idx++]); else player.seasonRedCards = 0;
    if (idx < fields.size()) player.matchesSuspended = stoi(fields[idx++]); else player.matchesSuspended = 0;
    if (idx < fields.size()) player.role = fields[idx++]; else player.role = defaultRoleForPosition(player.position);
    if (idx < fields.size()) player.setPieceSkill = stoi(fields[idx++]); else player.setPieceSkill = clampInt(player.skill + randInt(-6, 6), 25, 99);
    if (idx < fields.size()) player.leadership = stoi(fields[idx++]); else player.leadership = clampInt(35 + randInt(0, 45), 1, 99);
    if (idx < fields.size()) player.professionalism = stoi(fields[idx++]); else player.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
    if (idx < fields.size()) player.ambition = stoi(fields[idx++]); else player.ambition = clampInt(35 + randInt(0, 50), 1, 99);
    if (idx < fields.size()) player.happiness = stoi(fields[idx++]); else player.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
    if (idx < fields.size()) player.chemistry = stoi(fields[idx++]); else player.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
    if (idx < fields.size()) player.desiredStarts = stoi(fields[idx++]); else player.desiredStarts = 1;
    if (idx < fields.size()) player.startsThisSeason = stoi(fields[idx++]); else player.startsThisSeason = 0;
    if (idx < fields.size()) {
        string wantsOut = toLower(fields[idx++]);
        player.wantsToLeave = (wantsOut == "1" || wantsOut == "true");
    } else {
        player.wantsToLeave = false;
    }
    if (idx < fields.size()) player.developmentPlan = fields[idx++]; else player.developmentPlan = defaultDevelopmentPlanForPosition(player.position);
    if (idx < fields.size()) player.promisedRole = fields[idx++]; else player.promisedRole = "Sin promesa";
    if (idx < fields.size()) player.traits = decodeStringList(fields[idx++]);
    if (idx < fields.size()) player.preferredFoot = fields[idx++];
    if (idx < fields.size()) player.secondaryPositions = decodeStringList(fields[idx++]);
    if (idx < fields.size()) player.consistency = stoi(fields[idx++]); else player.consistency = 0;
    if (idx < fields.size()) player.bigMatches = stoi(fields[idx++]); else player.bigMatches = 0;
    if (idx < fields.size()) player.currentForm = stoi(fields[idx++]); else player.currentForm = 0;
    if (idx < fields.size()) player.tacticalDiscipline = stoi(fields[idx++]); else player.tacticalDiscipline = 0;
    if (idx < fields.size()) player.versatility = stoi(fields[idx++]); else player.versatility = 0;
    ensurePlayerProfile(player, player.traits.empty());
    return player;
}

}  // namespace

bool Career::saveCareer() {
    if (saveFile.rfind("saves/", 0) == 0 || saveFile.rfind("saves\\", 0) == 0) {
        ensureDirectory("saves");
    }
    ofstream file(saveFile);
    if (!file.is_open()) return false;

    file << "VERSION " << kCareerSaveVersion << "\n";
    file << "SEASON " << currentSeason << " WEEK " << currentWeek << "\n";
    file << "DIVISION " << activeDivision << "\n";
    file << "MYTEAM " << (myTeam ? myTeam->name : "") << "\n";
    file << "MANAGER " << managerName << "|" << managerReputation << "\n";
    file << "DYNOBJ " << boardMonthlyObjective << "|" << boardMonthlyTarget << "|" << boardMonthlyProgress
         << "|" << boardMonthlyDeadlineWeek << "\n";
    file << "ACHIEVEMENTS " << achievements.size() << "\n";
    for (const auto& achievement : achievements) file << "ACH " << achievement << "\n";
    file << "BOARD " << boardConfidence << "|" << boardExpectedFinish << "|" << boardBudgetTarget << "|"
         << boardYouthTarget << "|" << boardWarningWeeks << "\n";
    file << "NEWS " << encodeStringList(newsFeed) << "\n";
    file << "SCOUT " << encodeStringList(scoutInbox) << "\n";
    file << "SHORTLIST " << encodeStringList(scoutingShortlist) << "\n";
    file << "HISTORY " << encodeHistory(history) << "\n";
    file << "PENDING " << encodePendingTransfers(pendingTransfers) << "\n";
    file << "CUP " << (cupActive ? 1 : 0) << "|" << cupRound << "|" << encodeStringList(cupRemainingTeams) << "|"
         << cupChampion << "\n";
    file << "LASTMATCH " << lastMatchAnalysis << "\n";
    file << "TEAMS " << allTeams.size() << "\n";

    for (auto& team : allTeams) {
        ensureTeamIdentity(team);
        file << "TEAM " << team.name << "|" << team.division << "|" << team.tactics << "|" << team.formation << "|"
             << team.budget << "|" << team.morale << "|" << team.points << "|" << team.goalsFor << "|"
             << team.goalsAgainst << "|" << team.wins << "|" << team.draws << "|" << team.losses << "|"
             << team.trainingFocus << "|" << team.awayGoals << "|" << team.redCards << "|" << team.yellowCards << "|"
             << team.tiebreakerSeed << "|" << encodeHeadToHead(team.headToHead) << "|" << team.pressingIntensity
             << "|" << team.defensiveLine << "|" << team.tempo << "|" << team.width << "|" << team.markingStyle << "|"
             << encodeStringList(team.preferredXI) << "|" << encodeStringList(team.preferredBench) << "|"
             << team.captain << "|" << team.penaltyTaker << "|" << team.freeKickTaker << "|" << team.cornerTaker
             << "|" << team.rotationPolicy << "|" << team.assistantCoach << "|" << team.fitnessCoach << "|"
             << team.scoutingChief << "|" << team.youthCoach << "|" << team.medicalTeam << "|" << team.youthRegion
             << "|" << team.debt << "|" << team.sponsorWeekly << "|" << team.stadiumLevel << "|"
             << team.youthFacilityLevel << "|" << team.trainingFacilityLevel << "|" << team.fanBase << "|"
             << team.matchInstruction << "|" << teamPrestigeScore(team) << "|" << team.clubStyle << "|"
             << team.youthIdentity << "|" << team.primaryRival << "\n";
        file << "PLAYERS " << team.players.size() << "\n";
        for (const auto& player : team.players) {
            file << "PLAYER " << encodePlayerFields(player) << "\n";
        }
        file << "ENDTEAM\n";
    }
    return true;
}

bool Career::loadCareer() {
    string resolvedSave = saveFile;
    if (!pathExists(resolvedSave) && resolvedSave == "saves/career_save.txt" && pathExists("career_save.txt")) {
        resolvedSave = "career_save.txt";
    }
    ifstream file(resolvedSave);
    if (!file.is_open()) return false;

    string line;
    if (!getline(file, line)) return false;
    line = trim(line);

    int saveVersion = 1;
    if (line.rfind("VERSION ", 0) == 0) {
        saveVersion = stoi(trim(line.substr(8)));
        if (saveVersion > kCareerSaveVersion) return false;
        if (!getline(file, line)) return false;
        line = trim(line);
    }

    if (line.rfind("SEASON ", 0) == 0) {
        string token;
        stringstream ss(line);
        ss >> token >> currentSeason >> token >> currentWeek;

        string divisionLine;
        if (!getline(file, divisionLine)) return false;
        if (divisionLine.rfind("DIVISION ", 0) == 0) activeDivision = trim(divisionLine.substr(9));

        string myTeamLine;
        if (!getline(file, myTeamLine)) return false;
        string myTeamName;
        if (myTeamLine.rfind("MYTEAM ", 0) == 0) myTeamName = trim(myTeamLine.substr(7));

        string teamsLine;
        if (!getline(file, teamsLine)) return false;
        managerName = "Manager";
        managerReputation = 50;
        if (teamsLine.rfind("MANAGER ", 0) == 0) {
            auto managerFields = splitByDelimiter(trim(teamsLine.substr(8)), '|');
            if (!managerFields.empty()) managerName = managerFields[0];
            if (managerFields.size() > 1) managerReputation = stoi(managerFields[1]);
            if (!getline(file, teamsLine)) return false;
        }
        boardMonthlyObjective.clear();
        boardMonthlyTarget = 0;
        boardMonthlyProgress = 0;
        boardMonthlyDeadlineWeek = 0;
        if (teamsLine.rfind("DYNOBJ ", 0) == 0) {
            auto objectiveFields = splitByDelimiter(trim(teamsLine.substr(7)), '|');
            if (!objectiveFields.empty()) boardMonthlyObjective = objectiveFields[0];
            if (objectiveFields.size() > 1) boardMonthlyTarget = stoi(objectiveFields[1]);
            if (objectiveFields.size() > 2) boardMonthlyProgress = stoi(objectiveFields[2]);
            if (objectiveFields.size() > 3) boardMonthlyDeadlineWeek = stoi(objectiveFields[3]);
            if (!getline(file, teamsLine)) return false;
        }
        achievements.clear();
        if (teamsLine.rfind("ACHIEVEMENTS ", 0) == 0) {
            int achCount = stoi(trim(teamsLine.substr(13)));
            for (int i = 0; i < achCount; ++i) {
                if (!getline(file, line) || line.rfind("ACH ", 0) != 0) return false;
                achievements.push_back(trim(line.substr(4)));
            }
            if (!getline(file, teamsLine)) return false;
        }
        boardConfidence = 60;
        boardExpectedFinish = 0;
        boardBudgetTarget = 0;
        boardYouthTarget = 1;
        boardWarningWeeks = 0;
        if (teamsLine.rfind("BOARD ", 0) == 0) {
            auto boardFields = splitByDelimiter(trim(teamsLine.substr(6)), '|');
            if (!boardFields.empty()) boardConfidence = clampInt(stoi(boardFields[0]), 0, 100);
            if (boardFields.size() > 1) boardExpectedFinish = stoi(boardFields[1]);
            if (boardFields.size() > 2) boardBudgetTarget = stoll(boardFields[2]);
            if (boardFields.size() > 3) boardYouthTarget = stoi(boardFields[3]);
            if (boardFields.size() > 4) boardWarningWeeks = stoi(boardFields[4]);
            if (!getline(file, teamsLine)) return false;
        }
        newsFeed.clear();
        if (teamsLine.rfind("NEWS ", 0) == 0) {
            newsFeed = decodeStringList(trim(teamsLine.substr(5)));
            if (!getline(file, teamsLine)) return false;
        }
        scoutInbox.clear();
        if (teamsLine.rfind("SCOUT ", 0) == 0) {
            scoutInbox = decodeStringList(trim(teamsLine.substr(6)));
            if (!getline(file, teamsLine)) return false;
        }
        scoutingShortlist.clear();
        if (teamsLine.rfind("SHORTLIST ", 0) == 0) {
            scoutingShortlist = decodeStringList(trim(teamsLine.substr(10)));
            if (!getline(file, teamsLine)) return false;
        }
        history.clear();
        if (teamsLine.rfind("HISTORY ", 0) == 0) {
            history = decodeHistory(trim(teamsLine.substr(8)));
            if (!getline(file, teamsLine)) return false;
        }
        pendingTransfers.clear();
        if (teamsLine.rfind("PENDING ", 0) == 0) {
            pendingTransfers = decodePendingTransfers(trim(teamsLine.substr(8)));
            if (!getline(file, teamsLine)) return false;
        }
        cupActive = false;
        cupRound = 0;
        cupRemainingTeams.clear();
        cupChampion.clear();
        if (teamsLine.rfind("CUP ", 0) == 0) {
            auto cupFields = splitByDelimiter(trim(teamsLine.substr(4)), '|');
            if (!cupFields.empty()) cupActive = (cupFields[0] == "1");
            if (cupFields.size() > 1) cupRound = stoi(cupFields[1]);
            if (cupFields.size() > 2) cupRemainingTeams = decodeStringList(cupFields[2]);
            if (cupFields.size() > 3) cupChampion = cupFields[3];
            if (!getline(file, teamsLine)) return false;
        }
        lastMatchAnalysis.clear();
        if (teamsLine.rfind("LASTMATCH ", 0) == 0) {
            lastMatchAnalysis = trim(teamsLine.substr(10));
            if (!getline(file, teamsLine)) return false;
        }
        if (teamsLine.rfind("TEAMS ", 0) != 0) return false;
        int teamCount = stoi(trim(teamsLine.substr(6)));

        allTeams.clear();
        for (int i = 0; i < teamCount; ++i) {
            if (!getline(file, line) || line.rfind("TEAM ", 0) != 0) return false;
            auto fields = splitByDelimiter(trim(line.substr(5)), '|');
            if (fields.size() < 11) return false;

            Team team(fields[0]);
            team.division = fields[1];
            team.tactics = fields[2];
            team.formation = fields[3];
            team.budget = stoll(fields[4]);
            size_t base = 5;
            if (fields.size() >= 12) {
                team.morale = clampInt(stoi(fields[5]), 0, 100);
                base = 6;
            } else {
                team.morale = 50;
            }
            team.points = stoi(fields[base + 0]);
            team.goalsFor = stoi(fields[base + 1]);
            team.goalsAgainst = stoi(fields[base + 2]);
            team.wins = stoi(fields[base + 3]);
            team.draws = stoi(fields[base + 4]);
            team.losses = stoi(fields[base + 5]);
            size_t tfIndex = base + 6;
            if (fields.size() > tfIndex) team.trainingFocus = fields[tfIndex];
            else team.trainingFocus = "Balanceado";
            size_t extra = tfIndex + 1;
            if (fields.size() > extra) team.awayGoals = stoi(fields[extra]); else team.awayGoals = 0;
            if (fields.size() > extra + 1) team.redCards = stoi(fields[extra + 1]); else team.redCards = 0;
            if (fields.size() > extra + 2) team.yellowCards = stoi(fields[extra + 2]); else team.yellowCards = 0;
            if (fields.size() > extra + 3) team.tiebreakerSeed = stoi(fields[extra + 3]); else team.tiebreakerSeed = randInt(0, 1000000);
            if (fields.size() > extra + 4) decodeHeadToHead(fields[extra + 4], team); else team.headToHead.clear();
            if (fields.size() > extra + 5) team.pressingIntensity = clampInt(stoi(fields[extra + 5]), 1, 5); else team.pressingIntensity = 3;
            if (fields.size() > extra + 6) team.defensiveLine = clampInt(stoi(fields[extra + 6]), 1, 5); else team.defensiveLine = 3;
            if (fields.size() > extra + 7) team.tempo = clampInt(stoi(fields[extra + 7]), 1, 5); else team.tempo = 3;
            if (fields.size() > extra + 8) team.width = clampInt(stoi(fields[extra + 8]), 1, 5); else team.width = 3;
            if (fields.size() > extra + 9) team.markingStyle = fields[extra + 9]; else team.markingStyle = "Zonal";
            if (fields.size() > extra + 10) team.preferredXI = decodeStringList(fields[extra + 10]); else team.preferredXI.clear();
            if (fields.size() > extra + 11) team.preferredBench = decodeStringList(fields[extra + 11]); else team.preferredBench.clear();
            if (fields.size() > extra + 12) team.captain = fields[extra + 12]; else team.captain.clear();
            if (fields.size() > extra + 13) team.penaltyTaker = fields[extra + 13]; else team.penaltyTaker.clear();
            if (fields.size() > extra + 14) team.freeKickTaker = fields[extra + 14]; else team.freeKickTaker.clear();
            if (fields.size() > extra + 15) team.cornerTaker = fields[extra + 15]; else team.cornerTaker.clear();
            if (fields.size() > extra + 16) team.rotationPolicy = fields[extra + 16]; else team.rotationPolicy = "Balanceado";
            if (fields.size() > extra + 17) team.assistantCoach = clampInt(stoi(fields[extra + 17]), 1, 99); else team.assistantCoach = 55;
            if (fields.size() > extra + 18) team.fitnessCoach = clampInt(stoi(fields[extra + 18]), 1, 99); else team.fitnessCoach = 55;
            if (fields.size() > extra + 19) team.scoutingChief = clampInt(stoi(fields[extra + 19]), 1, 99); else team.scoutingChief = 55;
            if (fields.size() > extra + 20) team.youthCoach = clampInt(stoi(fields[extra + 20]), 1, 99); else team.youthCoach = 55;
            if (fields.size() > extra + 21) team.medicalTeam = clampInt(stoi(fields[extra + 21]), 1, 99); else team.medicalTeam = 55;
            if (fields.size() > extra + 22) team.youthRegion = fields[extra + 22]; else team.youthRegion = "Metropolitana";
            if (fields.size() > extra + 23) team.debt = stoll(fields[extra + 23]); else team.debt = 0;
            if (fields.size() > extra + 24) team.sponsorWeekly = stoll(fields[extra + 24]); else team.sponsorWeekly = 25000;
            if (fields.size() > extra + 25) team.stadiumLevel = stoi(fields[extra + 25]); else team.stadiumLevel = 1;
            if (fields.size() > extra + 26) team.youthFacilityLevel = stoi(fields[extra + 26]); else team.youthFacilityLevel = 1;
            if (fields.size() > extra + 27) team.trainingFacilityLevel = stoi(fields[extra + 27]); else team.trainingFacilityLevel = 1;
            if (fields.size() > extra + 28) team.fanBase = stoi(fields[extra + 28]); else team.fanBase = 12;
            if (fields.size() > extra + 29) team.matchInstruction = fields[extra + 29]; else team.matchInstruction = "Equilibrado";
            if (fields.size() > extra + 30) team.clubPrestige = clampInt(stoi(fields[extra + 30]), 1, 99); else team.clubPrestige = 0;
            if (fields.size() > extra + 31) team.clubStyle = fields[extra + 31]; else team.clubStyle.clear();
            if (fields.size() > extra + 32) team.youthIdentity = fields[extra + 32]; else team.youthIdentity.clear();
            if (fields.size() > extra + 33) team.primaryRival = fields[extra + 33]; else team.primaryRival.clear();
            if (fields.size() <= extra + 19) {
                team.preferredBench.clear();
                if (fields.size() > extra + 11) team.captain = fields[extra + 11];
                if (fields.size() > extra + 12) team.penaltyTaker = fields[extra + 12];
                if (fields.size() > extra + 13) team.rotationPolicy = fields[extra + 13];
                if (fields.size() > extra + 14) team.debt = stoll(fields[extra + 14]);
                if (fields.size() > extra + 15) team.sponsorWeekly = stoll(fields[extra + 15]);
                if (fields.size() > extra + 16) team.stadiumLevel = stoi(fields[extra + 16]);
                if (fields.size() > extra + 17) team.youthFacilityLevel = stoi(fields[extra + 17]);
                if (fields.size() > extra + 18) team.trainingFacilityLevel = stoi(fields[extra + 18]);
                if (fields.size() > extra + 19) team.fanBase = stoi(fields[extra + 19]);
                team.freeKickTaker = team.penaltyTaker;
                team.cornerTaker = team.penaltyTaker;
                team.assistantCoach = team.fitnessCoach = team.scoutingChief = team.youthCoach = team.medicalTeam = 55;
                team.youthRegion = "Metropolitana";
                team.matchInstruction = "Equilibrado";
            }
            ensureTeamIdentity(team);

            if (!getline(file, line) || line.rfind("PLAYERS ", 0) != 0) return false;
            int playersCount = stoi(trim(line.substr(8)));
            for (int j = 0; j < playersCount; ++j) {
                if (!getline(file, line) || line.rfind("PLAYER ", 0) != 0) return false;
                auto playerFields = splitByDelimiter(trim(line.substr(7)), '|');
                if (playerFields.size() < 13) continue;
                team.addPlayer(decodePlayerFields(playerFields));
            }
            if (!getline(file, line) || trim(line) != "ENDTEAM") return false;
            allTeams.push_back(std::move(team));
        }

        initialized = true;
        if (activeDivision.empty() && !allTeams.empty()) activeDivision = allTeams.front().division;
        setActiveDivision(activeDivision);

        myTeam = nullptr;
        for (auto& team : allTeams) {
            if (team.name == myTeamName) {
                myTeam = &team;
                break;
            }
        }
        if (boardExpectedFinish <= 0) initializeBoardObjectives();
        if (boardMonthlyObjective.empty()) initializeDynamicObjective();
        if (!cupActive && !activeTeams.empty()) initializeSeasonCup();
        return true;
    }

    file.clear();
    file.seekg(0);
    int season = 1;
    int week = 1;
    if (!(file >> season >> week)) return false;
    currentSeason = season;
    currentWeek = week;
    managerName = "Manager";
    managerReputation = 50;
    newsFeed.clear();
    scoutInbox.clear();
    scoutingShortlist.clear();
    history.clear();
    pendingTransfers.clear();
    boardMonthlyObjective.clear();
    boardMonthlyTarget = 0;
    boardMonthlyProgress = 0;
    boardMonthlyDeadlineWeek = 0;
    cupActive = false;
    cupRound = 0;
    cupRemainingTeams.clear();
    cupChampion.clear();
    lastMatchAnalysis.clear();
    string teamName;
    getline(file, teamName);
    getline(file, teamName);

    initializeLeague(true);
    myTeam = nullptr;
    for (auto& team : allTeams) {
        if (team.name == teamName) {
            myTeam = &team;
            break;
        }
    }

    if (myTeam) {
        myTeam->players.clear();
        string line2;
        getline(file, line2);
        while (getline(file, line2)) {
            if (line2.empty()) continue;
            stringstream ss(line2);
            string token;
            Player player;
            getline(ss, player.name, ',');
            getline(ss, player.position, ',');
            try {
                getline(ss, token, ','); player.attack = stoi(token);
                getline(ss, token, ','); player.defense = stoi(token);
                getline(ss, token, ','); player.stamina = stoi(token);
                player.fitness = player.stamina;
                getline(ss, token, ','); player.skill = stoi(token);
                player.potential = clampInt(player.skill + randInt(0, 6), player.skill, 95);
                getline(ss, token, ','); player.age = stoi(token);
                getline(ss, token, ','); player.value = stoll(token);
                player.onLoan = false;
                player.parentClub.clear();
                player.loanWeeksRemaining = 0;
                string injuredStr;
                getline(ss, injuredStr, ',');
                injuredStr = toLower(trim(injuredStr));
                player.injured = (injuredStr == "1" || injuredStr == "true");
                player.injuryType = player.injured ? "Leve" : "";
                getline(ss, token, ','); player.injuryWeeks = stoi(token);
                getline(ss, token, ','); player.goals = stoi(token);
                getline(ss, token, ','); player.assists = stoi(token);
                getline(ss, token, ','); player.matchesPlayed = stoi(token);
                player.lastTrainedSeason = -1;
                player.lastTrainedWeek = -1;
                player.wage = static_cast<long long>(player.skill) * 150 + randInt(0, 800);
                player.releaseClause = max(50000LL, player.value * 2);
                player.setPieceSkill = clampInt(player.skill + randInt(-6, 6), 25, 99);
                player.leadership = clampInt(35 + randInt(0, 45), 1, 99);
                player.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
                player.ambition = clampInt(35 + randInt(0, 50), 1, 99);
                player.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
                player.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
                player.desiredStarts = 1;
                player.startsThisSeason = 0;
                player.wantsToLeave = false;
                player.contractWeeks = randInt(52, 156);
                player.injuryHistory = 0;
                player.yellowAccumulation = 0;
                player.seasonYellowCards = 0;
                player.seasonRedCards = 0;
                player.matchesSuspended = 0;
                ensurePlayerProfile(player, true);
                myTeam->addPlayer(player);
            } catch (...) {
            }
        }
    }

    if (myTeam) activeDivision = myTeam->division;
    else if (!divisions.empty()) activeDivision = divisions.front().id;
    if (!activeDivision.empty()) setActiveDivision(activeDivision);
    initializeBoardObjectives();
    initializeSeasonCup();
    initializeDynamicObjective();
    return true;
}
