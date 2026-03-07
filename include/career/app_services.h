#pragma once

#include "engine/models.h"
#include "validators/validators.h"

#include <string>
#include <vector>

struct ServiceResult {
    bool ok = false;
    std::vector<std::string> messages;
};

enum class ClubUpgrade {
    Stadium,
    Youth,
    Training,
    Scouting,
    Medical
};

enum class NegotiationProfile {
    Safe,
    Balanced,
    Aggressive
};

enum class NegotiationPromise {
    None,
    Starter,
    Rotation,
    Prospect
};

ServiceResult startCareerService(Career& career,
                                 const std::string& divisionId,
                                 const std::string& teamName,
                                 const std::string& managerName);
ServiceResult loadCareerService(Career& career);
ServiceResult saveCareerService(Career& career);
ServiceResult simulateCareerWeekService(Career& career);
ServiceResult scoutPlayersService(Career& career,
                                  const std::string& region = "Todas",
                                  const std::string& focusPos = "");
ServiceResult upgradeClubService(Career& career, ClubUpgrade upgrade);
ServiceResult buyTransferTargetService(Career& career,
                                       const std::string& sellerTeamName,
                                       const std::string& playerName,
                                       NegotiationProfile profile = NegotiationProfile::Balanced,
                                       NegotiationPromise promise = NegotiationPromise::None);
ServiceResult signPreContractService(Career& career,
                                     const std::string& sellerTeamName,
                                     const std::string& playerName,
                                     NegotiationProfile profile = NegotiationProfile::Balanced,
                                     NegotiationPromise promise = NegotiationPromise::None);
ServiceResult renewPlayerContractService(Career& career,
                                         const std::string& playerName,
                                         NegotiationProfile profile = NegotiationProfile::Balanced,
                                         NegotiationPromise promise = NegotiationPromise::None);
ServiceResult sellPlayerService(Career& career, const std::string& playerName);
ServiceResult cyclePlayerDevelopmentPlanService(Career& career, const std::string& playerName);
ServiceResult cycleMatchInstructionService(Career& career);
ServiceResult shortlistPlayerService(Career& career,
                                     const std::string& sellerTeamName,
                                     const std::string& playerName);
ServiceResult followShortlistService(Career& career);
std::string buildCompetitionSummaryService(const Career& career);
std::string buildBoardSummaryService(const Career& career);
std::string buildClubSummaryService(const Career& career);
std::string buildScoutingSummaryService(const Career& career);
ValidationSuiteSummary runValidationService();
