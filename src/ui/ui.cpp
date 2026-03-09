#include "ui.h"

#include "ai/team_ai.h"
#include "career/career_reports.h"
#include "career/player_development.h"
#include "career/team_management.h"
#include "career/career_support.h"
#include "competition.h"
#include "simulation.h"
#include "transfers/negotiation_system.h"
#include "transfers/transfer_market.h"
#include "ui/career_reports_ui.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

static int playerIndexByName(const Team& team, const string& name) {
    return team_mgmt::playerIndexByName(team, name);
}

static string joinTeamNames(const vector<Team*>& teams) {
    string out;
    for (size_t i = 0; i < teams.size(); ++i) {
        if (!teams[i]) continue;
        if (!out.empty()) out += ", ";
        out += teams[i]->name;
    }
    return out;
}

void retirePlayer(Team& team) {
    cout << "\n=== Retiro de Jugadores ===" << endl;
    vector<int> eligibleIndices;
    cout << "Jugadores elegibles para retiro (35-45 anos):" << endl;
    for (size_t i = 0; i < team.players.size(); ++i) {
        if (team.players[i].age >= 35 && team.players[i].age <= 45) {
            eligibleIndices.push_back(static_cast<int>(i));
            cout << eligibleIndices.size() << ". " << team.players[i].name << " (Edad: " << team.players[i].age << ")" << endl;
        }
    }
    if (eligibleIndices.empty()) {
        cout << "No hay jugadores elegibles para retiro." << endl;
        return;
    }
    int choice = readInt("Selecciona un jugador para retirar (0 para cancelar): ", 0, static_cast<int>(eligibleIndices.size()));
    if (choice >= 1 && choice <= static_cast<int>(eligibleIndices.size())) {
        int index = eligibleIndices[choice - 1];
        cout << team.players[index].name << " se ha retirado." << endl;
        team.players.erase(team.players.begin() + index);
    }
}

void displayMainMenu() {
    cout << "\n=== Football Manager Game ===" << endl;
    cout << "1. Modo Carrera" << endl;
    cout << "2. Iniciar Juego Rapido" << endl;
    cout << "3. Modo Copa" << endl;
    cout << "4. Validar sistema" << endl;
    cout << "5. Salir" << endl;
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

void viewTeam(Team& team) {
    cout << "\nEquipo: " << team.name << endl;
    cout << "Tacticas: " << team.tactics << ", Formacion: " << team.formation
         << ", Plan: " << team.trainingFocus << ", Instruccion: " << team.matchInstruction
         << ", Presupuesto: $" << team.budget << endl;
    cout << "Presion " << team.pressingIntensity << " | Linea " << team.defensiveLine
         << " | Tempo " << team.tempo << " | Amplitud " << team.width
         << " | Marcaje " << team.markingStyle << " | Rotacion " << team.rotationPolicy << endl;
    cout << "Capitan: " << (team.captain.empty() ? "No definido" : team.captain)
         << " | Penales: " << (team.penaltyTaker.empty() ? "No definido" : team.penaltyTaker)
         << " | Tiros libres: " << (team.freeKickTaker.empty() ? "No definido" : team.freeKickTaker)
         << " | Corners: " << (team.cornerTaker.empty() ? "No definido" : team.cornerTaker) << endl;
    cout << "Staff A/F/S/J/M: " << team.assistantCoach << "/" << team.fitnessCoach << "/" << team.scoutingChief
         << "/" << team.youthCoach << "/" << team.medicalTeam
         << " | Region juvenil: " << team.youthRegion << endl;
    cout << "Moral: " << team.morale << " | Temporada: " << team.wins << "G-" << team.draws << "E-" << team.losses << "P"
         << " | GF " << team.goalsFor << " / GA " << team.goalsAgainst << " | Pts " << team.points << endl;
    if (team.players.empty()) {
        cout << "No hay jugadores en el equipo." << endl;
        return;
    }
    int injuredCount = 0;
    int suspendedCount = 0;
    int lowFitCount = 0;
    int totalFit = 0;
    for (const auto& p : team.players) {
        if (p.injured) injuredCount++;
        if (p.matchesSuspended > 0) suspendedCount++;
        if (p.fitness < 60) lowFitCount++;
        totalFit += p.fitness;
    }
    int avgFit = team.players.empty() ? 0 : totalFit / static_cast<int>(team.players.size());
    if (injuredCount >= 3 || suspendedCount >= 2 || avgFit < 65) {
        cout << "[AVISO] Plantel con baja condicion: " << injuredCount << " lesionados, "
             << suspendedCount << " suspendidos, "
             << lowFitCount << " con condicion <60. Promedio: " << avgFit << endl;
    }

    auto xi = team.getStartingXIIndices();
    vector<bool> isXI(team.players.size(), false);
    for (int idx : xi) {
        if (idx >= 0 && idx < static_cast<int>(isXI.size())) isXI[idx] = true;
    }
    cout << "\nXI Titular:" << endl;
    for (int idx : xi) {
        if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
        const auto& p = team.players[idx];
        cout << "- " << p.name << " (" << p.position << ")"
             << (p.injured ? " [LES]" : "") << " - Habilidad: " << p.skill
             << (p.matchesSuspended > 0 ? " [SUSP]" : "") << ", Condicion: " << p.fitness << endl;
    }

    auto bench = team.getBenchIndices();
    if (!bench.empty()) {
        cout << "\nBanca:" << endl;
        for (int idx : bench) {
            if (idx < 0 || idx >= static_cast<int>(team.players.size())) continue;
            const auto& p = team.players[idx];
            cout << "- " << p.name << " (" << p.position << ")"
                 << (p.injured ? " [LES]" : "")
                 << (p.matchesSuspended > 0 ? " [SUSP]" : "")
                 << " Hab " << p.skill << " Cond " << p.fitness << endl;
        }
    }

    cout << "\nPlantel (ordenado por posicion):" << endl;
    vector<string> order = {"ARQ", "DEF", "MED", "DEL", "N/A"};
    for (const auto& pos : order) {
        vector<int> idxs;
        for (size_t i = 0; i < team.players.size(); ++i) {
            string norm = normalizePosition(team.players[i].position);
            if (norm == pos || (pos == "N/A" && norm == "N/A")) idxs.push_back(static_cast<int>(i));
        }
        if (idxs.empty()) continue;
        sort(idxs.begin(), idxs.end(), [&](int a, int b) {
            if (team.players[a].skill != team.players[b].skill) return team.players[a].skill > team.players[b].skill;
            return team.players[a].fitness > team.players[b].fitness;
        });
        cout << pos << ":" << endl;
        for (int i : idxs) {
            const auto& p = team.players[i];
            cout << i + 1 << ". " << p.name << (isXI[i] ? " [XI]" : "")
                 << (p.injured ? " [LES]" : "") << " - Atq " << p.attack
                 << (p.matchesSuspended > 0 ? (string(" [SUSP ") + to_string(p.matchesSuspended) + "]") : "")
                 << ", Def " << p.defense << ", Res " << p.stamina
                 << ", Cond " << p.fitness << ", Hab " << p.skill << ", Pot " << p.potential
                 << ", Rol " << p.role << ", Edad " << p.age << ", Valor $" << p.value
                 << ", Salario $" << p.wage << ", Clausula $" << p.releaseClause
                 << ", Contrato " << p.contractWeeks << " sem"
                 << ", Pie " << p.preferredFoot
                 << ", Sec " << (p.secondaryPositions.empty() ? string("-") : joinStringValues(p.secondaryPositions, "/"))
                 << ", Forma " << playerFormLabel(p) << " (" << p.currentForm << ")"
                 << ", Fiabilidad " << playerReliabilityLabel(p)
                 << ", Partidos grandes " << p.bigMatches
                 << ", TA " << p.seasonYellowCards << ", TR " << p.seasonRedCards
                 << ", Fel " << p.happiness << ", Quim " << p.chemistry
                 << ", Lider " << p.leadership << ", Profesionalismo " << p.professionalism
                 << ", Disciplina tactica " << p.tacticalDiscipline
                 << ", Plan " << p.developmentPlan << ", Promesa " << p.promisedRole
                 << ", Rasgos " << joinStringValues(p.traits, ", ")
                 << (p.wantsToLeave ? ", Quiere salir" : "")
                 << (p.onLoan ? ", Prestamo desde " + p.parentClub + " (" + to_string(p.loanWeeksRemaining) + " sem)" : "")
                 << endl;
        }
    }
}

void addPlayer(Team& team) {
    Player p;
    p.name = readLine("Nombre del jugador: ");
    p.position = normalizePosition(readLine("Posicion (ARQ/DEF/MED/DEL): "));
    if (p.position == "N/A") p.position = "MED";
    p.attack = readInt("Ataque (0-100): ", 0, 100);
    p.defense = readInt("Defensa (0-100): ", 0, 100);
    p.stamina = readInt("Resistencia (0-100): ", 0, 100);
    p.fitness = p.stamina;
    p.skill = readInt("Habilidad (0-100): ", 0, 100);
    p.potential = clampInt(p.skill + randInt(0, 5), p.skill, 95);
    p.age = readInt("Edad: ", 15, 50);
    p.value = readLongLong("Valor: ", 0, 1000000000000LL);
    p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 600);
    p.releaseClause = max(50000LL, p.value * 2);
    p.setPieceSkill = clampInt(p.skill + randInt(-8, 8), 20, 99);
    p.leadership = clampInt(35 + randInt(0, 45), 1, 99);
    p.professionalism = clampInt(40 + randInt(0, 45), 1, 99);
    p.ambition = clampInt(35 + randInt(0, 50), 1, 99);
    p.happiness = clampInt(55 + randInt(-10, 20), 1, 99);
    p.chemistry = clampInt(45 + randInt(0, 35), 1, 99);
    p.desiredStarts = (p.skill >= 70) ? 2 : 1;
    p.startsThisSeason = 0;
    p.wantsToLeave = false;
    p.onLoan = false;
    p.parentClub.clear();
    p.loanWeeksRemaining = 0;
    p.contractWeeks = randInt(52, 156);
    p.injured = false;
    p.injuryType = "";
    p.injuryWeeks = 0;
    p.injuryHistory = 0;
    p.yellowAccumulation = 0;
    p.seasonYellowCards = 0;
    p.seasonRedCards = 0;
    p.matchesSuspended = 0;
    p.goals = 0;
    p.assists = 0;
    p.matchesPlayed = 0;
    p.lastTrainedSeason = -1;
    p.lastTrainedWeek = -1;
    ensurePlayerProfile(p, true);
    team.addPlayer(p);
    cout << "Jugador agregado!" << endl;
}

static int trainingDeltaForStat(int stat) {
    int roll = randInt(1, 5);
    if (stat >= 90) return 1;
    if (stat >= 80) return min(roll, 2);
    if (stat >= 70) return min(roll, 3);
    return roll;
}

void trainPlayer(Team& team, int season, int week) {
    if (team.players.empty()) {
        cout << "No hay jugadores para entrenar." << endl;
        return;
    }
    cout << "Selecciona jugador para entrenar:" << endl;
    for (size_t i = 0; i < team.players.size(); ++i) {
        cout << i + 1 << ". " << team.players[i].name << (team.players[i].injured ? " (Lesionado)" : "") << endl;
    }
    int playerIndex = readInt("Elige jugador: ", 1, static_cast<int>(team.players.size()));
    Player& p = team.players[playerIndex - 1];
    if (p.injured) {
        cout << "No puedes entrenar a un jugador lesionado." << endl;
        return;
    }
    if (season >= 0 && week >= 0 && p.lastTrainedSeason == season && p.lastTrainedWeek == week) {
        cout << "Este jugador ya entreno esta semana." << endl;
        return;
    }
    cout << "Que quieres entrenar?" << endl;
    cout << "1. Ataque" << endl;
    cout << "2. Defensa" << endl;
    cout << "3. Resistencia" << endl;
    cout << "4. Habilidad" << endl;
    int trainChoice = readInt("Elige opcion: ", 1, 4);
    long long cost = 5000;
    if (team.budget < cost) {
        cout << "Presupuesto insuficiente para entrenar." << endl;
        return;
    }
    team.budget -= cost;
    int improvement = 1;
    switch (trainChoice) {
        case 1:
            improvement = trainingDeltaForStat(p.attack);
            p.attack = min(100, p.attack + improvement);
            cout << "Ataque mejorado a " << p.attack << endl;
            break;
        case 2:
            improvement = trainingDeltaForStat(p.defense);
            p.defense = min(100, p.defense + improvement);
            cout << "Defensa mejorada a " << p.defense << endl;
            break;
        case 3:
            improvement = trainingDeltaForStat(p.stamina);
            p.stamina = min(100, p.stamina + improvement);
            p.fitness = min(p.stamina, p.fitness + improvement);
            cout << "Resistencia mejorada a " << p.stamina << endl;
            break;
        case 4:
            improvement = trainingDeltaForStat(p.skill);
            p.skill = min(100, p.skill + improvement);
            if (p.skill > p.potential) p.potential = p.skill;
            cout << "Habilidad mejorada a " << p.skill << endl;
            break;
        default:
            break;
    }
    if (season >= 0 && week >= 0) {
        p.lastTrainedSeason = season;
        p.lastTrainedWeek = week;
    }
}

