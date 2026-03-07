#include <iostream>
#include <limits>

#include "io.h"
#include "models.h"
#include "simulation.h"
#include "ui.h"
#include "utils.h"
#include "validators.h"

#ifdef _WIN32
#include "gui.h"
#include <windows.h>
#endif

using namespace std;

static int runConsoleApp() {
    Career career;
    career.initializeLeague();

    while (true) {
        displayMainMenu();
        int mainChoice = readInt("Elige una opcion: ", 1, 5);
        if (mainChoice == 5) {
            cout << "Saliendo del juego." << endl;
            break;
        }

        if (mainChoice == 1) {
            cout << "\n=== Modo Carrera ===" << endl;
            cout << "\nOpciones de Carrera:" << endl;
            cout << "1. Continuar Carrera" << endl;
            cout << "2. Nueva Carrera" << endl;
            cout << "3. Volver al Menu Principal" << endl;
            int careerStartChoice = readInt("Elige una opcion: ", 1, 3);

            bool careerStarted = false;
            if (careerStartChoice == 1) {
                career.initializeLeague();
                if (career.loadCareer()) {
                    cout << "Carrera cargada exitosamente." << endl;
                    cout << "Temporada " << career.currentSeason << ", Semana " << career.currentWeek << endl;
                    careerStarted = true;
                } else {
                    cout << "No se encontro una carrera guardada." << endl;
                }
            } else if (careerStartChoice == 2) {
                career.initializeLeague(true);
                if (career.divisions.empty()) {
                    cout << "No se encontraron divisiones disponibles." << endl;
                    continue;
                }
                cout << "\nSelecciona la division:" << endl;
                for (size_t i = 0; i < career.divisions.size(); ++i) {
                    cout << i + 1 << ". " << career.divisions[i].display << endl;
                }
                int divisionChoice = readInt("Elige una division: ", 1, static_cast<int>(career.divisions.size()));
                string divisionId = career.divisions[divisionChoice - 1].id;
                career.setActiveDivision(divisionId);

                auto teams = career.activeTeams;
                cout << "\nEquipos de " << divisionDisplay(divisionId) << ":" << endl;
                for (size_t i = 0; i < teams.size(); ++i) {
                    cout << i + 1 << ". " << teams[i]->name << endl;
                }
                int teamChoice = readInt("Elige un numero de equipo: ", 1, static_cast<int>(teams.size()));
                career.myTeam = teams[teamChoice - 1];
                string managerName = readLine("Nombre del manager (Enter para 'Manager'): ");
                if (!managerName.empty()) career.managerName = managerName;
                career.managerReputation = 50;
                career.newsFeed.clear();
                career.scoutInbox.clear();
                career.scoutingShortlist.clear();
                career.history.clear();
                career.pendingTransfers.clear();
                cout << "Has elegido: " << career.myTeam->name << endl;
                career.currentSeason = 1;
                career.currentWeek = 1;
                career.resetSeason();
                careerStarted = true;
            } else {
                continue;
            }

            if (careerStarted && career.myTeam) {
                int careerChoice = 0;
                do {
                    cout << "\nTemporada " << career.currentSeason << ", Semana " << career.currentWeek << endl;
                    displayCareerMenu();
                    careerChoice = readInt("Elige una opcion: ", 1, 20);
                    switch (careerChoice) {
                        case 1: viewTeam(*career.myTeam); break;
                        case 2: trainPlayer(*career.myTeam, career.currentSeason, career.currentWeek); break;
                        case 3: changeTactics(*career.myTeam); break;
                        case 4: simulateCareerWeek(career); break;
                        case 5: displayCompetitionCenter(career); break;
                        case 6: displayLeagueTables(career); break;
                        case 7: transferMarket(career); break;
                        case 8: scoutPlayers(career); break;
                        case 9: displayStatistics(*career.myTeam); break;
                        case 10: displayBoardStatus(career); break;
                        case 11: displayNewsFeed(career); break;
                        case 12: displaySeasonHistory(career); break;
                        case 13: manageLineup(*career.myTeam); break;
                        case 14: displayClubOperations(career); break;
                        case 15: displayAchievementsMenu(career); break;
                        case 16:
                            cout << (career.saveCareer() ? "Carrera guardada exitosamente." : "No se pudo guardar la carrera.") << endl;
                            break;
                        case 17: retirePlayer(*career.myTeam); break;
                        case 18: setTrainingPlan(*career.myTeam); break;
                        case 19: editTeam(*career.myTeam); break;
                        case 20: cout << "Volviendo al menu principal." << endl; break;
                        default: break;
                    }
                    if (careerChoice != 20) {
                        cout << "\nPresiona Enter para continuar...";
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                } while (careerChoice != 20);
            }
        } else if (mainChoice == 2) {
            if (career.divisions.empty()) {
                cout << "No se encontraron divisiones disponibles." << endl;
                continue;
            }
            cout << "\nSelecciona la division para Juego Rapido:" << endl;
            for (size_t i = 0; i < career.divisions.size(); ++i) {
                cout << i + 1 << ". " << career.divisions[i].display << endl;
            }
            int divisionChoice = readInt("Elige una division: ", 1, static_cast<int>(career.divisions.size()));
            string divisionId = career.divisions[divisionChoice - 1].id;
            auto teams = career.getDivisionTeams(divisionId);
            if (teams.empty()) {
                cout << "No hay equipos para esta division." << endl;
                continue;
            }
            cout << "\nEquipos de " << divisionDisplay(divisionId) << ":" << endl;
            for (size_t i = 0; i < teams.size(); ++i) {
                cout << i + 1 << ". " << teams[i]->name << endl;
            }
            int teamChoice = readInt("Elige un numero de equipo: ", 1, static_cast<int>(teams.size()));
            Team myTeam = *teams[teamChoice - 1];
            myTeam.resetSeasonStats();
            for (auto& p : myTeam.players) {
                p.injured = false;
                p.injuryWeeks = 0;
                p.goals = 0;
                p.assists = 0;
                p.matchesPlayed = 0;
            }

            Team opponent = myTeam;
            if (teams.size() > 1) {
                int oppIndex = teamChoice - 1;
                while (oppIndex == teamChoice - 1) {
                    oppIndex = randInt(0, static_cast<int>(teams.size()) - 1);
                }
                opponent = *teams[oppIndex];
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
                    case 5: playMatch(myTeam, opponent, true); break;
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
                if (gameChoice != 8) {
                    cout << "\nPresiona Enter para continuar...";
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }
            } while (gameChoice != 8);
        } else if (mainChoice == 3) {
            playCupMode(career);
        } else if (mainChoice == 4) {
            int result = runValidationSuite(true);
            cout << (result == 0 ? "Validacion completada sin fallas." : "Validacion con fallas detectadas.") << endl;
        }
    }

    return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    if (argc > 1 && string(argv[1]) == "--validate") {
        return runValidationSuite(true);
    }

    if (argc > 1 && string(argv[1]) == "--cli") {
        return runConsoleApp();
    }

#ifdef _WIN32
    HWND consoleWindow = GetConsoleWindow();
    if (consoleWindow) ShowWindow(consoleWindow, SW_HIDE);
    return runGuiApp();
#endif
    return runConsoleApp();
}
