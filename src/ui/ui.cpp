#include "ui.h"

#include "competition.h"
#include "ui/career_reports_ui.h"
#include "utils.h"

#include <iostream>

using namespace std;

void displayMainMenu() {
    cout << "\n=== Centro de juego ===" << endl;
    cout << "1. Modo Carrera" << endl;
    cout << "2. Iniciar Juego Rapido" << endl;
    cout << "3. Modo Copa" << endl;
    cout << "4. Validar sistema" << endl;
    cout << "5. Volver al menu principal" << endl;
}

void displayGameMenu() {
    cout << "\n=== Juego Rapido ===" << endl;
    cout << "1. Ver Equipo" << endl;
    cout << "2. Agregar Jugador" << endl;
    cout << "3. Entrenar Jugador" << endl;
    cout << "4. Cambiar Tacticas" << endl;
    cout << "5. Simular Partido" << endl;
    cout << "6. Cargar Equipo desde Archivo" << endl;
    cout << "7. Editar Equipo" << endl;
    cout << "8. Volver al Menu Principal" << endl;
}

void displayCareerMenu() {
    cout << "\nModo Carrera" << endl;
    cout << "1. Ver Equipo" << endl;
    cout << "2. Entrenar Jugador" << endl;
    cout << "3. Cambiar Tacticas" << endl;
    cout << "4. Simular Semana" << endl;
    cout << "5. Centro de Competicion" << endl;
    cout << "6. Ver Tabla de Posiciones" << endl;
    cout << "7. Mercado de Transferencias" << endl;
    cout << "8. Ojeo de Jugadores" << endl;
    cout << "9. Ver Estadisticas" << endl;
    cout << "10. Objetivos y Directiva" << endl;
    cout << "11. Noticias" << endl;
    cout << "12. Historial de Temporadas" << endl;
    cout << "13. Alineacion y Rotacion" << endl;
    cout << "14. Club y Finanzas" << endl;
    cout << "15. Ver Logros" << endl;
    cout << "16. Guardar Carrera" << endl;
    cout << "17. Retirar Jugador" << endl;
    cout << "18. Plan de Entrenamiento" << endl;
    cout << "19. Editar Equipo" << endl;
    cout << "20. Volver al Menu Principal" << endl;
}

void displayAchievementsMenu(Career& career) {
    cout << "\n=== Logros ===" << endl;
    if (career.achievements.empty()) {
        cout << "No hay logros desbloqueados." << endl;
        return;
    }
    for (size_t i = 0; i < career.achievements.size(); ++i) {
        cout << i + 1 << ". " << career.achievements[i] << endl;
    }
    int choice = readInt("Resetear logros? (1. Si, 2. No): ", 1, 2);
    if (choice == 1) {
        career.achievements.clear();
        cout << "Logros reiniciados." << endl;
    }
}

static LeagueTable buildTableFromTeams(const vector<Team*>& teams, const string& title, const string& ruleId) {
    LeagueTable table;
    table.title = title;
    table.ruleId = ruleId;
    for (auto* team : teams) {
        if (team) table.addTeam(team);
    }
    table.sortTable();
    return table;
}

static LeagueTable buildGroupTable(const Career& career, const vector<int>& idx, const string& title) {
    vector<Team*> teams;
    for (int i : idx) {
        if (i >= 0 && i < static_cast<int>(career.activeTeams.size())) {
            teams.push_back(career.activeTeams[i]);
        }
    }
    return buildTableFromTeams(teams, title, career.activeDivision);
}


static string regionalGroupTitle(const Career& career, bool north) {
    return competitionGroupTitle(career.activeDivision, north);
}

void displayLeagueTables(Career& career) {
    if (!career.usesGroupFormat() || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.leagueTable.displayTable();
        return;
    }
    LeagueTable north = buildGroupTable(career, career.groupNorthIdx, regionalGroupTitle(career, true));
    LeagueTable south = buildGroupTable(career, career.groupSouthIdx, regionalGroupTitle(career, false));
    north.displayTable();
    south.displayTable();
}

void displayCompetitionCenter(Career& career) {
    ui_reports::displayCompetitionCenter(career);
}

void displayBoardStatus(Career& career) {
    ui_reports::displayBoardStatus(career);
}

void displayNewsFeed(Career& career) {
    ui_reports::displayNewsFeed(career);
}

void displaySeasonHistory(const Career& career) {
    ui_reports::displaySeasonHistory(career);
}

void displayClubOperations(Career& career) {
    ui_reports::displayClubOperations(career);
}