void changeTactics(Team& team) {
    cout << "Tacticas actuales: " << team.tactics << endl;
    cout << "1. Defensive" << endl;
    cout << "2. Balanced" << endl;
    cout << "3. Offensive" << endl;
    cout << "4. Pressing" << endl;
    cout << "5. Counter" << endl;
    int tacticsChoice = readInt("Elige tactica: ", 1, 5);
    switch (tacticsChoice) {
        case 1: team.tactics = "Defensive"; break;
        case 2: team.tactics = "Balanced"; break;
        case 3: team.tactics = "Offensive"; break;
        case 4: team.tactics = "Pressing"; break;
        case 5: team.tactics = "Counter"; break;
        default: break;
    }
    cout << "Intensidad de presion actual: " << team.pressingIntensity << " (1-5)" << endl;
    team.pressingIntensity = readInt("Nueva intensidad de presion: ", 1, 5);
    cout << "Linea defensiva actual: " << team.defensiveLine << " (1-5)" << endl;
    team.defensiveLine = readInt("Nueva linea defensiva: ", 1, 5);
    cout << "Tempo actual: " << team.tempo << " (1-5)" << endl;
    team.tempo = readInt("Nuevo tempo: ", 1, 5);
    cout << "Amplitud actual: " << team.width << " (1-5)" << endl;
    team.width = readInt("Nueva amplitud: ", 1, 5);
    cout << "Marcaje actual: " << team.markingStyle << endl;
    cout << "1. Zonal" << endl;
    cout << "2. Hombre" << endl;
    int marking = readInt("Tipo de marcaje: ", 1, 2);
    team.markingStyle = (marking == 2) ? "Hombre" : "Zonal";
    cout << "Instruccion de partido actual: " << team.matchInstruction << endl;
    cout << "1. Equilibrado" << endl;
    cout << "2. Laterales altos" << endl;
    cout << "3. Bloque bajo" << endl;
    cout << "4. Balon parado" << endl;
    cout << "5. Presion final" << endl;
    cout << "6. Por bandas" << endl;
    cout << "7. Juego directo" << endl;
    cout << "8. Contra-presion" << endl;
    cout << "9. Pausar juego" << endl;
    int instruction = readInt("Nueva instruccion: ", 1, 9);
    if (instruction == 2) team.matchInstruction = "Laterales altos";
    else if (instruction == 3) team.matchInstruction = "Bloque bajo";
    else if (instruction == 4) team.matchInstruction = "Balon parado";
    else if (instruction == 5) team.matchInstruction = "Presion final";
    else if (instruction == 6) team.matchInstruction = "Por bandas";
    else if (instruction == 7) team.matchInstruction = "Juego directo";
    else if (instruction == 8) team.matchInstruction = "Contra-presion";
    else if (instruction == 9) team.matchInstruction = "Pausar juego";
    else team.matchInstruction = "Equilibrado";
    ensureTeamIdentity(team);
    cout << "Tacticas cambiadas a " << team.tactics << endl;
}

void manageLineup(Team& team) {
    if (team.players.empty()) {
        cout << "No hay jugadores disponibles." << endl;
        return;
    }
    auto selectNamedList = [&](vector<string>& target, int maxCount, bool excludePreferredXI) {
        vector<string> chosen;
        for (size_t i = 0; i < team.players.size(); ++i) {
            const Player& p = team.players[i];
            bool inPreferredXI = find(team.preferredXI.begin(), team.preferredXI.end(), p.name) != team.preferredXI.end();
            if (excludePreferredXI && inPreferredXI) continue;
            cout << i + 1 << ". " << p.name << " (" << p.position << ")"
                 << (p.injured ? " [LES]" : "")
                 << (p.matchesSuspended > 0 ? " [SUSP]" : "")
                 << " Hab " << p.skill << " Cond " << p.fitness << endl;
        }
        while (static_cast<int>(chosen.size()) < maxCount) {
            int idx = readInt("Jugador (0 para terminar): ", 0, static_cast<int>(team.players.size()));
            if (idx == 0) break;
            const Player& p = team.players[idx - 1];
            if (excludePreferredXI &&
                find(team.preferredXI.begin(), team.preferredXI.end(), p.name) != team.preferredXI.end()) {
                cout << "Ese jugador ya esta en los titulares preferidos." << endl;
                continue;
            }
            if (find(chosen.begin(), chosen.end(), p.name) != chosen.end()) {
                cout << "Ya fue elegido." << endl;
                continue;
            }
            chosen.push_back(p.name);
        }
        target = chosen;
    };

    auto selectSinglePlayer = [&](const string& prompt, string& target) {
        for (size_t i = 0; i < team.players.size(); ++i) {
            cout << i + 1 << ". " << team.players[i].name << " (" << team.players[i].position << ")" << endl;
        }
        int idx = readInt(prompt, 1, static_cast<int>(team.players.size())) - 1;
        target = team.players[idx].name;
    };

    while (true) {
        cout << "\n=== Alineacion y Rotacion ===" << endl;
        cout << "Capitan: " << (team.captain.empty() ? "No definido" : team.captain) << endl;
        cout << "Penales: " << (team.penaltyTaker.empty() ? "No definido" : team.penaltyTaker) << endl;
        cout << "Tiros libres: " << (team.freeKickTaker.empty() ? "No definido" : team.freeKickTaker)
             << " | Corners: " << (team.cornerTaker.empty() ? "No definido" : team.cornerTaker) << endl;
        cout << "Rotacion: " << team.rotationPolicy << endl;
        cout << "Titulares preferidos: " << team.preferredXI.size() << "/11" << endl;
        cout << "Banca preferida: " << team.preferredBench.size() << "/7" << endl;
        cout << "1. Autoasignar mejor XI" << endl;
        cout << "2. Elegir titulares manualmente" << endl;
        cout << "3. Autoasignar banca" << endl;
        cout << "4. Elegir banca manualmente" << endl;
        cout << "5. Elegir capitan" << endl;
        cout << "6. Elegir lanzador de penales" << endl;
        cout << "7. Elegir lanzador de tiros libres" << endl;
        cout << "8. Elegir lanzador de corners" << endl;
        cout << "9. Politica de rotacion" << endl;
        cout << "10. Ver plan actual" << endl;
        cout << "11. Volver" << endl;
        int choice = readInt("Elige opcion: ", 1, 11);
        if (choice == 11) break;

        if (choice == 1) {
            team.preferredXI.clear();
            auto xi = team.getStartingXIIndices();
            for (int idx : xi) {
                if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                    team.preferredXI.push_back(team.players[idx].name);
                }
            }
            cout << "XI preferido actualizado automaticamente." << endl;
        } else if (choice == 2) {
            selectNamedList(team.preferredXI, 11, false);
            if (!team.preferredXI.empty()) {
                cout << "Titulares preferidos actualizados." << endl;
            }
        } else if (choice == 3) {
            team.preferredBench.clear();
            auto bench = team.getBenchIndices();
            for (int idx : bench) {
                if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                    team.preferredBench.push_back(team.players[idx].name);
                }
            }
            cout << "Banca preferida actualizada automaticamente." << endl;
        } else if (choice == 4) {
            selectNamedList(team.preferredBench, 7, true);
            cout << "Banca preferida actualizada." << endl;
        } else if (choice == 5) {
            selectSinglePlayer("Jugador: ", team.captain);
            cout << "Capitan actualizado." << endl;
        } else if (choice == 6) {
            selectSinglePlayer("Jugador: ", team.penaltyTaker);
            cout << "Lanzador de penales actualizado." << endl;
        } else if (choice == 7) {
            selectSinglePlayer("Jugador: ", team.freeKickTaker);
            cout << "Lanzador de tiros libres actualizado." << endl;
        } else if (choice == 8) {
            selectSinglePlayer("Jugador: ", team.cornerTaker);
            cout << "Lanzador de corners actualizado." << endl;
        } else if (choice == 9) {
            cout << "1. Titulares" << endl;
            cout << "2. Balanceado" << endl;
            cout << "3. Rotacion" << endl;
            int mode = readInt("Politica: ", 1, 3);
            if (mode == 1) team.rotationPolicy = "Titulares";
            else if (mode == 2) team.rotationPolicy = "Balanceado";
            else team.rotationPolicy = "Rotacion";
            cout << "Politica actualizada." << endl;
        } else if (choice == 10) {
            auto xi = team.getStartingXIIndices();
            cout << "XI actual:" << endl;
            for (int idx : xi) {
                if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                    cout << "- " << team.players[idx].name << " (" << team.players[idx].position << ")" << endl;
                }
            }
            auto bench = team.getBenchIndices();
            if (!bench.empty()) {
                cout << "Banca actual:" << endl;
                for (int idx : bench) {
                    if (idx >= 0 && idx < static_cast<int>(team.players.size())) {
                        cout << "- " << team.players[idx].name << " (" << team.players[idx].position << ")" << endl;
                    }
                }
            }
        }
    }
}

void setTrainingPlan(Team& team) {
    cout << "\nPlan de entrenamiento actual: " << team.trainingFocus << endl;
    cout << "1. Balanceado" << endl;
    cout << "2. Fisico" << endl;
    cout << "3. Tecnico" << endl;
    cout << "4. Tactico" << endl;
    cout << "5. Preparacion partido" << endl;
    cout << "6. Recuperacion" << endl;
    int choice = readInt("Elige plan: ", 1, 6);
    switch (choice) {
        case 1: team.trainingFocus = "Balanceado"; break;
        case 2: team.trainingFocus = "Fisico"; break;
        case 3: team.trainingFocus = "Tecnico"; break;
        case 4: team.trainingFocus = "Tactico"; break;
        case 5: team.trainingFocus = "Preparacion partido"; break;
        case 6: team.trainingFocus = "Recuperacion"; break;
        default: break;
    }
    cout << "Plan actualizado a " << team.trainingFocus << endl;
}

void editTeam(Team& team) {
    while (true) {
        cout << "\n=== Editor Rapido de Equipo ===" << endl;
        cout << "1. Renombrar equipo" << endl;
        cout << "2. Cambiar formacion" << endl;
        cout << "3. Editar jugador" << endl;
        cout << "4. Eliminar jugador" << endl;
        cout << "5. Volver" << endl;
        int choice = readInt("Elige una opcion: ", 1, 5);
        if (choice == 5) break;

        if (choice == 1) {
            string name = readLine("Nuevo nombre del equipo: ");
            if (!name.empty()) team.name = name;
            cout << "Nombre actualizado." << endl;
        } else if (choice == 2) {
            string form = readLine("Nueva formacion (ej: 4-4-2): ");
            if (!form.empty()) team.formation = form;
            cout << "Formacion actualizada." << endl;
        } else if (choice == 3) {
            if (team.players.empty()) {
                cout << "No hay jugadores para editar." << endl;
                continue;
            }
            cout << "Selecciona jugador:" << endl;
            for (size_t i = 0; i < team.players.size(); ++i) {
                cout << i + 1 << ". " << team.players[i].name << " (" << team.players[i].position << ")" << endl;
            }
            int idx = readInt("Jugador: ", 1, static_cast<int>(team.players.size())) - 1;
            Player& p = team.players[idx];
            cout << "1. Nombre" << endl;
            cout << "2. Posicion" << endl;
            cout << "3. Ataque" << endl;
            cout << "4. Defensa" << endl;
            cout << "5. Resistencia" << endl;
            cout << "6. Habilidad" << endl;
            cout << "7. Edad" << endl;
            cout << "8. Valor" << endl;
            cout << "9. Condicion" << endl;
            cout << "10. Potencial" << endl;
            cout << "11. Rol" << endl;
            cout << "12. Salario" << endl;
            cout << "13. Contrato (semanas)" << endl;
            cout << "14. Volver" << endl;
            int field = readInt("Campo a editar: ", 1, 14);
            if (field == 14) continue;
            switch (field) {
                case 1: {
                    string v = readLine("Nuevo nombre: ");
                    if (!v.empty()) p.name = v;
                    break;
                }
                case 2: {
                    string v = readLine("Nueva posicion: ");
                    string pos = normalizePosition(v);
                    p.position = (pos == "N/A") ? "MED" : pos;
                    break;
                }
                case 3:
                    p.attack = readInt("Nuevo ataque (0-100): ", 0, 100);
                    break;
                case 4:
                    p.defense = readInt("Nueva defensa (0-100): ", 0, 100);
                    break;
                case 5:
                    p.stamina = readInt("Nueva resistencia (0-100): ", 0, 100);
                    if (p.fitness > p.stamina) p.fitness = p.stamina;
                    break;
                case 6:
                    p.skill = readInt("Nueva habilidad (0-100): ", 0, 100);
                    if (p.potential < p.skill) p.potential = p.skill;
                    break;
                case 7:
                    p.age = readInt("Nueva edad: ", 15, 50);
                    break;
                case 8:
                    p.value = readLongLong("Nuevo valor: ", 0, 1000000000000LL);
                    break;
                case 9:
                    p.fitness = readInt("Nueva condicion (0-100): ", 0, 100);
                    if (p.fitness > p.stamina) p.fitness = p.stamina;
                    break;
                case 10:
                    p.potential = readInt("Nuevo potencial (0-100): ", 0, 100);
                    if (p.potential < p.skill) p.potential = p.skill;
                    break;
                case 11: {
                    string v = readLine("Nuevo rol: ");
                    if (!v.empty()) p.role = v;
                    break;
                }
                case 12:
                    p.wage = readLongLong("Nuevo salario: ", 0, 1000000000000LL);
                    break;
                case 13:
                    p.contractWeeks = readInt("Nuevo contrato (semanas): ", 0, 520);
                    break;
                default:
                    break;
            }
            cout << "Jugador actualizado." << endl;
        } else if (choice == 4) {
            if (team.players.size() <= 11) {
                cout << "No puedes dejar al equipo con menos de 11 jugadores." << endl;
                continue;
            }
            cout << "Selecciona jugador para eliminar:" << endl;
            for (size_t i = 0; i < team.players.size(); ++i) {
                cout << i + 1 << ". " << team.players[i].name << endl;
            }
            int idx = readInt("Jugador: ", 1, static_cast<int>(team.players.size())) - 1;
            cout << team.players[idx].name << " eliminado." << endl;
            team.players.erase(team.players.begin() + idx);
        }
    }
}

