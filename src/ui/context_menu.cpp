#include "ui/context_menu.h"

#include "engine/models.h"

#include <sstream>

namespace context_menu {

// Static members initialization
MenuResult ContextMenuSystem::g_lastResult;
bool ContextMenuSystem::g_menuOpen = false;
int ContextMenuSystem::g_rightClickX = 0;
int ContextMenuSystem::g_rightClickY = 0;

void ContextMenuSystem::showPlayerMenu(AppState& state, Team& team, int playerIndex) {
    (void)state;
    if (playerIndex < 0 || playerIndex >= static_cast<int>(team.players.size())) {
        return;
    }
    
    g_menuOpen = true;
    g_lastResult.playerIndex = playerIndex;
    
    // In a full GUI implementation, this would show a context menu
    // For now, we're preparing the infrastructure for integration
}

MenuResult ContextMenuSystem::getMenuResult() {
    return g_lastResult;
}

bool ContextMenuSystem::isMenuOpen() {
    return g_menuOpen;
}

void ContextMenuSystem::onSellClick(AppState& state, Player& player, Team& team) {
    (void)state;
    (void)team;
    // Trigger transfer market logic for selling this player
    player.wantsToLeave = true;
    g_menuOpen = false;
}

void ContextMenuSystem::onRenewClick(AppState& state, Player& player) {
    (void)state;
    // Extend player contract
    if (player.contractWeeks > 0) {
        player.contractWeeks = std::max(player.contractWeeks, 52);  // At least 1 more year
    }
    g_menuOpen = false;
}

void ContextMenuSystem::onTrainClick(AppState& state, Player& player) {
    (void)state;
    // Improve player skill through training focus
    player.skill = std::min(99, player.skill + 1);
    player.leadership = std::min(99, player.leadership + 1);
    g_menuOpen = false;
}

void ContextMenuSystem::onRestClick(AppState& state, Player& player) {
    (void)state;
    // Give player rest to recover physical condition
    player.fitness = std::min(100, player.fitness + 10);
    player.fatigueLoad = std::max(0, player.fatigueLoad - 5);
    g_menuOpen = false;
}

void ContextMenuSystem::onCaptainClick(AppState& state, Team& team, int playerIndex) {
    (void)state;
    // Toggle captain status
    if (playerIndex >= 0 && playerIndex < static_cast<int>(team.players.size())) {
        Player& player = team.players[static_cast<size_t>(playerIndex)];
        
        // Remove captain from anyone else
        for (auto& p : team.players) {
            if (p.role == "Capitán") {
                p.role = "Regular";
                break;
            }
        }
        
        // Set new captain (toggle if already captain)
        if (player.role == "Capitán") {
            player.role = "Regular";
        } else {
            player.role = "Capitán";
            player.leadership = std::min(99, player.leadership + 2);  // Boost leadership
        }
    }
    g_menuOpen = false;
}

}  // namespace context_menu
