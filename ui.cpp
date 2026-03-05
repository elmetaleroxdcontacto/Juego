#include "ui.h"

#include "simulation.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <unordered_map>

using namespace std;

void transferMarket(Team& team) {
    const int minSquadSize = 18;
    cout << "\n=== Mercado de Transferencias ===" << endl;
    cout << "Presupuesto: $" << team.budget << endl;
    cout << "1. Comprar jugador" << endl;
    cout << "2. Vender jugador" << endl;
    cout << "3. Volver" << endl;
    int choice = readInt("Elige una opcion: ", 1, 3);

    if (choice == 1) {
        Player p;
        p.name = readLine("Nombre del jugador: ");
        string posInput = readLine("Posicion (ARQ/DEF/MED/DEL): ");
        p.position = normalizePosition(posInput);
        if (p.position == "N/A") p.position = "MED";
        int minSkill, maxSkill;
        getDivisionSkillRange(team.division, minSkill, maxSkill);
        p.skill = randInt(minSkill, maxSkill);
        p.age = randInt(18, 32);
        p.stamina = clampInt(p.skill + randInt(-5, 5), 30, 99);
        p.fitness = p.stamina;
        p.potential = clampInt(p.skill + randInt(0, 8), p.skill, 95);
        if (p.position == "ARQ") {
            p.attack = clampInt(p.skill - 30, 10, 70);
            p.defense = clampInt(p.skill + 10, 30, 99);
        } else if (p.position == "DEF") {
            p.attack = clampInt(p.skill - 10, 20, 90);
            p.defense = clampInt(p.skill + 10, 30, 99);
        } else if (p.position == "MED") {
            p.attack = clampInt(p.skill, 25, 99);
            p.defense = clampInt(p.skill, 25, 99);
        } else {
            p.attack = clampInt(p.skill + 10, 30, 99);
            p.defense = clampInt(p.skill - 10, 20, 90);
        }
        p.value = static_cast<long long>(p.skill) * 10000;
        p.wage = static_cast<long long>(p.skill) * 150 + randInt(0, 600);
        p.contractWeeks = randInt(52, 156);
        p.injured = false;
        p.injuryType = "";
        p.injuryWeeks = 0;
        p.injuryHistory = 0;
        p.goals = 0;
        p.assists = 0;
        p.matchesPlayed = 0;
        p.lastTrainedSeason = -1;
        p.lastTrainedWeek = -1;
        p.role = defaultRoleForPosition(p.position);
        if (team.budget >= p.value) {
            team.budget -= p.value;
            team.addPlayer(p);
            cout << "Jugador comprado." << endl;
        } else {
            cout << "Presupuesto insuficiente." << endl;
        }
    } else if (choice == 2) {
        if (team.players.size() <= static_cast<size_t>(minSquadSize)) {
            cout << "No puedes vender. Debes mantener al menos " << minSquadSize << " jugadores." << endl;
            return;
        }
        cout << "Selecciona jugador para vender:" << endl;
        for (size_t i = 0; i < team.players.size(); ++i) {
            cout << i + 1 << ". " << team.players[i].name << " ($" << team.players[i].value << ")" << endl;
        }
        int idx = readInt("Elige jugador: ", 1, static_cast<int>(team.players.size()));
        team.budget += team.players[idx - 1].value;
        cout << team.players[idx - 1].name << " vendido por $" << team.players[idx - 1].value << endl;
        team.players.erase(team.players.begin() + idx - 1);
    }
}

