#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

using namespace std;

// Player structure
struct Player {
    string name;
    string position;
    int attack;
    int defense;
    int stamina;
    int skill;
    int age;
    int value;
    bool injured;
    int injuryWeeks;
    int goals;
    int assists;
    int matchesPlayed;
};

// Team class
class Team {
public:
    string name;
    vector<Player> players;
    string tactics; // e.g., "Defensive", "Balanced", "Offensive"
    string formation; // e.g., "4-4-2"
    int budget;
    int points; // For league standings
    int goalsFor;
    int goalsAgainst;
    int wins;
    int draws;
    int losses;
    vector<string> achievements;

    Team(string n) : name(n), tactics("Balanced"), formation("4-4-2"), budget(1000000), points(0), goalsFor(0), goalsAgainst(0), wins(0), draws(0), losses(0) {}

    void addPlayer(Player p) {
        players.push_back(p);
    }

    int getTotalAttack() {
        int total = 0;
        for (auto& p : players) {
            total += p.attack;
        }
        return total;
    }

    int getTotalDefense() {
        int total = 0;
        for (auto& p : players) {
            total += p.defense;
        }
        return total;
    }

    int getAverageSkill() {
        if (players.empty()) return 0;
        int total = 0;
        for (auto& p : players) {
            total += p.skill;
        }
        return total / players.size();
    }

    void resetSeasonStats() {
        points = 0;
        goalsFor = 0;
        goalsAgainst = 0;
        wins = 0;
        draws = 0;
        losses = 0;
    }
};

// League Table structure
struct LeagueTable {
    vector<Team*> teams;

    void addTeam(Team* team) {
        teams.push_back(team);
    }

    void sortTable() {
        sort(teams.begin(), teams.end(), [](Team* a, Team* b) {
            if (a->points != b->points) return a->points > b->points;
            int aGD = a->goalsFor - a->goalsAgainst;
            int bGD = b->goalsFor - b->goalsAgainst;
            return aGD > bGD;
        });
    }

    void displayTable() {
        cout << "\n--- Liga Chilena Standings ---" << endl;
        cout << "Pos | Team                  | Pts | GF | GA | GD" << endl;
        cout << "----+-----------------------+-----+----+----+----" << endl;
        for (size_t i = 0; i < teams.size(); ++i) {
            Team* team = teams[i];
            int gd = team->goalsFor - team->goalsAgainst;
            cout << setw(3) << i+1 << " | " << setw(21) << left << team->name.substr(0, 21)
                 << " | " << setw(3) << team->points << " | " << setw(2) << team->goalsFor
                 << " | " << setw(2) << team->goalsAgainst << " | " << setw(2) << gd << endl;
        }
    }
};

// Forward declaration
bool loadTeamFromFile(const string& filename, Team& team);

string normalizeName(string name) {
    // Replace spaces
    replace(name.begin(), name.end(), ' ', '_');
    // Replace accented characters
    size_t pos;
    while ((pos = name.find("ñ")) != string::npos) name.replace(pos, 2, "n");
    while ((pos = name.find("Ñ")) != string::npos) name.replace(pos, 2, "n");
    while ((pos = name.find("á")) != string::npos) name.replace(pos, 2, "a");
    while ((pos = name.find("é")) != string::npos) name.replace(pos, 2, "e");
    while ((pos = name.find("í")) != string::npos) name.replace(pos, 2, "i");
    while ((pos = name.find("ó")) != string::npos) name.replace(pos, 2, "o");
    while ((pos = name.find("ú")) != string::npos) name.replace(pos, 2, "u");
    return name;
}

// Career structure
struct Career {
    Team* myTeam;
    LeagueTable leagueTable;
    int currentSeason;
    int currentWeek;
    vector<Team> allTeams;
    string saveFile;
    vector<string> achievements;

    Career() : myTeam(nullptr), currentSeason(1), currentWeek(1), saveFile("career_save.txt") {}

