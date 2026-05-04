#pragma once

#include "engine/models.h"

#include <deque>
#include <string>
#include <vector>

void assignMissingPositions(Team& team);
void trimSquadForDivision(Team& team);
bool loadTeamFromCsv(const std::string& filename, Team& team);
bool loadTeamFromJson(const std::string& filename, Team& team);
bool loadTeamFromPlayersTxt(const std::string& filename, Team& team);
bool loadTeamFromLegacyTxt(const std::string& filename, Team& team);
bool loadTeamFromFile(const std::string& filename, Team& team);
void ensureMinimumSquad(Team& team, int minPlayers);

struct DivisionLoadResult {
    std::vector<Team*> teams;
    std::vector<std::string> warnings;
};

struct ExternalJsonLoadResult {
    std::vector<DivisionInfo> divisions;
    std::vector<std::string> warnings;
    int teamsLoaded = 0;
    int playersLoaded = 0;
};

DivisionLoadResult loadDivisionFromFolder(const std::string& folder,
                                          const std::string& divisionId,
                                          std::deque<Team>& allTeams);

ExternalJsonLoadResult loadExternalJsonData(std::deque<Team>& allTeams);