void displayStatistics(Team& team) {
    cout << "\n--- Estadisticas del Equipo ---" << endl;
    cout << "Moral: " << team.morale << endl;
    cout << "Victorias: " << team.wins << endl;
    cout << "Empates: " << team.draws << endl;
    cout << "Derrotas: " << team.losses << endl;
    cout << "Goles a Favor: " << team.goalsFor << endl;
    cout << "Goles en Contra: " << team.goalsAgainst << endl;
    cout << "Goles de Visita: " << team.awayGoals << endl;
    cout << "Tarjetas Amarillas: " << team.yellowCards << endl;
    cout << "Tarjetas Rojas: " << team.redCards << endl;
    cout << "Puntos: " << team.points << endl;
    cout << "\n--- Estadisticas de Jugadores ---" << endl;
    for (const auto& p : team.players) {
        cout << p.name << ": Goles " << p.goals
             << ", Asistencias " << p.assists
             << ", Partidos " << p.matchesPlayed
             << ", TA " << p.seasonYellowCards
             << ", TR " << p.seasonRedCards
             << ", Susp " << p.matchesSuspended
             << ", Tit " << p.startsThisSeason
             << ", Fel " << p.happiness
             << ", Quim " << p.chemistry
             << (p.wantsToLeave ? ", Quiere salir" : "")
             << endl;
    }
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

static bool usesGroupFormat(const Career& career) {
    return career.usesGroupFormat();
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

static int groupForTeam(const Career& career, const Team* team) {
    return competitionGroupForTeam(career, team);
}

static string regionalGroupTitle(const Career& career, bool north) {
    return competitionGroupTitle(career.activeDivision, north);
}

void displayLeagueTables(Career& career) {
    if (!career.usesGroupFormat() || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.leagueTable.displayTable();
        return;
    }
    LeagueTable north = buildCompetitionGroupTable(career, true);
    LeagueTable south = buildCompetitionGroupTable(career, false);
    north.displayTable();
    south.displayTable();
}

void displayCompetitionCenter(Career& career) {
    ui_reports::displayCompetitionCenter(career);
}

void displayBoardStatus(Career& career) {
    ui_reports::displayBoardStatus(career);
}

void displayNewsFeed(const Career& career) {
    ui_reports::displayNewsFeed(career);
}

void displaySeasonHistory(const Career& career) {
    ui_reports::displaySeasonHistory(career);
}

void displayClubOperations(Career& career) {
    ui_reports::displayClubOperations(career);
}

static int teamRank(const LeagueTable& table, const Team* team) {
    for (size_t i = 0; i < table.teams.size(); ++i) {
        if (table.teams[i] == team) return static_cast<int>(i) + 1;
    }
    return -1;
}

static bool isKeyMatch(const LeagueTable& table, const Team* home, const Team* away) {
    int rh = teamRank(table, home);
    int ra = teamRank(table, away);
    if (rh <= 0 || ra <= 0) return false;
    if (rh <= 3 || ra <= 3) return true;
    if (abs(rh - ra) <= 2) return true;
    return false;
}

static long long divisionBaseIncome(const string& division) {
    return getCompetitionConfig(division).baseIncome;
}

static int divisionWageFactor(const string& division) {
    return getCompetitionConfig(division).wageFactor;
}

static long long weeklyWage(const Team& team) {
    long long sum = 0;
    for (const auto& p : team.players) sum += p.wage;
    int factor = divisionWageFactor(team.division);
    return sum * factor / 100;
}

static void applyWeeklyFinances(Career& career, const vector<int>& pointsBefore) {
    unordered_map<Team*, int> homeGames;
    if (career.currentWeek >= 1 && career.currentWeek <= static_cast<int>(career.schedule.size())) {
        for (const auto& match : career.schedule[career.currentWeek - 1]) {
            homeGames[career.activeTeams[match.first]]++;
        }
    }
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];
        int pointsDelta = team->points - pointsBefore[i];
        if (pointsDelta >= 3) team->fanBase = clampInt(team->fanBase + 1, 10, 99);
        else if (pointsDelta == 0 && team->fanBase > 12 && randInt(1, 100) <= 30) team->fanBase--;
        long long ticketIncome = static_cast<long long>(homeGames[team]) * (team->fanBase * 2500LL + team->stadiumLevel * 7000LL);
        long long seasonTickets = (career.currentWeek % 4 == 1) ? team->fanBase * 900LL : 0LL;
        long long sponsor = team->sponsorWeekly + max(0, pointsDelta) * 800LL;
        long long performanceBonus = pointsDelta * 4000LL;
        long long solidarity = randInt(0, 3000);
        long long income = divisionBaseIncome(team->division) + sponsor + ticketIncome + seasonTickets + performanceBonus + solidarity;
        long long wages = weeklyWage(*team);
        long long debtPayment = min(team->debt, max(0LL, income / 8));
        team->debt -= debtPayment;
        long long debtInterest = max(0LL, team->debt / 250);
        long long infrastructure = (team->trainingFacilityLevel + team->youthFacilityLevel + team->stadiumLevel - 3) * 1500LL;
        long long net = income - wages - debtPayment - debtInterest - infrastructure;
        team->budget = max(0LL, team->budget + net);
        if (career.currentWeek % 8 == 0 && pointsDelta >= 3) {
            team->sponsorWeekly += max(500LL, team->fanBase * 30LL);
        }
        if (career.myTeam == team) {
            ostringstream out;
            out << "Finanzas semanales: +" << income
                << " (entradas " << ticketIncome << ", abonos " << seasonTickets << ", sponsor " << sponsor << ")"
                << " / -" << wages << " salarios"
                << " / -" << debtPayment << " deuda"
                << " / -" << debtInterest << " interes"
                << " / -" << infrastructure << " infraestructura"
                << " = " << net;
            emitUiMessage(out.str());
        }
    }
}

static void weeklyDashboard(const Career& career) {
    if (!career.myTeam) return;
    int rank = teamRank(career.leagueTable, career.myTeam);
    if (usesGroupFormat(career)) {
        int group = groupForTeam(career, career.myTeam);
        if (group == 0) {
            LeagueTable north = buildGroupTable(career, career.groupNorthIdx, regionalGroupTitle(career, true));
            rank = teamRank(north, career.myTeam);
        } else if (group == 1) {
            LeagueTable south = buildGroupTable(career, career.groupSouthIdx, regionalGroupTitle(career, false));
            rank = teamRank(south, career.myTeam);
        }
    }
    int injured = 0;
    int suspended = 0;
    int expiring = 0;
    for (const auto& p : career.myTeam->players) {
        if (p.injured) injured++;
        if (p.matchesSuspended > 0) suspended++;
        if (p.contractWeeks > 0 && p.contractWeeks <= 4) expiring++;
    }
    emitUiMessage("");
    emitUiMessage("--- Resumen Semanal ---");
    {
        ostringstream out;
        out << "Posicion: " << rank << " | Pts: " << career.myTeam->points
            << " | Moral: " << career.myTeam->morale
            << " | Presupuesto: $" << career.myTeam->budget
            << " | Directiva: " << career.boardConfidence << "/100";
        emitUiMessage(out.str());
    }
    {
        ostringstream out;
        out << "Sponsor: $" << career.myTeam->sponsorWeekly << " | Deuda: $" << career.myTeam->debt
            << " | Infraestructura E/C/T: "
            << career.myTeam->stadiumLevel << "/" << career.myTeam->youthFacilityLevel << "/" << career.myTeam->trainingFacilityLevel;
        emitUiMessage(out.str());
    }
    {
        ostringstream out;
        out << "Lesionados: " << injured
            << " | Suspendidos: " << suspended
            << " | Contratos por vencer (<=4 sem): " << expiring
            << " | Shortlist: " << career.scoutingShortlist.size();
        emitUiMessage(out.str());
    }
    emitUiMessage("Identidad: " + career.myTeam->clubStyle +
                  " | Cantera: " + career.myTeam->youthIdentity +
                  " | Rival: " + (career.myTeam->primaryRival.empty() ? string("-") : career.myTeam->primaryRival));
    emitUiMessage("Estado por lineas: " + lineMap(*career.myTeam));
    emitUiMessage("Rival proximo: " + buildOpponentReport(career));
    if (!career.boardMonthlyObjective.empty()) {
        emitUiMessage("Objetivo mensual: " + career.boardMonthlyObjective +
                      " | " + to_string(career.boardMonthlyProgress) + "/" + to_string(career.boardMonthlyTarget));
    }
    if (!career.cupChampion.empty()) {
        emitUiMessage("Copa: campeon " + career.cupChampion);
    } else if (career.cupActive) {
        emitUiMessage("Copa: ronda " + to_string(career.cupRound + 1) +
                      " | vivos " + to_string(career.cupRemainingTeams.size()));
    }
}

static void applyClubEvent(Career& career) {
    if (!career.myTeam) return;
    if (randInt(1, 100) > 15) return;
    int event = randInt(1, 4);
    if (event == 1) {
        long long bonus = 50000 + randInt(0, 30000);
        career.myTeam->budget += bonus;
        career.addNews("Nuevo patrocinio para " + career.myTeam->name + " por $" + to_string(bonus) + ".");
        emitUiMessage("[Evento] Patrocinio sorpresa: +" + to_string(bonus));
    } else if (event == 2) {
        career.myTeam->morale = clampInt(career.myTeam->morale - 5, 0, 100);
        career.addNews("La hinchada presiona a " + career.myTeam->name + " tras los ultimos resultados.");
        emitUiMessage("[Evento] Protesta de hinchas: moral -5.");
    } else if (event == 3) {
        if (!career.myTeam->players.empty()) {
            int idx = randInt(0, static_cast<int>(career.myTeam->players.size()) - 1);
            Player& p = career.myTeam->players[idx];
            p.injured = true;
            p.injuryType = "Leve";
            p.injuryWeeks = randInt(1, 2);
            p.injuryHistory++;
            career.addNews(p.name + " sufre una lesion leve en entrenamiento.");
            emitUiMessage("[Evento] Accidente en entrenamiento: " + p.name +
                          " fuera " + to_string(p.injuryWeeks) + " semanas.");
        }
    } else {
        int maxSquad = getCompetitionConfig(career.myTeam->division).maxSquadSize;
        if (maxSquad > 0 && static_cast<int>(career.myTeam->players.size()) >= maxSquad) return;
        int minSkill, maxSkill;
        getDivisionSkillRange(career.myTeam->division, minSkill, maxSkill);
        int youthBoost = max(0, career.myTeam->youthFacilityLevel - 1);
        Player youth = makeRandomPlayer("MED", minSkill + youthBoost, maxSkill + youthBoost, 16, 18);
        youth.potential = clampInt(youth.skill + randInt(8 + youthBoost, 15 + youthBoost), youth.skill, 99);
        career.myTeam->addPlayer(youth);
        career.addNews("La cantera promociona a " + youth.name + " en " + career.myTeam->name + ".");
        emitUiMessage("[Evento] Cantera: se unio " + youth.name +
                      " (pot " + to_string(youth.potential) + ").");
    }
}

struct TeamTableSnapshot {
    int points;
    int goalsFor;
    int goalsAgainst;
    int awayGoals;
    int wins;
    int draws;
    int losses;
    int yellowCards;
    int redCards;
    vector<HeadToHeadRecord> headToHead;
};

static TeamTableSnapshot captureTableState(const Team& team) {
    return {team.points, team.goalsFor, team.goalsAgainst, team.awayGoals, team.wins, team.draws,
            team.losses, team.yellowCards, team.redCards, team.headToHead};
}

static void restoreTableState(Team& team, const TeamTableSnapshot& snapshot) {
    team.points = snapshot.points;
    team.goalsFor = snapshot.goalsFor;
    team.goalsAgainst = snapshot.goalsAgainst;
    team.awayGoals = snapshot.awayGoals;
    team.wins = snapshot.wins;
    team.draws = snapshot.draws;
    team.losses = snapshot.losses;
    team.yellowCards = snapshot.yellowCards;
    team.redCards = snapshot.redCards;
    team.headToHead = snapshot.headToHead;
}

static void storeMatchAnalysis(Career& career,
                               const Team& home,
                               const Team& away,
                               const MatchResult& result,
                               bool cupMatch) {
    if (!career.myTeam) return;
    bool myHome = (&home == career.myTeam);
    bool myAway = (&away == career.myTeam);
    if (!myHome && !myAway) return;

    int myGoals = myHome ? result.homeGoals : result.awayGoals;
    int oppGoals = myHome ? result.awayGoals : result.homeGoals;
    int myShots = myHome ? result.homeShots : result.awayShots;
    int oppShots = myHome ? result.awayShots : result.homeShots;
    int myPoss = myHome ? result.homePossession : result.awayPossession;
    int oppPoss = myHome ? result.awayPossession : result.homePossession;
    int mySubs = myHome ? result.homeSubstitutions : result.awaySubstitutions;
    int oppSubs = myHome ? result.awaySubstitutions : result.homeSubstitutions;
    int myCorners = myHome ? result.homeCorners : result.awayCorners;
    int oppCorners = myHome ? result.awayCorners : result.homeCorners;
    double myXg = max(0.2, myShots * 0.11 + myCorners * 0.05 + max(0, myPoss - 50) * 0.015);
    double oppXg = max(0.2, oppShots * 0.11 + oppCorners * 0.05 + max(0, oppPoss - 50) * 0.015);
    string opponent = myHome ? away.name : home.name;
    const Team& myTeam = myHome ? home : away;
    const Team& oppTeam = myHome ? away : home;
    string verdict = (myGoals > oppGoals) ? "Partido controlado" : (myGoals < oppGoals ? "Derrota con ajustes pendientes" : "Empate cerrado");
    string recommendation = "Mantener base y ajustar rotacion segun forma individual";
    string reason = (myTeam.matchInstruction == "Juego directo" && oppTeam.defensiveLine >= 4) ? "se encontro espacio a la espalda"
                    : (myTeam.pressingIntensity >= 4 && oppPoss <= 46) ? "la presion sostuvo recuperaciones altas"
                    : (oppTeam.pressingIntensity >= 4 && myPoss <= 45) ? "costó salir ante la presion rival"
                    : "el partido se definio por detalles";

    if (myShots < oppShots && myPoss < 50) verdict += ", falto presencia ofensiva";
    else if (myShots > oppShots && myGoals <= oppGoals) verdict += ", falto eficacia";
    else if (myPoss >= 55 && myGoals >= oppGoals) verdict += ", buen control del ritmo";
    if (myCorners > oppCorners + 2) verdict += ", se cargo el area rival";
    if (myPoss < 45 && myGoals > oppGoals) verdict += ", transiciones muy efectivas";
    if (myShots + 2 < oppShots && myPoss < 48) {
        recommendation = "Subir un punto la altura de linea o el tempo para recuperar iniciativa";
    } else if (myGoals < oppGoals && myShots >= oppShots) {
        recommendation = "Trabajar finalizacion y pelota parada en la semana";
    } else if (myCorners + 2 < oppCorners) {
        recommendation = "Explorar 'Por bandas' o laterales mas altos para cargar el area";
    } else if (myPoss >= 55 && myGoals == 0) {
        recommendation = "Mantener posesion pero acelerar ultimo tercio con juego mas directo";
    }

    career.lastMatchAnalysis =
        string(cupMatch ? "Copa" : "Liga") + ": " + career.myTeam->name + " " + to_string(myGoals) + "-" +
        to_string(oppGoals) + " " + opponent + " | Tiros " + to_string(myShots) + "-" + to_string(oppShots) +
        " | Posesion " + to_string(myPoss) + "-" + to_string(oppPoss) +
        " | Corners " + to_string(myCorners) + "-" + to_string(oppCorners) +
        " | xG " + to_string(static_cast<int>(myXg * 10 + 0.5)) + "/" + to_string(static_cast<int>(oppXg * 10 + 0.5)) +
        " | Cambios " + to_string(mySubs) + "-" + to_string(oppSubs) +
        " | Clima " + result.weather +
        " | " + lineMap(myTeam) +
        " | " + verdict +
        " | Clave: " + reason +
        " | Recomendacion: " + recommendation;
}