    void initializeLeague() {
        // Load Primera División teams from teams.txt
        ifstream file("LigaChilena/primera_division/teams.txt");
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                if (!line.empty()) {
                    Team team("");
                    team.name = line;
                    string filepath = "LigaChilena/primera_division/" + normalizeName(line) + ".txt";
                    if (loadTeamFromFile(filepath, team)) {
                        // Loaded from file with all players and their stats
                    } else {
                        // Generate random players
                        vector<string> positions = {"GK", "DF", "MF", "FW"};
                        for (int i = 0; i < 11; ++i) {
                            Player p;
                            p.name = "Player" + to_string(i+1);
                            p.position = positions[rand() % 4];
                            p.attack = rand() % 50 + 50; // higher for primera
                            p.defense = rand() % 50 + 50;
                            p.stamina = rand() % 50 + 50;
                            p.skill = rand() % 50 + 50;
                            p.age = rand() % 20 + 18;
                            p.value = p.skill * 10000;
                            p.injured = false;
                            p.injuryWeeks = 0;
                            p.goals = 0;
                            p.assists = 0;
                            p.matchesPlayed = 0;
                            team.addPlayer(p);
                        }
                    }
                    allTeams.push_back(team);
                    leagueTable.addTeam(&allTeams.back());
                }
            }
            file.close();
        }

        // Load second division teams
        ifstream file2("LigaChilena/segunda_division/SegundaDivision_teams.txt");
        if (file2.is_open()) {
            string line;
            while (getline(file2, line)) {
                if (!line.empty()) {
                    Team team(line);
                    string filepath = "LigaChilena/segunda_division/" + normalizeName(line) + ".txt";
                    if (loadTeamFromFile(filepath, team)) {
                        // Loaded from file
                    } else {
                        // Generate 11 random players if file not found
                        vector<string> positions = {"GK", "DF", "MF", "FW"};
                        for (int i = 0; i < 11; ++i) {
                            Player p;
                            p.name = "Player" + to_string(i+1);
                            p.position = positions[rand() % 4];
                            p.attack = rand() % 50 + 30; // 30-79
                            p.defense = rand() % 50 + 30;
                            p.stamina = rand() % 50 + 30;
                            p.skill = rand() % 50 + 30;
                            p.age = rand() % 20 + 18;
                            p.value = p.skill * 8000; // lower value for second division
                            p.injured = false;
                            p.injuryWeeks = 0;
                            p.goals = 0;
                            p.assists = 0;
                            p.matchesPlayed = 0;
                            team.addPlayer(p);
                        }
                    }
                    allTeams.push_back(team);
                    leagueTable.addTeam(&allTeams.back());
                }
            }
            file2.close();
        }

        // Load Primera B teams
        ifstream file3("LigaChilena/primera_b/PrimeraB_teams.txt");
        if (file3.is_open()) {
            string line;
            while (getline(file3, line)) {
                if (!line.empty()) {
                    Team team("");
                    team.name = line;
                    string filepath = "LigaChilena/primera_b/" + normalizeName(line) + ".txt";
                    if (loadTeamFromFile(filepath, team)) {
                        // Loaded from file
                    } else {
                        // Generate random players
                        vector<string> positions = {"GK", "DF", "MF", "FW"};
                        for (int i = 0; i < 11; ++i) {
                            Player p;
                            p.name = "Player" + to_string(i+1);
                            p.position = positions[rand() % 4];
                            p.attack = rand() % 40 + 20; // 20-59
                            p.defense = rand() % 40 + 20;
                            p.stamina = rand() % 40 + 20;
                            p.skill = rand() % 40 + 20;
                            p.age = rand() % 20 + 18;
                            p.value = p.skill * 5000; // lower value for Primera B
                            p.injured = false;
                            p.injuryWeeks = 0;
                            p.goals = 0;
                            p.assists = 0;
                            p.matchesPlayed = 0;
                            team.addPlayer(p);
                        }
                    }
                    allTeams.push_back(team);
                    leagueTable.addTeam(&allTeams.back());
                }
            }
            file3.close();
        }
    }

    void agePlayers() {
        for (auto& team : allTeams) {
            for (auto& player : team.players) {
                player.age++;
                // Slight skill decline with age
                if (player.age > 30) {
                    player.skill = max(1, player.skill - 1);
                    player.stamina = max(1, player.stamina - 1);
                }
            }
        }
    }

    void saveCareer() {
        ofstream file(saveFile);
        if (file.is_open()) {
            file << currentSeason << " " << currentWeek << endl;
            file << myTeam->name << endl;
            // Save team data
            for (const auto& player : myTeam->players) {
                file << player.name << "," << player.position << "," << player.attack << ","
                     << player.defense << "," << player.stamina << "," << player.skill << ","
                     << player.age << "," << player.value << "," << player.injured << ","
                     << player.injuryWeeks << "," << player.goals << "," << player.assists << ","
                     << player.matchesPlayed << endl;
            }
            file.close();
            cout << "Carrera guardada exitosamente." << endl;
        }
    }

    bool loadCareer() {
        ifstream file(saveFile);
        if (file.is_open()) {
            file >> currentSeason >> currentWeek;
            string teamName;
            getline(file, teamName); // consume newline
            getline(file, teamName);

            // Find the team
            for (auto& team : allTeams) {
                if (team.name == teamName) {
                    myTeam = &team;
                    break;
                }
            }

            if (myTeam) {
                myTeam->players.clear();
                string line;
                while (getline(file, line)) {
                    if (line.empty()) continue;
                    stringstream ss(line);
                    string token;
                    Player p;
                    getline(ss, p.name, ',');
                    getline(ss, p.position, ',');
                    try {
                        getline(ss, token, ','); p.attack = stoi(token);
                        getline(ss, token, ','); p.defense = stoi(token);
                        getline(ss, token, ','); p.stamina = stoi(token);
                        getline(ss, token, ','); p.skill = stoi(token);
                        getline(ss, token, ','); p.age = stoi(token);
                        getline(ss, token, ','); p.value = stoi(token);
                        string injuredStr;
                        getline(ss, injuredStr, ',');
                        // Convert injuredStr to lowercase
                        transform(injuredStr.begin(), injuredStr.end(), injuredStr.begin(), ::tolower);
                        if (injuredStr == "1" || injuredStr == "true") {
                            p.injured = true;
                        } else {
                            p.injured = false;
                        }
                        getline(ss, token, ','); p.injuryWeeks = stoi(token);
                        getline(ss, token, ','); p.goals = stoi(token);
                        getline(ss, token, ','); p.assists = stoi(token);
                        getline(ss, token, ','); p.matchesPlayed = stoi(token);
                        myTeam->addPlayer(p);
                    } catch (const std::invalid_argument& e) {
                        cout << "Error loading player data from save file: invalid data in line: " << line << endl;
                        // Skip this invalid player and continue
                    } catch (const std::out_of_range& e) {
                        cout << "Error loading player data from save file: value out of range in line: " << line << endl;
                        // Skip this invalid player and continue
                    }
                }
            }
            file.close();
            return true;
        }
        return false;
    }
};

