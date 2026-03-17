#include "models.h"

#include "competition/competition.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

namespace {

int goalDifference(const Team* team) {
    return team->goalsFor - team->goalsAgainst;
}

bool compareTerceraABase(const Team* a, const Team* b) {
    if (a->points != b->points) return a->points > b->points;
    int aGD = goalDifference(a);
    int bGD = goalDifference(b);
    if (aGD != bGD) return aGD > bGD;
    if (a->wins != b->wins) return a->wins > b->wins;
    if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
    if (a->redCards != b->redCards) return a->redCards < b->redCards;
    if (a->yellowCards != b->yellowCards) return a->yellowCards < b->yellowCards;
    if (a->tiebreakerSeed != b->tiebreakerSeed) return a->tiebreakerSeed < b->tiebreakerSeed;
    return a->name < b->name;
}

bool compareTerceraBBase(const Team* a, const Team* b) {
    if (a->points != b->points) return a->points > b->points;
    int aGD = goalDifference(a);
    int bGD = goalDifference(b);
    if (aGD != bGD) return aGD > bGD;
    if (a->wins != b->wins) return a->wins > b->wins;
    if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
    if (a->yellowCards != b->yellowCards) return a->yellowCards < b->yellowCards;
    if (a->redCards != b->redCards) return a->redCards < b->redCards;
    if (a->tiebreakerSeed != b->tiebreakerSeed) return a->tiebreakerSeed < b->tiebreakerSeed;
    return a->name < b->name;
}

int headToHeadPointsWithinTie(const Team* team, const vector<Team*>& tiedTeams) {
    int total = 0;
    for (const Team* other : tiedTeams) {
        if (other != team) total += team->headToHeadPointsAgainst(other->name);
    }
    return total;
}

void applyHeadToHeadOrder(vector<Team*>& teams, bool terceraA) {
    size_t start = 0;
    while (start < teams.size()) {
        size_t end = start + 1;
        while (end < teams.size()) {
            Team* a = teams[start];
            Team* b = teams[end];
            bool tied = a->points == b->points &&
                        goalDifference(a) == goalDifference(b) &&
                        a->wins == b->wins &&
                        a->goalsFor == b->goalsFor;
            if (terceraA) tied = tied && a->redCards == b->redCards && a->yellowCards == b->yellowCards;
            else tied = tied && a->yellowCards == b->yellowCards && a->redCards == b->redCards;
            if (!tied) break;
            ++end;
        }
        if (end - start > 1) {
            vector<Team*> tiedTeams(teams.begin() + static_cast<long long>(start), teams.begin() + static_cast<long long>(end));
            sort(teams.begin() + static_cast<long long>(start), teams.begin() + static_cast<long long>(end), [&](Team* a, Team* b) {
                int aH2H = headToHeadPointsWithinTie(a, tiedTeams);
                int bH2H = headToHeadPointsWithinTie(b, tiedTeams);
                if (aH2H != bH2H) return aH2H > bH2H;
                return terceraA ? compareTerceraABase(a, b) : compareTerceraBBase(a, b);
            });
        }
        start = end;
    }
}

}  // namespace

void LeagueTable::clear() {
    teams.clear();
}

void LeagueTable::addTeam(Team* team) {
    teams.push_back(team);
}

void LeagueTable::sortTable() {
    const CompetitionConfig& config = getCompetitionConfig(ruleId);
    if (config.tableProfile == CompetitionTableProfile::PrimeraLike) {
        sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
            if (a->points != b->points) return a->points > b->points;
            int aGD = goalDifference(a);
            int bGD = goalDifference(b);
            if (aGD != bGD) return aGD > bGD;
            if (a->wins != b->wins) return a->wins > b->wins;
            if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
            if (a->awayGoals != b->awayGoals) return a->awayGoals > b->awayGoals;
            if (a->redCards != b->redCards) return a->redCards < b->redCards;
            if (a->yellowCards != b->yellowCards) return a->yellowCards < b->yellowCards;
            if (a->tiebreakerSeed != b->tiebreakerSeed) return a->tiebreakerSeed < b->tiebreakerSeed;
            return a->name < b->name;
        });
        return;
    }
    if (config.tableProfile == CompetitionTableProfile::TerceraA) {
        sort(teams.begin(), teams.end(), compareTerceraABase);
        applyHeadToHeadOrder(teams, true);
        return;
    }
    if (config.tableProfile == CompetitionTableProfile::TerceraB) {
        sort(teams.begin(), teams.end(), compareTerceraBBase);
        applyHeadToHeadOrder(teams, false);
        return;
    }
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        if (a->points != b->points) return a->points > b->points;
        int aGD = goalDifference(a);
        int bGD = goalDifference(b);
        if (aGD != bGD) return aGD > bGD;
        if (a->goalsFor != b->goalsFor) return a->goalsFor > b->goalsFor;
        return a->name < b->name;
    });
}

vector<string> LeagueTable::formatLines() const {
    vector<string> lines;
    lines.push_back("");
    lines.push_back("--- Tabla " + (title.empty() ? string("Liga") : title) + " ---");
    lines.push_back("Pos | Equipo                 | Pts | PJ | G  | E  | P  | GF | GA | DG");
    lines.push_back("----+------------------------+-----+----+----+----+----+----+----+----");
    for (size_t i = 0; i < teams.size(); ++i) {
        Team* team = teams[i];
        int gd = team->goalsFor - team->goalsAgainst;
        int pj = team->wins + team->draws + team->losses;
        ostringstream line;
        line << setw(3) << i + 1 << " | " << setw(22) << left << team->name.substr(0, 22)
             << " | " << setw(3) << team->points
             << " | " << setw(2) << pj
             << " | " << setw(2) << team->wins
             << " | " << setw(2) << team->draws
             << " | " << setw(2) << team->losses
             << " | " << setw(2) << team->goalsFor
             << " | " << setw(2) << team->goalsAgainst
             << " | " << setw(2) << gd;
        lines.push_back(line.str());
    }
    return lines;
}

void LeagueTable::displayTable() {
    for (const auto& line : formatLines()) {
        cout << line << endl;
    }
}
