#pragma once

#include <functional>
#include <windows.h>

struct AppState;

namespace keyboard_shortcuts {

// Callback types for shortcut actions
using ShortcutCallback = std::function<void(AppState&)>;

// Shortcut command types
enum class ShortcutCommand {
    SearchPlayers,     // Ctrl+F - Open global player search
    ScoutingMode,      // 'S' - Open scouting assignment
    SellPlayer,        // 'V' - Sell/transfer player
    TacticsMenu,       // 'T' - Tactics configuration
    MarketWatch,       // 'M' - Transfer market watch
    ReportsMenu,       // 'R' - View career reports
    StaffBriefing,     // 'B' - Staff briefing
    InventoryView      // 'I' - Inventory/assets view
};

class KeyboardShortcutManager {
public:
    static void initialize();
    static bool handleShortcut(WPARAM key, AppState& state);
    static void registerCallback(ShortcutCommand cmd, ShortcutCallback callback);
    static bool isCtrlPressed();
    
private:
    static bool checkShortcut(WPARAM key, ShortcutCommand expected);
};

}  // namespace keyboard_shortcuts