// Function to simulate injury
void simulateInjury(Player& player) {
    if (rand() % 20 == 0) { // 5% chance
        player.injured = true;
        player.injuryWeeks = rand() % 4 + 1; // 1-4 weeks
        cout << player.name << " se lesionó y estará fuera por " << player.injuryWeeks << " semanas." << endl;
    }
}

// Function to heal injuries
void healInjuries(Team& team) {
    for (auto& player : team.players) {
        if (player.injured) {
            player.injuryWeeks--;
            if (player.injuryWeeks <= 0) {
                player.injured = false;
                cout << player.name << " se recuperó de su lesión." << endl;
            }
        }
    }
}

// Function for transfer market
void transferMarket(Team& team) {
    cout << "\n=== Mercado de Transferencias ===" << endl;
    cout << "Presupuesto: $" << team.budget << endl;
    cout << "1. Comprar jugador" << endl;
    cout << "2. Vender jugador" << endl;
    cout << "3. Volver" << endl;
    int choice;
    cin >> choice;
    if (choice == 1) {
        // Simple buy - create random player
        Player p;
        cout << "Nombre del jugador: ";
        cin >> p.name;
        cout << "Posición: ";
        cin >> p.position;
        p.attack = rand() % 50 + 50;
        p.defense = rand() % 50 + 50;
        p.stamina = rand() % 50 + 50;
        p.skill = rand() % 50 + 50;
        p.age = rand() % 20 + 18;
        p.value = p.skill * 10000;
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
        if (team.players.empty()) {
            cout << "No hay jugadores para vender." << endl;
            return;
        }
        cout << "Selecciona jugador para vender:" << endl;
        for (size_t i = 0; i < team.players.size(); ++i) {
            cout << i+1 << ". " << team.players[i].name << " ($" << team.players[i].value << ")" << endl;
        }
        int idx;
        cin >> idx;
        if (idx >= 1 && idx <= team.players.size()) {
            team.budget += team.players[idx-1].value;
            cout << team.players[idx-1].name << " vendido por $" << team.players[idx-1].value << endl;
            team.players.erase(team.players.begin() + idx - 1);
        }
    }
}

// Function to scout players
void scoutPlayers(Team& team) {
    cout << "\n=== Ojeo de Jugadores ===" << endl;
    cout << "Presupuesto: $" << team.budget << endl;
    cout << "Costo de ojeo: $5000" << endl;
    cout << "1. Otear jugadores" << endl;
    cout << "2. Volver" << endl;
    int choice;
    cin >> choice;
    if (choice == 1) {
        if (team.budget >= 5000) {
            team.budget -= 5000;
            // Generate 3 random players
            for (int i = 0; i < 3; ++i) {
                Player p;
                p.name = "Jugador Ojeado" + to_string(i+1);
                vector<string> positions = {"GK", "DF", "MF", "FW"};
                p.position = positions[rand() % 4];
                p.attack = rand() % 50 + 50;
                p.defense = rand() % 50 + 50;
                p.stamina = rand() % 50 + 50;
                p.skill = rand() % 50 + 50;
                p.age = rand() % 20 + 18;
                p.value = p.skill * 10000;
                p.injured = false;
                p.injuryWeeks = 0;
                p.goals = 0;
                p.assists = 0;
                p.matchesPlayed = 0;
                cout << "Jugador encontrado: " << p.name << " (" << p.position << ") - Ataque: " << p.attack << ", Defensa: " << p.defense << ", Valor: $" << p.value << endl;
                cout << "¿Comprar? (1. Sí, 2. No): ";
                int buy;
                cin >> buy;
                if (buy == 1 && team.budget >= p.value) {
                    team.budget -= p.value;
                    team.addPlayer(p);
                    cout << "Jugador comprado." << endl;
                } else if (buy == 1) {
                    cout << "Presupuesto insuficiente." << endl;
                }
            }
        } else {
            cout << "Presupuesto insuficiente para ojeo." << endl;
        }
    }
}

