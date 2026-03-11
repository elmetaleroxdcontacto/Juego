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

namespace {

string resolveRosterPosition(const string& position, const string& positionRaw) {
    const string normalizedPosition = normalizePosition(position);
    const string normalizedRaw = normalizePosition(positionRaw);
    if (normalizedPosition != "N/A" && normalizedRaw != "N/A") {
        return normalizedPosition == normalizedRaw ? normalizedPosition : normalizedRaw;
    }
    if (normalizedRaw != "N/A") return normalizedRaw;
    if (normalizedPosition != "N/A") return normalizedPosition;
    return "N/A";
}

Player buildLoadedPlayer(const string& name,
                         const string& position,
                         int age,
                         long long value,
                         const string& division) {
    const string normalizedPosition = normalizePosition(position) == "N/A" ? "MED" : normalizePosition(position);

    int skill = computeSkillFromValue(value, age, division);
    long long effectiveValue = value;
    if (effectiveValue == 0) {
        int minSkill = 40;
        int maxSkill = 65;
        getDivisionSkillRange(division, minSkill, maxSkill);
        skill = randInt(minSkill, maxSkill);
        effectiveValue = static_cast<long long>(skill) * 10000;
    }

    int attack = 0;
    int defense = 0;
    if (normalizedPosition == "ARQ") {
        attack = clampInt(skill - 30, 10, 70);
        defense = clampInt(skill + 10, 30, 99);
    } else if (normalizedPosition == "DEF") {
        attack = clampInt(skill - 10, 20, 90);
        defense = clampInt(skill + 10, 30, 99);
    } else if (normalizedPosition == "MED") {
        attack = clampInt(skill, 25, 99);
        defense = clampInt(skill, 25, 99);
    } else {
        attack = clampInt(skill + 10, 30, 99);
        defense = clampInt(skill - 10, 20, 90);
    }

    int stamina = clampInt(skill + randInt(-6, 6), 30, 99);
    if (age > 30) stamina = clampInt(stamina - (age - 30), 25, 99);

    Player player;
    player.name = name;
    player.position = normalizedPosition;
    player.attack = attack;
    player.defense = defense;
    player.stamina = stamina;
    player.fitness = stamina;
    player.skill = skill;
    player.potential = clampInt(player.skill + randInt(0, 10), player.skill, 95);
    player.age = age;
    player.value = effectiveValue;
    player.wage = static_cast<long long>(player.skill) * 150 + randInt(0, 600);
    player.releaseClause = max(50000LL, player.value * (18 + randInt(0, 8)) / 10);
    player.setPieceSkill = clampInt(player.skill + randInt(-8, 8), 25, 99);
    player.leadership = clampInt(35 + randInt(0, 45), 1, 99);
    player.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
    player.ambition = clampInt(35 + randInt(0, 50), 1, 99);
    player.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
    player.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
    player.desiredStarts = 1;
    player.startsThisSeason = 0;
    player.wantsToLeave = false;
    player.onLoan = false;
    player.parentClub.clear();
    player.loanWeeksRemaining = 0;
    player.contractWeeks = randInt(52, 156);
    player.injured = false;
    player.injuryType = "";
    player.injuryWeeks = 0;
    player.injuryHistory = 0;
    player.yellowAccumulation = 0;
    player.seasonYellowCards = 0;
    player.seasonRedCards = 0;
    player.matchesSuspended = 0;
    player.goals = 0;
    player.assists = 0;
    player.matchesPlayed = 0;
    player.lastTrainedSeason = -1;
    player.lastTrainedWeek = -1;
    ensurePlayerProfile(player, true);
    return player;
}

struct RoleCoverage {
    int goalkeepers = 0;
    int defenders = 0;
    int midfielders = 0;
    int forwards = 0;
};

int maxSquadForDivision(const string& division) {
    return getCompetitionConfig(division).maxSquadSize;
}

int roleMinimumForPosition(const string& position) {
    if (position == "ARQ") return 2;
    if (position == "DEF") return 3;
    if (position == "MED") return 3;
    if (position == "DEL") return 2;
    return 0;
}

int& coverageForPosition(RoleCoverage& coverage, const string& position) {
    if (position == "ARQ") return coverage.goalkeepers;
    if (position == "DEF") return coverage.defenders;
    if (position == "MED") return coverage.midfielders;
    return coverage.forwards;
}

RoleCoverage countRoleCoverage(const Team& team) {
    RoleCoverage coverage;
    for (const auto& player : team.players) {
        const string position = normalizePosition(player.position);
        if (position == "ARQ") coverage.goalkeepers++;
        else if (position == "DEF") coverage.defenders++;
        else if (position == "MED") coverage.midfielders++;
        else if (position == "DEL") coverage.forwards++;
    }
    return coverage;
}

bool rebalanceWeakestSurplusPlayer(Team& team, RoleCoverage& coverage, const string& targetPosition) {
    int bestIndex = -1;
    int bestSkill = 1000;
    string donorPosition;

    for (size_t i = 0; i < team.players.size(); ++i) {
        const string currentPosition = normalizePosition(team.players[i].position);
        if (currentPosition == "N/A" || currentPosition == targetPosition) continue;
        if (coverageForPosition(coverage, currentPosition) <= roleMinimumForPosition(currentPosition)) continue;
        if (team.players[i].skill < bestSkill) {
            bestIndex = static_cast<int>(i);
            bestSkill = team.players[i].skill;
            donorPosition = currentPosition;
        }
    }

    if (bestIndex < 0) return false;
    team.players[bestIndex].position = targetPosition;
    applyPositionStats(team.players[bestIndex]);
    coverageForPosition(coverage, donorPosition)--;
    coverageForPosition(coverage, targetPosition)++;
    return true;
}

void ensureCompetitiveRoleCoverage(Team& team) {
    RoleCoverage coverage = countRoleCoverage(team);
    const int maxSquad = maxSquadForDivision(team.division);

    auto topUpRole = [&](const string& position, int& currentCount, int minimumCount) {
        if (minimumCount <= 0) return;
        int minSkill = 40;
        int maxSkill = 65;
        getDivisionSkillRange(team.division, minSkill, maxSkill);
        while (currentCount < minimumCount) {
            if (rebalanceWeakestSurplusPlayer(team, coverage, position)) {
                currentCount = coverageForPosition(coverage, position);
                continue;
            }
            if (maxSquad > 0 && static_cast<int>(team.players.size()) >= maxSquad) {
                break;
            }
            team.addPlayer(makeRandomPlayer(position, minSkill, maxSkill, 18, 34));
            currentCount++;
        }
    };

    topUpRole("ARQ", coverage.goalkeepers, 2);
    topUpRole("DEF", coverage.defenders, 3);
    topUpRole("MED", coverage.midfielders, 3);
    topUpRole("DEL", coverage.forwards, 2);
}

void finalizeLoadedTeam(Team& team, int minimumPlayers = 18) {
    trimSquadForDivision(team);
    assignMissingPositions(team);
    int effectiveMinimum = minimumPlayers;
    const int maxSquad = maxSquadForDivision(team.division);
    if (maxSquad > 0) effectiveMinimum = min(effectiveMinimum, maxSquad);
    ensureMinimumSquad(team, effectiveMinimum);
    ensureCompetitiveRoleCoverage(team);
}

}  // namespace

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
    vector<string> lines;
    if (!readTextFileLines(filename, lines) || lines.empty()) return false;

    team.players.clear();
    unordered_set<string> seen;
    for (size_t lineIndex = 1; lineIndex < lines.size(); ++lineIndex) {
        string line = lines[lineIndex];
        if (trim(line).empty()) continue;
        auto cols = splitCsvLine(line);
        if (cols.size() < 7) continue;
        string name = trim(cols[0]);
        if (name.empty() || toLower(name) == "n/a") continue;

        string pos = cols[1];
        string posRaw = cols[2];
        string ageStr = cols[3];
        string valueStr = cols[5];

        const string norm = resolveRosterPosition(pos, posRaw);

        string key = toLower(name);
        if (seen.find(key) != seen.end()) continue;

        if (norm == "N/A" && ageStr == "N/A" && valueStr == "N/A") continue;

        team.addPlayer(buildLoadedPlayer(name, norm, parseAge(ageStr), parseMarketValue(valueStr), team.division));
        seen.insert(key);
    }
    finalizeLoadedTeam(team);
    return !team.players.empty();
}

