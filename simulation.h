#pragma once

#include "models.h"

#include <string>
#include <vector>

TeamStrength computeStrength(Team& team);
bool hasInjuredInXI(const Team& team, const std::vector<int>& xi);
void applyTactics(const std::string& tactics, int& attack, int& defense);
double calcLambda(int attack, int defense);
int samplePoisson(double lambda);
void simulateInjury(Player& player, bool verbose);
void healInjuries(Team& team, bool verbose);
void assignGoalsAndAssists(Team& team, int goals, const std::vector<int>& xi);
MatchResult playMatch(Team& home, Team& away, bool verbose);
