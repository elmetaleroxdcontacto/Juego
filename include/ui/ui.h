#pragma once

#include "career/career_runtime.h"
#include "career/week_simulation.h"
#include "engine/models.h"
#include "ui/team_ui.h"

void transferMarket(Career& career);
void scoutPlayers(Career& career);
void displayMainMenu();
void displayGameMenu();
void displayCareerMenu();
void displayCompetitionCenter(Career& career);
void displayBoardStatus(Career& career);
void displayNewsFeed(Career& career);
void displaySeasonHistory(const Career& career);
void displayClubOperations(Career& career);
void displayAchievementsMenu(Career& career);
void displayLeagueTables(Career& career);
void playCupMode(Career& career);
