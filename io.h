#pragma once

#include "models.h"

#include <deque>
#include <string>
#include <vector>

void assignMissingPositions(Team& team);
void trimSquadForDivision(Team& team);
bool loadTeamFromCsv(const std::string& filename, Team& team);
bool loadTeamFromPlayersTxt(const std::string& filename, Team& team);
bool loadTeamFromLegacyTxt(const std::string& filename, Team& team);
bool loadTeamFromFile(const std::string& filename, Team& team);
void ensureMinimumSquad(Team& team, int minPlayers);
std::vector<Team*> loadDivisionFromFolder(const std::string& folder, const std::string& divisionId, std::deque<Team>& allTeams);