static void simulateSeasonCupRound(Career& career) {
    if (!career.cupActive) return;
    vector<Team*> alive;
    for (const auto& name : career.cupRemainingTeams) {
        Team* team = career.findTeamByName(name);
        if (team && team->division == career.activeDivision) alive.push_back(team);
    }
    if (alive.size() <= 1) {
        career.cupActive = false;
        if (!alive.empty()) {
            career.cupChampion = alive.front()->name;
            career.addNews("Copa de temporada: " + career.cupChampion + " se consagra campeon.");
        }
        return;
    }

    career.cupRound++;
    cout << "\n--- Copa de temporada: ronda " << career.cupRound << " ---" << endl;
    vector<string> nextRound;
    if (alive.size() % 2 == 1) {
        Team* bye = alive.back();
        nextRound.push_back(bye->name);
        alive.pop_back();
        cout << "Pase libre: " << bye->name << endl;
    }

    for (size_t i = 0; i < alive.size(); i += 2) {
        Team* home = alive[i];
        Team* away = alive[i + 1];
        TeamTableSnapshot homeSnap = captureTableState(*home);
        TeamTableSnapshot awaySnap = captureTableState(*away);
        bool verbose = (home == career.myTeam || away == career.myTeam);
        cout << home->name << " vs " << away->name << endl;
        MatchResult result = playMatch(*home, *away, verbose, true, true);
        restoreTableState(*home, homeSnap);
        restoreTableState(*away, awaySnap);
        storeMatchAnalysis(career, *home, *away, result, true);

        Team* winner = home;
        if (result.awayGoals > result.homeGoals) winner = away;
        else if (result.homeGoals == result.awayGoals) {
            winner = (teamPenaltyStrength(*home) >= teamPenaltyStrength(*away)) ? home : away;
            cout << "Gana por penales: " << winner->name << endl;
        }
        nextRound.push_back(winner->name);
    }

    career.cupRemainingTeams = nextRound;
    if (career.cupRemainingTeams.size() == 1) {
        career.cupActive = false;
        career.cupChampion = career.cupRemainingTeams.front();
        career.addNews("Copa de temporada: " + career.cupChampion + " se consagra campeon.");
        cout << "Campeon de la copa: " << career.cupChampion << endl;
    }
}

static void updateSquadDynamics(Career& career, int pointsDelta) {
    if (!career.myTeam) return;
    int captainLeadership = 0;
    int captainIdx = playerIndexByName(*career.myTeam, career.myTeam->captain);
    if (captainIdx >= 0 && captainIdx < static_cast<int>(career.myTeam->players.size())) {
        captainLeadership = career.myTeam->players[captainIdx].leadership;
    }

    auto expectedStartsForPromise = [&](const Player& player) {
        if (player.promisedRole == "Titular") return max(2, career.currentWeek * 2 / 3);
        if (player.promisedRole == "Rotacion") return max(1, career.currentWeek / 3);
        if (player.promisedRole == "Proyecto") return (player.age <= 22) ? max(1, career.currentWeek / 4) : max(0, career.currentWeek / 6);
        return max(0, career.currentWeek / 2 + player.desiredStarts - 1);
    };

    for (auto& player : career.myTeam->players) {
        bool wasUnhappy = player.wantsToLeave;
        int expectedStarts = expectedStartsForPromise(player);
        int unmetStarts = max(0, expectedStarts - player.startsThisSeason);
        if (unmetStarts > 0) {
            player.happiness = clampInt(player.happiness - min(4, 1 + unmetStarts), 1, 99);
            if (player.skill >= career.myTeam->getAverageSkill() && unmetStarts >= 2) {
                player.chemistry = clampInt(player.chemistry - 1, 1, 99);
            }
            if (player.promisedRole != "Sin promesa" && unmetStarts >= 2) {
                player.happiness = clampInt(player.happiness - 2, 1, 99);
            }
        } else {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
            if (player.promisedRole != "Sin promesa") {
                player.happiness = clampInt(player.happiness + 1, 1, 99);
            }
        }

        if (pointsDelta == 3) {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        } else if (pointsDelta == 0 && player.ambition >= 65) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (player.injured) player.happiness = clampInt(player.happiness - 1, 1, 99);
        if (player.wage < wageDemandFor(player) * 80 / 100) player.happiness = clampInt(player.happiness - 1, 1, 99);
        if (captainLeadership >= 75) player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        if (career.myTeam->morale >= 65) player.happiness = clampInt(player.happiness + 1, 1, 99);
        if (playerHasTrait(player, "Lider") && career.myTeam->morale >= 55) {
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        }
        if (playerHasTrait(player, "Caliente") && pointsDelta == 0) {
            player.happiness = clampInt(player.happiness - 1, 1, 99);
        }
        if (player.leadership >= 75 && player.happiness >= 60) {
            player.chemistry = clampInt(player.chemistry + 1, 1, 99);
        }
        if (player.professionalism >= 75 && player.startsThisSeason >= expectedStarts) {
            player.happiness = clampInt(player.happiness + 1, 1, 99);
        }

        int unrestThreshold = (player.skill >= career.myTeam->getAverageSkill() + 3) ? 38 : 32;
        player.wantsToLeave = player.happiness <= unrestThreshold &&
                              (player.ambition >= 60 || player.professionalism <= 45 || unmetStarts >= 3 || player.promisedRole != "Sin promesa");
        if (!wasUnhappy && player.wantsToLeave) {
            career.addNews(player.name + " queda disconforme con su situacion en " + career.myTeam->name + ".");
        }
    }
}

static void runMonthlyDevelopment(Career& career) {
    if (career.currentWeek <= 0 || career.currentWeek % 4 != 0) return;
    for (auto* team : career.activeTeams) {
        int improved = 0;
        int eliteImproved = 0;
        for (auto& player : team->players) {
            if (player.age > 21 || player.skill >= player.potential) continue;
            int chance = 6 + max(0, team->youthCoach - 55) / 10 + max(0, team->trainingFacilityLevel - 1) * 2;
            chance += max(0, player.professionalism - 55) / 15;
            if (player.developmentPlan == "Finalizacion" || player.developmentPlan == "Creatividad" ||
                player.developmentPlan == "Defensa" || player.developmentPlan == "Reflejos") {
                chance += 2;
            }
            if (player.promisedRole == "Proyecto") chance += 2;
            if (playerHasTrait(player, "Proyecto") || playerHasTrait(player, "Competidor")) chance += 2;
            if (randInt(1, 100) <= clampInt(chance, 4, 28)) {
                int gain = 1;
                if (player.age <= 19 && player.potential - player.skill >= 10 && randInt(1, 100) <= 28) gain++;
                player.skill = min(100, player.skill + gain);
                string pos = normalizePosition(player.position);
                if (pos == "ARQ" || pos == "DEF") player.defense = min(100, player.defense + 1);
                else if (pos == "MED") {
                    player.attack = min(100, player.attack + 1);
                    player.defense = min(100, player.defense + 1);
                } else {
                    player.attack = min(100, player.attack + 1);
                }
                if (player.developmentPlan == "Liderazgo") {
                    player.leadership = min(99, player.leadership + 1);
                    player.professionalism = min(99, player.professionalism + 1);
                } else if (player.developmentPlan == "Creatividad") {
                    player.setPieceSkill = min(99, player.setPieceSkill + 1);
                } else if (player.developmentPlan == "Fisico") {
                    player.stamina = min(100, player.stamina + 1);
                    player.fitness = min(player.stamina, player.fitness + 1);
                }
                improved++;
                if (gain > 1) eliteImproved++;
            }
        }
        int maxSquad = getCompetitionConfig(team->division).maxSquadSize;
        if ((maxSquad <= 0 || static_cast<int>(team->players.size()) < maxSquad) &&
            randInt(1, 100) <= 4 + team->youthFacilityLevel * 3) {
            player_dev::addYouthPlayers(*team, 1);
            if (team == career.myTeam) {
                career.addNews("La cantera suma un nuevo prospecto desde la region " + team->youthRegion + ".");
            }
        }
        if (team == career.myTeam && improved > 0) {
            career.addNews("Informe juvenil: " + to_string(improved) + " jugador(es) joven(es) mejoran este mes.");
            if (eliteImproved > 0) {
                career.addNews("Informe de cantera: " + to_string(eliteImproved) + " prospecto(s) muestran una aceleracion especial.");
            }
        }
    }
}

static void updateShortlistAlerts(Career& career) {
    if (!career.myTeam || career.scoutingShortlist.empty()) return;
    if (career.currentWeek % 4 != 0) return;

    vector<string> active;
    for (const auto& item : career.scoutingShortlist) {
        auto parts = splitByDelimiter(item, '|');
        if (parts.size() < 2) continue;
        Team* seller = career.findTeamByName(parts[0]);
        if (!seller) continue;
        int idx = playerIndexByName(*seller, parts[1]);
        if (idx < 0) continue;
        const Player& player = seller->players[static_cast<size_t>(idx)];
        active.push_back(item);
        if (player.contractWeeks <= 12) {
            career.addNews("Alerta de shortlist: " + player.name + " entra en ventana de precontrato con " + seller->name + ".");
        } else if (player.value <= player.releaseClause * 60 / 100) {
            career.addNews("Alerta de shortlist: " + player.name + " mantiene un costo accesible en " + seller->name + ".");
        }
    }
    career.scoutingShortlist = active;
}

static const Player* leadingForward(const Team& team) {
    const Player* best = nullptr;
    for (const auto& player : team.players) {
        if (normalizePosition(player.position) != "DEL") continue;
        if (!best || player.skill > best->skill) best = &player;
    }
    return best;
}

static void addSquadAlerts(Career& career) {
    if (!career.myTeam) return;
    int defFit = averageFitnessForLine(*career.myTeam, "DEF");
    int midFit = averageFitnessForLine(*career.myTeam, "MED");
    int attFit = averageFitnessForLine(*career.myTeam, "DEL");
    if (defFit < 58 || midFit < 58 || attFit < 58) {
        string line = (defFit <= midFit && defFit <= attFit) ? "la linea defensiva"
                     : (midFit <= attFit ? "el mediocampo" : "el frente de ataque");
        career.addNews("Alerta fisica: " + line + " llega exigida a la proxima fecha.");
    }
    const Player* forward = leadingForward(*career.myTeam);
    if (forward && forward->matchesPlayed >= 5 && forward->goals == 0) {
        career.addNews("Alerta ofensiva: " + forward->name + " ya suma " +
                       to_string(forward->matchesPlayed) + " partido(s) sin marcar.");
    }
    int promiseWarnings = 0;
    for (const auto& player : career.myTeam->players) {
        if (promiseAtRisk(player, career.currentWeek)) promiseWarnings++;
    }
    if (promiseWarnings >= 2) {
        career.addNews("Alerta de vestuario: hay " + to_string(promiseWarnings) + " promesa(s) contractuales bajo revision.");
    }
}