bool loadTeamFromJson(const string& filename, Team& team) {
    vector<string> lines;
    if (!readTextFileLines(filename, lines)) return false;

    team.players.clear();
    unordered_set<string> seen;

    string currentName;
    string currentPosition;
    string currentPositionRaw;
    string currentAge;
    string currentValue;
    bool inObject = false;

    auto readJsonValue = [](const string& line, const string& key) -> string {
        const string marker = "\"" + key + "\"";
        size_t pos = line.find(marker);
        if (pos == string::npos) return "";
        pos = line.find(':', pos);
        if (pos == string::npos) return "";
        string value = trim(line.substr(pos + 1));
        if (!value.empty() && value.back() == ',') value.pop_back();
        value = trim(value);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        return value;
    };

    auto flushCurrent = [&]() {
        if (currentName.empty() || toLower(currentName) == "n/a") {
            currentName.clear();
            currentPosition.clear();
            currentPositionRaw.clear();
            currentAge.clear();
            currentValue.clear();
            inObject = false;
            return;
        }

        const string key = toLower(currentName);
        if (seen.find(key) == seen.end()) {
            const string resolvedPosition = resolveRosterPosition(currentPosition, currentPositionRaw);
            if (!(resolvedPosition == "N/A" && trim(currentAge) == "N/A" && trim(currentValue) == "N/A")) {
                team.addPlayer(buildLoadedPlayer(currentName,
                                                 resolvedPosition,
                                                 parseAge(currentAge),
                                                 parseMarketValue(currentValue),
                                                 team.division));
                seen.insert(key);
            }
        }

        currentName.clear();
        currentPosition.clear();
        currentPositionRaw.clear();
        currentAge.clear();
        currentValue.clear();
        inObject = false;
    };

    for (string line : lines) {
        line = trim(line);
        if (line == "{") {
            inObject = true;
            continue;
        }
        if (!inObject) continue;
        if (line == "}," || line == "}") {
            flushCurrent();
            continue;
        }

        string value = readJsonValue(line, "name");
        if (!value.empty()) currentName = value;
        value = readJsonValue(line, "position");
        if (!value.empty()) currentPosition = value;
        value = readJsonValue(line, "position_raw");
        if (!value.empty()) currentPositionRaw = value;
        value = readJsonValue(line, "age");
        if (!value.empty()) currentAge = value;
        value = readJsonValue(line, "market_value");
        if (!value.empty()) currentValue = value;
    }
    if (inObject) flushCurrent();

    finalizeLoadedTeam(team);
    return !team.players.empty();
}

