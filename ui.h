#pragma once

#include "models.h"

struct IncomingOfferDecision {
    int action = 3;
    long long counterOffer = 0;
};

using ManagerJobSelectionCallback = int (*)(const Career& career, const std::vector<Team*>& jobs);
using UiMessageCallback = void (*)(const std::string& message);
using IncomingOfferDecisionCallback = IncomingOfferDecision (*)(const Career& career,
                                                                const Player& player,
                                                                long long offer,
                                                                long long maxOffer);
using ContractRenewalDecisionCallback = bool (*)(const Career& career,
                                                 const Team& team,
                                                 const Player& player,
                                                 long long demandedWage,
                                                 int demandedWeeks,
                                                 long long demandedClause);

void setManagerJobSelectionCallback(ManagerJobSelectionCallback callback);
void setUiMessageCallback(UiMessageCallback callback);
void setIncomingOfferDecisionCallback(IncomingOfferDecisionCallback callback);
void setContractRenewalDecisionCallback(ContractRenewalDecisionCallback callback);

void transferMarket(Career& career);
void scoutPlayers(Career& career);
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
void manageLineup(Team& team);
void displayStatistics(Team& team);
void displayCompetitionCenter(Career& career);
void displayBoardStatus(Career& career);
void displayNewsFeed(const Career& career);
void displaySeasonHistory(const Career& career);
void displayClubOperations(Career& career);
void displayAchievementsMenu(Career& career);
void displayLeagueTables(Career& career);
void endSeason(Career& career);
void simulateCareerWeek(Career& career);
void playCupMode(Career& career);