// Function to retire players
void retirePlayer(Team& team) {
    cout << "\n=== Retiro de Jugadores ===" << endl;
    vector<int> eligibleIndices;
    cout << "Jugadores elegibles para retiro (35-45 años):" << endl;
    for (size_t i = 0; i < team.players.size(); ++i) {
        if (team.players[i].age >= 35 && team.players[i].age <= 45) {
            eligibleIndices.push_back(i);
            cout << eligibleIndices.size() << ". " << team.players[i].name << " (Edad: " << team.players[i].age << ")" << endl;
        }
    }
    if (eligibleIndices.empty()) {
        cout << "No hay jugadores elegibles para retiro." << endl;
        return;
    }
    cout << "Selecciona un jugador para retirar (0 para cancelar): ";
    int choice;
    cin >> choice;
    if (choice >= 1 && choice <= eligibleIndices.size()) {
        int index = eligibleIndices[choice - 1];
        cout << team.players[index].name << " se ha retirado." << endl;
        team.players.erase(team.players.begin() + index);
    } else if (choice != 0) {
        cout << "Opción inválida." << endl;
    }
}

// Function to check achievements
void checkAchievements(Career& career) {
    if (career.myTeam->wins >= 10 && find(career.achievements.begin(), career.achievements.end(), "10 Victorias") == career.achievements.end()) {
        career.achievements.push_back("10 Victorias");
        cout << "¡Logro desbloqueado: 10 Victorias!" << endl;
    }
    // Add more achievements
}

// Function to simulate a match
void simulateMatch(Team& myTeam, Team& opponent) {
    srand(time(0));

    // Adjust attack and defense based on tactics
    int myAttack = myTeam.getTotalAttack();
    int myDefense = myTeam.getTotalDefense();
    int oppAttack = opponent.getTotalAttack();
    int oppDefense = opponent.getTotalDefense();

    if (myTeam.tactics == "Defensive") {
        myAttack -= 10;
        myDefense += 10;
    } else if (myTeam.tactics == "Offensive") {
        myAttack += 10;
        myDefense -= 10;
    }

    if (opponent.tactics == "Defensive") {
        oppAttack -= 10;
        oppDefense += 10;
    } else if (opponent.tactics == "Offensive") {
        oppAttack += 10;
        oppDefense -= 10;
    }

    // Consider skill and stamina
    myAttack += myTeam.getAverageSkill() / 2;
    myDefense += myTeam.getAverageSkill() / 2;
    oppAttack += opponent.getAverageSkill() / 2;
    oppDefense += opponent.getAverageSkill() / 2;

    // Simple stamina effect
    int myStaminaAvg = 0;
    for (auto& p : myTeam.players) myStaminaAvg += p.stamina;
    myStaminaAvg /= myTeam.players.size();
    myAttack = myAttack * myStaminaAvg / 100;
    myDefense = myDefense * myStaminaAvg / 100;

    int oppStaminaAvg = 0;
    for (auto& p : opponent.players) oppStaminaAvg += p.stamina;
    oppStaminaAvg /= opponent.players.size();
    oppAttack = oppAttack * oppStaminaAvg / 100;
    oppDefense = oppDefense * oppStaminaAvg / 100;

    // Simulate score
    int myScore = rand() % (myAttack / 10 + 1);
    int oppScore = rand() % (oppAttack / 10 + 1);

    cout << "\n--- Match Simulation ---" << endl;
    cout << myTeam.name << " vs " << opponent.name << endl;
    cout << "Tactics: " << myTeam.tactics << " vs " << opponent.tactics << endl;
    cout << "Final Score: " << myTeam.name << " " << myScore << " - " << oppScore << " " << opponent.name << endl;

    // Add some random events
    if (rand() % 2 == 0) {
        cout << "Match Event: A spectacular goal by " << myTeam.players[rand() % myTeam.players.size()].name << "!" << endl;
    }
    if (rand() % 2 == 0) {
        cout << "Match Event: Great save by the goalkeeper!" << endl;
    }

    if (myScore > oppScore) {
        cout << "You won!" << endl;
    } else if (myScore < oppScore) {
        cout << "You lost!" << endl;
    } else {
        cout << "It's a draw!" << endl;
    }
}

// Function to display main menu
void displayMainMenu() {
    cout << "\n=== Football Manager Game ===" << endl;
    cout << "1. Modo Carrera" << endl;
    cout << "2. Iniciar Juego Rapido" << endl;
    cout << "3. Salir" << endl;
    cout << "Elige una opcion: ";
}

// Function to display game menu
void displayGameMenu() {
    cout << "\nFootball Manager Game" << endl;
    cout << "1. View Team" << endl;
    cout << "2. Add Player" << endl;
    cout << "3. Train Player" << endl;
    cout << "4. Change Tactics" << endl;
    cout << "5. Simulate Match" << endl;
    cout << "6. Load Team from File" << endl;
    cout << "7. Exit to Main Menu" << endl;
    cout << "Choose an option: ";
}

