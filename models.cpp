#include "models.h"

#include "io.h"
#include "utils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

void applyPositionStats(Player& p) {
    string norm = normalizePosition(p.position);
    if (norm == "ARQ") {
        p.attack = clampInt(p.skill - 30, 10, 70);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (norm == "DEF") {
        p.attack = clampInt(p.skill - 10, 20, 90);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (norm == "MED") {
        p.attack = clampInt(p.skill, 25, 99);
        p.defense = clampInt(p.skill, 25, 99);
    } else if (norm == "DEL") {
        p.attack = clampInt(p.skill + 10, 30, 99);
        p.defense = clampInt(p.skill - 10, 20, 90);
    }
}

Player makeRandomPlayer(const string& position, int skillMin, int skillMax, int ageMin, int ageMax) {
    Player p;
    p.name = "Jugador" + to_string(randInt(1000, 9999));
    p.position = position;
    p.skill = randInt(skillMin, skillMax);
    p.age = randInt(ageMin, ageMax);
    p.stamina = clampInt(p.skill + randInt(-5, 5), 30, 99);
    if (position == "ARQ") {
        p.attack = clampInt(p.skill - 30, 10, 70);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (position == "DEF") {
        p.attack = clampInt(p.skill - 10, 20, 90);
        p.defense = clampInt(p.skill + 10, 30, 99);
    } else if (position == "MED") {
        p.attack = clampInt(p.skill, 25, 99);
        p.defense = clampInt(p.skill, 25, 99);
    } else {
        p.attack = clampInt(p.skill + 10, 30, 99);
        p.defense = clampInt(p.skill - 10, 20, 90);
    }
    p.value = static_cast<long long>(p.skill) * 10000;
    p.injured = false;
    p.injuryWeeks = 0;
    p.goals = 0;
    p.assists = 0;
    p.matchesPlayed = 0;
    return p;
}

Team::Team(string n)
    : name(std::move(n)),
      division(""),
      tactics("Balanced"),
      formation("4-4-2"),
      budget(0),
      points(0),
      goalsFor(0),
      goalsAgainst(0),
      wins(0),
      draws(0),
      losses(0) {}

void Team::addPlayer(const Player& p) {
    players.push_back(p);
}

void Team::resetSeasonStats() {
    points = 0;
    goalsFor = 0;
    goalsAgainst = 0;
    wins = 0;
    draws = 0;
    losses = 0;
}

static void parseFormation(const string& formation, int& defCount, int& midCount, int& fwdCount) {
    defCount = 4;
    midCount = 4;
    fwdCount = 2;
    string s = trim(formation);
    if (s.empty()) return;
    stringstream ss(s);
    string token;
    vector<int> nums;
    while (getline(ss, token, '-')) {
        token = trim(token);
        if (token.empty()) continue;
        try {
            nums.push_back(stoi(token));
        } catch (...) {
        }
    }
    if (nums.size() == 3) {
        defCount = nums[0];
        midCount = nums[1];
        fwdCount = nums[2];
    }
    int total = 1 + defCount + midCount + fwdCount;
    if (total != 11) {
        defCount = 4;
        midCount = 4;
        fwdCount = 2;
    }
}

vector<int> Team::getStartingXIIndices() const {
    vector<int> xi;
    if (players.empty()) return xi;
    int defCount, midCount, fwdCount;
    parseFormation(formation, defCount, midCount, fwdCount);

    vector<bool> used(players.size(), false);
    auto pick = [&](const string& pos, int count) {
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (used[i]) continue;
            if (players[i].injured) continue;
            string norm = normalizePosition(players[i].position);
            if (norm == pos) candidates.push_back(static_cast<int>(i));
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            return players[a].skill > players[b].skill;
        });
        for (int i = 0; i < count && i < static_cast<int>(candidates.size()); ++i) {
            int idx = candidates[i];
            used[idx] = true;
            xi.push_back(idx);
        }
    };

    pick("ARQ", 1);
    pick("DEF", defCount);
    pick("MED", midCount);
    pick("DEL", fwdCount);

    if (xi.size() < 11) {
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (!used[i] && !players[i].injured) candidates.push_back(static_cast<int>(i));
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            return players[a].skill > players[b].skill;
        });
        for (int idx : candidates) {
            used[idx] = true;
            xi.push_back(idx);
            if (xi.size() >= 11) break;
        }
    }
    if (xi.size() < 11) {
        vector<int> candidates;
        for (size_t i = 0; i < players.size(); ++i) {
            if (!used[i]) candidates.push_back(static_cast<int>(i));
        }
        sort(candidates.begin(), candidates.end(), [&](int a, int b) {
            return players[a].skill > players[b].skill;
        });
        for (int idx : candidates) {
            used[idx] = true;
            xi.push_back(idx);
            if (xi.size() >= 11) break;
        }
    }
    return xi;
}