static void generateWeeklyNarratives(Career& career, int myTeamPointsDelta) {
    if (!career.myTeam) return;
    int rank = career.currentCompetitiveRank();
    int field = max(1, career.currentCompetitiveFieldSize());
    if (rank > 0 && rank <= max(2, field / 4)) {
        career.addNews("La prensa destaca a " + career.myTeam->name + " por su presencia en la zona alta.");
    } else if (rank >= max(2, field - field / 4)) {
        career.addNews("La prensa pone a " + career.myTeam->name + " en la pelea por evitar el fondo.");
    }

    if (myTeamPointsDelta == 3 && career.myTeam->morale >= 65) {
        career.addNews("El vestuario de " + career.myTeam->name + " atraviesa un momento de confianza.");
    } else if (myTeamPointsDelta == 0 && career.boardConfidence <= 30) {
        career.addNews("Crece la tension institucional alrededor de " + career.myTeam->name + ".");
    }

    int promiseAlerts = 0;
    int leaders = 0;
    for (const auto& player : career.myTeam->players) {
        if (player.promisedRole == "Titular" && player.startsThisSeason + 2 < max(2, career.currentWeek * 2 / 3)) promiseAlerts++;
        if (player.promisedRole == "Rotacion" && player.startsThisSeason + 1 < max(1, career.currentWeek / 3)) promiseAlerts++;
        if ((player.leadership >= 72 || playerHasTrait(player, "Lider")) && player.happiness >= 55) leaders++;
    }
    if (promiseAlerts > 0) {
        career.addNews("Se acumulan " + to_string(promiseAlerts) + " promesa(s) de rol bajo presion en el plantel.");
    }
    if (career.myTeam->fanBase >= 65 && myTeamPointsDelta == 3) {
        career.addNews("La aficion responde con entusiasmo y empuja la recaudacion del club.");
    } else if (career.myTeam->fanBase >= 45 && myTeamPointsDelta == 0) {
        career.addNews("La prensa cuestiona la falta de resultados recientes de " + career.myTeam->name + ".");
    }
    if (leaders >= 3 && career.myTeam->morale >= 60) {
        career.addNews("Los lideres del vestuario sostienen un ambiente competitivo en " + career.myTeam->name + ".");
    }
    if (career.myTeam->youthIdentity == "Cantera estructurada") {
        int youthMinutes = 0;
        for (const auto& player : career.myTeam->players) {
            if (player.age <= 20 && player.matchesPlayed > 0) youthMinutes++;
        }
        if (youthMinutes >= 2) {
            career.addNews("La identidad de cantera de " + career.myTeam->name + " gana peso esta semana.");
        }
    }
    const Team* opponent = nextOpponent(career);
    if (opponent) {
        career.addNews("Informe previo: " + buildOpponentReport(career) + ".");
        if (areRivalClubs(*career.myTeam, *opponent)) {
            career.addNews("La semana queda marcada por un clasico ante " + opponent->name + ".");
        }
    }
    if (teamPrestigeScore(*career.myTeam) >= 68 && myTeamPointsDelta == 0) {
        career.addNews("La exigencia institucional aprieta: el entorno de " + career.myTeam->name + " esperaba mas.");
    }

    for (const auto& player : career.myTeam->players) {
        if (player.contractWeeks > 0 && player.contractWeeks <= 4) {
            career.addNews("Contrato al limite: " + player.name + " entra en sus ultimas " + to_string(player.contractWeeks) + " semana(s).");
            break;
        }
    }
    addSquadAlerts(career);
}

static void processIncomingOffers(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->players.size() <= 18) return;
    bool squadUnrest = false;
    for (const auto& player : career.myTeam->players) {
        if (player.wantsToLeave) {
            squadUnrest = true;
            break;
        }
    }
    if (randInt(1, 100) > (squadUnrest ? 50 : 32)) return;
    vector<int> candidates;
    for (size_t i = 0; i < career.myTeam->players.size(); ++i) {
        const Player& player = career.myTeam->players[i];
        if (!player.injured && player.contractWeeks > 8) {
            candidates.push_back(static_cast<int>(i));
            if (player.wantsToLeave) candidates.push_back(static_cast<int>(i));
        }
    }
    if (candidates.empty()) return;
    int idx = candidates[randInt(0, static_cast<int>(candidates.size()) - 1)];
    Player& p = career.myTeam->players[idx];
    Team* bidder = nullptr;
    int bidderNeed = -100000;
    for (auto& club : career.allTeams) {
        if (&club == career.myTeam) continue;
        if (club.players.size() >= 26) continue;
        if (club.budget < p.value * 9 / 10) continue;
        int need = positionFitScore(p, transfer_market::weakestSquadPosition(club)) +
                   teamPrestigeScore(club) - teamPrestigeScore(*career.myTeam) / 2;
        if (areRivalClubs(club, *career.myTeam)) need += 8;
        if (need > bidderNeed) {
            bidderNeed = need;
            bidder = &club;
        }
    }
    if (!bidder) return;
    ensureTeamIdentity(*career.myTeam);
    ensureTeamIdentity(*bidder);
    long long maxOffer = max(p.value, p.value * (105 + randInt(0, 40)) / 100);
    if (areRivalClubs(*bidder, *career.myTeam)) maxOffer = maxOffer * 112 / 100;
    maxOffer = max(maxOffer, static_cast<long long>(p.value * (100 + teamPrestigeScore(*bidder) / 10) / 100));
    long long offer = max(p.value * 9 / 10, maxOffer * 85 / 100);
    emitUiMessage("");
    emitUiMessage("Oferta recibida por " + p.name + " desde " + bidder->name + ": $" + to_string(offer) +
                  " (tope estimado del mercado: $" + to_string(maxOffer) + ")" +
                  (areRivalClubs(*bidder, *career.myTeam) ? " [rival directo]" : ""));
    int choice = 3;
    long long counter = 0;
    if (incomingOfferDecisionCallback()) {
        IncomingOfferDecision decision = incomingOfferDecisionCallback()(career, p, offer, maxOffer);
        if (decision.action >= 1 && decision.action <= 3) {
            choice = decision.action;
            counter = decision.counterOffer;
        }
    } else {
        cout << "1. Aceptar" << endl;
        cout << "2. Contraofertar" << endl;
        cout << "3. Rechazar" << endl;
        choice = readInt("Respuesta: ", 1, 3);
        if (choice == 2) {
            counter = readLongLong("Tu contraoferta: ", 0, 1000000000000LL);
        }
    }
    if (choice == 1) {
        career.myTeam->budget += offer;
        bidder->budget = max(0LL, bidder->budget - offer);
        Player moved = p;
        moved.wantsToLeave = false;
        moved.onLoan = false;
        moved.parentClub.clear();
        moved.loanWeeksRemaining = 0;
        bidder->addPlayer(moved);
        emitUiMessage("Transferencia aceptada. " + p.name + " vendido a " + bidder->name + ".");
        career.addNews(p.name + " es vendido a " + bidder->name + " por $" + to_string(offer) + ".");
        team_mgmt::detachPlayerFromSelections(*career.myTeam, p.name);
        team_mgmt::applyDepartureShock(*career.myTeam, p);
        career.myTeam->players.erase(career.myTeam->players.begin() + idx);
    } else if (choice == 2) {
        if (counter <= maxOffer && bidder->budget >= counter) {
            career.myTeam->budget += counter;
            bidder->budget = max(0LL, bidder->budget - counter);
            Player moved = p;
            moved.wantsToLeave = false;
            moved.onLoan = false;
            moved.parentClub.clear();
            moved.loanWeeksRemaining = 0;
            bidder->addPlayer(moved);
            emitUiMessage("Contraoferta aceptada. " + p.name + " vendido a " + bidder->name + " por $" + to_string(counter));
            career.addNews(p.name + " es vendido a " + bidder->name + " tras contraoferta por $" + to_string(counter) + ".");
            team_mgmt::detachPlayerFromSelections(*career.myTeam, p.name);
            team_mgmt::applyDepartureShock(*career.myTeam, p);
            career.myTeam->players.erase(career.myTeam->players.begin() + idx);
        } else {
            emitUiMessage("La contraoferta fue rechazada.");
        }
    } else {
        emitUiMessage("Oferta rechazada.");
    }
}

static void updateContracts(Career& career) {
    for (auto& teamRef : career.allTeams) {
        Team* team = &teamRef;
        ensureTeamIdentity(*team);
        for (size_t i = 0; i < team->players.size();) {
            Player& p = team->players[i];
            if (p.contractWeeks > 0) p.contractWeeks--;
            if (p.contractWeeks > 0) {
                ++i;
                continue;
            }
            if (team == career.myTeam) {
                long long demandedWage = max(p.wage, wageDemandFor(p));
                if (p.wantsToLeave) demandedWage = demandedWage * 120 / 100;
                if (promiseAtRisk(p, career.currentWeek)) demandedWage = demandedWage * 108 / 100;
                if (p.skill >= team->getAverageSkill()) demandedWage = demandedWage * 110 / 100;
                int demandedWeeks = randInt(78, 182);
                long long demandedClause = max(p.value * 2, demandedWage * (p.skill >= team->getAverageSkill() ? 48 : 40));
                emitUiMessage("");
                emitUiMessage("Contrato expirado: " + p.name);
                emitUiMessage("Demanda renovar por " + to_string(demandedWeeks) +
                              " semanas | Salario $" + to_string(demandedWage) +
                              " | Clausula $" + to_string(demandedClause));
                if (p.wantsToLeave) {
                    emitUiMessage(p.name + " esta inquieto por su rol en el club y exige mejores condiciones.");
                }
                if (promiseAtRisk(p, career.currentWeek)) {
                    emitUiMessage("Advertencia: " + p.name + " siente que su promesa de rol fue incumplida.");
                }
                bool renew = false;
                if (contractRenewalDecisionCallback()) {
                    renew = contractRenewalDecisionCallback()(career, *team, p, demandedWage, demandedWeeks, demandedClause);
                } else {
                    int choice = readInt("Renovar? (1. Si, 2. No): ", 1, 2);
                    renew = (choice == 1);
                }
                if (renew) {
                    if (team->budget < demandedWage * 6) {
                        emitUiMessage("No hay margen salarial suficiente. " + p.name + " deja el club.");
                        career.addNews(p.name + " deja el club tras no acordar renovacion.");
                        team_mgmt::detachPlayerFromSelections(*team, p.name);
                        team_mgmt::applyDepartureShock(*team, p);
                        team->players.erase(team->players.begin() + i);
                    } else {
                        p.contractWeeks = demandedWeeks;
                        p.wage = demandedWage;
                        p.releaseClause = demandedClause;
                        p.wantsToLeave = false;
                        p.happiness = clampInt(p.happiness + 6, 1, 99);
                        emitUiMessage("Renovado. Nuevo salario $" + to_string(p.wage));
                        career.addNews(p.name + " renueva contrato en " + team->name + ".");
                        ++i;
                    }
                } else {
                    emitUiMessage(p.name + " deja el club.");
                    career.addNews(p.name + " deja el club al finalizar su contrato.");
                    team_mgmt::detachPlayerFromSelections(*team, p.name);
                    team_mgmt::applyDepartureShock(*team, p);
                    team->players.erase(team->players.begin() + i);
                }
            } else {
                if (team->budget > p.wage * 8 && randInt(1, 100) <= (promiseAtRisk(p, career.currentWeek) ? 45 : 70)) {
                    p.contractWeeks = randInt(52, 156);
                    p.wage = static_cast<long long>(p.wage * (1.05 + randInt(0, 15) / 100.0));
                    p.releaseClause = max(p.value * 2, p.wage * 45);
                    ++i;
                } else {
                    team_mgmt::detachPlayerFromSelections(*team, p.name);
                    team->players.erase(team->players.begin() + i);
                }
            }
        }
    }
}

static void updateManagerReputation(Career& career) {
    if (!career.myTeam) return;
    int rank = career.currentCompetitiveRank();
    if (rank > 0) {
        if (rank <= max(1, career.boardExpectedFinish - 1)) career.managerReputation = clampInt(career.managerReputation + 2, 1, 100);
        else if (rank > career.boardExpectedFinish + 2) career.managerReputation = clampInt(career.managerReputation - 1, 1, 100);
    }
    if ((career.myTeam->tactics == "Pressing" || career.myTeam->matchInstruction == "Juego directo") &&
        career.myTeam->goalsFor >= max(4, career.currentWeek * 2)) {
        career.managerReputation = clampInt(career.managerReputation + 1, 1, 100);
    }
    int promiseWarnings = 0;
    for (const auto& player : career.myTeam->players) {
        if (promiseAtRisk(player, career.currentWeek)) promiseWarnings++;
    }
    if (career.boardConfidence <= 25 && promiseWarnings >= 2) {
        career.managerReputation = clampInt(career.managerReputation - 1, 1, 100);
    }
}

static void handleManagerStatus(Career& career) {
    if (!career.myTeam) return;
    if (career.boardConfidence >= 20 && career.boardWarningWeeks < 6) return;
    emitUiMessage("");
    emitUiMessage("[Directiva] " + career.myTeam->name + " decide despedirte.");
    career.addNews(career.managerName + " fue despedido de " + career.myTeam->name + ".");
    career.managerReputation = clampInt(career.managerReputation - 8, 10, 100);
    vector<Team*> jobs = buildJobMarket(career, true);
    if (jobs.empty()) {
        for (auto& team : career.allTeams) {
            if (&team != career.myTeam) jobs.push_back(&team);
        }
    }
    emitUiMessage("Debes elegir nuevo club:");
    for (size_t i = 0; i < jobs.size(); ++i) {
        emitUiMessage(to_string(i + 1) + ". " + jobs[i]->name + " (" + divisionDisplay(jobs[i]->division) + ")");
    }
    int choice = 1;
    if (managerJobSelectionCallback()) {
        int selected = managerJobSelectionCallback()(career, jobs);
        if (selected >= 0 && selected < static_cast<int>(jobs.size())) {
            choice = selected + 1;
        }
    } else {
        choice = readInt("Club: ", 1, static_cast<int>(jobs.size()));
    }
    takeManagerJob(career, jobs[choice - 1], "Llega tras un despido reciente.");
}

static void recordSeasonHistory(Career& career,
                                const string& champion,
                                const vector<Team*>& promoted,
                                const vector<Team*>& relegated,
                                const string& note) {
    if (!career.myTeam) return;
    SeasonHistoryEntry entry;
    entry.season = career.currentSeason;
    entry.division = career.activeDivision;
    entry.club = career.myTeam->name;
    entry.finish = career.currentCompetitiveRank();
    entry.champion = champion;
    entry.promoted = joinTeamNames(promoted);
    entry.relegated = joinTeamNames(relegated);
    entry.note = note;
    career.history.push_back(entry);
    if (career.history.size() > 30) {
        career.history.erase(career.history.begin(), career.history.begin() + static_cast<long long>(career.history.size() - 30));
    }
}

