#include "ui.h"

#include "simulation.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

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
        p.injured = false;
        p.injuryType = "";
        p.injuryWeeks = 0;
        p.goals = 0;
        p.assists = 0;
        p.matchesPlayed = 0;
        p.lastTrainedSeason = -1;
        p.lastTrainedWeek = -1;
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
            cout << "Jugador encontrado: " << p.name << " (" << p.position << ") - Ataque: " << p.attack
                 << ", Defensa: " << p.defense << ", Valor: $" << p.value << endl;
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
    cout << "3. Salir" << endl;
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
    cout << "12. Volver al Menu Principal" << endl;
    cout << "13. Editar Equipo" << endl;
}

void viewTeam(Team& team) {
    cout << "\nEquipo: " << team.name << endl;
    cout << "Tacticas: " << team.tactics << ", Formacion: " << team.formation << ", Presupuesto: $" << team.budget << endl;
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
                 << ", Edad " << p.age << ", Valor $" << p.value << endl;
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
    p.injured = false;
    p.injuryType = "";
    p.injuryWeeks = 0;
    p.goals = 0;
    p.assists = 0;
    p.matchesPlayed = 0;
    p.lastTrainedSeason = -1;
    p.lastTrainedWeek = -1;
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
            cout << "11. Volver" << endl;
            int field = readInt("Campo a editar: ", 1, 11);
            if (field == 11) continue;
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
    int factor = divisionWageFactor(team.division);
    long long sum = 0;
    for (const auto& p : team.players) sum += p.skill;
    return sum * factor;
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

static vector<Team*> bottomByValue(vector<Team*> teams, int count) {
    sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
        return a->getSquadValue() < b->getSquadValue();
    });
    if (count < 0) count = 0;
    if (static_cast<int>(teams.size()) > count) teams.resize(count);
    return teams;
}

static int goalDiff(const Team* t) {
    return t->goalsFor - t->goalsAgainst;
}

void endSeason(Career& career) {
    cout << "\nFin de temporada!" << endl;
    career.leagueTable.displayTable();
    if (career.myTeam && career.myTeam->points > 50) {
        cout << "Tu equipo se clasifico para competiciones internacionales!" << endl;
    }
    int idx = divisionIndex(career.activeDivision);
    string higher = (idx > 0) ? kDivisions[idx - 1].id : "";
    string lower = (idx >= 0 && idx + 1 < static_cast<int>(kDivisions.size())) ? kDivisions[idx + 1].id : "";

    vector<Team*> table = career.leagueTable.teams;
    int n = static_cast<int>(table.size());
    vector<Team*> promote;
    vector<Team*> relegate;
    if (!higher.empty()) {
        if (n >= 2) {
            promote.push_back(table[0]);
            promote.push_back(table[1]);
        } else if (n == 1) {
            promote.push_back(table[0]);
        }
        if (n >= 3) {
            Team* second = table[1];
            Team* third = table[2];
            if (second->points == third->points && goalDiff(second) == goalDiff(third)) {
                cout << "\nPlayoff Ascenso: " << second->name << " vs " << third->name << endl;
                Team a = *second;
                Team b = *third;
                MatchResult r = playMatch(a, b, true, true);
                if (r.awayGoals > r.homeGoals) promote[1] = third;
            }
        }
    }
    if (!lower.empty()) {
        if (n >= 2) {
            relegate.push_back(table[n - 2]);
            relegate.push_back(table[n - 1]);
        }
        if (n >= 3) {
            Team* thirdLast = table[n - 3];
            Team* secondLast = table[n - 2];
            if (thirdLast->points == secondLast->points && goalDiff(thirdLast) == goalDiff(secondLast)) {
                cout << "\nPlayoff Descenso: " << thirdLast->name << " vs " << secondLast->name << endl;
                Team a = *thirdLast;
                Team b = *secondLast;
                MatchResult r = playMatch(a, b, true, true);
                if (r.homeGoals >= r.awayGoals) {
                    relegate[0] = secondLast;
                } else {
                    relegate[0] = thirdLast;
                }
            }
        }
    }

    vector<Team*> fromHigher = higher.empty() ? vector<Team*>() : bottomByValue(career.getDivisionTeams(higher), 2);
    vector<Team*> fromLower = lower.empty() ? vector<Team*>() : topByValue(career.getDivisionTeams(lower), 2);

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
    const auto& matches = career.schedule[career.currentWeek - 1];
    vector<int> pointsBefore;
    pointsBefore.reserve(career.activeTeams.size());
    for (auto* t : career.activeTeams) pointsBefore.push_back(t->points);
    for (const auto& match : matches) {
        Team* home = career.activeTeams[match.first];
        Team* away = career.activeTeams[match.second];
        bool verbose = (home == career.myTeam || away == career.myTeam);
        bool key = isKeyMatch(career.leagueTable, home, away);
        if (verbose && key) {
            cout << "[Aviso] Partido clave de la semana." << endl;
        }
        playMatch(*home, *away, verbose, key);
    }
    for (auto* team : career.activeTeams) {
        healInjuries(*team, false);
        recoverFitness(*team, 7);
    }
    processIncomingOffers(career);
    processCpuTransfers(career);
    applyWeeklyFinances(career, pointsBefore);
    career.leagueTable.sortTable();
    career.currentWeek++;
    checkAchievements(career);
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        endSeason(career);
    }
}