// Function to view team
void viewTeam(Team& team) {
    cout << "\nTeam: " << team.name << endl;
    cout << "Tactics: " << team.tactics << ", Formation: " << team.formation << ", Budget: $" << team.budget << endl;
    if (team.players.empty()) {
        cout << "No players in the team." << endl;
    } else {
        for (size_t i = 0; i < team.players.size(); ++i) {
            cout << i+1 << ". " << team.players[i].name << " (" << team.players[i].position << ") - Attack: " << team.players[i].attack << ", Defense: " << team.players[i].defense << ", Stamina: " << team.players[i].stamina << ", Skill: " << team.players[i].skill << ", Age: " << team.players[i].age << ", Value: $" << team.players[i].value << endl;
        }
    }
}

// Function to add player
void addPlayer(Team& team) {
    Player p;
    cout << "Enter player name: ";
    cin >> p.name;
    cout << "Enter position: ";
    cin >> p.position;
    cout << "Enter attack skill (0-100): ";
    cin >> p.attack;
    cout << "Enter defense skill (0-100): ";
    cin >> p.defense;
    cout << "Enter stamina (0-100): ";
    cin >> p.stamina;
    cout << "Enter skill (0-100): ";
    cin >> p.skill;
    cout << "Enter age: ";
    cin >> p.age;
    cout << "Enter value: ";
    cin >> p.value;
    team.addPlayer(p);
    cout << "Player added!" << endl;
}

// Function to train player
void trainPlayer(Team& team) {
    if (team.players.empty()) {
        cout << "No players to train." << endl;
        return;
    }
    cout << "Select player to train:" << endl;
    for (size_t i = 0; i < team.players.size(); ++i) {
        cout << i+1 << ". " << team.players[i].name << (team.players[i].injured ? " (Lesionado)" : "") << endl;
    }
    int playerIndex;
    cin >> playerIndex;
    if (playerIndex < 1 || playerIndex > team.players.size()) {
        cout << "Invalid player." << endl;
        return;
    }
    Player& p = team.players[playerIndex - 1];
    if (p.injured) {
        cout << "No puedes entrenar a un jugador lesionado." << endl;
        return;
    }
    cout << "Choose what to train:" << endl;
    cout << "1. Attack" << endl;
    cout << "2. Defense" << endl;
    cout << "3. Stamina" << endl;
    cout << "4. Skill" << endl;
    int trainChoice;
    cin >> trainChoice;
    int cost = 5000;
    if (team.budget < cost) {
        cout << "Presupuesto insuficiente para entrenar." << endl;
        return;
    }
    team.budget -= cost;
    int improvement = rand() % 5 + 1; // 1-5 points
    switch (trainChoice) {
        case 1:
            p.attack = min(100, p.attack + improvement);
            cout << p.name << "'s attack improved by " << improvement << " to " << p.attack << endl;
            break;
        case 2:
            p.defense = min(100, p.defense + improvement);
            cout << p.name << "'s defense improved by " << improvement << " to " << p.defense << endl;
            break;
        case 3:
            p.stamina = min(100, p.stamina + improvement);
            cout << p.name << "'s stamina improved by " << improvement << " to " << p.stamina << endl;
            break;
        case 4:
            p.skill = min(100, p.skill + improvement);
            cout << p.name << "'s skill improved by " << improvement << " to " << p.skill << endl;
            break;
        default:
            cout << "Invalid choice." << endl;
    }
}

// Function to change tactics
void changeTactics(Team& team) {
    cout << "Current tactics: " << team.tactics << endl;
    cout << "Choose new tactics:" << endl;
    cout << "1. Defensive" << endl;
    cout << "2. Balanced" << endl;
    cout << "3. Offensive" << endl;
    int tacticsChoice;
    cin >> tacticsChoice;
    switch (tacticsChoice) {
        case 1:
            team.tactics = "Defensive";
            break;
        case 2:
            team.tactics = "Balanced";
            break;
        case 3:
            team.tactics = "Offensive";
            break;
        default:
            cout << "Invalid choice." << endl;
            return;
    }
    cout << "Tactics changed to " << team.tactics << endl;
}