static void awardSeasonPrizeMoney(Career& career, const LeagueTable& table) {
    if (!career.myTeam) return;
    int rank = teamRank(table, career.myTeam);
    if (rank <= 0) return;
    int size = max(1, static_cast<int>(table.teams.size()));
    long long prize = max(10000LL, static_cast<long long>(size - rank + 1) * 12000LL);
    career.myTeam->budget += prize;
    career.addNews(career.myTeam->name + " recibe $" + to_string(prize) + " por su ubicacion final.");
}

static void advanceToNextSeason(Career& career) {
    career.currentSeason++;
    career.currentWeek = 1;
    career.agePlayers();
    career.executePendingTransfers();
    if (career.myTeam) {
        career.setActiveDivision(career.myTeam->division);
    } else {
        career.setActiveDivision(career.activeDivision);
    }
    career.resetSeason();
    for (auto* team : career.activeTeams) {
        player_dev::addYouthPlayers(*team, 1);
    }
}

static int divisionIndex(const string& id) {
    for (size_t i = 0; i < kDivisions.size(); ++i) {
        if (kDivisions[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

static vector<Team*> topByValue(vector<Team*> teams, int count) {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        return a->getSquadValue() > b->getSquadValue();
    });
    if (count < 0) count = 0;
    if (static_cast<int>(teams.size()) > count) teams.resize(count);
    return teams;
}

static vector<Team*> sortByValue(vector<Team*> teams) {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        return a->getSquadValue() > b->getSquadValue();
    });
    return teams;
}

static vector<Team*> bottomByValue(vector<Team*> teams, int count) {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        return a->getSquadValue() < b->getSquadValue();
    });
    if (count < 0) count = 0;
    if (static_cast<int>(teams.size()) > count) teams.resize(count);
    return teams;
}

static Team* simulatePlayoffMatch(Team* home, Team* away, const string& label) {
    if (!home) return away;
    if (!away) return home;
    cout << label << ": " << home->name << " vs " << away->name << endl;
    Team h1 = *home;
    Team a1 = *away;
    MatchResult r1 = playMatch(h1, a1, false, true);
    Team h2 = *home;
    Team a2 = *away;
    MatchResult r2 = playMatch(a2, h2, false, true);

    int homeAgg = r1.homeGoals + r2.awayGoals;
    int awayAgg = r1.awayGoals + r2.homeGoals;

    cout << "Ida: " << home->name << " " << r1.homeGoals << " - " << r1.awayGoals << " " << away->name << endl;
    cout << "Vuelta: " << away->name << " " << r2.homeGoals << " - " << r2.awayGoals << " " << home->name << endl;
    cout << "Global: " << home->name << " " << homeAgg << " - " << awayAgg << " " << away->name << endl;
    if (homeAgg > awayAgg) return home;
    if (awayAgg > homeAgg) return away;
    Team* winner = (teamPenaltyStrength(*home) >= teamPenaltyStrength(*away)) ? home : away;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

static vector<vector<pair<int, int>>> buildRoundRobinIndexSchedule(int teamCount, bool doubleRound) {
    vector<vector<pair<int, int>>> out;
    if (teamCount < 2) return out;
    vector<int> idx;
    idx.reserve(teamCount + 1);
    for (int i = 0; i < teamCount; ++i) idx.push_back(i);
    if (idx.size() % 2 == 1) idx.push_back(-1);

    int size = static_cast<int>(idx.size());
    int rounds = size - 1;
    for (int round = 0; round < rounds; ++round) {
        vector<pair<int, int>> matches;
        for (int i = 0; i < size / 2; ++i) {
            int a = idx[i];
            int b = idx[size - 1 - i];
            if (a == -1 || b == -1) continue;
            if (round % 2 == 0) matches.push_back({a, b});
            else matches.push_back({b, a});
        }
        out.push_back(matches);
        int last = idx.back();
        for (int i = size - 1; i > 1; --i) idx[i] = idx[i - 1];
        idx[1] = last;
    }

    if (doubleRound) {
        int base = static_cast<int>(out.size());
        for (int i = 0; i < base; ++i) {
            vector<pair<int, int>> rev;
            for (auto& m : out[i]) rev.push_back({m.second, m.first});
            out.push_back(rev);
        }
    }
    return out;
}

static void simulateBackgroundDivisionWeek(Career& career, const string& divisionId) {
    if (divisionId.empty() || divisionId == career.activeDivision) return;
    vector<Team*> teams = career.getDivisionTeams(divisionId);
    if (teams.size() < 2) return;
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        if (a->tiebreakerSeed != b->tiebreakerSeed) return a->tiebreakerSeed < b->tiebreakerSeed;
        return a->name < b->name;
    });
    auto schedule = buildRoundRobinIndexSchedule(static_cast<int>(teams.size()), true);
    int round = career.currentWeek - 1;
    if (round < 0 || round >= static_cast<int>(schedule.size())) return;

    int headlineMargin = -1;
    string headline;
    for (const auto& match : schedule[static_cast<size_t>(round)]) {
        Team* home = teams[static_cast<size_t>(match.first)];
        Team* away = teams[static_cast<size_t>(match.second)];
        MatchResult result = playMatch(*home, *away, false, false);
        int margin = abs(result.homeGoals - result.awayGoals);
        if (margin > headlineMargin) {
            headlineMargin = margin;
            headline = home->name + " " + to_string(result.homeGoals) + "-" + to_string(result.awayGoals) + " " + away->name;
        }
    }

    LeagueTable table;
    table.title = divisionDisplay(divisionId);
    table.ruleId = divisionId;
    for (Team* team : teams) table.addTeam(team);
    table.sortTable();
    if (!headline.empty() && (career.currentWeek % 4 == 0 || headlineMargin >= 3)) {
        Team* leader = table.teams.empty() ? nullptr : table.teams.front();
        string news = "[Mundo] " + divisionDisplay(divisionId) + ": " + headline;
        if (leader) news += " | Lider " + leader->name + " (" + to_string(leader->points) + " pts)";
        career.addNews(news);
    }
    if (!table.teams.empty() && randInt(1, 100) <= 14) {
        Team* leader = table.teams.front();
        career.addNews("[Mundo] " + divisionDisplay(divisionId) + ": " + leader->name +
                       " instala una historia de temporada con estilo " + leader->clubStyle + ".");
    }
    if (table.teams.size() >= 2 && randInt(1, 100) <= 10) {
        Team* bottom = table.teams.back();
        career.addNews("[Mundo] " + divisionDisplay(divisionId) + ": crece la presion en " + bottom->name +
                       " por su mala racha.");
    }
}

static void awardLegPoints(int goalsA, int goalsB, int& pointsA, int& pointsB) {
    if (goalsA > goalsB) {
        pointsA += 3;
    } else if (goalsB > goalsA) {
        pointsB += 3;
    } else {
        pointsA += 1;
        pointsB += 1;
    }
}