int Team::getTotalAttack() const {
    auto xi = getStartingXIIndices();
    int total = 0;
    for (int idx : xi) total += players[idx].attack;
    return total;
}

int Team::getTotalDefense() const {
    auto xi = getStartingXIIndices();
    int total = 0;
    for (int idx : xi) total += players[idx].defense;
    return total;
}

int Team::getAverageSkill() const {
    auto xi = getStartingXIIndices();
    if (xi.empty()) return 0;
    int total = 0;
    for (int idx : xi) total += players[idx].skill;
    return total / static_cast<int>(xi.size());
}

int Team::getAverageStamina() const {
    auto xi = getStartingXIIndices();
    if (xi.empty()) return 0;
    int total = 0;
    for (int idx : xi) total += players[idx].stamina;
    return total / static_cast<int>(xi.size());
}

long long Team::getSquadValue() const {
    long long total = 0;
    for (const auto& p : players) total += p.value;
    return total;
}

void LeagueTable::clear() {
    teams.clear();
}

void LeagueTable::addTeam(Team* team) {
    teams.push_back(team);
}

void LeagueTable::sortTable() {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        if (a->points != b->points) return a->points > b->points;
        int aGD = a->goalsFor - a->goalsAgainst;
        int bGD = b->goalsFor - b->goalsAgainst;
        if (aGD != bGD) return aGD > bGD;
        if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
        return a->name < b->name;
    });
}

void LeagueTable::displayTable() {
    cout << "\n--- Tabla " << (title.empty() ? "Liga" : title) << " ---" << endl;
    cout << "Pos | Equipo                 | Pts | PJ | G  | E  | P  | GF | GA | DG" << endl;
    cout << "----+------------------------+-----+----+----+----+----+----+----+----" << endl;
    for (size_t i = 0; i < teams.size(); ++i) {
        Team* team = teams[i];
        int gd = team->goalsFor - team->goalsAgainst;
        int pj = team->wins + team->draws + team->losses;
        cout << setw(3) << i + 1 << " | " << setw(22) << left << team->name.substr(0, 22)
             << " | " << setw(3) << team->points
             << " | " << setw(2) << pj
             << " | " << setw(2) << team->wins
             << " | " << setw(2) << team->draws
             << " | " << setw(2) << team->losses
             << " | " << setw(2) << team->goalsFor
             << " | " << setw(2) << team->goalsAgainst
             << " | " << setw(2) << gd << endl;
    }
}

const vector<DivisionInfo> kDivisions = {
    {"primera division", "LigaChilena/primera division", "Primera Division"},
    {"primera b", "LigaChilena/primera b", "Primera B"},
    {"segunda division", "LigaChilena/segunda division", "Segunda Division"},
    {"tercera division a", "LigaChilena/tercera division a", "Tercera Division A"},
    {"tercera division b", "LigaChilena/tercera division b", "Tercera Division B"}
};

Career::Career() : myTeam(nullptr), currentSeason(1), currentWeek(1), saveFile("career_save.txt"), initialized(false) {}