void scoutPlayers(Team& team) {
    cout << "\n=== Ojeo de Jugadores ===" << endl;
    cout << "Presupuesto: $" << team.budget << endl;
    cout << "Costo de ojeo: $5000" << endl;
    cout << "1. Otear jugadores" << endl;
    cout << "2. Volver" << endl;
    int choice = readInt("Elige una opcion: ", 1, 2);

    if (choice == 1) {
        if (team.budget < 5000LL) {
            cout << "Presupuesto insuficiente para ojeo." << endl;
            return;
        }
        team.budget -= 5000LL;
        int minSkill, maxSkill;
        getDivisionSkillRange(team.division, minSkill, maxSkill);
        vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
        for (int i = 0; i < 3; ++i) {
            string pos = positions[randInt(0, static_cast<int>(positions.size()) - 1)];
            Player p = makeRandomPlayer(pos, minSkill, maxSkill, 18, 32);
            int err = randInt(3, 8);
            int estAtkLo = clampInt(p.attack - err, 0, 100);
            int estAtkHi = clampInt(p.attack + err, 0, 100);
            int estDefLo = clampInt(p.defense - err, 0, 100);
            int estDefHi = clampInt(p.defense + err, 0, 100);
            int estSkillLo = clampInt(p.skill - err, 0, 100);
            int estSkillHi = clampInt(p.skill + err, 0, 100);
            cout << "Jugador encontrado: " << p.name << " (" << p.position << ") - Ataque: "
                 << estAtkLo << "-" << estAtkHi << ", Defensa: " << estDefLo << "-" << estDefHi
                 << ", Habilidad: " << estSkillLo << "-" << estSkillHi << ", Valor: $" << p.value << endl;
            int buy = readInt("Comprar? (1. Si, 2. No): ", 1, 2);
            if (buy == 1 && team.budget >= p.value) {
                team.budget -= p.value;
                team.addPlayer(p);
                cout << "Jugador comprado." << endl;
            } else if (buy == 1) {
                cout << "Presupuesto insuficiente." << endl;
            }
        }
    }
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

void checkAchievements(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->wins >= 10 &&
        find(career.achievements.begin(), career.achievements.end(), "10 Victorias") == career.achievements.end()) {
        career.achievements.push_back("10 Victorias");
        cout << "Logro desbloqueado: 10 Victorias!" << endl;
    }
}

void displayMainMenu() {
    cout << "\n=== Football Manager Game ===" << endl;
    cout << "1. Modo Carrera" << endl;
    cout << "2. Iniciar Juego Rapido" << endl;
    cout << "3. Modo Copa" << endl;
    cout << "4. Salir" << endl;
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
    cout << "5. Ver Tabla de Posiciones" << endl;
    cout << "6. Mercado de Transferencias" << endl;
    cout << "7. Ojeo de Jugadores" << endl;
    cout << "8. Ver Estadisticas" << endl;
    cout << "9. Ver Logros" << endl;
    cout << "10. Guardar Carrera" << endl;
    cout << "11. Retirar Jugador" << endl;
    cout << "12. Plan de Entrenamiento" << endl;
    cout << "13. Volver al Menu Principal" << endl;
    cout << "14. Editar Equipo" << endl;
}

