#pragma once

#include "engine/models.h"
#include "transfers/transfer_types.h"

#include <string>
#include <vector>

namespace transfer_market {

bool isTransferWindowOpen(const Career& career);
int nextTransferWindowWeek(const Career& career);
std::string transferWindowLabel(const Career& career);
std::vector<std::string> buildTransferWindowLines(const Career& career,
                                                  std::size_t limit = 3);
std::string weakestSquadPosition(const Team& team);
std::vector<TransferTarget> buildTransferShortlist(const Career& career,
                                                   const Team& team,
                                                   std::size_t maxTargets = 5);
void processCpuTransfers(Career& career);
void processLoanReturns(Career& career);

}  // namespace transfer_market
