#include "io.h"

#include "competition.h"
#include "utils.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>

using namespace std;

void assignMissingPositions(Team& team) {
    vector<int> unknown;
    int countARQ = 0;
    int countDEF = 0;
    int countMED = 0;
    int countDEL = 0;

    for (size_t i = 0; i < team.players.size(); ++i) {
        string norm = normalizePosition(team.players[i].position);
        if (norm == "ARQ") countARQ++;
        else if (norm == "DEF") countDEF++;
        else if (norm == "MED") countMED++;
        else if (norm == "DEL") countDEL++;
        else unknown.push_back(static_cast<int>(i));
    }

    if (unknown.empty()) return;

    int n = static_cast<int>(team.players.size());
    int targetARQ = max(1, static_cast<int>(round(n * 2.0 / 18.0)));
    int targetDEF = max(2, static_cast<int>(round(n * 6.0 / 18.0)));
    int targetMED = max(2, static_cast<int>(round(n * 6.0 / 18.0)));
    int targetDEL = max(1, static_cast<int>(round(n * 4.0 / 18.0)));

    int sumTargets = targetARQ + targetDEF + targetMED + targetDEL;
    while (sumTargets < n) {
        if (targetDEF <= targetMED) targetDEF++;
        else targetMED++;
        sumTargets++;
    }
    while (sumTargets > n) {
        if (targetDEF >= targetMED && targetDEF >= targetDEL && targetDEF > 1) targetDEF--;
        else if (targetMED >= targetDEL && targetMED > 1) targetMED--;
        else if (targetDEL > 1) targetDEL--;
        else if (targetARQ > 1) targetARQ--;
        sumTargets--;
    }

    auto assignPos = [&](int& count, int target, const string& pos) {
        while (count < target && !unknown.empty()) {
            int idx = unknown.back();
            unknown.pop_back();
            team.players[idx].position = pos;
            applyPositionStats(team.players[idx]);
            count++;
        }
    };

    assignPos(countARQ, targetARQ, "ARQ");
    assignPos(countDEF, targetDEF, "DEF");
    assignPos(countMED, targetMED, "MED");
    assignPos(countDEL, targetDEL, "DEL");

    const vector<string> fallback = {"DEF", "MED", "MED", "DEL", "MED", "DEF"};
    int k = 0;
    while (!unknown.empty()) {
        int idx = unknown.back();
        unknown.pop_back();
        team.players[idx].position = fallback[k % fallback.size()];
        applyPositionStats(team.players[idx]);
        k++;
    }
}

static int maxSquadForDivision(const string& division) {
    return getCompetitionConfig(division).maxSquadSize;
}

void trimSquadForDivision(Team& team) {
    int cap = maxSquadForDivision(team.division);
    if (cap <= 0) return;
    if (static_cast<int>(team.players.size()) <= cap) return;

    vector<int> idx;
    idx.reserve(team.players.size());
    for (size_t i = 0; i < team.players.size(); ++i) idx.push_back(static_cast<int>(i));

    sort(idx.begin(), idx.end(), [&](int a, int b) {
        if (team.players[a].skill != team.players[b].skill) return team.players[a].skill > team.players[b].skill;
        if (team.players[a].age != team.players[b].age) return team.players[a].age < team.players[b].age;
        return team.players[a].name < team.players[b].name;
    });

    vector<Player> trimmed;
    trimmed.reserve(cap);
    for (int i = 0; i < cap && i < static_cast<int>(idx.size()); ++i) {
        trimmed.push_back(team.players[idx[i]]);
    }
    team.players.swap(trimmed);
}