// Function to load team from file
bool loadTeamFromFile(const string& filename, Team& team) {
    cout << "Loading " << filename << endl;
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Failed to open file: " << filename << endl;
        return false;
    }

    string line;
    while (getline(file, line)) {
        if (line.find("Team: ") == 0) {
            team.name = line.substr(6);
        } else if (line.find("- Name: ") == 0) {
            Player p;
            size_t posName = line.find("Name: ") + 6;
            size_t posPos = line.find(", Position: ");
            p.name = line.substr(posName, posPos - posName);

            size_t posAttack = line.find("Attack: ");
            size_t posDefense = line.find(", Defense: ");
            p.position = line.substr(posPos + 11, posAttack - (posPos + 11));
            p.attack = stoi(line.substr(posAttack + 8, posDefense - (posAttack + 8)));
            p.defense = stoi(line.substr(posDefense + 10));

            // Parse additional attributes if present
            size_t posStamina = line.find(", Stamina: ");
            if (posStamina != string::npos) {
                size_t posSkill = line.find(", Skill: ");
                p.stamina = stoi(line.substr(posStamina + 10, posSkill - (posStamina + 10)));
                size_t posAge = line.find(", Age: ");
                p.skill = stoi(line.substr(posSkill + 8, posAge - (posSkill + 8)));
                size_t posValue = line.find(", Value: ");
                p.age = stoi(line.substr(posAge + 6, posValue - (posAge + 6)));
                p.value = stoi(line.substr(posValue + 8));
            } else {
                // Set defaults
                p.stamina = 80;
                p.skill = (p.attack + p.defense) / 2;
                p.age = 25;
                p.value = p.skill * 10000;
            }
            p.injured = false;
            p.injuryWeeks = 0;
            p.goals = 0;
            p.assists = 0;
            p.matchesPlayed = 0;
            team.addPlayer(p);
        }
    }
    file.close();
    return true;
}

// Function to display career menu
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
    cout << "9. Guardar Carrera" << endl;
    cout << "10. Retirar Jugador" << endl;
    cout << "11. Volver al Menu Principal" << endl;
    cout << "Elige una opcion: ";
}

// Function to display statistics
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

// Function to simulate career match with league updates
void simulateCareerMatch(Team& myTeam, Team& opponent, LeagueTable& leagueTable) {
    srand(time(0));

    // Adjust attack and defense based on tactics
    int myAttack = myTeam.getTotalAttack();
    int myDefense = myTeam.getTotalDefense();
    int oppAttack = opponent.getTotalAttack();
    int oppDefense = opponent.getTotalDefense();

    if (myTeam.tactics == "Defensive") {
        myAttack -= 10;
        myDefense += 10;
    } else if (myTeam.tactics == "Offensive") {
        myAttack += 10;
        myDefense -= 10;
    }

    if (opponent.tactics == "Defensive") {
        oppAttack -= 10;
        oppDefense += 10;
    } else if (opponent.tactics == "Offensive") {
        oppAttack += 10;
        oppDefense -= 10;
    }

    // Consider skill and stamina
    myAttack += myTeam.getAverageSkill() / 2;
    myDefense += myTeam.getAverageSkill() / 2;
    oppAttack += opponent.getAverageSkill() / 2;
    oppDefense += opponent.getAverageSkill() / 2;

    // Simple stamina effect
    int myStaminaAvg = 0;
    for (auto& p : myTeam.players) myStaminaAvg += p.stamina;
    myStaminaAvg /= myTeam.players.size();
    myAttack = myAttack * myStaminaAvg / 100;
    myDefense = myDefense * myStaminaAvg / 100;

    int oppStaminaAvg = 0;
    for (auto& p : opponent.players) oppStaminaAvg += p.stamina;
    oppStaminaAvg /= opponent.players.size();
    oppAttack = oppAttack * oppStaminaAvg / 100;
    oppDefense = oppDefense * oppStaminaAvg / 100;

    // Simulate score
    int myScore = rand() % (myAttack / 10 + 1);
    int oppScore = rand() % (oppAttack / 10 + 1);

    cout << "\n--- Partido de Liga ---" << endl;
    cout << myTeam.name << " vs " << opponent.name << endl;
    cout << "Tacticas: " << myTeam.tactics << " vs " << opponent.tactics << endl;
    cout << "Resultado Final: " << myTeam.name << " " << myScore << " - " << oppScore << " " << opponent.name << endl;

    // Update league stats
    myTeam.goalsFor += myScore;
    myTeam.goalsAgainst += oppScore;
    opponent.goalsFor += oppScore;
    opponent.goalsAgainst += myScore;

    if (myScore > oppScore) {
        myTeam.points += 3;
        myTeam.wins++;
        opponent.losses++;
        cout << "¡Ganaste!" << endl;
    } else if (myScore < oppScore) {
        opponent.points += 3;
        opponent.wins++;
        myTeam.losses++;
        cout << "Perdiste." << endl;
    } else {
        myTeam.points += 1;
        opponent.points += 1;
        myTeam.draws++;
        opponent.draws++;
        cout << "Empate." << endl;
    }

    // Simulate injuries
    for (auto& p : myTeam.players) {
        if (!p.injured) simulateInjury(p);
    }
    for (auto& p : opponent.players) {
        if (!p.injured) simulateInjury(p);
    }

    // Sort table after match
    leagueTable.sortTable();
}

