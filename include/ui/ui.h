#pragma once

#include "career/career_runtime.h"
#include "career/week_simulation.h"
#include "engine/models.h"

void transferMarket(Career& career);
void scoutPlayers(Career& career);
void retirePlayer(Team& team);
void displayMainMenu();
void displayGameMenu();
void displayCareerMenu();
void viewTeam(Team& team);
void addPlayer(Team& team);
void trainPlayer(Team& team, int season = -1, int week = -1);
void changeTactics(Team& team);
void editTeam(Team& team);
void setTrainingPlan(Team& team);
void manageLineup(Team& team);
void displayStatistics(Team& team);
void displayCompetitionCenter(Career& career);
void displayBoardStatus(Career& career);
void displayNewsFeed(const Career& career);
void displaySeasonHistory(const Career& career);
void displayClubOperations(Career& career);
void displayAchievementsMenu(Career& career);
void displayLeagueTables(Career& career);
void playCupMode(Career& career);
