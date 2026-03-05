#include <iostream>
#include <limits>

#include "io.h"
#include "models.h"
#include "simulation.h"
#include "ui.h"
#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    Career career;
    career.initializeLeague();

    while (true) {
        displayMainMenu();
        int mainChoice = readInt("Elige una opcion: ", 1, 4);
        if (mainChoice == 4) {
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
                    careerChoice = readInt("Elige una opcion: ", 1, 14);
                    switch (careerChoice) {
                        case 1: viewTeam(*career.myTeam); break;
                        case 2: trainPlayer(*career.myTeam, career.currentSeason, career.currentWeek); break;
                        case 3: changeTactics(*career.myTeam); break;
                        case 4: simulateCareerWeek(career); break;
                        case 5: career.leagueTable.displayTable(); break;
                        case 6: transferMarket(*career.myTeam); break;
                        case 7: scoutPlayers(*career.myTeam); break;
                        case 8: displayStatistics(*career.myTeam); break;
                        case 9: displayAchievementsMenu(career); break;
                        case 10: career.saveCareer(); break;
                        case 11: retirePlayer(*career.myTeam); break;
                        case 12: setTrainingPlan(*career.myTeam); break;
                        case 13: cout << "Volviendo al menu principal." << endl; break;
                        case 14: editTeam(*career.myTeam); break;
                        default: break;
                    }
                    if (careerChoice != 13) {
                        cout << "\nPresiona Enter para continuar...";
                        cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    }
                } while (careerChoice != 13);
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
        }
    }

    return 0;
}