bool loadTeamFromPlayersTxt(const string& filename, Team& team) {
    vector<string> lines;
    if (!readTextFileLines(filename, lines)) return false;
    team.players.clear();
    unordered_set<string> seen;
    for (string line : lines) {
        line = trim(line);
        if (line.empty()) continue;
        if (line[0] == '-') line = trim(line.substr(1));

        auto parts = splitByDelimiter(line, '|');
        if (parts.size() < 2) continue;

        string name = trim(parts[0]);
        if (name.empty() || toLower(name) == "n/a") continue;

        string posPart = trim(parts[1]);
        string positionRaw;
        string position = posPart;
        size_t open = posPart.find('(');
        size_t close = posPart.rfind(')');
        if (open != string::npos && close != string::npos && close > open) {
            positionRaw = trim(posPart.substr(open + 1, close - open - 1));
            position = trim(posPart.substr(0, open));
        }
        size_t sp = position.find(' ');
        if (sp != string::npos) position = position.substr(0, sp);
        const string norm = resolveRosterPosition(position, positionRaw);

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

        team.addPlayer(buildLoadedPlayer(name, norm, age, value, team.division));
        seen.insert(key);
    }
    finalizeLoadedTeam(team);
    return !team.players.empty();
}

bool loadTeamFromLegacyTxt(const string& filename, Team& team) {
    vector<string> lines;
    if (!readTextFileLines(filename, lines)) return false;
    team.players.clear();
    for (const string& line : lines) {
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
    finalizeLoadedTeam(team, 11);
    return !team.players.empty();
}

bool loadTeamFromFile(const string& filename, Team& team) {
    if (isDirectory(filename)) {
        string csv = joinPath(filename, "players.csv");
        if (pathExists(csv) && loadTeamFromCsv(csv, team)) return true;
        string txt = joinPath(filename, "players.txt");
        if (pathExists(txt) && loadTeamFromPlayersTxt(txt, team)) return true;
        if (pathExists(txt) && loadTeamFromLegacyTxt(txt, team)) return true;
        string json = joinPath(filename, "players.json");
        if (pathExists(json) && loadTeamFromJson(json, team)) return true;
        return false;
    }
    string ext = toLower(pathExtension(filename));
    if (ext == ".csv") return loadTeamFromCsv(filename, team);
    if (ext == ".json") return loadTeamFromJson(filename, team);
    if (ext == ".txt") {
        if (loadTeamFromPlayersTxt(filename, team)) return true;
        return loadTeamFromLegacyTxt(filename, team);
    }
    return false;
}

void ensureMinimumSquad(Team& team, int minPlayers) {
    const int maxSquad = maxSquadForDivision(team.division);
    if (maxSquad > 0) minPlayers = min(minPlayers, maxSquad);
    if (static_cast<int>(team.players.size()) >= minPlayers) return;
    int minSkill, maxSkill;
    getDivisionSkillRange(team.division, minSkill, maxSkill);
    vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
    while (static_cast<int>(team.players.size()) < minPlayers) {
        string pos = positions[randInt(0, static_cast<int>(positions.size()) - 1)];
        team.addPlayer(makeRandomPlayer(pos, minSkill, maxSkill, 18, 34));
    }
}

DivisionLoadResult loadDivisionFromFolder(const string& folder, const string& divisionId, deque<Team>& allTeams) {
    DivisionLoadResult result;
    if (!isDirectory(folder)) return result;

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
            result.warnings.push_back("Carpeta de equipo no encontrada: " + teamFolder +
                                      " (" + divisionDisplay(divisionId) + ")");
            continue;
        }
        string teamId = normalizeTeamId(teamName);
        if (teamId.empty()) teamId = normalizeTeamId(teamFolder);
        if (!teamId.empty() && usedIds.find(teamId) != usedIds.end()) {
            result.warnings.push_back("Equipo duplicado ignorado: " + teamName +
                                      " (" + divisionDisplay(divisionId) + ")");
            continue;
        }
        if (!teamId.empty()) usedIds.insert(teamId);

        Team team(teamName);
        team.division = divisionId;

        bool loaded = false;
        string csv = joinPath(dir, "players.csv");
        if (pathExists(csv) && loadTeamFromCsv(csv, team)) loaded = true;

        string txt = joinPath(dir, "players.txt");
        if (!loaded && pathExists(txt) && loadTeamFromPlayersTxt(txt, team)) loaded = true;
        if (!loaded && pathExists(txt) && loadTeamFromLegacyTxt(txt, team)) loaded = true;

        string json = joinPath(dir, "players.json");
        if (!loaded && pathExists(json) && loadTeamFromJson(json, team)) loaded = true;

        if (!loaded) {
            result.warnings.push_back("Plantilla invalida o incompleta, se genera una base temporal para " + teamName +
                                      " (" + divisionDisplay(divisionId) + ").");
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

        finalizeLoadedTeam(team);

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
        result.teams.push_back(&allTeams.back());
    }
    return result;
}
