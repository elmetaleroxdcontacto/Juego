#pragma once

#include <deque>
#include <string>
#include <utility>
#include <vector>

struct Player {
    std::string name;
    std::string position;
    int attack;
    int defense;
    int stamina;
    int fitness;
    int skill;
    int potential;
    int age;
    long long value;
    bool injured;
    std::string injuryType;
    int injuryWeeks;
    int goals;
    int assists;
    int matchesPlayed;
    int lastTrainedSeason;
    int lastTrainedWeek;
};

void applyPositionStats(Player& p);
Player makeRandomPlayer(const std::string& position, int skillMin, int skillMax, int ageMin, int ageMax);

class Team {
public:
    std::string name;
    std::string division;
    std::vector<Player> players;
    std::string tactics;
    std::string formation;
    long long budget;
    int morale;
    int points;
    int goalsFor;
    int goalsAgainst;
    int wins;
    int draws;
    int losses;
    std::vector<std::string> achievements;

    Team(std::string n = "");

    void addPlayer(const Player& p);

    std::vector<int> getStartingXIIndices() const;
    int getTotalAttack() const;
    int getTotalDefense() const;
    int getAverageSkill() const;
    int getAverageStamina() const;
    long long getSquadValue() const;

    void resetSeasonStats();
};

struct LeagueTable {
    std::vector<Team*> teams;
    std::string title;

    void clear();
    void addTeam(Team* team);
    void sortTable();
    void displayTable();
};

struct DivisionInfo {
    std::string id;
    std::string folder;
    std::string display;
};

extern const std::vector<DivisionInfo> kDivisions;

struct Career {
    Team* myTeam;
    LeagueTable leagueTable;
    int currentSeason;
    int currentWeek;
    std::deque<Team> allTeams;
    std::vector<Team*> activeTeams;
    std::vector<std::vector<std::pair<int, int>>> schedule;
    std::vector<DivisionInfo> divisions;
    std::string activeDivision;
    std::string saveFile;
    std::vector<std::string> achievements;
    bool initialized;

    Career();

    void initializeLeague(bool forceReload = false);
    std::vector<Team*> getDivisionTeams(const std::string& id);
    void buildSchedule();
    void setActiveDivision(const std::string& id);
    void resetSeason();
    void agePlayers();
    void saveCareer();
    bool loadCareer();
};

struct MatchResult {
    int homeGoals;
    int awayGoals;
};

struct TeamStrength {
    std::vector<int> xi;
    int attack;
    int defense;
    int avgSkill;
    int avgStamina;
};
