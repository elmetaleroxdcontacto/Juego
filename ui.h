#pragma once

#include "models.h"

void transferMarket(Team& team);
void scoutPlayers(Team& team);
void retirePlayer(Team& team);
void checkAchievements(Career& career);
void displayMainMenu();
void displayGameMenu();
void displayCareerMenu();
void viewTeam(Team& team);
void addPlayer(Team& team);
void trainPlayer(Team& team, int season = -1, int week = -1);
void changeTactics(Team& team);
void editTeam(Team& team);
void setTrainingPlan(Team& team);
void displayStatistics(Team& team);
void displayAchievementsMenu(Career& career);
void endSeason(Career& career);
void simulateCareerWeek(Career& career);
void playCupMode(Career& career);
