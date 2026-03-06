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
    long long wage;
    long long releaseClause;
    int setPieceSkill;
    int leadership;
    int professionalism;
    int ambition;
    int happiness;
    int chemistry;
    int desiredStarts;
    int startsThisSeason;
    bool wantsToLeave;
    bool onLoan;
    std::string parentClub;
    int loanWeeksRemaining;
    int contractWeeks;
    bool injured;
    std::string injuryType;
    int injuryWeeks;
    int injuryHistory;
    int yellowAccumulation;
    int seasonYellowCards;
    int seasonRedCards;
    int matchesSuspended;
    int goals;
    int assists;
    int matchesPlayed;
    int lastTrainedSeason;
    int lastTrainedWeek;
    std::string role;
};

struct HeadToHeadRecord {
    std::string opponent;
    int points;
};

struct SeasonHistoryEntry {
    int season;
    std::string division;
    std::string club;
    int finish;
    std::string champion;
    std::string promoted;
    std::string relegated;
    std::string note;
};

struct PendingTransfer {
    std::string playerName;
    std::string fromTeam;
    std::string toTeam;
    int effectiveSeason;
    int loanWeeks;
    long long fee;
    long long wage;
    int contractWeeks;
    bool preContract;
    bool loan;
};

void applyPositionStats(Player& p);
std::string defaultRoleForPosition(const std::string& position);
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
    std::string trainingFocus;
    int points;
    int goalsFor;
    int goalsAgainst;
    int awayGoals;
    int wins;
    int draws;
    int losses;
    int yellowCards;
    int redCards;
    int tiebreakerSeed;
    int pressingIntensity;
    int defensiveLine;
    int tempo;
    int width;
    std::string markingStyle;
    std::vector<std::string> preferredXI;
    std::vector<std::string> preferredBench;
    std::string captain;
    std::string penaltyTaker;
    std::string freeKickTaker;
    std::string cornerTaker;
    std::string rotationPolicy;
    int assistantCoach;
    int fitnessCoach;
    int scoutingChief;
    int youthCoach;
    int medicalTeam;
    std::string youthRegion;
    long long debt;
    long long sponsorWeekly;
    int stadiumLevel;
    int youthFacilityLevel;
    int trainingFacilityLevel;
    int fanBase;
    std::vector<HeadToHeadRecord> headToHead;
    std::vector<std::string> achievements;

    Team(std::string n = "");

    void addPlayer(const Player& p);
    void addHeadToHeadPoints(const std::string& opponent, int points);
    int headToHeadPointsAgainst(const std::string& opponent) const;

    std::vector<int> getStartingXIIndices() const;
    std::vector<int> getBenchIndices(int count = 7) const;
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
    std::string ruleId;

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
    std::vector<int> groupNorthIdx;
    std::vector<int> groupSouthIdx;
    std::vector<DivisionInfo> divisions;
    std::string activeDivision;
    std::string saveFile;
    std::vector<std::string> achievements;
    std::string managerName;
    int managerReputation;
    int boardConfidence;
    int boardExpectedFinish;
    long long boardBudgetTarget;
    int boardYouthTarget;
    int boardWarningWeeks;
    std::string boardMonthlyObjective;
    int boardMonthlyTarget;
    int boardMonthlyProgress;
    int boardMonthlyDeadlineWeek;
    std::vector<std::string> newsFeed;
    std::vector<std::string> scoutInbox;
    std::vector<SeasonHistoryEntry> history;
    std::vector<PendingTransfer> pendingTransfers;
    bool cupActive;
    int cupRound;
    std::vector<std::string> cupRemainingTeams;
    std::string cupChampion;
    std::string lastMatchAnalysis;
    bool initialized;

    Career();

    bool usesSegundaFormat() const;
    bool usesTerceraBFormat() const;
    bool usesGroupFormat() const;
    void buildSegundaGroups();
    void buildRegionalGroups();
    void initializeLeague(bool forceReload = false);
    std::vector<Team*> getDivisionTeams(const std::string& id);
    void buildSchedule();
    void setActiveDivision(const std::string& id);
    void resetSeason();
    void agePlayers();
    Team* findTeamByName(const std::string& name);
    const Team* findTeamByName(const std::string& name) const;
    void addNews(const std::string& item);
    void executePendingTransfers();
    void initializeSeasonCup();
    void initializeDynamicObjective();
    void updateDynamicObjectiveStatus();
    int currentCompetitiveRank() const;
    int currentCompetitiveFieldSize() const;
    void initializeBoardObjectives();
    void updateBoardConfidence();
    void saveCareer();
    bool loadCareer();
};

struct MatchResult {
    int homeGoals;
    int awayGoals;
    int homeShots;
    int awayShots;
    int homePossession;
    int awayPossession;
    int homeSubstitutions;
    int awaySubstitutions;
};

struct TeamStrength {
    std::vector<int> xi;
    int attack;
    int defense;
    int avgSkill;
    int avgStamina;
};
