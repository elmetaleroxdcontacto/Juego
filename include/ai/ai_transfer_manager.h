#pragma once

#include "ai/ai_squad_planner.h"
#include "transfers/transfer_types.h"

namespace ai_transfer_manager {

ClubTransferStrategy buildClubTransferStrategy(const Career& career, const Team& team);
TransferTarget evaluateTarget(const Career& career,
                              const Team& buyer,
                              const Team& seller,
                              const Player& player,
                              const ClubTransferStrategy& strategy);

}  // namespace ai_transfer_manager