bool loadTeamFromCsv(const string& filename, Team& team) {
    ifstream file(filename);
    if (!file.is_open()) return false;

    team.players.clear();
    string line;
    if (!getline(file, line)) return false;

    unordered_set<string> seen;
    while (getline(file, line)) {
        if (trim(line).empty()) continue;
        auto cols = splitCsvLine(line);
        if (cols.size() < 7) continue;
        string name = trim(cols[0]);
        if (name.empty() || toLower(name) == "n/a") continue;

        string pos = cols[1];
        string posRaw = cols[2];
        string ageStr = cols[3];
        string valueStr = cols[5];

        string normPos = normalizePosition(pos);
        string normRaw = normalizePosition(posRaw);
        string norm = "N/A";
        if (normPos != "N/A" && normRaw != "N/A") {
            norm = (normPos == normRaw) ? normPos : normRaw;
        } else if (normRaw != "N/A") {
            norm = normRaw;
        } else if (normPos != "N/A") {
            norm = normPos;
        }
        string statPos = (norm == "N/A") ? "MED" : norm;

        string key = toLower(name);
        if (seen.find(key) != seen.end()) continue;

        if (norm == "N/A" && ageStr == "N/A" && valueStr == "N/A") continue;

        int age = parseAge(ageStr);
        long long value = parseMarketValue(valueStr);
        int skill = computeSkillFromValue(value, age, team.division);
        if (value == 0) {
            int minSkill, maxSkill;
            getDivisionSkillRange(team.division, minSkill, maxSkill);
            skill = randInt(minSkill, maxSkill);
            value = static_cast<long long>(skill) * 10000;
        }

        int attack = 0;
        int defense = 0;
        if (statPos == "ARQ") {
            attack = clampInt(skill - 30, 10, 70);
            defense = clampInt(skill + 10, 30, 99);
        } else if (statPos == "DEF") {
            attack = clampInt(skill - 10, 20, 90);
            defense = clampInt(skill + 10, 30, 99);
        } else if (statPos == "MED") {
            attack = clampInt(skill, 25, 99);
            defense = clampInt(skill, 25, 99);
        } else {
            attack = clampInt(skill + 10, 30, 99);
            defense = clampInt(skill - 10, 20, 90);
        }

        int stamina = clampInt(skill + randInt(-6, 6), 30, 99);
        if (age > 30) stamina = clampInt(stamina - (age - 30), 25, 99);

        Player p;
        p.name = name;
        p.position = norm;
        p.attack = attack;
        p.defense = defense;
        p.stamina = stamina;
        p.fitness = stamina;
        p.skill = skill;
        p.potential = clampInt(p.skill + randInt(0, 10), p.skill, 95);
        p.age = age;
        p.value = value;
        p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 600);
        p.releaseClause = max(50000LL, p.value * (18 + randInt(0, 8)) / 10);
        p.setPieceSkill = clampInt(p.skill + randInt(-8, 8), 25, 99);
        p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
        p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
        p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
        p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
        p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
        p.desiredStarts = 1;
        p.startsThisSeason = 0;
        p.wantsToLeave = false;
        p.onLoan = false;
        p.parentClub.clear();
        p.loanWeeksRemaining = 0;
        p.contractWeeks = randInt(52, 156);
        p.injured = false;
        p.injuryType = "";
        p.injuryWeeks = 0;
        p.injuryHistory = 0;
        p.yellowAccumulation = 0;
        p.seasonYellowCards = 0;
        p.seasonRedCards = 0;
        p.matchesSuspended = 0;
        p.goals = 0;
        p.assists = 0;
        p.matchesPlayed = 0;
        p.lastTrainedSeason = -1;
        p.lastTrainedWeek = -1;
        ensurePlayerProfile(p, true);
        team.addPlayer(p);
        seen.insert(key);
    }
    trimSquadForDivision(team);
    assignMissingPositions(team);
    return !team.players.empty();
}