void viewTeam(Team& team) {
    cout << "\nEquipo: " << team.name << endl;
    cout << "Tacticas: " << team.tactics << ", Formacion: " << team.formation
         << ", Plan: " << team.trainingFocus << ", Presupuesto: $" << team.budget << endl;
    cout << "Moral: " << team.morale << " | Temporada: " << team.wins << "G-" << team.draws << "E-" << team.losses << "P"
         << " | GF " << team.goalsFor << " / GA " << team.goalsAgainst << " | Pts " << team.points << endl;
    if (team.players.empty()) {
        cout << "No hay jugadores en el equipo." << endl;
        return;
    }
    int injuredCount = 0;
    int lowFitCount = 0;
    int totalFit = 0;
    for (const auto& p : team.players) {
        if (p.injured) injuredCount++;
        if (p.fitness < 60) lowFitCount++;
        totalFit += p.fitness;
    }
    int avgFit = team.players.empty() ? 0 : totalFit / static_cast<int>(team.players.size());
    if (injuredCount >= 3 || avgFit < 65) {
        cout << "[AVISO] Plantel con baja condicion: " << injuredCount << " lesionados, "
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
             << ", Condicion: " << p.fitness << endl;
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
                 << ", Def " << p.defense << ", Res " << p.stamina
                 << ", Cond " << p.fitness << ", Hab " << p.skill << ", Pot " << p.potential
                 << ", Rol " << p.role << ", Edad " << p.age << ", Valor $" << p.value
                 << ", Salario $" << p.wage << ", Contrato " << p.contractWeeks << " sem" << endl;
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
    p.contractWeeks = randInt(52, 156);
    p.injured = false;
    p.injuryType = "";
    p.injuryWeeks = 0;
    p.injuryHistory = 0;
    p.goals = 0;
    p.assists = 0;
    p.matchesPlayed = 0;
    p.lastTrainedSeason = -1;
    p.lastTrainedWeek = -1;
    p.role = defaultRoleForPosition(p.position);
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
    cout << "Tacticas cambiadas a " << team.tactics << endl;
}

void setTrainingPlan(Team& team) {
    cout << "\nPlan de entrenamiento actual: " << team.trainingFocus << endl;
    cout << "1. Balanceado" << endl;
    cout << "2. Fisico" << endl;
    cout << "3. Tecnico" << endl;
    cout << "4. Tactico" << endl;
    int choice = readInt("Elige plan: ", 1, 4);
    switch (choice) {
        case 1: team.trainingFocus = "Balanceado"; break;
        case 2: team.trainingFocus = "Fisico"; break;
        case 3: team.trainingFocus = "Tecnico"; break;
        case 4: team.trainingFocus = "Tactico"; break;
        default: break;
    }
    cout << "Plan actualizado a " << team.trainingFocus << endl;
}

static void applyTrainingPlan(Team& team) {
    string focus = team.trainingFocus.empty() ? "Balanceado" : team.trainingFocus;
    if (focus == "Fisico") {
        for (auto& p : team.players) {
            if (p.injured) continue;
            p.fitness = clampInt(p.fitness + 2, 15, p.stamina);
            if (randInt(1, 100) <= 12) {
                p.stamina = min(100, p.stamina + 1);
                if (p.fitness > p.stamina) p.fitness = p.stamina;
            }
        }
    } else if (focus == "Tecnico") {
        for (auto& p : team.players) {
            if (p.injured) continue;
            if (p.skill >= p.potential) continue;
            if (randInt(1, 100) <= 18) {
                p.skill = min(100, p.skill + 1);
                string pos = normalizePosition(p.position);
                if (pos == "ARQ" || pos == "DEF") p.defense = min(100, p.defense + 1);
                else if (pos == "MED") {
                    p.attack = min(100, p.attack + 1);
                    p.defense = min(100, p.defense + 1);
                } else {
                    p.attack = min(100, p.attack + 1);
                }
            }
        }
    } else if (focus == "Tactico") {
        team.morale = clampInt(team.morale + 2, 0, 100);
        for (auto& p : team.players) {
            if (p.injured) continue;
            p.fitness = clampInt(p.fitness + 1, 15, p.stamina);
        }
    } else {
        for (auto& p : team.players) {
            if (p.injured) continue;
            p.fitness = clampInt(p.fitness + 1, 15, p.stamina);
        }
    }
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
        cout << p.name << ": Goles " << p.goals << ", Asistencias " << p.assists << ", Partidos " << p.matchesPlayed << endl;
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

static bool isSegundaFormat(const Career& career) {
    return career.usesSegundaFormat();
}

static LeagueTable buildGroupTable(const Career& career, const vector<int>& idx, const string& title) {
    LeagueTable table;
    table.title = title;
    for (int i : idx) {
        if (i >= 0 && i < static_cast<int>(career.activeTeams.size())) {
            table.addTeam(career.activeTeams[i]);
        }
    }
    table.sortTable();
    return table;
}

static int groupForTeam(const Career& career, const Team* team) {
    for (int i : career.groupNorthIdx) {
        if (i >= 0 && i < static_cast<int>(career.activeTeams.size()) && career.activeTeams[i] == team) return 0;
    }
    for (int i : career.groupSouthIdx) {
        if (i >= 0 && i < static_cast<int>(career.activeTeams.size()) && career.activeTeams[i] == team) return 1;
    }
    return -1;
}

void displayLeagueTables(Career& career) {
    if (!isSegundaFormat(career) || career.groupNorthIdx.empty() || career.groupSouthIdx.empty()) {
        career.leagueTable.displayTable();
        return;
    }
    LeagueTable north = buildGroupTable(career, career.groupNorthIdx, "Grupo Norte");
    LeagueTable south = buildGroupTable(career, career.groupSouthIdx, "Grupo Sur");
    north.displayTable();
    south.displayTable();
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
    if (division == "primera division") return 60000;
    if (division == "primera b") return 45000;
    if (division == "segunda division") return 35000;
    if (division == "tercera division a") return 25000;
    return 20000;
}

static int divisionWageFactor(const string& division) {
    if (division == "primera division") return 85;
    if (division == "primera b") return 70;
    if (division == "segunda division") return 60;
    if (division == "tercera division a") return 50;
    return 45;
}

static long long weeklyWage(const Team& team) {
    long long sum = 0;
    for (const auto& p : team.players) sum += p.wage;
    int factor = divisionWageFactor(team.division);
    return sum * factor / 100;
}

static void applyWeeklyFinances(Career& career, const vector<int>& pointsBefore) {
    for (size_t i = 0; i < career.activeTeams.size(); ++i) {
        Team* team = career.activeTeams[i];
        int pointsDelta = team->points - pointsBefore[i];
        long long income = divisionBaseIncome(team->division) + pointsDelta * 4000LL + randInt(0, 3000);
        long long wages = weeklyWage(*team);
        long long net = income - wages;
        team->budget = max(0LL, team->budget + net);
        if (career.myTeam == team) {
            cout << "Finanzas semanales: +" << income << " / -" << wages << " = " << net << endl;
        }
    }
}

static void weeklyDashboard(const Career& career) {
    if (!career.myTeam) return;
    int rank = teamRank(career.leagueTable, career.myTeam);
    if (isSegundaFormat(career)) {
        int group = groupForTeam(career, career.myTeam);
        if (group == 0) {
            LeagueTable north = buildGroupTable(career, career.groupNorthIdx, "Grupo Norte");
            rank = teamRank(north, career.myTeam);
        } else if (group == 1) {
            LeagueTable south = buildGroupTable(career, career.groupSouthIdx, "Grupo Sur");
            rank = teamRank(south, career.myTeam);
        }
    }
    int injured = 0;
    int expiring = 0;
    for (const auto& p : career.myTeam->players) {
        if (p.injured) injured++;
        if (p.contractWeeks > 0 && p.contractWeeks <= 4) expiring++;
    }
    cout << "\n--- Resumen Semanal ---" << endl;
    cout << "Posicion: " << rank << " | Pts: " << career.myTeam->points
         << " | Moral: " << career.myTeam->morale
         << " | Presupuesto: $" << career.myTeam->budget << endl;
    cout << "Lesionados: " << injured << " | Contratos por vencer (<=4 sem): " << expiring << endl;
}

static void applyClubEvent(Career& career) {
    if (!career.myTeam) return;
    if (randInt(1, 100) > 15) return;
    int event = randInt(1, 4);
    if (event == 1) {
        long long bonus = 50000 + randInt(0, 30000);
        career.myTeam->budget += bonus;
        cout << "[Evento] Patrocinio sorpresa: +" << bonus << endl;
    } else if (event == 2) {
        career.myTeam->morale = clampInt(career.myTeam->morale - 5, 0, 100);
        cout << "[Evento] Protesta de hinchas: moral -5." << endl;
    } else if (event == 3) {
        if (!career.myTeam->players.empty()) {
            int idx = randInt(0, static_cast<int>(career.myTeam->players.size()) - 1);
            Player& p = career.myTeam->players[idx];
            p.injured = true;
            p.injuryType = "Leve";
            p.injuryWeeks = randInt(1, 2);
            p.injuryHistory++;
            cout << "[Evento] Accidente en entrenamiento: " << p.name << " fuera " << p.injuryWeeks << " semanas." << endl;
        }
    } else {
        int minSkill, maxSkill;
        getDivisionSkillRange(career.myTeam->division, minSkill, maxSkill);
        Player youth = makeRandomPlayer("MED", minSkill, maxSkill, 16, 18);
        youth.potential = clampInt(youth.skill + randInt(8, 15), youth.skill, 99);
        career.myTeam->addPlayer(youth);
        cout << "[Evento] Cantera: se unio " << youth.name << " (pot " << youth.potential << ")." << endl;
    }
}

static void adjustCpuTactics(Team& team, const Team& opponent, const Team* myTeam) {
    if (&team == myTeam) return;
    int diff = team.getAverageSkill() - opponent.getAverageSkill();
    if (team.morale >= 70) {
        team.tactics = "Pressing";
    } else if (diff >= 6) {
        team.tactics = "Offensive";
    } else if (diff <= -6) {
        team.tactics = "Defensive";
    } else if (team.morale <= 35) {
        team.tactics = "Defensive";
    } else {
        team.tactics = "Balanced";
    }
}

static void addYouthPlayers(Team& team, int count) {
    int minSkill, maxSkill;
    getDivisionSkillRange(team.division, minSkill, maxSkill);
    for (int i = 0; i < count; ++i) {
        vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
        string pos = positions[randInt(0, static_cast<int>(positions.size()) - 1)];
        Player youth = makeRandomPlayer(pos, minSkill, maxSkill, 16, 18);
        youth.potential = clampInt(youth.skill + randInt(10, 18), youth.skill, 99);
        team.addPlayer(youth);
    }
}

static void processIncomingOffers(Career& career) {
    if (!career.myTeam) return;
    if (career.myTeam->players.size() <= 11) return;
    if (randInt(1, 100) > 25) return;
    vector<int> candidates;
    for (size_t i = 0; i < career.myTeam->players.size(); ++i) {
        if (!career.myTeam->players[i].injured) candidates.push_back(static_cast<int>(i));
    }
    if (candidates.empty()) return;
    int idx = candidates[randInt(0, static_cast<int>(candidates.size()) - 1)];
    Player& p = career.myTeam->players[idx];
    long long offer = static_cast<long long>(p.value * (0.9 + randInt(0, 40) / 100.0));
    cout << "\nOferta recibida por " << p.name << ": $" << offer << endl;
    int choice = readInt("Aceptar? (1. Si, 2. No): ", 1, 2);
    if (choice == 1) {
        career.myTeam->budget += offer;
        cout << "Transferencia aceptada. " << p.name << " vendido." << endl;
        career.myTeam->players.erase(career.myTeam->players.begin() + idx);
    } else {
        cout << "Oferta rechazada." << endl;
    }
}

static void processCpuTransfers(Career& career) {
    for (auto* team : career.activeTeams) {
        if (career.myTeam == team) continue;
        if (randInt(1, 100) > 8) continue;
        if (team->players.size() <= 18) continue;
        int sellIdx = randInt(0, static_cast<int>(team->players.size()) - 1);
        long long saleValue = team->players[sellIdx].value;
        team->players.erase(team->players.begin() + sellIdx);
        team->budget += saleValue;
        int minSkill, maxSkill;
        getDivisionSkillRange(team->division, minSkill, maxSkill);
        vector<string> positions = {"ARQ", "DEF", "MED", "DEL"};
        string pos = positions[randInt(0, static_cast<int>(positions.size()) - 1)];
        Player newP = makeRandomPlayer(pos, minSkill, maxSkill, 18, 30);
        if (team->budget >= newP.value) {
            team->budget -= newP.value;
            team->addPlayer(newP);
        }
    }
}

static void updateContracts(Career& career) {
    for (auto* team : career.activeTeams) {
        for (size_t i = 0; i < team->players.size();) {
            Player& p = team->players[i];
            if (p.contractWeeks > 0) p.contractWeeks--;
            if (p.contractWeeks > 0) {
                ++i;
                continue;
            }
            if (team == career.myTeam) {
                cout << "\nContrato expirado: " << p.name << " (Salario $" << p.wage << ")" << endl;
                int choice = readInt("Renovar? (1. Si, 2. No): ", 1, 2);
                if (choice == 1) {
                    p.contractWeeks = randInt(52, 156);
                    p.wage = static_cast<long long>(p.wage * (1.1 + randInt(0, 20) / 100.0));
                    cout << "Renovado por " << p.contractWeeks << " semanas. Nuevo salario $" << p.wage << endl;
                    ++i;
                } else {
                    cout << p.name << " deja el club." << endl;
                    team->players.erase(team->players.begin() + i);
                }
            } else {
                if (team->budget > p.wage * 8 && randInt(1, 100) <= 70) {
                    p.contractWeeks = randInt(52, 156);
                    p.wage = static_cast<long long>(p.wage * (1.05 + randInt(0, 15) / 100.0));
                    ++i;
                } else {
                    team->players.erase(team->players.begin() + i);
                }
            }
        }
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
    Team* winner = (randInt(0, 1) == 0) ? home : away;
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

static Team* simulateSingleLegKnockout(Team* home, Team* away, const string& label, bool verbose) {
    if (!home) return away;
    if (!away) return home;
    cout << label << ": " << home->name << " vs " << away->name << endl;
    Team h1 = *home;
    Team a1 = *away;
    MatchResult r = playMatch(h1, a1, verbose, true);
    cout << "Resultado: " << home->name << " " << r.homeGoals << " - " << r.awayGoals << " " << away->name << endl;
    if (r.homeGoals > r.awayGoals) return home;
    if (r.awayGoals > r.homeGoals) return away;
    Team* winner = (randInt(0, 1) == 0) ? home : away;
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

    int homeAgg = r1.homeGoals + r2.awayGoals;
    int awayAgg = r1.awayGoals + r2.homeGoals;

    cout << "Ida: " << firstHome->name << " " << r1.homeGoals << " - " << r1.awayGoals << " " << firstAway->name << endl;
    cout << "Vuelta: " << firstAway->name << " " << r2.homeGoals << " - " << r2.awayGoals << " " << firstHome->name << endl;
    cout << "Global: " << firstHome->name << " " << homeAgg << " - " << awayAgg << " " << firstAway->name << endl;

    if (homeAgg > awayAgg) return firstHome;
    if (awayAgg > homeAgg) return firstAway;

    if (extraTimeFinal) {
        int etHome = randInt(0, 1);
        int etAway = randInt(0, 1);
        cout << "Prorroga: " << firstHome->name << " " << etHome << " - " << etAway << " " << firstAway->name << endl;
        if (etHome > etAway) return firstHome;
        if (etAway > etHome) return firstAway;
    }

    Team* winner = (randInt(0, 1) == 0) ? firstHome : firstAway;
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
                adjustCpuTactics(*home, *away, career.myTeam);
                adjustCpuTactics(*away, *home, career.myTeam);
                playMatch(*home, *away, verbose, true);
            }
            for (auto* team : descensoTeams) {
                healInjuries(*team, false);
                recoverFitness(*team, 7);
                applyTrainingPlan(*team);
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

    career.currentSeason++;
    career.currentWeek = 1;
    career.agePlayers();
    if (career.myTeam) {
        career.setActiveDivision(career.myTeam->division);
    } else {
        career.setActiveDivision(career.activeDivision);
    }
    career.resetSeason();
    for (auto* team : career.activeTeams) {
        addYouthPlayers(*team, 1);
    }
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
                champion = simulateSingleLegKnockout(a, b, "Final", a == career.myTeam || b == career.myTeam);
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
            Team* winner = simulateSingleLegKnockout(a, b, "Definicion descenso", a == career.myTeam || b == career.myTeam);
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

    career.currentSeason++;
    career.currentWeek = 1;
    career.agePlayers();
    if (career.myTeam) {
        career.setActiveDivision(career.myTeam->division);
    } else {
        career.setActiveDivision(career.activeDivision);
    }
    career.resetSeason();
    for (auto* team : career.activeTeams) {
        addYouthPlayers(*team, 1);
    }
}

void endSeason(Career& career) {
    if (isSegundaFormat(career)) {
        endSeasonSegundaDivision(career);
        return;
    }
    if (career.activeDivision == "primera b") {
        endSeasonPrimeraB(career);
        return;
    }
    cout << "\nFin de temporada!" << endl;
    career.leagueTable.displayTable();
    Team* champion = nullptr;
    if (!career.leagueTable.teams.empty()) {
        if (career.activeDivision == "primera division" && career.leagueTable.teams.size() >= 2) {
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
                champion = simulateSingleLegKnockout(a, b, "Final", a == career.myTeam || b == career.myTeam);
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
    int relegateSlots = lower.empty() ? 0 : (career.activeDivision == "primera b" ? 1 : 2);

    if (promoteSlots > 0) {
        if (career.activeDivision == "primera b") {
            if (n > 0) promote.push_back(table[0]);
            if (promoteSlots > 1) {
                Team* playoffWinner = liguillaAscensoPrimeraB(table);
                if (playoffWinner &&
                    find(promote.begin(), promote.end(), playoffWinner) == promote.end()) {
                    promote.push_back(playoffWinner);
                } else if (n > 1 && promote.size() < static_cast<size_t>(promoteSlots)) {
                    promote.push_back(table[1]);
                }
            }
        } else {
            int count = min(promoteSlots, n);
            for (int i = 0; i < count; ++i) {
                promote.push_back(table[i]);
            }
        }
    }

    int actualPromote = static_cast<int>(promote.size());
    int relegateCount = min(relegateSlots, max(0, n - actualPromote));
    for (int i = 0; i < relegateCount; ++i) {
        relegate.push_back(table[n - 1 - i]);
    }

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), actualPromote);
    vector<Team*> fromLower;
    if (!lower.empty() && career.activeDivision == "primera division" && lower == "primera b") {
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

    career.currentSeason++;
    career.currentWeek = 1;
    career.agePlayers();
    if (career.myTeam) {
        career.setActiveDivision(career.myTeam->division);
    } else {
        career.setActiveDivision(career.activeDivision);
    }
    career.resetSeason();
    for (auto* team : career.activeTeams) {
        addYouthPlayers(*team, 1);
    }
}

void simulateCareerWeek(Career& career) {
    if (career.activeTeams.empty() || career.schedule.empty()) {
        cout << "No hay calendario disponible." << endl;
        return;
    }
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        endSeason(career);
        return;
    }

    cout << "\nSimulando semana " << career.currentWeek << "..." << endl;
    career.leagueTable.sortTable();
    LeagueTable northTable;
    LeagueTable southTable;
    bool useGroups = isSegundaFormat(career);
    if (useGroups) {
        northTable = buildGroupTable(career, career.groupNorthIdx, "Grupo Norte");
        southTable = buildGroupTable(career, career.groupSouthIdx, "Grupo Sur");
    }
    const auto& matches = career.schedule[career.currentWeek - 1];
    vector<int> pointsBefore;
    pointsBefore.reserve(career.activeTeams.size());
    for (auto* t : career.activeTeams) pointsBefore.push_back(t->points);
    for (const auto& match : matches) {
        Team* home = career.activeTeams[match.first];
        Team* away = career.activeTeams[match.second];
        bool verbose = (home == career.myTeam || away == career.myTeam);
        adjustCpuTactics(*home, *away, career.myTeam);
        adjustCpuTactics(*away, *home, career.myTeam);
        bool key = false;
        if (useGroups) {
            int group = groupForTeam(career, home);
            if (group == groupForTeam(career, away) && group == 0) {
                key = isKeyMatch(northTable, home, away);
            } else if (group == groupForTeam(career, away) && group == 1) {
                key = isKeyMatch(southTable, home, away);
            } else {
                key = isKeyMatch(career.leagueTable, home, away);
            }
        } else {
            key = isKeyMatch(career.leagueTable, home, away);
        }
        if (verbose && key) {
            cout << "[Aviso] Partido clave de la semana." << endl;
        }
        playMatch(*home, *away, verbose, key);
    }
    for (auto* team : career.activeTeams) {
        healInjuries(*team, false);
        recoverFitness(*team, 7);
        applyTrainingPlan(*team);
    }
    updateContracts(career);
    processIncomingOffers(career);
    processCpuTransfers(career);
    applyWeeklyFinances(career, pointsBefore);
    career.leagueTable.sortTable();
    weeklyDashboard(career);
    applyClubEvent(career);
    career.currentWeek++;
    checkAchievements(career);
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        endSeason(career);
    }
}

void playCupMode(Career& career) {
    if (career.divisions.empty()) {
        cout << "No hay divisiones disponibles." << endl;
        return;
    }
    cout << "\nSelecciona la division para Copa:" << endl;
    for (size_t i = 0; i < career.divisions.size(); ++i) {
        cout << i + 1 << ". " << career.divisions[i].display << endl;
    }
    int divisionChoice = readInt("Elige una division: ", 1, static_cast<int>(career.divisions.size()));
    string divisionId = career.divisions[divisionChoice - 1].id;
    auto teams = career.getDivisionTeams(divisionId);
    if (teams.size() < 2) {
        cout << "No hay suficientes equipos para una copa." << endl;
        return;
    }
    cout << "\nElige un equipo para seguir (0 para ninguno):" << endl;
    for (size_t i = 0; i < teams.size(); ++i) {
        cout << i + 1 << ". " << teams[i]->name << endl;
    }
    int followChoice = readInt("Equipo: ", 0, static_cast<int>(teams.size()));
    int followIdx = (followChoice == 0) ? -1 : (followChoice - 1);

    vector<Team> cupTeams;
    cupTeams.reserve(teams.size());
    for (auto* t : teams) cupTeams.push_back(*t);

    vector<int> alive;
    for (int i = 0; i < static_cast<int>(cupTeams.size()); ++i) alive.push_back(i);
    int round = 1;
    while (alive.size() > 1) {
        cout << "\n--- Copa: Ronda " << round << " ---" << endl;
        vector<int> next;
        if (alive.size() % 2 == 1) {
            int bye = alive.back();
            alive.pop_back();
            next.push_back(bye);
            cout << "Pase directo: " << cupTeams[bye].name << endl;
        }
        for (size_t i = 0; i < alive.size(); i += 2) {
            int aIdx = alive[i];
            int bIdx = alive[i + 1];
            Team& a = cupTeams[aIdx];
            Team& b = cupTeams[bIdx];
            bool verbose = (followIdx == aIdx || followIdx == bIdx);
            cout << a.name << " vs " << b.name << endl;
            MatchResult r = playMatch(a, b, verbose, true);
            int winner = aIdx;
            if (r.homeGoals < r.awayGoals) winner = bIdx;
            else if (r.homeGoals == r.awayGoals) {
                winner = (randInt(0, 1) == 0) ? aIdx : bIdx;
                cout << "Gana por penales: " << cupTeams[winner].name << endl;
            }
            next.push_back(winner);
        }
        alive.swap(next);
        round++;
    }
    cout << "\nCampeon de la Copa: " << cupTeams[alive.front()].name << endl;
}
