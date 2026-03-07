#pragma once

#include "engine/models.h"

#include <string>
#include <vector>

std::string boardStatusLabel(int confidence);
std::string managerStyleLabel(const Team& team);

std::vector<Team*> buildJobMarket(const Career& career, bool emergency = false);
void takeManagerJob(Career& career, Team* newClub, const std::string& reason);

const Team* nextOpponent(const Career& career);
int averageFitnessForLine(const Team& team, const std::string& line);
int lineThreatScore(const Team& team, const std::string& line);
std::string lineMap(const Team& team);
std::string buildOpponentReport(const Career& career);
