#pragma once

#include <vector>
#include <string>

struct AppState;
struct Team;
struct Player;

namespace context_menu {

// Menu action types for player context menu
enum class PlayerAction {
    Sell,           // Vender jugador
    Renew,          // Renovar contrato
    Loan,           // Prestar jugador
    Train,          // Entrenar (mejorar habilidad)
    Rest,           // Descansar (recuperar estado físico)
    Scout,          // Explorar opciones de fichaje
    ViewStats,      // Ver estadísticas detalladas
    SetCaptain,     // Designar capitán
    RemoveCaptain   // Quitar capitán
};

// Context menu result
struct MenuResult {
    bool selected = false;
    PlayerAction action = PlayerAction::ViewStats;
    int playerIndex = -1;
};

class ContextMenuSystem {
public:
    static void showPlayerMenu(AppState& state, Team& team, int playerIndex);
    static MenuResult getMenuResult();
    static bool isMenuOpen();
    
    static void onSellClick(AppState& state, Player& player, Team& team);
    static void onRenewClick(AppState& state, Player& player);
    static void onTrainClick(AppState& state, Player& player);
    static void onRestClick(AppState& state, Player& player);
    static void onCaptainClick(AppState& state, Team& team, int playerIndex);
    
private:
    static MenuResult g_lastResult;
    static bool g_menuOpen;
    static int g_rightClickX;
    static int g_rightClickY;
};

}  // namespace context_menu