void Career::initializeLeague(bool forceReload) {
    if (initialized && !forceReload) return;
    allTeams.clear();
    activeTeams.clear();
    schedule.clear();
    leagueTable.clear();
    divisions.clear();
    activeDivision.clear();

    for (const auto& div : kDivisions) {
        if (!isDirectory(div.folder)) continue;
        auto teams = loadDivisionFromFolder(div.folder, div.id, allTeams);
        if (!teams.empty()) {
            divisions.push_back(div);
        }
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

    vector<int> idx;
    for (int i = 0; i < n; ++i) idx.push_back(i);
    if (n % 2 == 1) idx.push_back(-1);

    int size = static_cast<int>(idx.size());
    int rounds = size - 1;
    for (int round = 0; round < rounds; ++round) {
        vector<pair<int, int>> matches;
        for (int i = 0; i < size / 2; ++i) {
            int a = idx[i];
            int b = idx[size - 1 - i];
            if (a == -1 || b == -1) continue;
            if (round % 2 == 0) matches.push_back({a, b});
            else matches.push_back({b, a});
        }
        schedule.push_back(matches);
        int last = idx.back();
        for (int i = size - 1; i > 1; --i) idx[i] = idx[i - 1];
        idx[1] = last;
    }

    int base = static_cast<int>(schedule.size());
    for (int i = 0; i < base; ++i) {
        vector<pair<int, int>> rev;
        for (auto& m : schedule[i]) rev.push_back({m.second, m.first});
        schedule.push_back(rev);
    }
}

void Career::setActiveDivision(const string& id) {
    activeDivision = id;
    activeTeams = getDivisionTeams(id);
    leagueTable.clear();
    leagueTable.title = divisionDisplay(id);
    for (auto* t : activeTeams) leagueTable.addTeam(t);
    leagueTable.sortTable();
    buildSchedule();
}

void Career::resetSeason() {
    for (auto* team : activeTeams) {
        team->resetSeasonStats();
        for (auto& p : team->players) {
            p.injured = false;
            p.injuryWeeks = 0;
        }
    }
    buildSchedule();
}

void Career::agePlayers() {
    for (auto& team : allTeams) {
        for (auto& player : team.players) {
            player.age++;
            if (player.age > 30) {
                player.skill = max(1, player.skill - 1);
                player.stamina = max(1, player.stamina - 1);
            }
        }
    }
}

void Career::saveCareer() {
    ofstream file(saveFile);
    if (!file.is_open()) return;

    file << "SEASON " << currentSeason << " WEEK " << currentWeek << "\n";
    file << "DIVISION " << activeDivision << "\n";
    file << "MYTEAM " << (myTeam ? myTeam->name : "") << "\n";
    file << "ACHIEVEMENTS " << achievements.size() << "\n";
    for (const auto& ach : achievements) {
        file << "ACH " << ach << "\n";
    }
    file << "TEAMS " << allTeams.size() << "\n";

    for (const auto& team : allTeams) {
        file << "TEAM " << team.name << "|" << team.division << "|" << team.tactics << "|" << team.formation
             << "|" << team.budget << "|" << team.points << "|" << team.goalsFor << "|" << team.goalsAgainst
             << "|" << team.wins << "|" << team.draws << "|" << team.losses << "\n";
        file << "PLAYERS " << team.players.size() << "\n";
        for (const auto& p : team.players) {
            file << "PLAYER " << p.name << "|" << p.position << "|" << p.attack << "|" << p.defense << "|"
                 << p.stamina << "|" << p.skill << "|" << p.age << "|" << p.value << "|" << p.injured << "|"
                 << p.injuryWeeks << "|" << p.goals << "|" << p.assists << "|" << p.matchesPlayed << "\n";
        }
        file << "ENDTEAM\n";
    }
    cout << "Carrera guardada exitosamente." << endl;
}

bool Career::loadCareer() {
    ifstream file(saveFile);
    if (!file.is_open()) return false;

    string line;
    if (!getline(file, line)) return false;
    line = trim(line);

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
        achievements.clear();
        if (teamsLine.rfind("ACHIEVEMENTS ", 0) == 0) {
            int achCount = stoi(trim(teamsLine.substr(13)));
            for (int i = 0; i < achCount; ++i) {
                if (!getline(file, line)) return false;
                if (line.rfind("ACH ", 0) != 0) return false;
                achievements.push_back(trim(line.substr(4)));
            }
            if (!getline(file, teamsLine)) return false;
        }
        if (teamsLine.rfind("TEAMS ", 0) != 0) return false;
        int teamCount = stoi(trim(teamsLine.substr(6)));

        allTeams.clear();
        for (int i = 0; i < teamCount; ++i) {
            if (!getline(file, line)) return false;
            if (line.rfind("TEAM ", 0) != 0) return false;
            auto fields = splitByDelimiter(trim(line.substr(5)), '|');
            if (fields.size() < 11) return false;

            Team team(fields[0]);
            team.division = fields[1];
            team.tactics = fields[2];
            team.formation = fields[3];
            team.budget = stoll(fields[4]);
            team.points = stoi(fields[5]);
            team.goalsFor = stoi(fields[6]);
            team.goalsAgainst = stoi(fields[7]);
            team.wins = stoi(fields[8]);
            team.draws = stoi(fields[9]);
            team.losses = stoi(fields[10]);

            if (!getline(file, line)) return false;
            if (line.rfind("PLAYERS ", 0) != 0) return false;
            int playersCount = stoi(trim(line.substr(8)));
            for (int j = 0; j < playersCount; ++j) {
                if (!getline(file, line)) return false;
                if (line.rfind("PLAYER ", 0) != 0) return false;
                auto pf = splitByDelimiter(trim(line.substr(7)), '|');
                if (pf.size() < 13) continue;
                Player p;
                p.name = pf[0];
                p.position = pf[1];
                p.attack = stoi(pf[2]);
                p.defense = stoi(pf[3]);
                p.stamina = stoi(pf[4]);
                p.skill = stoi(pf[5]);
                p.age = stoi(pf[6]);
                p.value = stoll(pf[7]);
                string injuredStr = toLower(pf[8]);
                p.injured = (injuredStr == "1" || injuredStr == "true");
                p.injuryWeeks = stoi(pf[9]);
                p.goals = stoi(pf[10]);
                p.assists = stoi(pf[11]);
                p.matchesPlayed = stoi(pf[12]);
                team.addPlayer(p);
            }
            if (!getline(file, line)) return false;
            if (trim(line) != "ENDTEAM") return false;

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
        return true;
    }

    file.clear();
    file.seekg(0);
    int season = 1;
    int week = 1;
    if (!(file >> season >> week)) return false;
    currentSeason = season;
    currentWeek = week;
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
            Player p;
            getline(ss, p.name, ',');
            getline(ss, p.position, ',');
            try {
                getline(ss, token, ','); p.attack = stoi(token);
                getline(ss, token, ','); p.defense = stoi(token);
                getline(ss, token, ','); p.stamina = stoi(token);
                getline(ss, token, ','); p.skill = stoi(token);
                getline(ss, token, ','); p.age = stoi(token);
                getline(ss, token, ','); p.value = stoll(token);
                string injuredStr;
                getline(ss, injuredStr, ',');
                injuredStr = toLower(trim(injuredStr));
                p.injured = (injuredStr == "1" || injuredStr == "true");
                getline(ss, token, ','); p.injuryWeeks = stoi(token);
                getline(ss, token, ','); p.goals = stoi(token);
                getline(ss, token, ','); p.assists = stoi(token);
                getline(ss, token, ','); p.matchesPlayed = stoi(token);
                myTeam->addPlayer(p);
            } catch (...) {
            }
        }
    }

    if (myTeam) activeDivision = myTeam->division;
    else if (!divisions.empty()) activeDivision = divisions.front().id;
    if (!activeDivision.empty()) setActiveDivision(activeDivision);
    return true;
}
