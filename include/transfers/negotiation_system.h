#pragma once

#include "career/app_services.h"
#include "engine/models.h"

#include <string>
#include <vector>

double negotiationFeeFactor(NegotiationProfile profile);
double negotiationWageFactor(NegotiationProfile profile);
double negotiationClauseFactor(NegotiationProfile profile);
std::string negotiationLabel(NegotiationProfile profile);

std::string promiseLabel(NegotiationPromise promise);
double promiseWageFactor(NegotiationPromise promise);
int promiseContractWeeks(NegotiationPromise promise, int currentWeeks);
int desiredStartsForPromise(NegotiationPromise promise, const Player& player);

std::string personalityLabel(const Player& player);
std::vector<const Player*> dressingRoomLeaders(const Team& team);
std::string dressingRoomClimate(const Team& team);

bool promiseAtRisk(const Player& player, int currentWeek);
int promisesAtRisk(const Team& team, int currentWeek);
void applyNegotiatedPromise(Player& player, NegotiationPromise promise);

long long estimatedAgentFee(const Player& player, long long transferFee);
long long wageDemandFor(const Player& player);
int agentDifficulty(const Player& player);
int squadCompetitionAtRole(const Team& team, const Player& target);
long long rivalrySurcharge(const Team& buyer, const Team& seller, long long baseFee);
int clubAppealScore(const Career& career, const Team& team);
bool playerRejectsMove(const Career& career,
                       const Team& buyer,
                       const Team& seller,
                       const Player& player,
                       NegotiationPromise promise,
                       std::string& reason);
bool renewalNeedsStrongerPromise(const Player& player, NegotiationPromise promise, int currentWeek);
