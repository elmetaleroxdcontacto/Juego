#include "ui/keyboard_shortcuts.h"

#include <windows.h>
#include <unordered_map>

namespace keyboard_shortcuts {

static std::unordered_map<int, ShortcutCallback> g_callbacks;

void KeyboardShortcutManager::initialize() {
    // Default shortcuts will be set by game on launch
}

bool KeyboardShortcutManager::handleShortcut(WPARAM key, AppState& state) {
    bool ctrlPressed = isCtrlPressed();
    
    // Ctrl+F - Global search
    if (ctrlPressed && key == 'F') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::SearchPlayers)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::SearchPlayers)](state);
            return true;
        }
    }
    
    // 'S' - Scouting mode
    if (!ctrlPressed && key == 'S') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::ScoutingMode)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::ScoutingMode)](state);
            return true;
        }
    }
    
    // 'V' - Sell/Vender
    if (!ctrlPressed && key == 'V') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::SellPlayer)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::SellPlayer)](state);
            return true;
        }
    }
    
    // 'T' - Tactics
    if (!ctrlPressed && key == 'T') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::TacticsMenu)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::TacticsMenu)](state);
            return true;
        }
    }
    
    // 'M' - Market watch
    if (!ctrlPressed && key == 'M') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::MarketWatch)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::MarketWatch)](state);
            return true;
        }
    }
    
    // 'R' - Reports
    if (!ctrlPressed && key == 'R') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::ReportsMenu)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::ReportsMenu)](state);
            return true;
        }
    }
    
    // 'B' - Staff briefing
    if (!ctrlPressed && key == 'B') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::StaffBriefing)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::StaffBriefing)](state);
            return true;
        }
    }
    
    // 'I' - Inventory
    if (!ctrlPressed && key == 'I') {
        if (g_callbacks.find(static_cast<int>(ShortcutCommand::InventoryView)) != g_callbacks.end()) {
            g_callbacks[static_cast<int>(ShortcutCommand::InventoryView)](state);
            return true;
        }
    }
    
    return false;
}

void KeyboardShortcutManager::registerCallback(ShortcutCommand cmd, ShortcutCallback callback) {
    g_callbacks[static_cast<int>(cmd)] = callback;
}

bool KeyboardShortcutManager::isCtrlPressed() {
    return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
}

bool KeyboardShortcutManager::checkShortcut(WPARAM key, ShortcutCommand expected) {
    // Helper function for future extensibility
    return true;
}

}  // namespace keyboard_shortcuts
