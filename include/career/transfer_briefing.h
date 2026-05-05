#pragma once

#include "engine/models.h"
#include "transfers/transfer_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace transfer_briefing {

struct TransferOptionBrief {
    std::string sellerName;
    std::string playerName;
    std::string position;
    int skill = 0;
    int potential = 0;
    std::string skillLabel;
    std::string potentialLabel;
    int age = 0;
    int contractWeeks = 0;
    long long marketValue = 0;
    long long releaseClause = 0;
    long long wage = 0;
    std::string marketValueLabel;
    std::string wageLabel;
    bool availableForLoan = false;
    bool contractRunningOut = false;
    bool onShortlist = false;
    int scoutingConfidence = 0;
    int readinessScore = 0;
    int medicalRisk = 0;
    std::string competitionLabel;
    std::string actionLabel;
    std::string packageLabel;
    std::string scoutingNote;
    double totalScore = 0.0;
};

std::string scoutingAttributeLabel(int value, int scoutingConfidence);
std::string scoutingMoneyLabel(long long value, int scoutingConfidence);

std::vector<TransferOptionBrief> buildTransferOptions(const Career& career,
                                                      const std::string& filterPos = "",
                                                      bool includeShortSquads = false,
                                                      std::size_t limit = 25);
std::vector<TransferOptionBrief> buildPreContractOptions(const Career& career,
                                                         const std::string& filterPos = "",
                                                         std::size_t limit = 12);
std::vector<TransferOptionBrief> buildLoanOptions(const Career& career,
                                                  const std::string& filterPos = "",
                                                  std::size_t limit = 12);

std::vector<std::string> buildMarketPulseLines(const Career& career, std::size_t limit = 4);
std::vector<std::string> buildTransferOpportunityLines(const Career& career,
                                                       const std::string& filterPos = "",
                                                       std::size_t limit = 3);

}  // namespace transfer_briefing