static Team* simulateSingleLegKnockout(Team* home, Team* away, const string& label, bool verbose, bool neutralVenue = false) {
    if (!home) return away;
    if (!away) return home;
    cout << label << ": " << home->name << " vs " << away->name << endl;
    Team h1 = *home;
    Team a1 = *away;
    MatchResult r = playMatch(h1, a1, verbose, true, neutralVenue);
    cout << "Resultado: " << home->name << " " << r.homeGoals << " - " << r.awayGoals << " " << away->name << endl;
    if (r.homeGoals > r.awayGoals) return home;
    if (r.awayGoals > r.homeGoals) return away;
    Team* winner = (teamPenaltyStrength(*home) >= teamPenaltyStrength(*away)) ? home : away;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

static Team* simulateTwoLegTie(Team* firstHome, Team* firstAway, const string& label, bool extraTimeFinal) {
    if (!firstHome) return firstAway;
    if (!firstAway) return firstHome;
    cout << label << ": " << firstAway->name << " vs " << firstHome->name << endl;

    Team h1 = *firstHome;
    Team a1 = *firstAway;
    MatchResult r1 = playMatch(h1, a1, false, true);
    Team h2 = *firstAway;
    Team a2 = *firstHome;
    MatchResult r2 = playMatch(h2, a2, false, true);

    int firstHomePoints = 0;
    int firstAwayPoints = 0;
    awardLegPoints(r1.homeGoals, r1.awayGoals, firstHomePoints, firstAwayPoints);
    awardLegPoints(r2.awayGoals, r2.homeGoals, firstHomePoints, firstAwayPoints);

    int firstHomeAgg = r1.homeGoals + r2.awayGoals;
    int firstAwayAgg = r1.awayGoals + r2.homeGoals;
    int firstHomeGD = firstHomeAgg - firstAwayAgg;
    int firstAwayGD = -firstHomeGD;

    cout << "Ida: " << firstHome->name << " " << r1.homeGoals << " - " << r1.awayGoals << " " << firstAway->name << endl;
    cout << "Vuelta: " << firstAway->name << " " << r2.homeGoals << " - " << r2.awayGoals << " " << firstHome->name << endl;
    cout << "Puntos serie: " << firstHome->name << " " << firstHomePoints << " - " << firstAwayPoints << " " << firstAway->name << endl;
    cout << "Global: " << firstHome->name << " " << firstHomeAgg << " - " << firstAwayAgg << " " << firstAway->name << endl;

    if (firstHomePoints > firstAwayPoints) return firstHome;
    if (firstAwayPoints > firstHomePoints) return firstAway;
    if (firstHomeGD > firstAwayGD) return firstHome;
    if (firstAwayGD > firstHomeGD) return firstAway;

    if (extraTimeFinal) {
        cout << "Serie igualada en puntos y diferencia de gol." << endl;
        int etHome = randInt(0, 1);
        int etAway = randInt(0, 1);
        cout << "Prorroga (vuelta): " << firstAway->name << " " << etHome << " - " << etAway << " " << firstHome->name << endl;
        if (etHome > etAway) return firstAway;
        if (etAway > etHome) return firstHome;
    }

    Team* winner = (teamPenaltyStrength(*firstHome) >= teamPenaltyStrength(*firstAway)) ? firstHome : firstAway;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

static Team* liguillaAscensoPrimeraB(const vector<Team*>& table) {
    if (table.size() < 2) return table.empty() ? nullptr : table.front();

    unordered_map<Team*, int> seedPos;
    for (size_t i = 0; i < table.size(); ++i) seedPos[table[i]] = static_cast<int>(i) + 1;

    Team* s2 = table.size() > 1 ? table[1] : nullptr;
    Team* s3 = table.size() > 2 ? table[2] : nullptr;
    Team* s4 = table.size() > 3 ? table[3] : nullptr;
    Team* s5 = table.size() > 4 ? table[4] : nullptr;
    Team* s6 = table.size() > 5 ? table[5] : nullptr;
    Team* s7 = table.size() > 6 ? table[6] : nullptr;
    Team* s8 = table.size() > 7 ? table[7] : nullptr;

    cout << "\nLiguilla de Ascenso (2°-8°)" << endl;
    Team* w1 = simulateTwoLegTie(s8, s3, "Primera ronda 1 (3° vs 8°)", false);
    Team* w2 = simulateTwoLegTie(s7, s4, "Primera ronda 2 (4° vs 7°)", false);
    Team* w3 = simulateTwoLegTie(s6, s5, "Primera ronda 3 (5° vs 6°)", false);

    vector<Team*> winners;
    if (w1) winners.push_back(w1);
    if (w2) winners.push_back(w2);
    if (w3) winners.push_back(w3);

    auto seed = [&](Team* t) {
        auto it = seedPos.find(t);
        return it == seedPos.end() ? 99 : it->second;
    };

    Team* worst = nullptr;
    int worstSeed = -1;
    for (auto* w : winners) {
        int s = seed(w);
        if (s > worstSeed) {
            worstSeed = s;
            worst = w;
        }
    }

    Team* semiA = nullptr;
    if (s2 && worst) {
        semiA = simulateTwoLegTie(worst, s2, "Semifinal A (2° vs peor clasificado)", false);
    } else {
        semiA = s2 ? s2 : worst;
    }

    vector<Team*> remaining;
    for (auto* w : winners) {
        if (w && w != worst) remaining.push_back(w);
    }

    Team* semiB = nullptr;
    if (remaining.size() == 2) {
        Team* a = remaining[0];
        Team* b = remaining[1];
        Team* firstHome = seed(a) > seed(b) ? a : b;
        Team* firstAway = (firstHome == a) ? b : a;
        semiB = simulateTwoLegTie(firstHome, firstAway, "Semifinal B", false);
    } else if (remaining.size() == 1) {
        semiB = remaining[0];
    }

    if (!semiA) return semiB;
    if (!semiB) return semiA;

    Team* firstHome = seed(semiA) > seed(semiB) ? semiA : semiB;
    Team* firstAway = (firstHome == semiA) ? semiB : semiA;
    Team* winner = simulateTwoLegTie(firstHome, firstAway, "Final Liguilla", true);
    if (winner) cout << "Ganador liguilla: " << winner->name << endl;
    return winner;
}

static Team* teamAtPos(const LeagueTable& table, int pos) {
    int idx = pos - 1;
    if (idx < 0 || idx >= static_cast<int>(table.teams.size())) return nullptr;
    return table.teams[idx];
}

static Team* loserOfTwoTeamTie(Team* winner, Team* a, Team* b) {
    if (!a) return b;
    if (!b) return a;
    return winner == a ? b : a;
}

static Team* simulateTwoLegAggregateTie(Team* firstHome, Team* firstAway, const string& label) {
    if (!firstHome) return firstAway;
    if (!firstAway) return firstHome;
    cout << label << ": " << firstAway->name << " vs " << firstHome->name << endl;

    Team h1 = *firstHome;
    Team a1 = *firstAway;
    MatchResult r1 = playMatch(h1, a1, false, true);
    Team h2 = *firstAway;
    Team a2 = *firstHome;
    MatchResult r2 = playMatch(h2, a2, false, true);

    int firstHomeAgg = r1.homeGoals + r2.awayGoals;
    int firstAwayAgg = r1.awayGoals + r2.homeGoals;

    cout << "Ida: " << firstHome->name << " " << r1.homeGoals << " - " << r1.awayGoals << " " << firstAway->name << endl;
    cout << "Vuelta: " << firstAway->name << " " << r2.homeGoals << " - " << r2.awayGoals << " " << firstHome->name << endl;
    cout << "Global: " << firstHome->name << " " << firstHomeAgg << " - " << firstAwayAgg << " " << firstAway->name << endl;

    if (firstHomeAgg > firstAwayAgg) return firstHome;
    if (firstAwayAgg > firstHomeAgg) return firstAway;

    Team* winner = (teamPenaltyStrength(*firstHome) >= teamPenaltyStrength(*firstAway)) ? firstHome : firstAway;
    cout << "Gana por penales: " << winner->name << endl;
    return winner;
}

struct TerceraBSeasonOutcome {
    vector<Team*> directPromoted;
    vector<Team*> promotionCandidates;
    Team* champion = nullptr;
};

struct TerceraARelegationOutcome {
    vector<Team*> promotionTeams;
    vector<Team*> directRelegated;
};

static TerceraBSeasonOutcome resolveTerceraBSeason(const vector<Team*>& northRanked,
                                                   const vector<Team*>& southRanked,
                                                   bool usePlayoffLosersForPromotion) {
    TerceraBSeasonOutcome out;
    Team* northChampion = northRanked.size() > 0 ? northRanked[0] : nullptr;
    Team* southChampion = southRanked.size() > 0 ? southRanked[0] : nullptr;

    if (northChampion) out.directPromoted.push_back(northChampion);
    if (southChampion && find(out.directPromoted.begin(), out.directPromoted.end(), southChampion) == out.directPromoted.end()) {
        out.directPromoted.push_back(southChampion);
    }

    if (northChampion && southChampion) {
        cout << "\nFinal por el campeonato de Tercera B:" << endl;
        out.champion = simulateSingleLegKnockout(northChampion, southChampion, "Final Tercera B", false, true);
        if (out.champion) cout << "Campeon de Tercera B: " << out.champion->name << endl;
    } else {
        out.champion = northChampion ? northChampion : southChampion;
    }

    Team* south2 = southRanked.size() > 1 ? southRanked[1] : nullptr;
    Team* north3 = northRanked.size() > 2 ? northRanked[2] : nullptr;
    Team* north2 = northRanked.size() > 1 ? northRanked[1] : nullptr;
    Team* south3 = southRanked.size() > 2 ? southRanked[2] : nullptr;

    cout << "\nPlayoffs de promocion Tercera B:" << endl;
    Team* tie1Winner = simulateTwoLegAggregateTie(south2, north3, "Cruce 1 (2° Sur vs 3° Norte)");
    Team* tie2Winner = simulateTwoLegAggregateTie(north2, south3, "Cruce 2 (2° Norte vs 3° Sur)");

    Team* candidate1 = usePlayoffLosersForPromotion ? loserOfTwoTeamTie(tie1Winner, south2, north3) : tie1Winner;
    Team* candidate2 = usePlayoffLosersForPromotion ? loserOfTwoTeamTie(tie2Winner, north2, south3) : tie2Winner;

    if (candidate1) out.promotionCandidates.push_back(candidate1);
    if (candidate2 && find(out.promotionCandidates.begin(), out.promotionCandidates.end(), candidate2) == out.promotionCandidates.end()) {
        out.promotionCandidates.push_back(candidate2);
    }
    return out;
}

static TerceraBSeasonOutcome inferTerceraBSeasonByValue(Career& career) {
    auto teams = career.getDivisionTeams("tercera division b");
    vector<Team*> north;
    vector<Team*> south;
    for (size_t i = 0; i < teams.size(); ++i) {
        if (i < 14) north.push_back(teams[i]);
        else if (i < 28) south.push_back(teams[i]);
    }
    north = sortByValue(north);
    south = sortByValue(south);
    return resolveTerceraBSeason(north, south, true);
}

static TerceraARelegationOutcome getInactiveTerceraARelegationByValue(Career& career) {
    TerceraARelegationOutcome out;
    auto teams = career.getDivisionTeams("tercera division a");
    teams = bottomByValue(teams, 4);
    if (teams.size() > 0) out.directRelegated.push_back(teams[0]);
    if (teams.size() > 1) out.directRelegated.push_back(teams[1]);
    if (teams.size() > 3) {
        out.promotionTeams.push_back(teams[3]);
        out.promotionTeams.push_back(teams[2]);
    } else if (teams.size() > 2) {
        out.promotionTeams.push_back(teams[2]);
    }
    return out;
}

static Team* simulateTerceraAPlayoff(const vector<Team*>& table) {
    if (table.size() < 5) return nullptr;
    Team* s2 = table[1];
    Team* s3 = table[2];
    Team* s4 = table[3];
    Team* s5 = table[4];

    cout << "\nPlayoff de ascenso Tercera A:" << endl;
    Team* sf1 = simulateTwoLegAggregateTie(s5, s2, "Semifinal 1 (2° vs 5°)");
    Team* sf2 = simulateTwoLegAggregateTie(s4, s3, "Semifinal 2 (3° vs 4°)");
    if (!sf1) return sf2;
    if (!sf2) return sf1;

    Team* winner = simulateSingleLegKnockout(sf1, sf2, "Final playoff Tercera A", false, true);
    if (winner) cout << "Ganador playoff Tercera A: " << winner->name << endl;
    return winner;
}

static Team* simulateSegundaPlayoff(const vector<Team*>& seeds) {
    if (seeds.empty()) return nullptr;
    if (seeds.size() == 1) return seeds.front();

    Team* s1 = seeds.size() > 0 ? seeds[0] : nullptr;
    Team* s2 = seeds.size() > 1 ? seeds[1] : nullptr;
    Team* s3 = seeds.size() > 2 ? seeds[2] : nullptr;
    Team* s4 = seeds.size() > 3 ? seeds[3] : nullptr;
    Team* s5 = seeds.size() > 4 ? seeds[4] : nullptr;
    Team* s6 = seeds.size() > 5 ? seeds[5] : nullptr;
    Team* s7 = seeds.size() > 6 ? seeds[6] : nullptr;

    cout << "\n--- Playoff de Ascenso ---" << endl;
    Team* q1 = simulatePlayoffMatch(s2, s7, "Cuartos 1");
    Team* q2 = simulatePlayoffMatch(s3, s6, "Cuartos 2");
    Team* q3 = simulatePlayoffMatch(s4, s5, "Cuartos 3");

    Team* semi1 = simulatePlayoffMatch(s1, q3, "Semifinal 1");
    Team* semi2 = simulatePlayoffMatch(q1, q2, "Semifinal 2");
    Team* champion = simulatePlayoffMatch(semi1, semi2, "Final");
    return champion;
}

static void endSeasonSegundaDivision(Career& career) {
    cout << "\nFin de temporada (Segunda Division)!" << endl;
    if (career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.buildSegundaGroups();
    }
    LeagueTable north = buildGroupTable(career, career.groupNorthIdx, "Grupo Norte");
    LeagueTable south = buildGroupTable(career, career.groupSouthIdx, "Grupo Sur");
    north.displayTable();
    south.displayTable();

    vector<Team*> playoffTeams;
    for (int pos = 1; pos <= 3; ++pos) {
        if (Team* t = teamAtPos(north, pos)) playoffTeams.push_back(t);
        if (Team* t = teamAtPos(south, pos)) playoffTeams.push_back(t);
    }

    Team* north4 = teamAtPos(north, 4);
    Team* south4 = teamAtPos(south, 4);
    Team* playoffExtra = nullptr;
    Team* descensoExtra = nullptr;
    if (north4 && south4) {
        cout << "\nPartido 4° vs 4°:" << endl;
        Team* winner = simulateSingleLegKnockout(north4, south4, "Repechaje 4°", north4 == career.myTeam || south4 == career.myTeam);
        playoffExtra = winner;
        descensoExtra = (winner == north4) ? south4 : north4;
    } else if (north4 || south4) {
        playoffExtra = north4 ? north4 : south4;
    }
    if (playoffExtra &&
        find(playoffTeams.begin(), playoffTeams.end(), playoffExtra) == playoffTeams.end()) {
        playoffTeams.push_back(playoffExtra);
    }

    vector<Team*> descensoTeams;
    for (int pos = 5; pos <= 7; ++pos) {
        if (Team* t = teamAtPos(north, pos)) descensoTeams.push_back(t);
        if (Team* t = teamAtPos(south, pos)) descensoTeams.push_back(t);
    }
    if (descensoExtra) descensoTeams.push_back(descensoExtra);

    vector<Team*> playoffSeeds = playoffTeams;
    if (!playoffSeeds.empty()) {
        LeagueTable seedTable;
        for (auto* t : playoffSeeds) seedTable.addTeam(t);
        seedTable.sortTable();
        playoffSeeds = seedTable.teams;
    }

    for (auto* team : career.activeTeams) {
        team->resetSeasonStats();
    }

    Team* champion = simulateSegundaPlayoff(playoffSeeds);
    if (champion) {
        cout << "Campeon del playoff: " << champion->name << endl;
    }

    LeagueTable descensoTable;
    if (!descensoTeams.empty()) {
        for (auto* t : descensoTeams) t->resetSeasonStats();
        auto schedule = buildRoundRobinIndexSchedule(static_cast<int>(descensoTeams.size()), false);
        cout << "\n--- Grupo de Descenso ---" << endl;
        int round = 1;
        for (const auto& roundMatches : schedule) {
            cout << "\nFecha Descenso " << round << endl;
            for (const auto& match : roundMatches) {
                Team* home = descensoTeams[match.first];
                Team* away = descensoTeams[match.second];
                bool verbose = (home == career.myTeam || away == career.myTeam);
                team_ai::adjustCpuTactics(*home, *away, career.myTeam);
                team_ai::adjustCpuTactics(*away, *home, career.myTeam);
                playMatch(*home, *away, verbose, true);
            }
            for (auto* team : descensoTeams) {
                healInjuries(*team, false);
                recoverFitness(*team, 7);
                player_dev::applyWeeklyTrainingPlan(*team);
            }
            round++;
        }
        descensoTable.title = "Grupo Descenso";
        for (auto* t : descensoTeams) descensoTable.addTeam(t);
        descensoTable.sortTable();
        descensoTable.displayTable();
    }

    vector<Team*> relegate;
    if (!descensoTable.teams.empty()) {
        int count = min(2, static_cast<int>(descensoTable.teams.size()));
        for (int i = 0; i < count; ++i) {
            relegate.push_back(descensoTable.teams[descensoTable.teams.size() - 1 - i]);
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> promote;
    if (!higher.empty() && champion) promote.push_back(champion);

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), static_cast<int>(promote.size()));
    vector<Team*> fromLower = lower.empty() ? vector<Team*>() : topByValue(career.getDivisionTeams(lower), static_cast<int>(relegate.size()));

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 50000;
            t->morale = 60;
        }
    }
    for (auto* t : relegate) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 20000);
            t->morale = 40;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : fromLower) {
        t->division = career.activeDivision;
        t->morale = 55;
    }

    if (!higher.empty() && !promote.empty()) {
        cout << "Ascenso: " << promote.front()->name << endl;
    }
    if (!lower.empty() && !relegate.empty()) {
        cout << "Descensos: ";
        for (size_t i = 0; i < relegate.size(); ++i) {
            if (i) cout << ", ";
            cout << relegate[i]->name;
        }
        cout << endl;
    }
    awardSeasonPrizeMoney(career, buildRelevantCompetitionTable(career));
    recordSeasonHistory(career, champion ? champion->name : "", promote, relegate, "Temporada con playoff de ascenso y grupo de descenso.");
    advanceToNextSeason(career);
}

static void endSeasonPrimeraB(Career& career) {
    cout << "\nFin de temporada (Primera B)!" << endl;
    career.leagueTable.displayTable();

    vector<Team*> table = career.leagueTable.teams;
    vector<Team*> seeded = table;
    Team* champion = nullptr;
    if (!table.empty()) {
        if (table.size() >= 2) {
            int topPts = table[0]->points;
            int tiedCount = 0;
            for (auto* t : table) {
                if (t->points == topPts) tiedCount++;
                else break;
            }
            if (tiedCount >= 2) {
                Team* a = table[0];
                Team* b = table[1];
                cout << "\nFinal por el titulo (empate en puntos):" << endl;
                if (tiedCount > 2) {
                    cout << "Empate multiple reducido por criterios de tabla a: "
                         << a->name << " y " << b->name << endl;
                }
                champion = simulateSingleLegKnockout(a, b, "Final", a == career.myTeam || b == career.myTeam, true);
                if (champion && champion != seeded[0]) {
                    auto it = find(seeded.begin(), seeded.end(), champion);
                    if (it != seeded.end()) {
                        Team* oldFirst = seeded[0];
                        size_t idx = static_cast<size_t>(it - seeded.begin());
                        seeded[idx] = oldFirst;
                        seeded[0] = champion;
                    }
                }
            } else {
                champion = table[0];
            }
        } else {
            champion = table[0];
        }
    }
    if (champion) {
        cout << "Campeon fase regular: " << champion->name << endl;
    }

    Team* liguillaWinner = liguillaAscensoPrimeraB(seeded);

    Team* relegated = nullptr;
    if (!table.empty()) {
        int n = static_cast<int>(table.size());
        int bottomPts = table[n - 1]->points;
        int tied = 0;
        for (int i = n - 1; i >= 0; --i) {
            if (table[i]->points == bottomPts) tied++;
            else break;
        }
        if (tied >= 2 && n >= 2) {
            Team* a = table[n - 1];
            Team* b = table[n - 2];
            cout << "\nDefinicion de descenso (empate en el ultimo lugar):" << endl;
            if (tied > 2) {
                cout << "Empate multiple reducido por criterios de tabla a: "
                     << b->name << " y " << a->name << endl;
            }
            Team* winner = simulateSingleLegKnockout(a, b, "Definicion descenso", a == career.myTeam || b == career.myTeam, true);
            relegated = (winner == a) ? b : a;
        } else {
            relegated = table[n - 1];
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> promote;
    if (!higher.empty() && champion) promote.push_back(champion);
    if (!higher.empty() && liguillaWinner &&
        find(promote.begin(), promote.end(), liguillaWinner) == promote.end()) {
        promote.push_back(liguillaWinner);
    }

    vector<Team*> relegate;
    if (!lower.empty() && relegated) relegate.push_back(relegated);

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), static_cast<int>(promote.size()));
    vector<Team*> fromLower = lower.empty() ? vector<Team*>() : topByValue(career.getDivisionTeams(lower), static_cast<int>(relegate.size()));

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 50000;
            t->morale = 60;
        }
    }
    for (auto* t : relegate) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 20000);
            t->morale = 40;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : fromLower) {
        t->division = career.activeDivision;
        t->morale = 55;
    }

    if (!higher.empty() && !promote.empty()) {
        cout << "Ascensos: ";
        for (size_t i = 0; i < promote.size(); ++i) {
            if (i) cout << ", ";
            cout << promote[i]->name;
        }
        cout << endl;
    }
    if (!lower.empty() && !relegate.empty()) {
        cout << "Descenso: " << relegate.front()->name << endl;
    }
    awardSeasonPrizeMoney(career, career.leagueTable);
    recordSeasonHistory(career, champion ? champion->name : "", promote, relegate, "Campeon regular y liguilla de ascenso.");
    advanceToNextSeason(career);
}

