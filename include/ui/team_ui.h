#pragma once

#include "engine/models.h"

void retirePlayer(Team& team);
void viewTeam(Team& team);
void addPlayer(Team& team);
void trainPlayer(Team& team, int season = -1, int week = -1);
void changeTactics(Team& team);
void manageLineup(Career& career);
void setTrainingPlan(Team& team);
void editTeam(Team& team);
void displayStatistics(Team& team);
