#pragma once

#include "models.h"

#include <string>
#include <vector>

TeamStrength computeStrength(Team& team);
bool hasInjuredInXI(const Team& team, const std::vector<int>& xi);
void applyTactics(const std::string& tactics, int& attack, int& defense);
double calcLambda(int attack, int defense);
int samplePoisson(double lambda);
bool simulateInjury(Player& player, const std::string& tactics, bool verbose, std::vector<std::string>* events);
void healInjuries(Team& team, bool verbose);
void recoverFitness(Team& team, int days);
void assignGoalsAndAssists(Team& team, int goals, const std::vector<int>& xi, const std::string& teamName, std::vector<std::string>* events);
MatchResult playMatch(Team& home, Team& away, bool verbose, bool keyMatch = false);
