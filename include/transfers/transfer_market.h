#pragma once

#include "engine/models.h"

#include <string>

namespace transfer_market {

std::string weakestSquadPosition(const Team& team);
void processCpuTransfers(Career& career);
void processLoanReturns(Career& career);

}  // namespace transfer_market
