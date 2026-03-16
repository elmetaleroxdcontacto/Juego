#include "engine/game_controller.h"

#include "gui.h"
#include "io.h"
#include "ui.h"
#include "utils.h"
#include "validators.h"

#include <clocale>
#include <functional>
#include <iostream>
#include <limits>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

namespace {

size_t warningSignature(const vector<string>& warnings) {
    size_t seed = warnings.size();
    hash<string> hasher;
    for (const auto& warning : warnings) {
        seed ^= hasher(warning) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

bool startsWith(const string& value, const string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

void configureConsoleEncoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    setlocale(LC_ALL, ".UTF-8");
}

}  // namespace

void GameController::showLoadWarnings() const {
    const auto& warnings = engine_.career().loadWarnings;
    if (warnings.empty()) return;

    const size_t signature = warningSignature(warnings);
    if (signature == lastLoadWarningSignature_) return;
    lastLoadWarningSignature_ = signature;

    string summary;
    string reportLine;
    string okLine;
    vector<string> highlighted;
    size_t warningLines = 0;
    for (const auto& warning : warnings) {
        if (summary.empty() && startsWith(warning, "Errores:")) {
            summary = warning;
            continue;
        }
        if (startsWith(warning, "[WARNING]")) {
            warningLines++;
            if (highlighted.size() < 3) highlighted.push_back(warning);
            continue;
        }
        if (reportLine.empty() && warning.find("Reporte completo disponible") != string::npos) {
            reportLine = warning;
            continue;
        }
        if (okLine.empty() && startsWith(warning, "[OK]")) {
            okLine = warning;
        }
    }

    if (!summary.empty()) {
        cout << "[AVISO] " << summary << endl;
    }
    if (!highlighted.empty()) {
        cout << "[AVISO] Incidencias destacadas:" << endl;
        for (const auto& line : highlighted) {
            cout << "[AVISO] - " << line << endl;
        }
        if (warningLines > highlighted.size()) {
            cout << "[AVISO] ... y " << (warningLines - highlighted.size())
                 << " advertencia(s) adicional(es)." << endl;
        }
    }
    if (!reportLine.empty()) {
        cout << "[AVISO] " << reportLine << endl;
    }
    if (!okLine.empty()) {
        cout << "[AVISO] " << okLine << endl;
    }
}

void GameController::pauseForContinue() const {
    cout << "\nPresiona Enter para continuar...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int GameController::run(int argc, char* argv[]) {
    if (argc > 1 && string(argv[1]) == "--validate") {
        return runValidationSuite(true);
    }
#ifdef FM_CONSOLE_DEFAULT
    return runConsoleApp();
#else
    if (argc > 1 && string(argv[1]) == "--cli") {
        return runConsoleApp();
    }

#ifdef _WIN32
    return runGuiApp();
#else
    return runConsoleApp();
#endif
#endif
}

int GameController::runConsoleApp() {
    configureConsoleEncoding();
    engine_.initialize();
    showLoadWarnings();

    while (true) {
        displayMainMenu();
        int mainChoice = readInt("Elige una opcion: ", 1, 5);
        if (mainChoice == 5) {
            cout << "Saliendo del juego." << endl;
            break;
        }

        if (mainChoice == 1) {
            runCareerMode();
        } else if (mainChoice == 2) {
            runQuickGame();
        } else if (mainChoice == 3) {
            playCupMode(engine_.career());
        } else if (mainChoice == 4) {
            int result = runValidationSuite(true);
            cout << (result == 0 ? "Validacion completada sin fallas." : "Validacion con fallas detectadas.") << endl;
        }
    }

    return 0;
}

void GameController::runCareerMode() {
    Career& career = engine_.career();
    cout << "\n=== Modo Carrera ===" << endl;
    cout << "\nOpciones de Carrera:" << endl;
    cout << "1. Continuar Carrera" << endl;
    cout << "2. Nueva Carrera" << endl;
    cout << "3. Volver al Menu Principal" << endl;
    int careerStartChoice = readInt("Elige una opcion: ", 1, 3);

    bool careerStarted = false;
    if (careerStartChoice == 1) {
        ServiceResult loadResult = engine_.loadCareer();
        for (const auto& message : loadResult.messages) cout << message << endl;
        showLoadWarnings();
        careerStarted = loadResult.ok && career.myTeam;
    } else if (careerStartChoice == 2) {
        engine_.initialize(true);
        showLoadWarnings();
        if (career.divisions.empty()) {
            cout << "No se encontraron divisiones disponibles." << endl;
            return;
        }

        cout << "\nSelecciona la division:" << endl;
        for (size_t i = 0; i < career.divisions.size(); ++i) {
            cout << i + 1 << ". " << career.divisions[i].display << endl;
        }
        int divisionChoice = readInt("Elige una division: ", 1, static_cast<int>(career.divisions.size()));
        const string divisionId = career.divisions[static_cast<size_t>(divisionChoice - 1)].id;
        auto teams = career.getDivisionTeams(divisionId);
        if (teams.empty()) {
            cout << "No hay equipos para esta division." << endl;
            return;
        }

        cout << "\nEquipos de " << divisionDisplay(divisionId) << ":" << endl;
        for (size_t i = 0; i < teams.size(); ++i) {
            cout << i + 1 << ". " << teams[i]->name << endl;
        }
        int teamChoice = readInt("Elige un numero de equipo: ", 1, static_cast<int>(teams.size()));
        const string managerName = readLine("Nombre del manager (Enter para 'Manager'): ");
        ServiceResult startResult =
            engine_.startCareer(divisionId, teams[static_cast<size_t>(teamChoice - 1)]->name, managerName);
        for (const auto& message : startResult.messages) cout << message << endl;
        careerStarted = startResult.ok && career.myTeam;
    } else {
        return;
    }

    if (!careerStarted || !career.myTeam) return;

    int careerChoice = 0;
    do {
        cout << "\nTemporada " << career.currentSeason << ", Semana " << career.currentWeek << endl;
        displayCareerMenu();
        careerChoice = readInt("Elige una opcion: ", 1, 20);
        switch (careerChoice) {
            case 1: viewTeam(*career.myTeam); break;
            case 2: trainPlayer(*career.myTeam, career.currentSeason, career.currentWeek); break;
            case 3: changeTactics(*career.myTeam); break;
            case 4: {
                ServiceResult simResult = engine_.simulateCareerWeek();
                for (const auto& message : simResult.messages) cout << message << endl;
                break;
            }
            case 5: displayCompetitionCenter(career); break;
            case 6: displayLeagueTables(career); break;
            case 7: transferMarket(career); break;
            case 8: scoutPlayers(career); break;
            case 9: displayStatistics(*career.myTeam); break;
            case 10: displayBoardStatus(career); break;
            case 11: displayNewsFeed(career); break;
            case 12: displaySeasonHistory(career); break;
            case 13: manageLineup(career); break;
            case 14: displayClubOperations(career); break;
            case 15: displayAchievementsMenu(career); break;
            case 16: {
                ServiceResult saveResult = engine_.saveCareer();
                for (const auto& message : saveResult.messages) cout << message << endl;
                break;
            }
            case 17: retirePlayer(*career.myTeam); break;
            case 18: setTrainingPlan(*career.myTeam); break;
            case 19: editTeam(*career.myTeam); break;
            case 20: cout << "Volviendo al menu principal." << endl; break;
            default: break;
        }
        if (careerChoice != 20) pauseForContinue();
    } while (careerChoice != 20);
}

void GameController::runQuickGame() {
    Career& career = engine_.career();
    if (career.divisions.empty()) {
        cout << "No se encontraron divisiones disponibles." << endl;
        return;
    }

    cout << "\nSelecciona la division para Juego Rapido:" << endl;
    for (size_t i = 0; i < career.divisions.size(); ++i) {
        cout << i + 1 << ". " << career.divisions[i].display << endl;
    }
    int divisionChoice = readInt("Elige una division: ", 1, static_cast<int>(career.divisions.size()));
    const string divisionId = career.divisions[static_cast<size_t>(divisionChoice - 1)].id;
    auto teams = career.getDivisionTeams(divisionId);
    if (teams.empty()) {
        cout << "No hay equipos para esta division." << endl;
        return;
    }

    cout << "\nEquipos de " << divisionDisplay(divisionId) << ":" << endl;
    for (size_t i = 0; i < teams.size(); ++i) {
        cout << i + 1 << ". " << teams[i]->name << endl;
    }
    int teamChoice = readInt("Elige un numero de equipo: ", 1, static_cast<int>(teams.size()));
    Team myTeam = *teams[static_cast<size_t>(teamChoice - 1)];
    myTeam.resetSeasonStats();

    Team opponent = myTeam;
    if (teams.size() > 1) {
        int oppIndex = teamChoice - 1;
        while (oppIndex == teamChoice - 1) {
            oppIndex = randInt(0, static_cast<int>(teams.size()) - 1);
        }
        opponent = *teams[static_cast<size_t>(oppIndex)];
    } else {
        opponent.name = "Rival";
    }

    int gameChoice = 0;
    do {
        displayGameMenu();
        gameChoice = readInt("Elige una opcion: ", 1, 8);
        switch (gameChoice) {
            case 1: viewTeam(myTeam); break;
            case 2: addPlayer(myTeam); break;
            case 3: trainPlayer(myTeam); break;
            case 4: changeTactics(myTeam); break;
            case 5: engine_.runQuickMatch(myTeam, opponent, true); break;
            case 6: {
                string fname = readLine("Ingresa ruta de archivo o carpeta del equipo: ");
                if (!loadTeamFromFile(fname, myTeam)) {
                    cout << "No se pudo cargar el equipo." << endl;
                } else {
                    cout << "Equipo cargado correctamente." << endl;
                }
                break;
            }
            case 7: editTeam(myTeam); break;
            case 8: cout << "Volviendo al menu principal." << endl; break;
            default: break;
        }
        if (gameChoice != 8) pauseForContinue();
    } while (gameChoice != 8);
}
