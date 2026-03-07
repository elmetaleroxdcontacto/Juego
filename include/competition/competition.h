#pragma once

#include <string>

enum class CompetitionTableProfile {
    Default,
    PrimeraLike,
    TerceraA,
    TerceraB
};

enum class CompetitionSeasonHandler {
    Generic,
    PrimeraDivision,
    PrimeraB,
    SegundaGroups,
    TerceraA,
    TerceraB
};

struct CompetitionGroupConfig {
    bool enabled = false;
    int groupSize = 0;
    bool doubleRoundRobin = true;
    std::string northTitle = "Grupo Norte";
    std::string southTitle = "Grupo Sur";
};

struct CompetitionConfig {
    std::string id;
    CompetitionTableProfile tableProfile = CompetitionTableProfile::Default;
    CompetitionSeasonHandler seasonHandler = CompetitionSeasonHandler::Generic;
    CompetitionGroupConfig groups;
    long long baseIncome = 20000;
    int wageFactor = 45;
    int budgetDivisor = 6;
    int maxSquadSize = 0;
    int expectedTeamCount = 0;
};

const CompetitionConfig& getCompetitionConfig(const std::string& id);
bool competitionUsesGroupStage(const std::string& id, int teamCount);
std::string competitionGroupTitle(const std::string& id, bool north);
