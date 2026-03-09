#pragma once

#include "engine/models.h"
#include "validators/validators.h"

#include <string>
#include <vector>

struct ServiceResult {
    bool ok = false;
    std::vector<std::string> messages;
};

struct ScoutingCandidate {
    std::string playerName;
    std::string clubName;
    std::string region;
    std::string position;
    std::string preferredFoot;
    std::string fitLabel;
    std::string formLabel;
    std::string reliabilityLabel;
    std::string personalityLabel;
    std::vector<std::string> secondaryPositions;
    std::vector<std::string> traits;
    int estimatedSkillMin = 0;
    int estimatedSkillMax = 0;
    int estimatedPotentialMin = 0;
    int estimatedPotentialMax = 0;
    int fitScore = 0;
    int bigMatches = 0;
    long long marketValue = 0;
};

struct ScoutingSessionResult {
    ServiceResult service;
    std::string resolvedRegion;
    std::string resolvedFocusPosition;
    long long scoutingCost = 0;
    std::vector<ScoutingCandidate> candidates;
};

enum class ClubUpgrade {
    Stadium,
    Youth,
    Training,
    Scouting,
    Medical,
    AssistantCoach,
    FitnessCoach,
    YouthCoach
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
ScoutingSessionResult runScoutingSessionService(Career& career,
                                                const std::string& region = "Todas",
                                                const std::string& focusPos = "");
ServiceResult scoutPlayersService(Career& career,
                                  const std::string& region = "Todas",
                                  const std::string& focusPos = "");
ServiceResult upgradeClubService(Career& career, ClubUpgrade upgrade);
ServiceResult changeYouthRegionService(Career& career, const std::string& region);
ServiceResult takeManagerJobService(Career& career,
                                    const std::string& teamName,
                                    const std::string& reason = "Cambio de club voluntario.");
ServiceResult buyTransferTargetService(Career& career,
                                       const std::string& sellerTeamName,
                                       const std::string& playerName,
                                       NegotiationProfile profile = NegotiationProfile::Balanced,
                                       NegotiationPromise promise = NegotiationPromise::None);
ServiceResult triggerReleaseClauseService(Career& career,
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
ServiceResult loanInPlayerService(Career& career,
                                  const std::string& sellerTeamName,
                                  const std::string& playerName,
                                  int loanWeeks);
ServiceResult loanOutPlayerService(Career& career,
                                   const std::string& playerName,
                                   const std::string& destinationTeamName,
                                   int loanWeeks);
ServiceResult cyclePlayerDevelopmentPlanService(Career& career, const std::string& playerName);
ServiceResult cycleMatchInstructionService(Career& career);
ServiceResult shortlistPlayerService(Career& career,
                                     const std::string& sellerTeamName,
                                     const std::string& playerName);
ServiceResult followShortlistService(Career& career);
std::vector<std::string> listYouthRegionsService();
std::string buildCompetitionSummaryService(const Career& career);
std::string buildBoardSummaryService(const Career& career);
std::string buildClubSummaryService(const Career& career);
std::string buildScoutingSummaryService(const Career& career);
ValidationSuiteSummary runValidationService();