bool loadTeamFromPlayersTxt(const string& filename, Team& team) {
    ifstream file(filename);
    if (!file.is_open()) return false;
    team.players.clear();
    unordered_set<string> seen;
    string line;
    while (getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line[0] == '-') line = trim(line.substr(1));

        auto parts = splitByDelimiter(line, '|');
        if (parts.size() < 2) continue;

        string name = trim(parts[0]);
        if (name.empty() || toLower(name) == "n/a") continue;

        string posPart = parts[1];
        string posToken = posPart;
        size_t sp = posPart.find(' ');
        if (sp != string::npos) posToken = posPart.substr(0, sp);
        string norm = normalizePosition(posToken);
        string statPos = (norm == "N/A") ? "MED" : norm;

        int age = 24;
        long long value = 0;
        for (const auto& seg : parts) {
            if (seg.find("Edad:") != string::npos) {
                string ageStr = trim(seg.substr(seg.find("Edad:") + 5));
                age = parseAge(ageStr);
            }
            if (seg.find("Valor:") != string::npos) {
                string valStr = trim(seg.substr(seg.find("Valor:") + 6));
                value = parseMarketValue(valStr);
            }
        }

        string key = toLower(name);
        if (seen.find(key) != seen.end()) continue;

        int skill = computeSkillFromValue(value, age, team.division);
        if (value == 0) {
            int minSkill, maxSkill;
            getDivisionSkillRange(team.division, minSkill, maxSkill);
            skill = randInt(minSkill, maxSkill);
            value = static_cast<long long>(skill) * 10000;
        }

        int attack = 0;
        int defense = 0;
        if (statPos == "ARQ") {
            attack = clampInt(skill - 30, 10, 70);
            defense = clampInt(skill + 10, 30, 99);
        } else if (statPos == "DEF") {
            attack = clampInt(skill - 10, 20, 90);
            defense = clampInt(skill + 10, 30, 99);
        } else if (statPos == "MED") {
            attack = clampInt(skill, 25, 99);
            defense = clampInt(skill, 25, 99);
        } else {
            attack = clampInt(skill + 10, 30, 99);
            defense = clampInt(skill - 10, 20, 90);
        }
        int stamina = clampInt(skill + randInt(-6, 6), 30, 99);
        if (age > 30) stamina = clampInt(stamina - (age - 30), 25, 99);

        Player p;
        p.name = name;
        p.position = norm;
        p.attack = attack;
        p.defense = defense;
        p.stamina = stamina;
        p.fitness = stamina;
        p.skill = skill;
        p.potential = clampInt(p.skill + randInt(0, 10), p.skill, 95);
        p.age = age;
        p.value = value;
        p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 600);
        p.releaseClause = max(50000LL, p.value * (18 + randInt(0, 8)) / 10);
        p.setPieceSkill = clampInt(p.skill + randInt(-8, 8), 25, 99);
        p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
        p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
        p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
        p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
        p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
        p.desiredStarts = 1;
        p.startsThisSeason = 0;
        p.wantsToLeave = false;
        p.onLoan = false;
        p.parentClub.clear();
        p.loanWeeksRemaining = 0;
        p.contractWeeks = randInt(52, 156);
        p.injured = false;
        p.injuryType = "";
        p.injuryWeeks = 0;
        p.injuryHistory = 0;
        p.yellowAccumulation = 0;
        p.seasonYellowCards = 0;
        p.seasonRedCards = 0;
        p.matchesSuspended = 0;
        p.goals = 0;
        p.assists = 0;
        p.matchesPlayed = 0;
        p.lastTrainedSeason = -1;
        p.lastTrainedWeek = -1;
        ensurePlayerProfile(p, true);
        team.addPlayer(p);
        seen.insert(key);
    }
    trimSquadForDivision(team);
    assignMissingPositions(team);
    return !team.players.empty();
}

