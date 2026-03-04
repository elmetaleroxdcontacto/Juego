#include "ui.h"

#include "simulation.h"
#include "utils.h"

#include <algorithm>
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
        p.injuryWeeks = 0;
        p.goals = 0;
        p.assists = 0;
        p.matchesPlayed = 0;
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
    cout << "7. Volver al Menu Principal" << endl;
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
}

void viewTeam(Team& team) {
    cout << "\nEquipo: " << team.name << endl;
    cout << "Tacticas: " << team.tactics << ", Formacion: " << team.formation << ", Presupuesto: $" << team.budget << endl;
    if (team.players.empty()) {
        cout << "No hay jugadores en el equipo." << endl;
        return;
    }
    for (size_t i = 0; i < team.players.size(); ++i) {
        const auto& p = team.players[i];
        cout << i + 1 << ". " << p.name << " (" << p.position << ") - Ataque: " << p.attack
             << ", Defensa: " << p.defense << ", Resistencia: " << p.stamina
             << ", Habilidad: " << p.skill << ", Edad: " << p.age << ", Valor: $" << p.value << endl;
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
    p.skill = readInt("Habilidad (0-100): ", 0, 100);
    p.age = readInt("Edad: ", 15, 50);
    p.value = readLongLong("Valor: ", 0, 1000000000000LL);
    p.injured = false;
    p.injuryWeeks = 0;
    p.goals = 0;
    p.assists = 0;
    p.matchesPlayed = 0;
    team.addPlayer(p);
    cout << "Jugador agregado!" << endl;
}

void trainPlayer(Team& team) {
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
    int improvement = randInt(1, 5);
    switch (trainChoice) {
        case 1:
            p.attack = min(100, p.attack + improvement);
            cout << "Ataque mejorado a " << p.attack << endl;
            break;
        case 2:
            p.defense = min(100, p.defense + improvement);
            cout << "Defensa mejorada a " << p.defense << endl;
            break;
        case 3:
            p.stamina = min(100, p.stamina + improvement);
            cout << "Resistencia mejorada a " << p.stamina << endl;
            break;
        case 4:
            p.skill = min(100, p.skill + improvement);
            cout << "Habilidad mejorada a " << p.skill << endl;
            break;
        default:
            break;
    }
}

void changeTactics(Team& team) {
    cout << "Tacticas actuales: " << team.tactics << endl;
    cout << "1. Defensive" << endl;
    cout << "2. Balanced" << endl;
    cout << "3. Offensive" << endl;
    int tacticsChoice = readInt("Elige tactica: ", 1, 3);
    switch (tacticsChoice) {
        case 1: team.tactics = "Defensive"; break;
        case 2: team.tactics = "Balanced"; break;
        case 3: team.tactics = "Offensive"; break;
        default: break;
    }
    cout << "Tacticas cambiadas a " << team.tactics << endl;
}

void displayStatistics(Team& team) {
    cout << "\n--- Estadisticas del Equipo ---" << endl;
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

void endSeason(Career& career) {
    cout << "\nFin de temporada!" << endl;
    career.leagueTable.displayTable();
    if (career.myTeam && career.myTeam->points > 50) {
        cout << "Tu equipo se clasifico para competiciones internacionales!" << endl;
    }
    career.currentSeason++;
    career.currentWeek = 1;
    career.agePlayers();
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
    const auto& matches = career.schedule[career.currentWeek - 1];
    for (const auto& match : matches) {
        Team* home = career.activeTeams[match.first];
        Team* away = career.activeTeams[match.second];
        bool verbose = (home == career.myTeam || away == career.myTeam);
        playMatch(*home, *away, verbose);
    }
    for (auto* team : career.activeTeams) {
        healInjuries(*team, false);
    }
    career.leagueTable.sortTable();
    career.currentWeek++;
    checkAchievements(career);
    if (career.currentWeek > static_cast<int>(career.schedule.size())) {
        endSeason(career);
    }
}