int main() {
    Career career;
    Team myTeam("My Team");
    Team opponent("Opponent Team");

    int mainChoice;
    bool careerStarted = false;
    do {
        displayMainMenu();
        cin >> mainChoice;
        switch (mainChoice) {
            case 1: { // Modo Carrera
                cout << "\n=== Modo Carrera ===" << endl;

                int careerStartChoice;
                do {
                    cout << "\nOpciones de Carrera:" << endl;
                    cout << "1. Continuar Carrera" << endl;
                    cout << "2. Nueva Carrera" << endl;
                    cout << "3. Volver al Menu Principal" << endl;
                    cout << "Elige una opcion: ";
                    cin >> careerStartChoice;
                    switch (careerStartChoice) {
                        case 1: {
                            career.initializeLeague();
                            if (career.loadCareer()) {
                                cout << "Carrera cargada exitosamente." << endl;
                                cout << "Temporada " << career.currentSeason << ", Semana " << career.currentWeek << endl;
                                careerStarted = true;
                            } else {
                                cout << "No se encontró una carrera guardada." << endl;
                            }
                            break;
                        }
                        case 2: {
                            career.initializeLeague();
                            // Division Selection
                            cout << "\nSelecciona la división:" << endl;
                            cout << "1. Primera División" << endl;
                            cout << "2. Primera B" << endl;
                            cout << "3. Segunda División" << endl;
                            cout << "Elige una división: ";
                            int divisionChoice;
                            cin >> divisionChoice;
                            int teamChoice;
                            if (divisionChoice == 1) {
                                // Primera División
                                cout << "\nEquipos de Primera División:" << endl;
                                for (int i = 0; i < 16; ++i) {
                                    cout << i + 1 << ". " << career.allTeams[i].name << endl;
                                }
                                cout << "Elige un numero de equipo: ";
                                cin >> teamChoice;
                                if (teamChoice >= 1 && teamChoice <= 16) {
                                    career.myTeam = &career.allTeams[teamChoice - 1];
                                    cout << "Has elegido: " << career.myTeam->name << endl;
                                    careerStarted = true;
                                } else {
                                    cout << "Opcion invalida." << endl;
                                }
                            } else if (divisionChoice == 2) {
                                // Primera B
                                cout << "\nEquipos de Primera B:" << endl;
                                for (int i = 0; i < 16; ++i) {
                                    cout << i + 1 << ". " << career.allTeams[26 + i].name << endl;
                                }
                                cout << "Elige un numero de equipo: ";
                                cin >> teamChoice;
                                if (teamChoice >= 1 && teamChoice <= 16) {
                                    career.myTeam = &career.allTeams[26 + teamChoice - 1];
                                    cout << "Has elegido: " << career.myTeam->name << endl;
                                    careerStarted = true;
                                } else {
                                    cout << "Opcion invalida." << endl;
                                }
                            } else if (divisionChoice == 3) {
                                // Segunda División
                                cout << "\nEquipos de Segunda División:" << endl;
                                for (int i = 0; i < 14; ++i) {
                                    cout << i + 1 << ". " << career.allTeams[16 + i].name << endl;
                                }
                                cout << "Elige un numero de equipo: ";
                                cin >> teamChoice;
                                if (teamChoice >= 1 && teamChoice <= 14) {
                                    career.myTeam = &career.allTeams[16 + teamChoice - 1];
                                    cout << "Has elegido: " << career.myTeam->name << endl;
                                    careerStarted = true;
                                } else {
                                    cout << "Opcion invalida." << endl;
                                }
                            } else {
                                cout << "Opcion invalida." << endl;
                            }
                            break;
                        }
                        case 3: {
                            break;
                        }
                        default:
                            cout << "Opcion invalida." << endl;
                            break;
                    }

                } while (!careerStarted && careerStartChoice != 3);

                if (careerStarted) {
                    // Career Game Loop
                    int careerChoice;
                    do {
                        cout << "\nTemporada " << career.currentSeason << ", Semana " << career.currentWeek << endl;
                        displayCareerMenu();
                        cin >> careerChoice;
                        switch (careerChoice) {
                            case 1:
                                viewTeam(*career.myTeam);
                                break;
                            case 2:
                                trainPlayer(*career.myTeam);
                                break;
                            case 3:
                                changeTactics(*career.myTeam);
                                break;
                            case 4: {
                                // Simulate week - play matches
                                cout << "\nSimulando semana " << career.currentWeek << "..." << endl;
                                // Find opponents for this week (simplified - each team plays one match per week)
                                vector<Team*> opponents;
                                for (auto& team : career.allTeams) {
                                    if (&team != career.myTeam) {
                                        opponents.push_back(&team);
                                    }
                                }
                                // Play match against random opponent
                                if (!opponents.empty()) {
                                    int oppIndex = rand() % opponents.size();
                                    simulateCareerMatch(*career.myTeam, *opponents[oppIndex], career.leagueTable);
                                }
                                // Simulate other matches
                                for (size_t i = 0; i < career.allTeams.size(); ++i) {
                                    for (size_t j = i + 1; j < career.allTeams.size(); ++j) {
                                        if (rand() % 3 == 0) { // 33% chance of additional matches
                                            simulateCareerMatch(career.allTeams[i], career.allTeams[j], career.leagueTable);
                                        }
                                    }
                                }
                                career.currentWeek++;
                                if (career.currentWeek > 34) { // End of season
                                    cout << "\n¡Fin de temporada!" << endl;
                                    career.leagueTable.displayTable();
                                    // Promotions and relegations
                                    if (career.myTeam->points > 50) { // Top position
                                        cout << "¡Tu equipo se clasificó para competiciones internacionales!" << endl;
                                    }
                                    career.currentSeason++;
                                    career.currentWeek = 1;
                                    career.agePlayers();
                                    // Reset season stats
                                    for (auto& team : career.allTeams) {
                                        team.resetSeasonStats();
                                    }
                                    // Heal all injuries at end of season
                                    for (auto& team : career.allTeams) {
                                        for (auto& p : team.players) {
                                            p.injured = false;
                                            p.injuryWeeks = 0;
                                        }
                                    }
                                }
                                // Heal injuries weekly
                                healInjuries(*career.myTeam);
                                // Check achievements
                                checkAchievements(career);
                                break;
                            }
                            case 5:
                                career.leagueTable.displayTable();
                                break;
                            case 6:
                                transferMarket(*career.myTeam);
                                break;
                            case 7:
                                scoutPlayers(*career.myTeam);
                                break;
                            case 8:
                                displayStatistics(*career.myTeam);
                                break;
                            case 9:
                                career.saveCareer();
                                break;
                            case 10:
                                retirePlayer(*career.myTeam);
                                break;
                            case 11:
                                cout << "Volviendo al menu principal." << endl;
                                careerStarted = false;
                                break;
                            default:
                                cout << "Opcion invalida." << endl;
                                break;
                        }

                    } while (careerChoice != 11);
                }
                break;
            }

            case 2: { // Juego Rapido
                // Start Quick Game - Team Selection
                cout << "\nSelecciona tu equipo de Primera División:" << endl;
                ifstream teamsFile("LigaChilena/primera_division/teams.txt");
                vector<string> teamNames;
                if (teamsFile.is_open()) {
                    string line;
                    int count = 1;
                    while (getline(teamsFile, line)) {
                        if (!line.empty()) {
                            cout << count << ". " << line << endl;
                            teamNames.push_back(line);
                            count++;
                        }
                    }
                    teamsFile.close();
                } else {
                    cout << "Error al cargar la lista de equipos." << endl;
                    break;
                }
                cout << "Elige un numero de equipo: ";
                int teamChoice;
                cin >> teamChoice;
                if (teamChoice >= 1 && teamChoice <= teamNames.size()) {
                    string teamName = teamNames[teamChoice - 1];
                    myTeam.name = teamName;
                    string filename = "LigaChilena/primera_division/" + normalizeName(teamName) + ".txt";
                    if (!loadTeamFromFile(filename, myTeam)) {
                        cout << "Error al cargar el equipo. Generando jugadores aleatorios." << endl;
                        // Generate random players
                        vector<string> positions = {"GK", "DF", "MF", "FW"};
                        for (int i = 0; i < 11; ++i) {
                            Player p;
                            p.name = "Player" + to_string(i+1);
                            p.position = positions[rand() % 4];
                            p.attack = rand() % 50 + 50;
                            p.defense = rand() % 50 + 50;
                            p.stamina = rand() % 50 + 50;
                            p.skill = rand() % 50 + 50;
                            p.age = rand() % 20 + 18;
                            p.value = p.skill * 10000;
                            p.injured = false;
                            p.injuryWeeks = 0;
                            p.goals = 0;
                            p.assists = 0;
                            p.matchesPlayed = 0;
                            myTeam.addPlayer(p);
                        }
                    }
                } else {
                    cout << "Opción inválida." << endl;
                    break;
                }
                // Quick Game Loop
                int gameChoice;
                do {
                    displayGameMenu();
                    cin >> gameChoice;
                    switch (gameChoice) {
                        case 1:
                            viewTeam(myTeam);
                            break;
                        case 2:
                            addPlayer(myTeam);
                            break;
                        case 3:
                            trainPlayer(myTeam);
                            break;
                        case 4:
                            changeTactics(myTeam);
                            break;
                        case 5:
                            simulateMatch(myTeam, opponent);
                            break;
                        case 6: {
                            cout << "Ingresa el nombre del archivo: ";
                            string fname;
                            cin >> fname;
                            loadTeamFromFile(fname, myTeam);
                            break;
                        }
                        case 7:
                            cout << "Volviendo al menú principal." << endl;
                            break;
                        default:
                            cout << "Opción inválida." << endl;
                            break;
                        }

                    } while (gameChoice != 7);
                break;
            }
            case 3:
                cout << "Saliendo del juego." << endl;
                break;
            default:
                cout << "Opción inválida." << endl;
        }

    } while (mainChoice != 3 && !careerStarted);

    return 0;
}