bool loadTeamFromLegacyTxt(const string& filename, Team& team) {
    ifstream file(filename);
    if (!file.is_open()) return false;
    team.players.clear();
    string line;
    while (getline(file, line)) {
        if (line.find("Team: ") == 0) {
            team.name = line.substr(6);
        } else if (line.find("- Name: ") == 0) {
            Player p;
            size_t posName = line.find("Name: ") + 6;
            size_t posPos = line.find(", Position: ");
            p.name = line.substr(posName, posPos - posName);

            size_t posAttack = line.find("Attack: ");
            size_t posDefense = line.find(", Defense: ");
            p.position = line.substr(posPos + 11, posAttack - (posPos + 11));
            p.attack = stoi(line.substr(posAttack + 8, posDefense - (posAttack + 8)));
            p.defense = stoi(line.substr(posDefense + 10));

            size_t posStamina = line.find(", Stamina: ");
            if (posStamina != string::npos) {
                size_t posSkill = line.find(", Skill: ");
                p.stamina = stoi(line.substr(posStamina + 10, posSkill - (posStamina + 10)));
                size_t posAge = line.find(", Age: ");
                p.skill = stoi(line.substr(posSkill + 8, posAge - (posSkill + 8)));
                size_t posValue = line.find(", Value: ");
                p.age = stoi(line.substr(posAge + 6, posValue - (posAge + 6)));
                p.value = stoll(line.substr(posValue + 8));
            } else {
                p.stamina = 80;
                p.skill = (p.attack + p.defense) / 2;
                p.age = 25;
                p.value = static_cast<long long>(p.skill) * 10000;
            }
            p.injured = false;
            p.injuryType = "";
            p.injuryWeeks = 0;
            p.injuryHistory = 0;
            p.goals = 0;
            p.assists = 0;
            p.matchesPlayed = 0;
            p.fitness = p.stamina;
            p.potential = clampInt(p.skill + randInt(0, 8), p.skill, 95);
            p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 600);
            p.releaseClause = max(50000LL, p.value * (18 + randInt(0, 8)) / 10);
            p.setPieceSkill = clampInt(p.skill + randInt(-8, 8), 25, 99);
            p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
            p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
            p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
            p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
            p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
            p.desiredStarts = 1;
            p.startsThisSeason = 0;
            p.wantsToLeave = false;
            p.onLoan = false;
            p.parentClub.clear();
            p.loanWeeksRemaining = 0;
            p.contractWeeks = randInt(52, 156);
            p.lastTrainedSeason = -1;
            p.lastTrainedWeek = -1;
            p.yellowAccumulation = 0;
            p.seasonYellowCards = 0;
            p.seasonRedCards = 0;
            p.matchesSuspended = 0;
            ensurePlayerProfile(p, true);
            team.addPlayer(p);
        }
    }
    assignMissingPositions(team);
    return !team.players.empty();
}

bool loadTeamFromFile(const string& filename, Team& team) {
    if (isDirectory(filename)) {
        string csv = joinPath(filename, "players.csv");
        if (pathExists(csv)) return loadTeamFromCsv(csv, team);
        return false;
    }
    string ext = toLower(pathExtension(filename));
    if (ext == ".csv") return loadTeamFromCsv(filename, team);
    if (ext == ".txt") {
        if (loadTeamFromPlayersTxt(filename, team)) return true;
        return loadTeamFromLegacyTxt(filename, team);
    }
    return false;
}

void ensureMinimumSquad(Team& team, int minPlayers) {
    if (static_cast<int>(team.players.size()) >= minPlayers) return;
    int minSkill, maxSkill;
    getDivisionSkillRange(team.division, minSkill, maxSkill);
    vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    while (static_cast<int>(team.players.size()) < minPlayers) {
        string pos = positions[randInt(0, static_cast<int>(positions.size()) - 1)];
        team.addPlayer(makeRandomPlayer(pos, minSkill, maxSkill, 18, 34));
    }
}

