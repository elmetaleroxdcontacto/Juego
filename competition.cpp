#include "competition.h"

#include <vector>

using namespace std;

namespace {

const CompetitionConfig kDefaultCompetitionConfig = {
    "",
    CompetitionTableProfile::Default,
    CompetitionSeasonHandler::Generic,
    {},
    20000,
    45,
    6,
    0,
    0
};

const vector<CompetitionConfig> kCompetitionConfigs = {
    {
        "primera division",
        CompetitionTableProfile::PrimeraLike,
        CompetitionSeasonHandler::PrimeraDivision,
        {},
        60000,
        85,
        3,
        0,
        16
    },
    {
        "primera b",
        CompetitionTableProfile::PrimeraLike,
        CompetitionSeasonHandler::PrimeraB,
        {},
        45000,
        70,
        4,
        0,
        16
    },
    {
        "segunda division",
        CompetitionTableProfile::Default,
        CompetitionSeasonHandler::SegundaGroups,
        {true, 7, true, "Grupo Norte", "Grupo Sur"},
        35000,
        60,
        5,
        0,
        14
    },
    {
        "tercera division a",
        CompetitionTableProfile::TerceraA,
        CompetitionSeasonHandler::TerceraA,
        {},
        25000,
        50,
        6,
        30,
        16
    },
    {
        "tercera division b",
        CompetitionTableProfile::TerceraB,
        CompetitionSeasonHandler::TerceraB,
        {true, 14, false, "Zona Norte", "Zona Sur"},
        20000,
        45,
        6,
        30,
        28
    }
};

}  // namespace

const CompetitionConfig& getCompetitionConfig(const string& id) {
    for (const auto& config : kCompetitionConfigs) {
        if (config.id == id) return config;
    }
    return kDefaultCompetitionConfig;
}

bool competitionUsesGroupStage(const string& id, int teamCount) {
    const CompetitionConfig& config = getCompetitionConfig(id);
    if (!config.groups.enabled) return false;
    return config.expectedTeamCount <= 0 || teamCount == config.expectedTeamCount;
}

string competitionGroupTitle(const string& id, bool north) {
    const CompetitionConfig& config = getCompetitionConfig(id);
    if (!config.groups.enabled) return north ? "Grupo Norte" : "Grupo Sur";
    return north ? config.groups.northTitle : config.groups.southTitle;
}
