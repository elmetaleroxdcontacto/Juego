#pragma once

#include <string>
#include <vector>

struct TransferTarget {
    std::string clubName;
    std::string playerName;
    std::string position;
    int qualityScore = 0;
    int potentialScore = 0;
    long long expectedFee = 0;
    long long expectedWage = 0;
    int age = 0;
    bool availableForLoan = false;
    bool contractRunningOut = false;
    int squadNeedScore = 0;
    int fitScore = 0;
    int affordabilityScore = 0;
    int scoutingConfidence = 0;
    int competitionScore = 0;
    bool onShortlist = false;
    bool urgentNeed = false;
    std::string scoutingNote;
    double totalScore = 0.0;
};

struct TransferOffer {
    long long transferFee = 0;
    long long signingBonus = 0;
    long long sellOnPercentage = 0;
    long long appearanceBonus = 0;
    bool loan = false;
    int loanWeeks = 0;
    bool triggerReleaseClause = false;
};

struct ContractOffer {
    long long wage = 0;
    long long signingBonus = 0;
    long long releaseClause = 0;
    int contractWeeks = 0;
    std::string promisedRole;
};

struct NegotiationState {
    int round = 0;
    bool clubAccepted = false;
    bool playerAccepted = false;
    bool competingClubPresent = false;
    std::string status;
    long long sellerExpectation = 0;
    long long playerDemand = 0;
    long long latestCounter = 0;
    long long agreedFee = 0;
    long long agreedWage = 0;
    long long agreedBonus = 0;
    long long agreedClause = 0;
    int agreedContractWeeks = 0;
    std::string agreedPromisedRole;
    std::vector<std::string> roundSummaries;
};

struct ClubTransferStrategy {
    std::string weakestPosition;
    std::string surplusPosition;
    bool needsLiquidity = false;
    bool youthFocus = false;
    bool promotionPush = false;
    bool needsStarter = false;
    bool trustYouthCover = false;
    int rotationRisk = 0;
    int salePressure = 0;
    int maxTargets = 0;
    long long maxTransferBudget = 0;
    long long maxWageBudget = 0;
    std::vector<std::string> priorityPositions;
    std::vector<std::string> thinPositions;
    std::vector<std::string> youthCoverPositions;
    std::vector<std::string> saleCandidates;
};