vector<Team*> loadDivisionFromFolder(const string& folder, const string& divisionId, deque<Team>& allTeams) {
    vector<Team*> out;
    if (!isDirectory(folder)) return out;

    vector<pair<string, string>> teamEntries;
    if (!loadTeamsList(folder, teamEntries)) {
        vector<string> teamDirs = listDirectories(folder);
        sort(teamDirs.begin(), teamDirs.end(), [](const string& a, const string& b) {
            return toLower(pathFilename(a)) < toLower(pathFilename(b));
        });
        for (const auto& dir : teamDirs) {
            string name = pathFilename(dir);
            teamEntries.push_back({name, name});
        }
    }

    unordered_set<string> usedIds;
    for (const auto& entry : teamEntries) {
        string teamName = entry.first;
        string teamFolder = entry.second;
        string dir = joinPath(folder, teamFolder);
        if (!isDirectory(dir)) {
            cout << "[AVISO] Carpeta de equipo no encontrada: " << teamFolder << " (" << divisionDisplay(divisionId) << ")" << endl;
            continue;
        }
        string teamId = normalizeTeamId(teamName);
        if (teamId.empty()) teamId = normalizeTeamId(teamFolder);
        if (!teamId.empty() && usedIds.find(teamId) != usedIds.end()) {
            cout << "[AVISO] Equipo duplicado ignorado: " << teamName << " (" << divisionDisplay(divisionId) << ")" << endl;
            continue;
        }
        if (!teamId.empty()) usedIds.insert(teamId);

        Team team(teamName);
        team.division = divisionId;

        bool loaded = false;
        string csv = joinPath(dir, "players.csv");
        if (pathExists(csv)) {
            loaded = loadTeamFromCsv(csv, team);
        }

        if (!loaded) {
            int minSkill, maxSkill;
            getDivisionSkillRange(divisionId, minSkill, maxSkill);
            vector<string> positions = {
                "ARQ", "ARQ",
                "DEF", "DEF", "DEF", "DEF", "DEF", "DEF",
                "MED", "MED", "MED", "MED", "MED", "MED",
                "DEL", "DEL", "DEL", "DEL"
            };
            for (const auto& pos : positions) {
                team.addPlayer(makeRandomPlayer(pos, minSkill, maxSkill, 18, 34));
            }
        }

        ensureMinimumSquad(team, 18);

        int divisor = getCompetitionConfig(divisionId).budgetDivisor;
        if (divisor <= 0) divisor = 6;
        long long budget = team.getSquadValue() / divisor;
        team.budget = max(200000LL, budget);
        team.debt = 0;
        team.stadiumLevel = 1;
        team.youthFacilityLevel = 1;
        team.trainingFacilityLevel = 1;
        team.pressingIntensity = 3;
        team.defensiveLine = 3;
        team.tempo = 3;
        team.width = 3;
        team.markingStyle = "Zonal";
        team.rotationPolicy = "Balanceado";
        team.preferredXI.clear();
        team.preferredBench.clear();
        team.captain = team.players.empty() ? "" : team.players.front().name;
        team.penaltyTaker = team.players.empty() ? "" : team.players.back().name;
        team.freeKickTaker = team.penaltyTaker;
        team.cornerTaker = team.penaltyTaker;
        team.assistantCoach = clampInt(50 + randInt(0, 25), 1, 99);
        team.fitnessCoach = clampInt(50 + randInt(0, 25), 1, 99);
        team.scoutingChief = clampInt(50 + randInt(0, 25), 1, 99);
        team.youthCoach = clampInt(50 + randInt(0, 25), 1, 99);
        team.medicalTeam = clampInt(50 + randInt(0, 25), 1, 99);
        vector<string> regions = {"Metropolitana", "Norte", "Centro", "Sur", "Patagonia"};
        team.youthRegion = regions[randInt(0, static_cast<int>(regions.size()) - 1)];
        team.sponsorWeekly = max(12000LL, team.getSquadValue() / 25);
        team.fanBase = clampInt(static_cast<int>(team.getSquadValue() / 120000LL), 8, 40);
        ensureTeamIdentity(team);

        allTeams.push_back(std::move(team));
        out.push_back(&allTeams.back());
    }
    return out;
}