static void endSeasonTerceraA(Career& career) {
    cout << "\nFin de temporada (Tercera Division A)!" << endl;
    career.leagueTable.displayTable();

    vector<Team*> table = career.leagueTable.teams;
    Team* champion = table.empty() ? nullptr : table.front();
    if (champion) {
        cout << "Ascenso directo a Segunda: " << champion->name << endl;
    }

    Team* playoffWinner = simulateTerceraAPlayoff(table);

    vector<Team*> promotionTeamsA;
    vector<Team*> directRelegated;
    if (table.size() >= 14) {
        promotionTeamsA.push_back(table[12]);
        promotionTeamsA.push_back(table[13]);
    } else if (table.size() >= 12) {
        promotionTeamsA.push_back(table[table.size() - 2]);
        promotionTeamsA.push_back(table.back());
    }
    if (table.size() >= 16) {
        directRelegated.push_back(table[14]);
        directRelegated.push_back(table[15]);
    } else if (table.size() >= 14) {
        directRelegated.push_back(table[table.size() - 2]);
        directRelegated.push_back(table.back());
    }

    TerceraBSeasonOutcome lowerOutcome = inferTerceraBSeasonByValue(career);
    vector<Team*> promotedByPromotion;
    vector<Team*> relegatedByPromotion;
    int promotionTies = min(static_cast<int>(promotionTeamsA.size()), static_cast<int>(lowerOutcome.promotionCandidates.size()));
    for (int i = 0; i < promotionTies; ++i) {
        Team* aTeam = promotionTeamsA[i];
        Team* bTeam = lowerOutcome.promotionCandidates[i];
        cout << "\nPromocion Tercera A/B " << i + 1 << ":" << endl;
        Team* winner = simulateTwoLegAggregateTie(bTeam, aTeam, "Llave de promocion");
        if (winner == bTeam) {
            promotedByPromotion.push_back(bTeam);
            relegatedByPromotion.push_back(aTeam);
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> promote;
    if (!higher.empty() && champion) promote.push_back(champion);
    if (!higher.empty() && playoffWinner &&
        find(promote.begin(), promote.end(), playoffWinner) == promote.end()) {
        promote.push_back(playoffWinner);
    }

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), static_cast<int>(promote.size()));
    vector<Team*> fromLower = lowerOutcome.directPromoted;

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 40000;
            t->morale = 60;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : directRelegated) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 15000);
            t->morale = 40;
        }
    }
    for (auto* t : fromLower) {
        if (!lower.empty()) {
            t->division = career.activeDivision;
            t->morale = 58;
        }
    }
    for (auto* t : relegatedByPromotion) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 15000);
            t->morale = 42;
        }
    }
    for (auto* t : promotedByPromotion) {
        if (!lower.empty()) {
            t->division = career.activeDivision;
            t->morale = 58;
        }
    }

    if (!promote.empty()) {
        cout << "Ascensos a Segunda: ";
        for (size_t i = 0; i < promote.size(); ++i) {
            if (i) cout << ", ";
            cout << promote[i]->name;
        }
        cout << endl;
    }
    if (!directRelegated.empty()) {
        cout << "Descensos directos a Tercera B: ";
        for (size_t i = 0; i < directRelegated.size(); ++i) {
            if (i) cout << ", ";
            cout << directRelegated[i]->name;
        }
        cout << endl;
    }
    if (!fromLower.empty()) {
        cout << "Ascensos directos desde Tercera B: ";
        for (size_t i = 0; i < fromLower.size(); ++i) {
            if (i) cout << ", ";
            cout << fromLower[i]->name;
        }
        cout << endl;
    }
    if (!promotedByPromotion.empty()) {
        cout << "Ascensos por promocion desde Tercera B: ";
        for (size_t i = 0; i < promotedByPromotion.size(); ++i) {
            if (i) cout << ", ";
            cout << promotedByPromotion[i]->name;
        }
        cout << endl;
    }
    if (!relegatedByPromotion.empty()) {
        cout << "Perdieron la promocion y bajan a Tercera B: ";
        for (size_t i = 0; i < relegatedByPromotion.size(); ++i) {
            if (i) cout << ", ";
            cout << relegatedByPromotion[i]->name;
        }
        cout << endl;
    }
    vector<Team*> allRelegated = directRelegated;
    allRelegated.insert(allRelegated.end(), relegatedByPromotion.begin(), relegatedByPromotion.end());
    vector<Team*> allPromoted = promote;
    allPromoted.insert(allPromoted.end(), promotedByPromotion.begin(), promotedByPromotion.end());
    awardSeasonPrizeMoney(career, career.leagueTable);
    recordSeasonHistory(career, champion ? champion->name : "", allPromoted, allRelegated, "Ascenso directo, playoff y promocion interdivisional.");
    advanceToNextSeason(career);
}

static void endSeasonTerceraB(Career& career) {
    cout << "\nFin de temporada (Tercera Division B)!" << endl;
    if (career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.buildRegionalGroups();
    }

    LeagueTable north = buildGroupTable(career, career.groupNorthIdx, "Zona Norte");
    LeagueTable south = buildGroupTable(career, career.groupSouthIdx, "Zona Sur");
    north.displayTable();
    south.displayTable();

    TerceraBSeasonOutcome outcome = resolveTerceraBSeason(north.teams, south.teams, true);
    TerceraARelegationOutcome higherOutcome = getInactiveTerceraARelegationByValue(career);

    vector<Team*> promotedByPromotion;
    vector<Team*> relegatedByPromotion;
    int promotionTies = min(static_cast<int>(higherOutcome.promotionTeams.size()), static_cast<int>(outcome.promotionCandidates.size()));
    for (int i = 0; i < promotionTies; ++i) {
        Team* aTeam = higherOutcome.promotionTeams[i];
        Team* bTeam = outcome.promotionCandidates[i];
        cout << "\nPromocion Tercera A/B " << i + 1 << ":" << endl;
        Team* winner = simulateTwoLegAggregateTie(bTeam, aTeam, "Llave de promocion");
        if (winner == bTeam) {
            promotedByPromotion.push_back(bTeam);
            relegatedByPromotion.push_back(aTeam);
        }
    }

    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";

    for (auto* t : outcome.directPromoted) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 25000;
            t->morale = 60;
        }
    }
    for (auto* t : higherOutcome.directRelegated) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : promotedByPromotion) {
        if (!higher.empty()) {
            t->division = higher;
            t->morale = 58;
        }
    }
    for (auto* t : relegatedByPromotion) {
        t->division = career.activeDivision;
        t->morale = 45;
    }

    if (!outcome.directPromoted.empty()) {
        cout << "Ascensos directos a Tercera A: ";
        for (size_t i = 0; i < outcome.directPromoted.size(); ++i) {
            if (i) cout << ", ";
            cout << outcome.directPromoted[i]->name;
        }
        cout << endl;
    }
    if (!promotedByPromotion.empty()) {
        cout << "Ascensos por promocion a Tercera A: ";
        for (size_t i = 0; i < promotedByPromotion.size(); ++i) {
            if (i) cout << ", ";
            cout << promotedByPromotion[i]->name;
        }
        cout << endl;
    }
    vector<Team*> allPromoted = outcome.directPromoted;
    allPromoted.insert(allPromoted.end(), promotedByPromotion.begin(), promotedByPromotion.end());
    awardSeasonPrizeMoney(career, buildRelevantCompetitionTable(career));
    recordSeasonHistory(career, outcome.champion ? outcome.champion->name : "", allPromoted, {}, "Campeones zonales, final y promocion por playoff.");
    advanceToNextSeason(career);
}

void endSeason(Career& career) {
    const CompetitionConfig& config = getCompetitionConfig(career.activeDivision);
    switch (config.seasonHandler) {
        case CompetitionSeasonHandler::SegundaGroups:
            endSeasonSegundaDivision(career);
            return;
        case CompetitionSeasonHandler::PrimeraB:
            endSeasonPrimeraB(career);
            return;
        case CompetitionSeasonHandler::TerceraA:
            endSeasonTerceraA(career);
            return;
        case CompetitionSeasonHandler::TerceraB:
            endSeasonTerceraB(career);
            return;
        default:
            break;
    }
    cout << "\nFin de temporada!" << endl;
    career.leagueTable.displayTable();
    Team* champion = nullptr;
    if (!career.leagueTable.teams.empty()) {
        if (config.seasonHandler == CompetitionSeasonHandler::PrimeraDivision &&
            career.leagueTable.teams.size() >= 2) {
            int topPts = career.leagueTable.teams.front()->points;
            int tiedCount = 0;
            for (auto* t : career.leagueTable.teams) {
                if (t->points == topPts) tiedCount++;
                else break;
            }
            if (tiedCount >= 2) {
                Team* a = career.leagueTable.teams[0];
                Team* b = career.leagueTable.teams[1];
                cout << "\nFinal por el titulo (empate en puntos):" << endl;
                if (tiedCount > 2) {
                    cout << "Empate multiple reducido por criterios de tabla a: "
                         << a->name << " y " << b->name << endl;
                }
                champion = simulateSingleLegKnockout(a, b, "Final", a == career.myTeam || b == career.myTeam, true);
            } else {
                champion = career.leagueTable.teams.front();
            }
        } else {
            champion = career.leagueTable.teams.front();
        }
    }
    if (champion) {
        cout << "Campeon: " << champion->name << endl;
    }
    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> table = career.leagueTable.teams;
    int n = static_cast<int>(table.size());
    vector<Team*> promote;
    vector<Team*> relegate;
    int promoteSlots = higher.empty() ? 0 : 2;
    int relegateSlots = lower.empty() ? 0 : 2;

    if (promoteSlots > 0) {
        int count = min(promoteSlots, n);
        for (int i = 0; i < count; ++i) {
            promote.push_back(table[i]);
        }
    }

    int actualPromote = static_cast<int>(promote.size());
    int relegateCount = min(relegateSlots, max(0, n - actualPromote));
    for (int i = 0; i < relegateCount; ++i) {
        relegate.push_back(table[n - 1 - i]);
    }

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), actualPromote);
    vector<Team*> fromLower;
    if (!lower.empty() &&
        config.seasonHandler == CompetitionSeasonHandler::PrimeraDivision &&
        getCompetitionConfig(lower).seasonHandler == CompetitionSeasonHandler::PrimeraB) {
        auto pbTable = sortByValue(career.getDivisionTeams(lower));
        if (!pbTable.empty()) {
            Team* pbChampion = pbTable[0];
            Team* pbPlayoff = liguillaAscensoPrimeraB(pbTable);
            if (pbChampion) fromLower.push_back(pbChampion);
            if (pbPlayoff && pbPlayoff != pbChampion) fromLower.push_back(pbPlayoff);
            for (size_t i = 1; i < pbTable.size() && static_cast<int>(fromLower.size()) < relegateCount; ++i) {
                Team* candidate = pbTable[i];
                if (find(fromLower.begin(), fromLower.end(), candidate) == fromLower.end()) {
                    fromLower.push_back(candidate);
                }
            }
        }
    } else {
        fromLower = lower.empty() ? vector<Team*>() : topByValue(career.getDivisionTeams(lower), relegateCount);
    }

    for (auto* t : promote) {
        if (!higher.empty()) {
            t->division = higher;
            t->budget += 50000;
            t->morale = 60;
        }
    }
    for (auto* t : relegate) {
        if (!lower.empty()) {
            t->division = lower;
            t->budget = max(0LL, t->budget - 20000);
            t->morale = 40;
        }
    }
    for (auto* t : fromHigher) {
        t->division = career.activeDivision;
        t->morale = 45;
    }
    for (auto* t : fromLower) {
        t->division = career.activeDivision;
        t->morale = 55;
    }

    if (!higher.empty() && !promote.empty()) {
        cout << "Ascensos: ";
        for (size_t i = 0; i < promote.size(); ++i) {
            if (i) cout << ", ";
            cout << promote[i]->name;
        }
        cout << endl;
    }
    if (!lower.empty() && !relegate.empty()) {
        cout << "Descensos: ";
        for (size_t i = 0; i < relegate.size(); ++i) {
            if (i) cout << ", ";
            cout << relegate[i]->name;
        }
        cout << endl;
    }
    awardSeasonPrizeMoney(career, career.leagueTable);
    recordSeasonHistory(career, champion ? champion->name : "", promote, relegate, "Cierre de temporada regular.");
    advanceToNextSeason(career);
}

